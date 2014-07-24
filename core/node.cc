/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "node.h"

namespace emulator {

Node::~Node ()
{
  if (m_isListening)
    {
      // Ignore errors
      boost::system::error_code error;
      m_acceptor.close (error);
      boost::filesystem::remove (m_socketPath, error);
    }
}

void
Node::Start ()
{
  //TODO: check socket path for conflict
  boost::filesystem::remove (m_socketPath);

  m_acceptor.open ();
  m_acceptor.bind (m_endpoint);
  m_acceptor.listen ();
  m_isListening = true;

  int faceId = m_faceCounter++;
  boost::shared_ptr<Face> client =
    boost::make_shared<Face> (faceId,
                              boost::ref (m_acceptor.get_io_service ()),
                              boost::bind (&Node::HandleMessage, this, _1, _2, _3),
                              boost::bind (&Node::RemoveFace, this, _1));

  m_acceptor.async_accept (client->GetSocket (),
			   boost::bind (&Node::HandleAccept, this, client, _1));
}

void
Node::Forward (const uint8_t* data, std::size_t length)
{
  //TODO: implement FIB (for Interest) & PIT (for Data)

  // For now, broadcast to all faces
  std::map<int, boost::shared_ptr<Face> >::iterator it;
  for (it = m_faceTable.begin (); it != m_faceTable.end (); it++)
    {
      it->second->Send (data, length);
    }
}

void
Node::HandleAccept (const boost::shared_ptr<Face>& face,
		    const boost::system::error_code& error)
{
  // Wait for the next client to connect
  int faceId = m_faceCounter++;
  boost::shared_ptr<Face> next =
    boost::make_shared<Face> (faceId,
                              boost::ref (m_acceptor.get_io_service ()),
                              boost::bind (&Node::HandleMessage, this, _1, _2, _3),
                              boost::bind (&Node::RemoveFace, this, _1));

  m_acceptor.async_accept (next->GetSocket (),
			   boost::bind (&Node::HandleAccept, this, next, _1));

  // Store accepted face in table
  m_faceTable[face->GetId ()] = face;

  face->Start ();
}

void
Node::HandleMessage (const int faceId, const uint8_t* data, std::size_t length)
{
  std::string msg (reinterpret_cast<const char*> (data), length);
  std::cout << "[Node::HandleMessage] id = " << m_id << ", Face = " << faceId << ": " << msg << std::endl;

  //TODO: implement PIT

  // Pass message to emulator core
  m_emulatorCallback (m_id, data, length);
}

void
Node::RemoveFace (const int faceId)
{
  //std::cout << "[Node::RemoveFace] id = " << m_id << ": remove face " << faceId << std::endl;
  m_faceTable.erase (faceId);
}

} // namespace emulator
