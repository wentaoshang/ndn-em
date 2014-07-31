/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>

class SimpleConsumer {
public:
  SimpleConsumer (long delay, const std::string& path)
    : m_delay (ndn::time::milliseconds (delay))
    , m_path (path)
    , m_transport (ndn::make_shared<ndn::UnixTransport> (path))
    , m_face (m_transport, m_ioService)
    , m_scheduler (m_ioService)
  {
  }

  void
  Start ()
  {
    ndn::Interest i (ndn::Name ("/test/app/data"));
    i.setScope (1);
    i.setInterestLifetime (ndn::time::seconds(1));
    i.setMustBeFresh (true);

    m_face.expressInterest(i,
                           ndn::bind (&SimpleConsumer::HandleData, this, _1, _2),
                           ndn::bind (&SimpleConsumer::HandleTimeout, this, _1));

    // ioService.run() will block until all events finished or ioService.stop() is called
    m_ioService.run();
  }

private:
  void
  HandleData (const ndn::Interest& interest, ndn::Data& data)
  {
    std::cout << "I: " << interest.toUri () << std::endl;
    std::cout << "D: " << data.getName ().toUri () << std::endl;
  }

  void
  HandleTimeout (const ndn::Interest& interest)
  {
    std::cout << "Timeout" << std::endl;

    // Schedule a new event
    m_scheduler.scheduleEvent (m_delay,
                               ndn::bind (&SimpleConsumer::SendInterest, this));
  }

  void
  SendInterest ()
  {
    std::cout << "Interest scheduled by the cxx scheduler" << std::endl;

    ndn::Interest i (ndn::Name ("/test/app/data"));
    i.setScope (1);
    i.setInterestLifetime (ndn::time::milliseconds (1000));
    i.setMustBeFresh (true);

    m_face.expressInterest (i,
                            ndn::bind (&SimpleConsumer::HandleData, this, _1, _2),
                            ndn::bind (&SimpleConsumer::HandleTimeout, this, _1));
  }

private:
  ndn::time::system_clock::Duration m_delay;
  const std::string m_path;
  ndn::shared_ptr<ndn::UnixTransport> m_transport;
  boost::asio::io_service m_ioService;
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
};
