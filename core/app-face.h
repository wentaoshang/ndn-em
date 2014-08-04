/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __APP_FACE_H__
#define __APP_FACE_H__

#include "face.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>

#include <ndn-cxx/encoding/block.hpp>

namespace emulator {

const std::size_t MAX_NDN_PACKET_SIZE = 8800;

class AppFace : public Face {
public:
  AppFace (const int faceId, const std::string& nodeId, boost::asio::io_service& ioService,
           const boost::function<void (const int, const ndn::Block&)>& nodeMessageCallback,
           const boost::function<void (const int)>& closeFaceCallback)
    : Face (faceId, nodeId)
    , m_nodeMessageCallback (nodeMessageCallback)
    , m_closeFaceCallback (closeFaceCallback)
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
    m_socket.async_receive (boost::asio::buffer (m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                            boost::bind (&AppFace::HandleReceive, this, _1, _2));
  }

  virtual void
  Send (const uint8_t* data, std::size_t length)
  {
    m_socket.async_send (boost::asio::buffer (data, length),
                         boost::bind (&AppFace::HandleSend, this, _1, _2));
  }

private:
  void
  HandleSend (const boost::system::error_code& error, std::size_t nBytesTransforred)
  {
    if (error)
      {
        std::cerr << "[AppFace::HandleSend] (" << this->GetNodeId ()
                  << ":" << this->GetId () << ") error = " << error.message () << std::endl;
        //TODO: close face
      }
  }
  
  void
  HandleReceive (const boost::system::error_code& error,
                 std::size_t nBytesReceived)
  {
    if (!error)
      {
        // Try to parse message data
        m_inputBufferSize += nBytesReceived;

        std::size_t offset = 0;

        bool isOk = true;
        ndn::Block element;
        while (m_inputBufferSize - offset > 0)
          {
            isOk = ndn::Block::fromBuffer (m_inputBuffer + offset, m_inputBufferSize - offset, element);
            if (!isOk)
              break;

            offset += element.size();

            // Pass message to the node
            m_nodeMessageCallback (this->GetId (), element);
          }

        if (!isOk && m_inputBufferSize == MAX_NDN_PACKET_SIZE && offset == 0)
          {
            throw std::runtime_error ("Incoming packet too large to process");
          }

        if (offset > 0)
          {
            if (offset != m_inputBufferSize)
              {
                std::copy (m_inputBuffer + offset, m_inputBuffer + m_inputBufferSize,
                           m_inputBuffer);
                m_inputBufferSize -= offset;
              }
            else
              {
                m_inputBufferSize = 0;
              }
          }

        m_socket.async_receive (boost::asio::buffer (m_inputBuffer + m_inputBufferSize,
                                                     MAX_NDN_PACKET_SIZE - m_inputBufferSize), 0,
                                boost::bind (&AppFace::HandleReceive, this, _1, _2));

      }
    else
      {
        //std::cout << "[AppFace::HandleReceive] (" << this->GetNodeId ()
        //          << ":" << this->GetId () << ") error = " << error.message () << std::endl;
        m_closeFaceCallback (this->GetId ());
      }
  }

private:
  const boost::function<void (const int, const ndn::Block&)> m_nodeMessageCallback;
  const boost::function<void (const int)> m_closeFaceCallback;
  boost::asio::local::stream_protocol::socket m_socket; // receive socket
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE]; // receive buffer
  std::size_t m_inputBufferSize;
};

} // namespace emulator

#endif // __APP_FACE_H__
