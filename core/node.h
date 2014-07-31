/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __NODE_H__
#define __NODE_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <set>

#include "app-face.h"
#include "link-face.h"
#include "pit.h"

namespace emulator {

class Node : boost::noncopyable {
public:
  Node (const std::string& id, const std::string& socketPath,
        boost::asio::io_service& ioService)
    : m_id (id)
    , m_socketPath (socketPath)
    , m_endpoint (m_socketPath)
    , m_ioService (ioService)
    , m_acceptor (ioService)
    , m_isListening (false)
    , m_faceCounter (0)
  {
  }

  ~Node ();

  const std::string&
  GetId () const
  {
    return m_id;
  }

  const std::string&
  GetPath () const
  {
    return m_socketPath;
  }

  void
  AddLink (const std::string&, boost::shared_ptr<Link>&);

  void
  Start ();

  void
  HandleLinkMessage (const std::string& linkId, const uint8_t* data, std::size_t length)
  {
    int faceId = m_linkTable[linkId];
    this->HandleFaceMessage (faceId, data, length);
  }

private:
  void
  HandleAccept (const boost::shared_ptr<AppFace>&,
                const boost::system::error_code&);

  void
  HandleFaceMessage (const int, const uint8_t*, std::size_t);

  void
  HandleInterest (const int, const boost::shared_ptr<ndn::Interest>&, std::set<int>&);

  void
  HandleData (const int, const boost::shared_ptr<ndn::Data>&, std::set<int>&);

  void
  RemoveFace (const int);

private:
  std::string m_id; // node id
  std::string m_socketPath; // unix domain socket path
  boost::asio::local::stream_protocol::endpoint m_endpoint; // local listening endpoint
  boost::asio::io_service& m_ioService;
  boost::asio::local::stream_protocol::acceptor m_acceptor; // local listening socket
  bool m_isListening;

  int m_faceCounter;
  std::map<int, boost::shared_ptr<Face> > m_faceTable;

  // Map from link id to local face id
  std::map<std::string, int> m_linkTable;

  // PIT
  node::Pit m_pit;

  //TODO: implement CS and FIB
};

} // namespace emulator

#endif // __NODE_H__
