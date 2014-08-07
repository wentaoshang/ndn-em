/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link.h"
#include "node.h"

namespace emulator {

void
Link::Transmit (const std::string& nodeId, const uint8_t* data, std::size_t length)
{
  // Check for collision
  if (m_busy)
    {
      std::cout << "[Link::Transmit] node " << nodeId << " tries to transmit while link "
                << m_id << " is busy" << std::endl;
      m_delayTimer.cancel ();
      return;
    }

  m_busy = true;

  // Make a copy of the data into a local "pipe" buffer
  assert (length <= MAX_NDN_PACKET_SIZE);
  std::copy (data, data + length, m_pipe);

  // Schedule delayed transmission
  //TODO: implement per-node delay based on tx rate
  long delay = static_cast<long> ((static_cast<double> (length) * 8.0 * 1000.0 / (m_txRate * 1024.0)));
  m_delayTimer.expires_from_now (boost::posix_time::milliseconds (delay));
  m_delayTimer.async_wait
    (boost::bind (&Link::PostTransmit, this, boost::cref (nodeId), length, _1));
}

void
Link::PostTransmit (const std::string& nodeId, std::size_t dataLen,
                    const boost::system::error_code& error)
{
  if (error)
    {
      std::cerr << "[Link::PostTransmit] (" << m_id << ") error = "
                << error.message () << std::endl;
      m_busy = false;
      return;
    }

  // Transmit to other nodes on the link according to link attribute matrix
  std::map<std::string, boost::shared_ptr<LinkAttribute> >& neighbors = m_linkMatrix[nodeId];
  std::map<std::string, boost::shared_ptr<LinkAttribute> >::iterator it;
  for (it = neighbors.begin (); it != neighbors.end (); it++)
    {
      std::cout << "[Link::PostTransmit] (" << m_id << ") " << nodeId << " -> " << it->first
                << ", LossRate = " << it->second->GetLossRate () << std::endl;
      if (!it->second->DropPacket ())
        {
          m_nodeTable[it->first]->HandleLinkMessage (m_id, m_pipe, dataLen);
        }
    }

  // Clear channel
  m_busy = false;
}

void
Link::PrintLinkMatrix ()
{
  std::map<std::string, std::map<std::string, boost::shared_ptr<LinkAttribute> > >::iterator outer;
  for (outer = m_linkMatrix.begin (); outer != m_linkMatrix.end (); outer++)
    {
      std::map<std::string, boost::shared_ptr<LinkAttribute> >& neighbors = outer->second;
      std::map<std::string, boost::shared_ptr<LinkAttribute> >::iterator inner;
      for (inner = neighbors.begin (); inner != neighbors.end (); inner++)
        {
          std::cout << "[Link::PrintLinkMatrix] from " << outer->first << " to " << inner->first
                    << ", LossRate = " << inner->second->GetLossRate () << std::endl;
        }
    }
}

} // namespace emulator
