/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link-face.h"
#include "link.h"

namespace emulator {

const int LinkFace::SYMBOL_TIME = 16;  // 16 us
const int LinkFace::BACKOFF_PERIOD = 20 * LinkFace::SYMBOL_TIME;
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
  , m_delayTimer (ioService)
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
        m_delayTimer.expires_from_now (boost::posix_time::milliseconds (delay));
        m_delayTimer.async_wait
          (boost::bind (&LinkFace::PostRx, this, _1));
      }
      break;

    case RX:
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
  m_link->Transmit (m_nodeId, pkt);
}

} // namespace emulator
