/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef __DUMMY_SENSOR_H__
#define __DUMMY_SENSOR_H__

#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include "sensor-info.h"

namespace ndnsensor {

class Sensor {
public:
  enum OptMode {
    POLL,
    PUSH,
    NOTIFY
  };

  Sensor (OptMode mode, const std::string& path)
    : m_mode (mode)
    , m_path (path)
    , m_transport (ndn::make_shared<ndn::UnixTransport> (path))
    , m_face (m_transport, m_ioService)
    , m_scheduler (m_ioService)
    , m_sequence (0)
    , m_reading (75)
  {
  }

  void
  HandleInterest (const ndn::Name&, const ndn::Interest&);

  void
  HandleRegisterFailed (const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon (" << reason << ")"
	      << std::endl;
    m_face.shutdown ();
    m_ioService.stop ();
  }

  void
  SchedulePush ();

  void
  HandlePushAck (const ndn::Interest& interest, ndn::Data& data)
  {
    std::cout << "[PushAck] <<D: " << data.getName () << std::endl;
  }

  void
  HandlePushTimeout (const ndn::Interest& interest)
  {
    std::cerr << "[Timeout] I: " << interest.getName () << std::endl;
  }

  void
  ScheduleNotify ();

  void
  HandleNotifyAck (const ndn::Interest& interest, ndn::Data& data)
  {
    std::cout << "[NotifyAck] <<D: " << data.getName () << std::endl;
  }

  void
  HandleNotifyTimeout (const ndn::Interest& interest)
  {
    std::cerr << "[Timeout] I: " << interest.getName () << std::endl;
  }

  void
  ScheduleNewReading ();

  void
  Start ();

private:
  const OptMode m_mode;
  const std::string m_path;
  ndn::shared_ptr<ndn::UnixTransport> m_transport;
  boost::asio::io_service m_ioService;
  ndn::Face m_face;
  ndn::KeyChain m_keyChain;
  ndn::Scheduler m_scheduler;
  int m_sequence;
  uint32_t m_reading;
};


} // namespace ndnsensor

#endif // __DUMMY_SENSOR_H__
