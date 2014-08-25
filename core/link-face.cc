/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link-face.h"
#include "link-device.h"
#include "node.h"
#include <iomanip>

namespace emulator {

LinkFace::LinkFace (const int faceId, boost::shared_ptr<Node> node,
                    boost::asio::io_service& ioService,
                    const uint64_t remoteMac,
                    boost::shared_ptr<LinkDevice>& dev)
  : Face (faceId, node, ioService)
  , m_remoteMac (remoteMac)
  , m_device (dev)
{
  NDNEM_LOG_TRACE ("[LinkFace::LinkFace] (" << m_nodeId << ":" << m_id
                   << ") remote mac 0x" << std::hex << std::setfill ('0')
                   << std::setw (4) << m_remoteMac << std::dec);
}

inline void
LinkFace::Send (boost::shared_ptr<Packet>& pkt)
{
  pkt->SetDst (m_remoteMac);
  m_device->StartTx (pkt);
}


//TODO: remove link face after certain time of inactivity

} // namespace emulator
