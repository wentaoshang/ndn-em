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

#include "face.h"

namespace emulator {

class Node : boost::noncopyable {
public:
  Node (const std::string& id, const std::string& socketPath,
        boost::asio::io_service& ioService,
        const boost::function<void (const std::string&, const uint8_t*, std::size_t)>& callback)
    : m_id (id)
    , m_socketPath (socketPath)
    , m_endpoint (m_socketPath)
    , m_acceptor (ioService)
    , m_isListening (false)
    , m_faceCounter (0)
    , m_emulatorCallback (callback)
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
  Start ();

  void
  Forward (const uint8_t*, std::size_t);

private:
  void
  HandleAccept (const boost::shared_ptr<Face>&,
                const boost::system::error_code&);

  void
  HandleMessage (const int, const uint8_t*, std::size_t);

  void
  RemoveFace (const int);

private:
  std::string m_id; // node id
  std::string m_socketPath; // unix domain socket path
  boost::asio::local::stream_protocol::endpoint m_endpoint; // local listening endpoint
  boost::asio::local::stream_protocol::acceptor m_acceptor; // local listening socket
  bool m_isListening;
  int m_faceCounter;
  std::map <int, boost::shared_ptr<Face> > m_faceTable;
  // callback to notify received message to emulator
  const boost::function<void (const std::string&, const uint8_t*, std::size_t)> m_emulatorCallback;
};

} // namespace emulator

#endif // __NODE_H__
