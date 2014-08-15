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

#include "logging.h"
#include "app-face.h"
#include "link-face.h"
#include "pit.h"
#include "fib.h"
#include "fib-manager.h"
#include "cache-manager.h"

namespace emulator {

class Node : boost::noncopyable {
public:
  Node (const std::string& id, const std::string& socketPath,
        int cacheLimit, boost::asio::io_service& ioService)
    : m_id (id)
    , m_socketPath (socketPath)
    , m_endpoint (m_socketPath)
    , m_ioService (ioService)
    , m_acceptor (ioService)
    , m_isListening (false)
    , m_faceCounter (1)  // face id 0 is reserved for fib manager
    , m_pit (10000, ioService)  // Cleanup Pit every 10 sec
    , m_cacheManager (m_id, cacheLimit, ioService)
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

  bool
  AddLink (const boost::shared_ptr<Link>&, boost::shared_ptr<LinkFace>&);

  void
  AddRoute (const std::string&, const std::string&);

  void
  Start ();

  void
  PrintInfo ();

private:
  void
  HandleAccept (const boost::shared_ptr<AppFace>&,
                const boost::system::error_code&);

  void
  HandleFaceMessage (const int, const ndn::Block&);

  void
  ForwardToFace (const boost::shared_ptr<Packet>& pkt, int outId)
  {
    // Check whether the face still exists
    // It is possible that the face sent an
    // interest and then crashes. There is no
    // mechanism to remove dead faces from PIT.
    std::map<int, boost::shared_ptr<Face> >::iterator fit = m_faceTable.find (outId);
    if (fit != m_faceTable.end ())
      fit->second->Send (pkt);
  }

  void
  ForwardToFaces (const boost::shared_ptr<Packet>& pkt, std::set<int>& out)
  {
    // Forward to the faces listed in out face list
    std::set<int>::iterator it;
    for (it = out.begin (); it != out.end (); it++)
      {
        this->ForwardToFace (pkt, *it);
      }
  }

  void
  HandleInterest (const int, const boost::shared_ptr<ndn::Interest>&);

  void
  HandleData (const int, const boost::shared_ptr<ndn::Data>&);

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

  // FIB
  node::Fib m_fib;

  boost::shared_ptr<node::FibManager> m_fibManager;

  // CS
  node::CacheManager m_cacheManager;
};

} // namespace emulator

#endif // __NODE_H__
