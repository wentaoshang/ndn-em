/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __FACE_H__
#define __FACE_H__

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include <ndn-cxx/encoding/block.hpp>

#include "packet.h"

namespace emulator {

class Face : boost::noncopyable {
public:
  Face (const int faceId, const std::string& nodeId,
        const boost::function<void (const int, const ndn::Block&)>& nodeMessageCallback)
    : m_id (faceId)
    , m_nodeId (nodeId)
    , m_nodeMessageCallback (nodeMessageCallback)
  {
  }

  virtual
  ~Face ()
  {
    std::cout << "[Face::~Face] (" << m_nodeId << ":" << m_id << ")" << std::endl;
  }

  int
  GetId () const
  {
    return m_id;
  }

  const std::string&
  GetNodeId () const
  {
    return m_nodeId;
  }

  virtual void
  Send (const boost::shared_ptr<Packet>&) = 0;

protected:
  const int m_id;  // face id
  const std::string& m_nodeId;  // node id
  const boost::function<void (const int, const ndn::Block&)> m_nodeMessageCallback;
};

} // namespace emulator

#endif // __FACE_H__
