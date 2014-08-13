/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef __INMEMORY_REPO_H__
#define __INMEMORY_REPO_H__

#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include "sensor-info.h"

namespace ndnsensor {

class Repo {
public:
  enum OptMode {
    POLL,
    PUSH,
    PHONE_HOME
  };

  Repo (OptMode mode, const std::string& path)
    : m_mode (mode)
    , m_path (path)
    , m_transport (ndn::make_shared<ndn::UnixTransport> (path))
    , m_face (m_transport, m_ioService)
    , m_scheduler (m_ioService)
    , m_sequence (-1)
  {
  }

  void
  Start ();

private:
  void
  HandleData (const ndn::Interest&, ndn::Data&);

  void
  HandleTimeout (const ndn::Interest& interest)
  {
    std::cerr << "Timeout: " << interest.getName () << std::endl;
  }

  void
  HandleSensorInterest (const ndn::Name&, const ndn::Interest&);

  void
  HandleUserInterest (const ndn::Name&, const ndn::Interest&);

  void
  HandleRegisterFailed (const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon (" << reason << ")"
	      << std::endl;
    m_face.shutdown ();
    m_ioService.stop ();
  }

  void
  PollData ();

private:
  const OptMode m_mode;
  const std::string m_path;
  ndn::shared_ptr<ndn::UnixTransport> m_transport;
  boost::asio::io_service m_ioService;
  ndn::Face m_face;
  ndn::KeyChain m_keyChain;
  ndn::Scheduler m_scheduler;
  std::map<int, ndn::shared_ptr<ndn::Data> > m_store;  // sensor data indexed by seq number
  int m_sequence;
};

} // namespace ndnsensor

#endif // __INMEMORY_REPO_H__
