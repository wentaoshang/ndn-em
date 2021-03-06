/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __APP_FACE_H__
#define __APP_FACE_H__

#include "face.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>

namespace emulator {

class AppFace : public Face {
public:
  AppFace (const int faceId, boost::shared_ptr<Node> node,
           boost::asio::io_service& ioService)
    : Face (faceId, node, ioService)
    , m_socket (ioService)
    , m_inputBufferSize (0)
  {
  }

  virtual
  ~AppFace ()
  {
    // Ignore errors
    boost::system::error_code error;
    m_socket.close (error);
  }

  boost::asio::local::stream_protocol::socket&
  GetSocket ()
  {
    return m_socket;
  }

  void
  Start ()
  {
    m_socket.async_receive (boost::asio::buffer (m_inputBuffer, ndn::MAX_NDN_PACKET_SIZE), 0,
                            boost::bind (&AppFace::HandleReceive, this, _1, _2));
  }

  virtual void
  Send (boost::shared_ptr<Packet>& pkt)
  {
    const uint8_t* data = pkt->GetBytes ();
    std::size_t length = pkt->GetLength ();
    m_socket.async_send (boost::asio::buffer (data, length),
                         boost::bind (&AppFace::HandleSend, this, _1, _2));
  }

private:
  void
  HandleSend (const boost::system::error_code& error, std::size_t nBytesTransforred)
  {
    if (error)
      {
        NDNEM_LOG_ERROR ("[AppFace::HandleSend] (" << m_nodeId
                         << ":" << m_id << ") error = " << error.message ());
        //TODO: close face
      }
  }
  
  void
  HandleReceive (const boost::system::error_code& error,
                 std::size_t nBytesReceived);

private:
  boost::asio::local::stream_protocol::socket m_socket; // receive socket
  uint8_t m_inputBuffer[ndn::MAX_NDN_PACKET_SIZE]; // receive buffer
  std::size_t m_inputBufferSize;
};

} // namespace emulator

#endif // __APP_FACE_H__
