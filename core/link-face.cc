/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <boost/random/random_device.hpp>
#include "link-face.h"
#include "link.h"

namespace emulator {

const int LinkFace::SYMBOL_TIME = 16;  // 16 us
const int LinkFace::BACKOFF_PERIOD = 20 * LinkFace::SYMBOL_TIME;  // in us
const int LinkFace::MIN_BE = 3;
const int LinkFace::MAX_BE = 5;
const int LinkFace::MAX_CSMA_BACKOFFS = 4;

LinkFace::LinkFace (const int faceId, const std::string& nodeId,
                    boost::asio::io_service& ioService,
                    const boost::function<void (const int, const ndn::Block&)>&
                    nodeMessageCallback,
                    boost::shared_ptr<Link> link)
  : Face (faceId, nodeId, ioService, nodeMessageCallback)
  , m_link (link)
  , m_rxTimer (ioService)
  , m_csmaTimer (ioService)
  , m_state (IDLE) // PhyState.IDLE
  , m_txQueueLimit (5)
{
  boost::random::random_device rng;
  m_engine.seed (rng ());
  NDNEM_LOG_TRACE ("[LinkFace::LinkFace] (" << m_nodeId
                   << ":" << m_id << ") on link " << m_link->GetId ());
}


inline const std::string&
LinkFace::GetLinkId () const
{
  return m_link->GetId ();
}

void
LinkFace::StartRx (const boost::shared_ptr<Packet>& pkt)
{
  NDNEM_LOG_TRACE ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                   << ") prior state = " << PhyStateToString (m_state));
  switch (m_state)
    {
    case IDLE:
      {
        m_state = RX;
        m_pendingRx = pkt;
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1E6 / (Link::TX_RATE * 1024.0)));

        NDNEM_LOG_TRACE ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                         << ") set rx timer in " << delay << " us");

        m_rxTimer.expires_from_now (boost::posix_time::microseconds (delay));
        m_rxTimer.async_wait
          (boost::bind (&LinkFace::PostRx, this, _1));
      }
      break;

    case RX:
    case RX_COLLIDE:
      {
        NDNEM_LOG_TRACE ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                         << ") called while in RX/RX_COLLIDE");
        m_state = RX_COLLIDE;
        m_pendingRx = pkt;
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1E6 / (Link::TX_RATE * 1024.0)));

        NDNEM_LOG_TRACE ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                         << ") set rx timer in " << delay << " us");

        // Cancel previous timer and set new timer based on the new packet size
        m_rxTimer.expires_from_now (boost::posix_time::microseconds (delay));
        m_rxTimer.async_wait
          (boost::bind (&LinkFace::PostRx, this, _1));
      }
      break;

    case TX:
      NDNEM_LOG_TRACE ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                       << ") called while in TX");
      // Ignore this RX request. Collision will happen at other nodes on the link
      break;

    default:
      NDNEM_LOG_ERROR ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                       << ") illegal state: " << PhyStateToString (m_state));
      throw std::runtime_error ("[LinkFace::StartRx] illegal state: " + PhyStateToString (m_state));
      break;
    }
  NDNEM_LOG_TRACE ("[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                   << ") after state = " << PhyStateToString (m_state));
}

void
LinkFace::PostRx (const boost::system::error_code& error)
{
  if (error)
    {
      NDNEM_LOG_TRACE ("[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                       << ") RX timer cancelled");
      return;
    }

  NDNEM_LOG_TRACE ("[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                   << ") prior state = " << PhyStateToString (m_state));
  switch (m_state)
    {
    case RX:
      {
        const ndn::Block& wire = m_pendingRx->GetBlock ();
        // Post the message asynchronously
        m_ioService.post (boost::bind (m_nodeMessageCallback, m_id, wire));
      }
      break;
    case RX_COLLIDE:
      NDNEM_LOG_INFO ("[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                      << ") RX failed due to packet collision");
      break;
    case IDLE:
      // There are some cases where the callback is still raised when
      // it should have been cancelled. This may happen when the delay
      // in emulator processing is large enough and therefore the timer
      // has already expired before we have a chance to cancel it.
      // In that case, the state may have been already set to IDLE by the
      // previous execution of PostRx (which should have been cancelled).
      // In order not to crash the emulator, we have to allow for this case
      // as a special hack to get around the problem.
      NDNEM_LOG_INFO ("[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                      << ") called when the state is IDLE");
      break;
    default:
      NDNEM_LOG_ERROR ("[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                       << ") illegal state: " << PhyStateToString (m_state));
      throw std::runtime_error ("[LinkFace::PostRx] illegal state: "
                                + PhyStateToString (m_state));
      break;
    }

  // Clear PHY state
  m_state = IDLE;
  NDNEM_LOG_TRACE ("[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                   << ") after state = " << PhyStateToString (m_state));
}

void
LinkFace::StartTx (const boost::shared_ptr<Packet>& pkt)
{
  NDNEM_LOG_TRACE ("[LinkFace::StartTx] (" << m_nodeId << ":" << m_id
                   << ") device in " << PhyStateToString (m_state)
                   << ". Queue size = " << m_txQueue.size ());

  if (m_txQueue.size () < m_txQueueLimit)
    {
      NDNEM_LOG_TRACE ("[LinkFace::StartTx] (" << m_nodeId << ":" << m_id
                       << ") enqueue the packet and start csma ");
      m_txQueue.push_back (pkt);
      this->StartCsma ();
    }
  else
    NDNEM_LOG_INFO ("[LinkFace::StartTx] (" << m_nodeId << ":" << m_id
                    << ") reached max queue size. Drop tail");
}

void
LinkFace::StartCsma ()
{
  NDNEM_LOG_TRACE ("[LinkFace::StartCsma] (" << m_nodeId << ":" << m_id
                   << ") start CCA");

  int NB = 0, BE = LinkFace::MIN_BE;
  boost::random::uniform_int_distribution<> rand (0, (1 << BE) - 1);
  long backoff = rand (m_engine) * LinkFace::BACKOFF_PERIOD;

  NDNEM_LOG_TRACE ("[LinkFace::StartCsma] (" << m_nodeId << ":" << m_id
                   << ") set csma timer in " << backoff << " us for CCA");

  m_csmaTimer.expires_from_now (boost::posix_time::microseconds (backoff));
  m_csmaTimer.async_wait
    (boost::bind (&LinkFace::DoCsma, this, NB, BE, _1));
}

void
LinkFace::DoCsma (int NB, int BE, const boost::system::error_code& error)
{
  if (error)
    {
      NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                       << ") csma timer cancelled");
      //m_txQueue.clear ();
      return;
    }

  assert (m_txQueue.size () >= 1);
  NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                   << ") prior state = " << PhyStateToString (m_state));

  switch (m_state)
    {
    case IDLE:
      {
        NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") channel clear after " << NB << " backoffs. Start Tx");
        m_state = TX;

        // Send the message to the link asynchronously
        boost::shared_ptr<Packet>& pkt = m_txQueue.front ();
        m_ioService.post (boost::bind (&Link::Transmit, m_link, m_nodeId, pkt));

        // Set timer to clear TX state later
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1E6 / (Link::TX_RATE * 1024.0)));

        NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") set csma timer in " << delay << " us for TX");

        m_csmaTimer.expires_from_now (boost::posix_time::microseconds (delay));
        m_csmaTimer.async_wait
          (boost::bind (&LinkFace::DoCsma, this, -1, -1, _1));
      }
      break;

    case RX:
    case RX_COLLIDE:
      {
        NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") channel busy after " << NB << " backoffs");
        NB = NB + 1;
        if (NB > LinkFace::MAX_CSMA_BACKOFFS)
          {
            NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                             << ") reach max backoff. Give up Tx");
            m_txQueue.pop_front ();

            // Leave m_state as it is. RX path will reset it back to IDLE

            // Should we clear the entire queue?
            if (m_txQueue.size () != 0)
              {
                // Schedule tx of the next packet in queue
                this->StartCsma ();
              }

            return;
          }
        BE = BE + 1;
        if (BE > LinkFace::MAX_BE)
          BE = LinkFace::MAX_BE;
        boost::random::uniform_int_distribution<> rand (0, (1 << BE) - 1);
        long backoff = rand (m_engine) * LinkFace::BACKOFF_PERIOD;

        NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") set csma timer in " << backoff << " us for backoff");

        m_csmaTimer.expires_from_now (boost::posix_time::microseconds (backoff));
        m_csmaTimer.async_wait
          (boost::bind (&LinkFace::DoCsma, this, NB, BE, _1));
      }
      break;

    case TX:
      m_txQueue.pop_front ();

      NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                       << ") Tx finish. Queue size = " << m_txQueue.size ());
      m_state = IDLE;

      if (m_txQueue.size () != 0)
        {
          // Schedule tx of the next packet in queue
          this->StartCsma ();
        }

      break;

    default:
      NDNEM_LOG_ERROR ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                       << ") illegal state: " << PhyStateToString (m_state));
      throw std::runtime_error ("[LinkFace::DoCsma] illegal state: " + PhyStateToString (m_state));
      break;

    }
  NDNEM_LOG_TRACE ("[LinkFace::DoCsma] (" << m_nodeId << ":" << m_id
                   << ") after state = " << PhyStateToString (m_state));
}

} // namespace emulator
