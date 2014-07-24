/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __DUMMY_REPEATOR_H__
#define __DUMMY_REPEATOR_H__

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

const std::size_t MAX_NDN_PACKET_SIZE = 200;

class DummyRepeater {
public:
  DummyRepeater (long interval, const std::string& path,
		 boost::asio::io_service& ioService)
    : m_interval (boost::posix_time::milliseconds (interval))
    , m_path (path)
    , m_endpoint (m_path)
    , m_socket (ioService)
    , m_counter (0)
    , m_timer (ioService)
  {
    m_socket.async_connect (m_endpoint,
			    boost::bind (&DummyRepeater::OnConnect, this, _1));
  }

private:
  void
  SendAndRepeat (const boost::system::error_code& error)
  {
    if (!error)
      {
        std::string msg;
        msg.append ("Interval = ").append (boost::lexical_cast<std::string> (m_interval))
          .append (" SEQ = ").append (boost::lexical_cast<std::string> (m_counter++));

        std::cout << "[DummyRepeater::SendAndRepeat] msg = " << msg << std::endl;

        m_socket.async_send (boost::asio::buffer (msg.c_str (), msg.length ()),
                             boost::bind (&DummyRepeater::HandleSend, this, _1, _2));

        m_timer.expires_from_now (m_interval);
        m_timer.async_wait (boost::bind (&DummyRepeater::SendAndRepeat, this, _1));
      }
    else
      std::cerr << "[DummyRepeater::SendAndRepeat] error = " << error.message () << std::endl;
  }

  void
  HandleSend (const boost::system::error_code& error, std::size_t nBytesTransferred)
  {
    if (error)
      {
        std::cerr << "[DummyRepeater::HandleSend] " << error.message () << std::endl;
        m_timer.cancel ();
      }
  }

  void
  OnConnect (const boost::system::error_code& error)
  {
    if (!error)
      {
	m_socket.async_receive (boost::asio::buffer (m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
				boost::bind (&DummyRepeater::OnReceive, this, _1, _2));

        SendAndRepeat (boost::system::error_code ());
      }
  }

  void
  OnReceive (const boost::system::error_code& error, std::size_t nBytesReceived)
  {
    if (!error)
      {
        std::string msg (reinterpret_cast<char*> (m_inputBuffer), nBytesReceived);
        std::cout << "[DummyRepeater::OnReceive]: " << msg << std::endl;

        m_socket.async_receive (boost::asio::buffer (m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
				boost::bind (&DummyRepeater::OnReceive, this, _1, _2));
      }
  }

private:
  boost::posix_time::time_duration m_interval;
  const std::string m_path;
  boost::asio::local::stream_protocol::endpoint m_endpoint;
  boost::asio::local::stream_protocol::socket m_socket; // connect socket
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE]; // receive buffer
  int m_counter; // internal sequence counter
  boost::asio::deadline_timer m_timer;
};

#endif // __DUMMY_REPEATOR_H__
