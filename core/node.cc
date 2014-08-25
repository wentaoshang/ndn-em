/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "node.h"

#include <iomanip>
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
  boost::shared_ptr<Node> self = this->shared_from_this ();
  boost::shared_ptr<AppFace> client =
    boost::make_shared<AppFace> (faceId, boost::ref (self),
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
  boost::shared_ptr<Node> self = this->shared_from_this ();
  boost::shared_ptr<AppFace> next =
    boost::make_shared<AppFace> (faceId, boost::ref (self),
                                 boost::ref (m_ioService));

  m_acceptor.async_accept (next->GetSocket (),
			   boost::bind (&Node::HandleAccept, this, next, _1));

  // Store accepted face in table
  m_faceTable[face->GetId ()] = face;

  face->Start ();
}

boost::shared_ptr<LinkDevice>
Node::AddDevice (const std::string& devId, const uint64_t macAddr,
                 boost::shared_ptr<Link>& link)
{
  if (m_deviceTable.find (devId) != m_deviceTable.end ())
    throw std::runtime_error ("[Node::AddDevice] failed to add device " + devId
                              + " to node " + m_id);

  boost::shared_ptr<Node> self = this->shared_from_this ();
  boost::shared_ptr<LinkDevice> dev =
    boost::make_shared<LinkDevice> (devId, macAddr,
                                    boost::ref (link),
                                    boost::ref (self),
                                    boost::ref (m_ioService));
  dev->AddBroadcastFace ();
  
  // Add device to device table
  // Assume mac address is globally unique!
  m_deviceTable[devId] = dev;

  return dev;
}

boost::shared_ptr<LinkFace>
Node::AddLinkFace (const uint64_t remoteMac, boost::shared_ptr<LinkDevice>& dev)
{
  int faceId = m_faceCounter++;
  boost::shared_ptr<Node> self = this->shared_from_this ();
  boost::shared_ptr<LinkFace> face =
    boost::make_shared<LinkFace> (faceId, boost::ref (self),
                                  boost::ref (m_ioService), remoteMac,
                                  boost::ref (dev));

  m_faceTable[faceId] = face;

  return face;
}

void
Node::AddRoute (const std::string& prefix, const std::string& devId, const uint64_t nexthop)
{
  std::map<std::string, boost::shared_ptr<LinkDevice> >::iterator it
    = m_deviceTable.find (devId);
  if (it != m_deviceTable.end ())
    {
      boost::shared_ptr<LinkDevice>& dev = it->second;
      boost::optional<boost::shared_ptr<LinkFace> > face = dev->GetLinkFace (nexthop);
      if (!face)
        face = AddLinkFace (nexthop, dev);

      int faceId = (*face)->GetId ();
      ndn::Name p (prefix);
      m_fib.AddRoute (p, faceId);
    }
  else
    {
      NDNEM_LOG_ERROR ("[Node::AddRoute] (" << m_id << ") dev " << devId
                       << " doesn't exist in local device table");
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
      boost::shared_ptr<Packet> pkt (boost::make_shared<DataPacket> (d));
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
          boost::shared_ptr<Packet> pkt (boost::make_shared<InterestPacket> (i));
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

      boost::shared_ptr<Packet> pkt (boost::make_shared<DataPacket> (d));
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
  std::cout << "  Unix socket path: " << m_socketPath << std::endl;
  std::cout << "  Cache limit: " << (m_cacheManager.GetLimit () >> 10) << " KB" << std::endl;
  std::map<std::string, boost::shared_ptr<LinkDevice> >::iterator it;
  std::cout << "  Device table:" << std::endl;
  for (it = m_deviceTable.begin (); it != m_deviceTable.end (); it++)
    {
      std::cout << "    id: " << it->first << ", mac: 0x" << std::hex
                << std::setfill ('0') << std::setw (4)
                << it->second->GetMacAddr () << std::dec << std::endl;
    }
  std::cout << "  FIB:" << std::endl;
  m_fib.Print ("    ");
}

} // namespace emulator
