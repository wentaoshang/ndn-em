/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __NODE_H__
#define __NODE_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>
#include <map>
#include <set>

#include "logging.h"
#include "app-face.h"
#include "link-face.h"
#include "link.h"
#include "link-device.h"
#include "pit.h"
#include "fib.h"
#include "fib-manager.h"
#include "cache-manager.h"

namespace emulator {

class Node : public boost::enable_shared_from_this<Node>, boost::noncopyable {
public:
  Node (const std::string& id, const std::string& path,
        int cacheLimit, boost::asio::io_service& ioService)
    : m_id (id)
    , m_path (path)
    , m_endpoint (m_path)
    , m_ioService (ioService)
    , m_acceptor (ioService)
    , m_isListening (false)
    , m_faceCounter (1)  // face id 0 is reserved for fib manager
    , m_pit (10000, ioService)  // Cleanup Pit every 10 sec
    , m_fib (m_id)
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
    return m_path;
  }

  boost::shared_ptr<LinkDevice>
  AddDevice (const std::string&, const uint64_t, boost::shared_ptr<Link>&);

  boost::shared_ptr<LinkDevice>
  GetDevice (const std::string& devId)
  {
    std::map<std::string, boost::shared_ptr<LinkDevice> >::iterator it
      = m_deviceTable.find (devId);
      if (it == m_deviceTable.end ())
        throw std::runtime_error ("[Node::GetDevice] device " + devId
                                  + " doesn't exist on node " + m_id);
      else
        return it->second;
  }

  boost::shared_ptr<LinkFace>
  AddLinkFace (const uint64_t remoteMac, boost::shared_ptr<LinkDevice>& dev);

  void
  AddRoute (const std::string&, const std::string&, const uint64_t);

  void
  RemoveFace (const int);

  void
  Start ();

  void
  HandleInterest (const int, const boost::shared_ptr<ndn::Interest>&);

  void
  HandleData (const int, const boost::shared_ptr<ndn::Data>&);

  void
  PrintInfo ();

private:
  void
  HandleAccept (const boost::shared_ptr<AppFace>&,
                const boost::system::error_code&);

  void
  ForwardToFace (boost::shared_ptr<Packet>& pkt, int outId)
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
  ForwardToFaces (boost::shared_ptr<Packet>& pkt, std::set<int>& out)
  {
    // Forward to the faces listed in out face list
    std::set<int>::iterator it;
    for (it = out.begin (); it != out.end (); it++)
      {
        this->ForwardToFace (pkt, *it);
      }
  }

private:
  const std::string m_id; // node id
  const std::string m_path; // unix domain socket path
  boost::asio::local::stream_protocol::endpoint m_endpoint; // local listening endpoint
  boost::asio::io_service& m_ioService;
  boost::asio::local::stream_protocol::acceptor m_acceptor; // local listening socket
  bool m_isListening;

  int m_faceCounter;
  std::map<int, boost::shared_ptr<Face> > m_faceTable;

  // Layer-2 devices
  std::map<std::string, boost::shared_ptr<LinkDevice> > m_deviceTable;

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
