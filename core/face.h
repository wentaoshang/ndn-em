/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>

namespace emulator {

const std::size_t MAX_NDN_PACKET_SIZE = 8800;

class Face : boost::noncopyable {
public:
  Face (const int faceId, boost::asio::io_service& ioService,
        const boost::function<void (const int, const uint8_t*, std::size_t)>& nodeMessageCallback,
        const boost::function<void (const int)>& closeFaceCallback)
    : m_id (faceId)
    , m_socket (ioService)
    , m_nodeMessageCallback (nodeMessageCallback)
    , m_closeFaceCallback (closeFaceCallback)
  {
  }

  ~Face ()
  {
    //std::cout << "[Face::~Face] id = " << m_id << std::endl;

    // Ignore errors
    boost::system::error_code error;
    m_socket.close (error);
  }

  int
  GetId ()
  {
    return m_id;
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
                            boost::bind (&Face::HandleReceive, this, _1, _2));
  }

  void
  Send (const uint8_t* data, std::size_t length)
  {
    m_socket.async_send (boost::asio::buffer (data, length),
                         boost::bind (&Face::HandleSend, this, _1, _2));
  }

private:
  void
  HandleSend (const boost::system::error_code& error, std::size_t nBytesTransforred)
  {
    if (error)
      {
        std::cerr << "[Face::HandleSend] id = " << m_id << " error = " << error.message () << std::endl;
        //TODO: close face
      }
  }
  
  void
  HandleReceive (const boost::system::error_code& error,
                 std::size_t nBytesReceived)
  {
    if (!error)
      {
        //TODO: Check message is valid and complete

        // Pass message to the node
        m_nodeMessageCallback (m_id, m_inputBuffer, nBytesReceived);

        // Wait for next message
        m_socket.async_receive (boost::asio::buffer (m_inputBuffer, MAX_NDN_PACKET_SIZE), 0,
                                boost::bind (&Face::HandleReceive, this, _1, _2));
      }
    else
      {
        //std::cout << "[Face::HandleReceive] id = " << m_id << " error = " << error.message () << std::endl;
        m_closeFaceCallback (m_id);
      }
  }

private:
  const int m_id;  // face id
  boost::asio::local::stream_protocol::socket m_socket; // receive socket
  uint8_t m_inputBuffer[MAX_NDN_PACKET_SIZE]; // receive buffer
  const boost::function<void (const int, const uint8_t*, std::size_t)> m_nodeMessageCallback;
  const boost::function<void (const int)> m_closeFaceCallback;
};

} // namespace emulator
