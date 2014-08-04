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
#include "fib.h"
#include "fib-manager.h"

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
    , m_faceCounter (1)  // face id 0 is reserved for fib manager
    , m_pit (3000, ioService)  // Cleanup Pit every 3 sec
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
  HandleLinkMessage (const std::string&, const uint8_t*, std::size_t);

private:
  void
  HandleAccept (const boost::shared_ptr<AppFace>&,
                const boost::system::error_code&);

  void
  HandleFaceMessage (const int, const ndn::Block&);

  void
  ForwardToFaces (const uint8_t* data, std::size_t length, std::set<int>& out)
  {
    // Forward to the faces listed in out face list
    std::set<int>::iterator it;
    for (it = out.begin (); it != out.end (); it++)
      {
        m_faceTable[*it]->Send (data, length);
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

  boost::shared_ptr<FibManager> m_fibManager;

  //TODO: implement CS
};

} // namespace emulator

#endif // __NODE_H__
