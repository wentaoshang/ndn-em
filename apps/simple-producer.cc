/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <iostream>

using namespace ndn;

class SimpleProducer
{
public:
  SimpleProducer (const std::string& path)
    : m_path (path)
    , m_transport (ndn::make_shared<ndn::UnixTransport> (path))
    , m_face (m_transport, m_ioService)
  {
  }

  void
  onInterest (const Name& name, const Interest& interest)
  {
    std::cout << "<< I: " << interest << std::endl;

    // Create new name, based on Interest's name
    Name dataName (interest.getName ());
    dataName
      .append("123") // add "testApp" component to Interest name
      .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)

    static const std::string content = "HELLO KITTY";

    // Create Data packet
    Data data;
    data.setName (dataName);
    data.setFreshnessPeriod (time::seconds(10));
    data.setContent (reinterpret_cast<const uint8_t*> (content.c_str ()), content.size ());

    // Sign Data packet with default identity
    m_keyChain.sign (data);

    // Return Data packet to the requester
    std::cout << ">> D: " << data << std::endl;
    m_face.put (data);
  }

  void
  onRegisterFailed (const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix in local hub's daemon (" << reason << ")"
	      << std::endl;
    m_face.shutdown ();
  }

  void
  Run ()
  {
    m_face.setInterestFilter ("/test/app/data",
			      bind (&SimpleProducer::onInterest, this, _1, _2),
			      RegisterPrefixSuccessCallback (),
			      bind (&SimpleProducer::onRegisterFailed, this, _2));
    m_face.processEvents ();
  }

private:
  const std::string m_path;
  shared_ptr<ndn::UnixTransport> m_transport;
  boost::asio::io_service m_ioService;
  Face m_face;
  KeyChain m_keyChain;
};

int
main(int argc, char* argv[])
{
  try
    {
      SimpleProducer producer (argv[1]);
      producer.Run ();
    }
  catch (std::exception& e)
    {
      std::cerr << "ERROR: " << e.what () << std::endl;
    }
  return 0;
}
