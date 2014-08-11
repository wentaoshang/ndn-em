/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

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
  , m_ccaTimer (ioService)
  , m_csmaTimer (ioService)
  , m_state (IDLE) // PhyState.IDLE
{
  std::cout << "[LinkFace::LinkFace] (" << m_nodeId
            << ":" << m_id << ") on link " << m_link->GetId () << std::endl;
}


inline const std::string&
LinkFace::GetLinkId () const
{
  return m_link->GetId ();
}

void
LinkFace::StartRx (const boost::shared_ptr<Packet>& pkt)
{
  std::cout << "[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
            << ") prior state = " << PhyStateToString (m_state) << std::endl;
  switch (m_state)
    {
    case IDLE:
      {
        m_state = RX;
        m_pendingRx = pkt;
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1000.0 / (Link::TX_RATE * 1024.0)));
        m_rxTimer.expires_from_now (boost::posix_time::milliseconds (delay));
        m_rxTimer.async_wait
          (boost::bind (&LinkFace::PostRx, this, _1));
      }
      break;

    case CCA:
    case CCA_BAD:
      {
        std::cout << "[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                  << ") called while in CCA" << std::endl;
        m_state = CCA_BAD;

        // Clear channel if the colliding transmission finishes before CSMA backoff ends
        std::size_t pkt_len = pkt->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1000.0 / (Link::TX_RATE * 1024.0)));
        m_ccaTimer.cancel ();
        m_ccaTimer.expires_from_now (boost::posix_time::milliseconds (delay));
        m_ccaTimer.async_wait
          (boost::bind (&LinkFace::ResetCCA, this, _1));
      }
      break;

    case RX:
    case RX_COLLIDE:
      std::cout << "[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                << ") called while in RX" << std::endl;
      m_state = RX_COLLIDE;
      break;

    case TX:
      std::cout << "[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
                << ") called while in TX" << std::endl;
      break;

    default:
      throw std::runtime_error ("[LinkFace::StartRX] illegal state: "
                                + PhyStateToString (m_state));
      break;
    }
  std::cout << "[LinkFace::StartRx] (" << m_nodeId << ":" << m_id
            << ") after state = " << PhyStateToString (m_state) << std::endl;
}

void
LinkFace::PostRx (const boost::system::error_code& error)
{
  if (error)
    {
      std::cerr << "[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                << ") error = " << error.message () << std::endl;
      m_state = FAILURE;
      return;
    }

  std::cout << "[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
            << ") prior state = " << PhyStateToString (m_state) << std::endl;
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
      std::cout << "[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
                << ") RX failed due to packet collision" << std::endl;
      break;
    default:
      throw std::runtime_error ("[LinkFace::PostRx] illegal state: "
                                + PhyStateToString (m_state));
      break;
    }

  // Clear PHY state
  m_state = IDLE;
  std::cout << "[LinkFace::PostRx] (" << m_nodeId << ":" << m_id
            << ") after state = " << PhyStateToString (m_state) << std::endl;
}

void
LinkFace::StartTx (const boost::shared_ptr<Packet>& pkt)
{
  if (m_state != IDLE)
    {
      //TODO: implement sending queue
      std::cerr << "[LinkFace::StartTx] (" << m_nodeId << ":" << m_id
                << ") device not idle. Drop packet." << std::endl;
      return;
    }

  std::cerr << "[LinkFace::StartTx] (" << m_nodeId << ":" << m_id
            << ") start CCA" << std::endl;

  m_state = CCA;
  m_pendingTx = pkt;

  int NB = 0, BE = LinkFace::MIN_BE;
  boost::random::uniform_int_distribution<> rand (0, (1 << BE) - 1);
  long backoff = rand (m_engine) * LinkFace::BACKOFF_PERIOD;
  m_csmaTimer.expires_from_now (boost::posix_time::microseconds (backoff));
  m_csmaTimer.async_wait
    (boost::bind (&LinkFace::PostTx, this, NB, BE, _1));
}

void
LinkFace::PostTx (int NB, int BE, const boost::system::error_code& error)
{
  if (error)
    {
      std::cerr << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
                << ") error = " << error.message () << std::endl;
      m_state = FAILURE;
      return;
    }

  std::cout << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
            << ") prior state = " << PhyStateToString (m_state) << std::endl;

  m_ccaTimer.cancel ();

  switch (m_state)
    {
    case CCA:
      {
        std::cout << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
                  << ") channel clear after " << NB << " backoffs. Start tx" << std::endl;
        m_state = TX;

        // Send the message to the link
        m_link->Transmit (m_nodeId, m_pendingTx);

        // Set timer to clear TX state later
        std::size_t pkt_len = m_pendingTx->GetLength ();
        long delay = static_cast<long>
          ((static_cast<double> (pkt_len) * 8.0 * 1000.0 / (Link::TX_RATE * 1024.0)));
        m_csmaTimer.expires_from_now (boost::posix_time::milliseconds (delay));
        m_csmaTimer.async_wait
          (boost::bind (&LinkFace::PostTx, this, -1, -1, _1));
      }
      break;

    case CCA_BAD:
      {
        std::cout << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
                  << ") channel busy after " << NB << " backoffs" << std::endl;
        NB = NB + 1;
        if (NB > LinkFace::MAX_CSMA_BACKOFFS)
          {
            std::cout << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
                      << ") reach max backoff. Give up Tx" << std::endl;
            return;
          }
        BE = BE + 1;
        if (BE > LinkFace::MAX_BE)
          BE = LinkFace::MAX_BE;
        boost::random::uniform_int_distribution<> rand (0, (1 << BE) - 1);
        long backoff = rand (m_engine) * LinkFace::BACKOFF_PERIOD;
        m_csmaTimer.expires_from_now (boost::posix_time::microseconds (backoff));
        m_csmaTimer.async_wait
          (boost::bind (&LinkFace::PostTx, this, NB, BE, _1));
      }
      break;

    case TX:
      std::cout << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
                << ") TX finish" << std::endl;
      m_state = IDLE;
      break;

    default:
      throw std::runtime_error ("[LinkFace::PostTx] illegal state: "
                                + PhyStateToString (m_state));
      break;

    }
  std::cout << "[LinkFace::PostTx] (" << m_nodeId << ":" << m_id
            << ") after state = " << PhyStateToString (m_state) << std::endl;
}

} // namespace emulator
