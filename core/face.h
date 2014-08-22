/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __FACE_H__
#define __FACE_H__

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include <ndn-cxx/encoding/block.hpp>

#include "logging.h"
#include "packet.h"

namespace emulator {

class Node;

class Face : boost::noncopyable {
protected:
  Face (const int faceId, boost::shared_ptr<Node>& node,
        boost::asio::io_service& ioService);

  virtual
  ~Face ()
  {
    NDNEM_LOG_TRACE ("[Face::~Face] (" << m_nodeId << ":" << m_id << ")");
  }

public:
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
  Send (boost::shared_ptr<Packet>&) = 0;

  void
  Dispatch (const ndn::Block&);

protected:
  const int m_id;  // face id
  const std::string& m_nodeId; // node id
  boost::shared_ptr<Node> m_node;
  boost::asio::io_service& m_ioService;
};

} // namespace emulator

#endif // __FACE_H__
