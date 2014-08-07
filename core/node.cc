/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link.h"
#include "node.h"

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

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

  // Schedule clean up routine for PIT
  m_pit.ScheduleCleanUp ();

  // Setup fib manager
  m_fibManager = boost::make_shared<node::FibManager>
    (0, m_id, boost::ref (m_fib), boost::cref (m_faceTable),
     boost::bind (&Node::HandleData, this, _1, _2));

  // Setup cache manager
  m_cacheManager = boost::make_shared<node::CacheManager>
    (m_id, 1000, 10000, boost::ref (m_ioService));
  m_cacheManager->ScheduleCleanUp ();

  // Wait for connection from clients
  int faceId = m_faceCounter++;
  boost::shared_ptr<AppFace> client =
    boost::make_shared<AppFace> (faceId, m_id,
                                 boost::ref (m_ioService),
                                 boost::bind (&Node::HandleFaceMessage, this, _1, _2),
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
                                 boost::ref (m_ioService),
                                 boost::bind (&Node::HandleFaceMessage, this, _1, _2),
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
 * Handle message received from link
 */
void
Node::HandleLinkMessage (const std::string& linkId, const uint8_t* data, std::size_t length)
{
  int faceId = m_linkTable[linkId];
  // Try to parse message data
  bool isOk = true;
  ndn::Block element;
  isOk = ndn::Block::fromBuffer (data, length, element);
  if (!isOk)
    {
      throw std::runtime_error ("Incomplete packet in buffer");
    }

  //this->HandleFaceMessage (faceId, element);
  // Execute the message callback asynchronously
  m_ioService.post (boost::bind (&Node::HandleFaceMessage, this, faceId, element));
}


/**
 * Handle message received from local face
 */
void
Node::HandleFaceMessage (const int faceId, const ndn::Block& element)
{
  //std::cout << "[Node::HandleFaceMessage] (" << m_id << ":" << faceId
  //          << ") element type = " << element.type () << std::endl;
  try
    {
      if (element.type () == ndn::Tlv::Interest)
        {
          boost::shared_ptr<ndn::Interest> i = boost::make_shared<ndn::Interest> ();
          i->wireDecode (element);
          this->HandleInterest (faceId, i);
        }
      else if (element.type () == ndn::Tlv::Data)
        {
          boost::shared_ptr<ndn::Data> d = boost::make_shared<ndn::Data> ();
          d->wireDecode (element);
          this->HandleData (faceId, d);
        }
      else
        throw std::runtime_error ("Unknown NDN packet type");
    }
  catch (ndn::Tlv::Error&)
    {
      throw std::runtime_error ("NDN packet decoding error");
    }
}

void
Node::HandleInterest (const int faceId, const boost::shared_ptr<ndn::Interest>& i)
{
  std::cout << "[Node::HandleInterest] (" << m_id << ":" << faceId << ") " << (*i) << std::endl;

  // Check cache
  boost::shared_ptr<ndn::Data> d;
  if (m_cacheManager->FindMatchingData (i, d))
    {
      std::cout << "[Node::HandleInterest] (" << m_id << ":" << faceId
                << ") Match found in cache" << std::endl;
      const ndn::Block& wire = d->wireEncode ();
      const uint8_t* data = wire.wire ();
      std::size_t length = wire.size ();
      this->ForwardToFace (data, length, faceId);
      return;
    }

  // Record interest in PIT
  if (m_pit.AddInterest (faceId, i))
    {
      std::set<int> outList;
      m_fib.LookUp (i->getName (), outList);
      if (outList.empty ())
        {
          //std::cerr << "[Node::HandleInterest] (" << m_id << ":" << faceId
          //          << ") no route to name " << i->getName () << std::endl;

          // If there is no route for the interest, broadcast to all faces
          // except the one where the interest comes from
          const ndn::Block& wire = i->wireEncode ();
          const uint8_t* data = wire.wire ();
          std::size_t length = wire.size ();
          std::map<int, boost::shared_ptr<Face> >::iterator it;
          for (it = m_faceTable.begin (); it != m_faceTable.end (); it++)
            {
              if (it->first != faceId)
                {
                  it->second->Send (data, length);
                }
            }
          return;
        }

      std::set<int>::iterator it = outList.find (0);
      if (it != outList.end ())
        {
          // This interest should go to fib manager
          m_fibManager->ProcessCommand (faceId, i);
        }
      else
        {
          // Forward to faces
          const ndn::Block& wire = i->wireEncode ();
          const uint8_t* data = wire.wire ();
          std::size_t length = wire.size ();
          this->ForwardToFaces (data, length, outList);
        }
    }
  else
    {
      std::cout << "[Node::HandleInterest] (" << m_id << ":" << faceId
                << ") Looping Interest with nonce " << i->getNonce () << " detected." << std::endl;
    }

  //m_pit.Print ();
}

void
Node::HandleData (const int faceId, const boost::shared_ptr<ndn::Data>& d)
{
  std::cout << "[Node::HandleData] (" << m_id << ":" << faceId << ") " << (*d);
  std::set<int> outList;
  m_pit.ConsumeInterestWithDataName (d->getName (), outList);

  if (!outList.empty ())
    {
      // Cache the data only when we have pending interest for it
      m_cacheManager->Insert (d);

      const ndn::Block& wire = d->wireEncode ();
      const uint8_t* data = wire.wire ();
      std::size_t length = wire.size ();
      this->ForwardToFaces (data, length, outList);
    }
}

void
Node::RemoveFace (const int faceId)
{
  std::cout << "[Node::RemoveFace] id = " << m_id << ": remove face " << faceId << std::endl;
  m_faceTable.erase (faceId);
  m_fibManager->CleanUpFib (faceId);
}

} // namespace emulator
