/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>

class SimpleConsumer {
public:
  SimpleConsumer (long delay, const std::string& name, const std::string& path)
    : m_delay (delay)
    , m_name (name)
    , m_path (path)
    , m_transport (ndn::make_shared<ndn::UnixTransport> (path))
    , m_face (m_transport, m_ioService)
    , m_scheduler (m_ioService)
  {
  }

  void
  Start ()
  {
    this->SendInterest ();

    m_ioService.run ();
  }

private:
  void
  HandleData (const ndn::Interest& interest, ndn::Data& data)
  {
    std::cout << "<< D: " << data.getName () << std::endl;
  }

  void
  HandleTimeout (const ndn::Interest& interest)
  {
    std::cout << "Timeout" << std::endl;
  }

  void
  SendInterest ()
  {
    ndn::Interest i (m_name);
    i.setInterestLifetime (ndn::time::milliseconds (1000));
    i.setMustBeFresh (true);

    m_face.expressInterest (i,
                            ndn::bind (&SimpleConsumer::HandleData, this, _1, _2),
                            ndn::bind (&SimpleConsumer::HandleTimeout, this, _1));

    std::cout << ">> I: " << i.getName () << std::endl;

    if (m_delay > 0)
      // Schedule a new event
      m_scheduler.scheduleEvent (ndn::time::milliseconds (m_delay),
                                 ndn::bind (&SimpleConsumer::SendInterest, this));
  }

private:
  long m_delay;
  const ndn::Name m_name;
  const std::string m_path;
  ndn::shared_ptr<ndn::UnixTransport> m_transport;
  boost::asio::io_service m_ioService;
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
};
