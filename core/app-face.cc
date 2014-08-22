/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "app-face.h"
#include "node.h"

namespace emulator {

void
AppFace::HandleReceive (const boost::system::error_code& error,
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
	  this->Dispatch (element);
	}

      if (!isOk && m_inputBufferSize == ndn::MAX_NDN_PACKET_SIZE && offset == 0)
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
						   ndn::MAX_NDN_PACKET_SIZE - m_inputBufferSize), 0,
			      boost::bind (&AppFace::HandleReceive, this, _1, _2));

    }
  else
    {
      NDNEM_LOG_TRACE ("[AppFace::HandleReceive] (" << m_nodeId
                       << ":" << m_id << ") error = " << error.message ());
      m_node->RemoveFace (m_id);
    }
}

} // namespace emulator
