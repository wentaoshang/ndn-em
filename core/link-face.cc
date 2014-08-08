/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link-face.h"
#include "link.h"

namespace emulator {

LinkFace::LinkFace (const int faceId, const std::string& nodeId,
                    const boost::function<void (const int, const ndn::Block&)>&
                    nodeMessageCallback,
                    boost::shared_ptr<Link> link,
                    boost::asio::io_service& ioService)
  : Face (faceId, nodeId, nodeMessageCallback)
  , m_linkId (link->GetId ())
  , m_link (link)
  , m_ioService (ioService)
{
  std::cout << "[LinkFace::LinkFace] (" << m_nodeId
            << ":" << m_id << ") on link " << m_linkId << std::endl;
}


inline const std::string&
LinkFace::GetLinkId ()
{
  return m_link->GetId ();
}

void
LinkFace::HandleLinkMessage (const uint8_t* data, std::size_t length)
{
  // Try to parse message data
  bool isOk = true;
  ndn::Block element;
  isOk = ndn::Block::fromBuffer (data, length, element);
  if (!isOk)
    {
      throw std::runtime_error ("Incomplete packet in buffer");
    }

  // Execute the message callback asynchronously
  m_ioService.post (boost::bind (m_nodeMessageCallback, m_id, element));
}

void
LinkFace::Send (const uint8_t* data, std::size_t length)
{
  m_link->Transmit (m_nodeId, data, length);
}

} // namespace emulator
