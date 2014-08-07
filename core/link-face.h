/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_FACE_H__
#define __LINK_FACE_H__

#include "face.h"

namespace emulator {

class LinkFace : public Face {
public:
  LinkFace (const int faceId, const std::string& nodeId,
	    const boost::function<void (const std::string&, const uint8_t*, std::size_t)>& linkMessageCallback)
    : Face (faceId, nodeId)
    , m_linkMessageCallback (linkMessageCallback)
  {
    std::cout << "[LinkFace::LinkFace] (" << m_nodeId << ":" << m_id << ")" << std::endl;
  }

  virtual void
  Send (const uint8_t* data, std::size_t length)
  {
    m_linkMessageCallback (this->GetNodeId (), data, length);
  }

private:
  const boost::function<void (const std::string&, const uint8_t*, std::size_t)> m_linkMessageCallback;
};

} // namespace emulator

#endif // __LINK_FACE_H__
