/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link-face.h"
#include "link.h"

namespace emulator {

const double Link::TX_RATE = 40.0;  // kbit/s

void
Link::AddNode (boost::shared_ptr<LinkFace>& node)
{
  const std::string& id = node->GetNodeId ();
  m_nodeTable[id] = node;
}

void
Link::Transmit (const std::string& nodeId, const boost::shared_ptr<Packet>& pkt)
{
  // Transmit to other nodes on the link according to link attribute matrix
  std::map<std::string, boost::shared_ptr<LinkAttribute> >& neighbors = m_linkMatrix[nodeId];
  std::map<std::string, boost::shared_ptr<LinkAttribute> >::iterator it;
  for (it = neighbors.begin (); it != neighbors.end (); it++)
    {
      NDNEM_LOG_DEBUG ("[Link::Transmit] (" << m_id << ") " << nodeId << " -> " << it->first
                       << ", LossRate = " << it->second->GetLossRate ());
      if (!it->second->DropPacket ())
        {
          m_nodeTable[it->first]->HandleLinkMessage (pkt);
        }
      else
        NDNEM_LOG_DEBUG ("[Link::Transmit] (" << m_id << ") " << nodeId << " -> " << it->first
                         << ": drop packet");
    }
}

void
Link::PrintLinkMatrix (const std::string& pad)
{
  std::map<std::string, std::map<std::string, boost::shared_ptr<LinkAttribute> > >::iterator outer;
  for (outer = m_linkMatrix.begin (); outer != m_linkMatrix.end (); outer++)
    {
      std::map<std::string, boost::shared_ptr<LinkAttribute> >& neighbors = outer->second;
      std::map<std::string, boost::shared_ptr<LinkAttribute> >::iterator inner;
      for (inner = neighbors.begin (); inner != neighbors.end (); inner++)
        {
          std::cout << pad << "from " << outer->first
                    << " to " << inner->first << ", LossRate = "
                    << inner->second->GetLossRate () << std::endl;
        }
    }
}

} // namespace emulator
