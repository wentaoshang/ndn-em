/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

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
  m_cacheManager.ScheduleCleanUp ();

  // Wait for connection from clients
  int faceId = m_faceCounter++;
  boost::shared_ptr<AppFace> client =
    boost::make_shared<AppFace> (faceId, shared_from_this (),
                                 boost::ref (m_ioService));

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
    boost::make_shared<AppFace> (faceId, shared_from_this (),
                                 boost::ref (m_ioService));

  m_acceptor.async_accept (next->GetSocket (),
			   boost::bind (&Node::HandleAccept, this, next, _1));

  // Store accepted face in table
  m_faceTable[face->GetId ()] = face;

  face->Start ();
}

bool
Node::AddLink (const boost::shared_ptr<Link>& link, boost::shared_ptr<LinkFace>& out)
{
  const std::string& linkId = link->GetId ();

  if (m_linkTable.find (linkId) != m_linkTable.end ())
    return false;

  int faceId = m_faceCounter++;
  boost::shared_ptr<LinkFace> face =
    boost::make_shared<LinkFace> (faceId, shared_from_this (),
                                  m_macAddr, boost::ref (m_ioService), link);

  // Add link face to face table
  m_faceTable[faceId] = face;

  out = face;

  // Add link id to link table
  m_linkTable[linkId] = faceId;

  return true;
}

void
Node::AddRoute (const std::string& prefix, const std::string& linkId)
{
  std::map<std::string, int>::iterator it = m_linkTable.find (linkId);
  if (it != m_linkTable.end ())
    {
      int faceId = it->second;
      ndn::Name p (prefix);
      m_fib.AddRoute (p, faceId);
    }
  else
    {
      NDNEM_LOG_ERROR ("[Node::AddRoute] (" << m_id << ") link id " << linkId
                       << " doesn't exist in local link table");
    }
}

void
Node::HandleInterest (const int faceId, const boost::shared_ptr<ndn::Interest>& i)
{
  NDNEM_LOG_DEBUG ("[Node::HandleInterest] (" << m_id << ":" << faceId << ") " << (*i));

  // Check cache
  boost::shared_ptr<ndn::Data> d;
  if (m_cacheManager.FindMatchingData (i, d))
    {
      NDNEM_LOG_TRACE ("[Node::HandleInterest] (" << m_id << ":" << faceId
                       << ") found match in cache");
      const boost::shared_ptr<Packet> pkt
        (boost::make_shared<DataPacket> (0xffff, m_macAddr, d));
      this->ForwardToFace (pkt, faceId);
      return;
    }

  // Record interest in PIT
  if (m_pit.AddInterest (faceId, i))
    {
      std::set<int> outList;
      m_fib.LookUp (i->getName (), outList);
      //outList.erase (faceId);  // Do not forward back to incoming face (???)

      if (outList.empty ())
        {
          NDNEM_LOG_TRACE ("[Node::HandleInterest] (" << m_id << ":" << faceId
                           << ") no route to " << i->getName ());
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
          boost::shared_ptr<Packet> pkt
            (boost::make_shared<InterestPacket> (0xffff, m_macAddr, i));
          this->ForwardToFaces (pkt, outList);
        }
    }
  else
    {
      NDNEM_LOG_DEBUG ("[Node::HandleInterest] (" << m_id << ":" << faceId
                      << ") Looping Interest with nonce " << i->getNonce ());
    }

  //m_pit.Print ();
}

void
Node::HandleData (const int faceId, const boost::shared_ptr<ndn::Data>& d)
{
  NDNEM_LOG_DEBUG ("[Node::HandleData] (" << m_id << ":" << faceId << ") " << d->getName ());
  std::set<int> outList;
  m_pit.ConsumeInterestWithDataName (d->getName (), outList);

  if (!outList.empty ())
    {
      // Cache the data only when we have pending interest for it
      m_cacheManager.Insert (d);

      boost::shared_ptr<Packet> pkt
        (boost::make_shared<DataPacket> (0xffff, m_macAddr, d));
      this->ForwardToFaces (pkt, outList);
    }
  else
    NDNEM_LOG_DEBUG ("[Node::HandleData] (" << m_id << ":" << faceId << ") no pending interest");
}

void
Node::RemoveFace (const int faceId)
{
  NDNEM_LOG_TRACE ("[Node::RemoveFace] (" << m_id << ":" << faceId << ")");
  m_faceTable.erase (faceId);
  m_fibManager->CleanUpFib (faceId);
}

void
Node::PrintInfo ()
{
  std::cout << "Node id: " << m_id << std::endl;
  std::cout << "  MAC address: 0x" << std::hex << m_macAddr << std::dec << std::endl;
  std::cout << "  Unix socket path: " << m_socketPath << std::endl;
  std::cout << "  Cache limit: " << (m_cacheManager.GetLimit () >> 10) << " KB" << std::endl;
  std::map<std::string, int>::iterator it;
  std::cout << "  Link table:" << std::endl;
  for (it = m_linkTable.begin (); it != m_linkTable.end (); it++)
    {
      std::cout << "    faceId: " << it->second << ", linkId:" << it->first << std::endl; 
    }
  std::cout << "  FIB:" << std::endl;
  m_fib.Print ("    ");
}

} // namespace emulator
