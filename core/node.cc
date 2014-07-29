/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link.h"
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
  boost::shared_ptr<AppFace> client =
    boost::make_shared<AppFace> (faceId, m_id,
                                 boost::ref (m_acceptor.get_io_service ()),
                                 boost::bind (&Node::HandleFaceMessage, this, _1, _2, _3),
                                 boost::bind (&Node::RemoveFace, this, _1));

  m_acceptor.async_accept (client->GetSocket (),
			   boost::bind (&Node::HandleAccept, this, client, _1));
}

void
Node::HandleAccept (const boost::shared_ptr<AppFace>& face,
		    const boost::system::error_code& error)
{
  // Wait for the next client to connect
  int faceId = m_faceCounter++;
  boost::shared_ptr<AppFace> next =
    boost::make_shared<AppFace> (faceId, m_id,
                                 boost::ref (m_acceptor.get_io_service ()),
                                 boost::bind (&Node::HandleFaceMessage, this, _1, _2, _3),
                                 boost::bind (&Node::RemoveFace, this, _1));

  m_acceptor.async_accept (next->GetSocket (),
			   boost::bind (&Node::HandleAccept, this, next, _1));

  // Store accepted face in table
  m_faceTable[face->GetId ()] = face;

  face->Start ();
}

void
Node::AddLink (const std::string& linkId, boost::shared_ptr<Link>& link)
{
  if (m_linkTable.find (linkId) != m_linkTable.end ())
    return;

  int faceId = m_faceCounter++;
  boost::shared_ptr<LinkFace> face =
    boost::make_shared<LinkFace> (faceId, m_id,
                                  boost::bind (&Link::Transmit, link, _1, _2, _3));

  // Add link face to face table
  m_faceTable[faceId] = face;

  // Add link id to link table
  m_linkTable[linkId] = faceId;
}

/**
 * Handle message received from local face
 */
void
Node::HandleFaceMessage (const int faceId, const uint8_t* data, std::size_t length)
{
  std::string msg (reinterpret_cast<const char*> (data), length);
  std::cout << "[Node::HandleFaceMessage] (" << m_id << ":" << faceId << ") : " << msg << std::endl;

  //TODO: implement PIT, CS & FIB

  // For now, broadcast to all faces
  // except the face where the data is received
  std::map<int, boost::shared_ptr<Face> >::iterator it_face;
  for (it_face = m_faceTable.begin (); it_face != m_faceTable.end (); it_face++)
    {
      if (it_face->first != faceId)
        {
          it_face->second->Send (data, length);
        }
    }
}

void
Node::RemoveFace (const int faceId)
{
  //std::cout << "[Node::RemoveFace] id = " << m_id << ": remove face " << faceId << std::endl;
  m_faceTable.erase (faceId);
}

} // namespace emulator
