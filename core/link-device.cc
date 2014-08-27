/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "logging.h"
#include "link-device.h"
#include "link-face.h"
#include "link.h"
#include "node.h"
#include <boost/random/random_device.hpp>
#include <iomanip>

namespace emulator {

const int LinkDevice::SYMBOL_TIME = 16;  // 16 us
const int LinkDevice::BACKOFF_PERIOD = 20 * LinkDevice::SYMBOL_TIME;  // in us
const int LinkDevice::MIN_BE = 3;
const int LinkDevice::MAX_BE = 5;
const int LinkDevice::MAX_CSMA_BACKOFFS = 4;

LinkDevice::LinkDevice (const std::string& id,
                        const uint64_t macAddr,
			boost::shared_ptr<Link>& link,
			boost::shared_ptr<Node>& node,			
			boost::asio::io_service& ioService,
			const std::size_t txLimit)
  : m_id (id)
  , m_macAddr (macAddr)
  , m_nodeId (node->GetId ())
  , m_link (link)
  , m_node (node)
  , m_ioService (ioService)
  , m_rxTimer (ioService)
  , m_csmaTimer (ioService)
  , m_state (IDLE) // PhyState.IDLE
  , m_txQueueLimit (txLimit)
{
  boost::random::random_device rng;
  m_engine.seed (rng ());
  NDNEM_LOG_TRACE ("[LinkDevice::LinkDevice] (" << m_nodeId
		   << ":" << m_id << ") attached to link " << m_link->GetId ()
		   << ", mac addr 0x" << std::hex << std::setfill ('0')
                   << std::setw (4) << m_macAddr << std::dec
		   << ", queue size " << m_txQueueLimit);
}

void
LinkDevice::AddBroadcastFace ()
{
  // Create broadcast face
  boost::shared_ptr<LinkDevice> self = this->shared_from_this ();
  boost::shared_ptr<LinkFace> face = m_node->AddLinkFace (0xffff, self);
  m_faces[0xffff] = face;
  NDNEM_LOG_TRACE ("[LinkDevice::AddBroadcastFace] (" << m_nodeId << ":" << m_id
                   << ") create broadcast face id " << face->GetId ()
                   << " to remote mac 0xffff");
}

void
LinkDevice::StartRx (const boost::shared_ptr<Packet>& pkt)
{
  NDNEM_LOG_TRACE ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                   << ") prior state = " << PhyStateToString (m_state));
  switch (m_state)
    {
    case IDLE:
      {
        m_state = RX;
        m_pendingRx = pkt;
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1E6
            / (m_link->GetTxRate () * 1024.0)));

        NDNEM_LOG_TRACE ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                         << ") set rx timer in " << delay << " us");

        m_rxTimer.expires_from_now (boost::posix_time::microseconds (delay));
        m_rxTimer.async_wait
          (boost::bind (&LinkDevice::PostRx, this, _1));
      }
      break;

    case RX:
    case RX_COLLIDE:
      {
        NDNEM_LOG_TRACE ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                         << ") called while in RX/RX_COLLIDE");
        m_state = RX_COLLIDE;
        m_pendingRx = pkt;
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1E6
            / (m_link->GetTxRate () * 1024.0)));

        NDNEM_LOG_TRACE ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                         << ") set rx timer in " << delay << " us");

        // Cancel previous timer and set new timer based on the new packet size
        m_rxTimer.expires_from_now (boost::posix_time::microseconds (delay));
        m_rxTimer.async_wait
          (boost::bind (&LinkDevice::PostRx, this, _1));
      }
      break;

    case TX:
      NDNEM_LOG_TRACE ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                       << ") called while in TX");
      // Ignore this RX request. Collision will happen at other nodes on the link
      break;

    default:
      NDNEM_LOG_ERROR ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                       << ") illegal state: " << PhyStateToString (m_state));
      throw std::runtime_error ("[LinkDevice::StartRx] illegal state: "
                                + PhyStateToString (m_state));
      break;
    }
  NDNEM_LOG_TRACE ("[LinkDevice::StartRx] (" << m_nodeId << ":" << m_id
                   << ") after state = " << PhyStateToString (m_state));
}

void
LinkDevice::PostRx (const boost::system::error_code& error)
{
  if (error)
    {
      NDNEM_LOG_TRACE ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
                       << ") RX timer cancelled");
      return;
    }

  NDNEM_LOG_TRACE ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
                   << ") prior state = " << PhyStateToString (m_state));
  switch (m_state)
    {
    case RX:
      {
        const uint64_t dst = m_pendingRx->GetDst ();
	const uint64_t src = m_pendingRx->GetSrc ();
        const ndn::Block& wire = m_pendingRx->GetBlock ();
        if (dst == m_macAddr || dst == 0xffff)  //XXX: assume 0xffff is broadcast mac
	  {
	    std::map<uint64_t, boost::shared_ptr<LinkFace> >::iterator it =
	      m_faces.find (src);
	    boost::shared_ptr<LinkFace> face;
	    if (it == m_faces.end ())
	      {
		// Create on-demand link face for the source
		boost::shared_ptr<LinkDevice> self = this->shared_from_this ();
		face = m_node->AddLinkFace (src, self);
		m_faces[src] = face;
                NDNEM_LOG_TRACE ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
                                 << ") create on-demand face id " << face->GetId ()
                                 << " to remote mac " << std::hex << std::setfill ('0')
                                 << std::setw (4) << src << std::dec);
	      }
	    else
	      face = it->second;

	    // Post the message asynchronously
	    m_ioService.post (boost::bind (&Face::Dispatch, face, wire));
	  }
      }
      break;
    case RX_COLLIDE:
      NDNEM_LOG_INFO ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
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
      NDNEM_LOG_INFO ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
                      << ") called when the state is IDLE");
      break;
    default:
      NDNEM_LOG_ERROR ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
                       << ") illegal state: " << PhyStateToString (m_state));
      throw std::runtime_error ("[LinkDevice::PostRx] illegal state: "
                                + PhyStateToString (m_state));
      break;
    }

  // Clear PHY state
  m_state = IDLE;
  NDNEM_LOG_TRACE ("[LinkDevice::PostRx] (" << m_nodeId << ":" << m_id
                   << ") after state = " << PhyStateToString (m_state));
}

void
LinkDevice::StartTx (boost::shared_ptr<Packet>& pkt)
{
  NDNEM_LOG_TRACE ("[LinkDevice::StartTx] (" << m_nodeId << ":" << m_id
                   << ") device in " << PhyStateToString (m_state)
                   << ". Queue size = " << m_txQueue.size ());

  if (pkt->GetLength () > m_link->GetMtu ())
    {
      NDNEM_LOG_INFO ("[LinkDevice::StartTx] (" << m_nodeId << ":" << m_id
                      << ") packet size exceed link mtu. Drop packet.");
      return;
    }

  if (m_txQueue.size () < m_txQueueLimit)
    {
      NDNEM_LOG_TRACE ("[LinkDevice::StartTx] (" << m_nodeId << ":" << m_id
                       << ") enqueue the packet and start csma");
      pkt->SetSrc (m_macAddr);
      m_txQueue.push_back (pkt);
      this->StartCsma ();
    }
  else
    NDNEM_LOG_INFO ("[LinkDevice::StartTx] (" << m_nodeId << ":" << m_id
                    << ") reached max queue size. Drop tail");
}

void
LinkDevice::StartCsma ()
{
  NDNEM_LOG_TRACE ("[LinkDevice::StartCsma] (" << m_nodeId << ":" << m_id
                   << ") start CCA");

  int NB = 0, BE = LinkDevice::MIN_BE;
  boost::random::uniform_int_distribution<> rand (0, (1 << BE) - 1);
  long backoff = rand (m_engine) * LinkDevice::BACKOFF_PERIOD;

  NDNEM_LOG_TRACE ("[LinkDevice::StartCsma] (" << m_nodeId << ":" << m_id
                   << ") set csma timer in " << backoff << " us for CCA");

  m_csmaTimer.expires_from_now (boost::posix_time::microseconds (backoff));
  m_csmaTimer.async_wait
    (boost::bind (&LinkDevice::DoCsma, this, NB, BE, _1));
}

void
LinkDevice::DoCsma (int NB, int BE, const boost::system::error_code& error)
{
  if (error)
    {
      NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                       << ") csma timer cancelled");
      //m_txQueue.clear ();
      return;
    }

  assert (m_txQueue.size () >= 1);
  NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                   << ") prior state = " << PhyStateToString (m_state));

  switch (m_state)
    {
    case IDLE:
      {
        NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") channel clear after " << NB << " backoffs. Start Tx");
        m_state = TX;

        // Send the message to the link asynchronously
        boost::shared_ptr<Packet>& pkt = m_txQueue.front ();
        m_ioService.post (boost::bind (&Link::Transmit, m_link, m_nodeId, pkt));

        // Set timer to clear TX state later
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1E6
            / (m_link->GetTxRate () * 1024.0)));

        NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") set csma timer in " << delay << " us for TX");

        m_csmaTimer.expires_from_now (boost::posix_time::microseconds (delay));
        m_csmaTimer.async_wait
          (boost::bind (&LinkDevice::DoCsma, this, -1, -1, _1));
      }
      break;

    case RX:
    case RX_COLLIDE:
      {
        NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") channel busy after " << NB << " backoffs");
        NB = NB + 1;
        if (NB > LinkDevice::MAX_CSMA_BACKOFFS)
          {
            NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
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
        if (BE > LinkDevice::MAX_BE)
          BE = LinkDevice::MAX_BE;
        boost::random::uniform_int_distribution<> rand (0, (1 << BE) - 1);
        long backoff = rand (m_engine) * LinkDevice::BACKOFF_PERIOD;

        NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                         << ") set csma timer in " << backoff << " us for backoff");

        m_csmaTimer.expires_from_now (boost::posix_time::microseconds (backoff));
        m_csmaTimer.async_wait
          (boost::bind (&LinkDevice::DoCsma, this, NB, BE, _1));
      }
      break;

    case TX:
      m_txQueue.pop_front ();

      NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                       << ") Tx finish. Queue size = " << m_txQueue.size ());
      m_state = IDLE;

      if (m_txQueue.size () != 0)
        {
          // Schedule tx of the next packet in queue
          this->StartCsma ();
        }

      break;

    default:
      NDNEM_LOG_ERROR ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                       << ") illegal state: " << PhyStateToString (m_state));
      throw std::runtime_error ("[LinkDevice::DoCsma] illegal state: " + PhyStateToString (m_state));
      break;

    }
  NDNEM_LOG_TRACE ("[LinkDevice::DoCsma] (" << m_nodeId << ":" << m_id
                   << ") after state = " << PhyStateToString (m_state));
}

} // namespace emulator
