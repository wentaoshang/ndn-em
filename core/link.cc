/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "link.h"
#include "node.h"

namespace emulator {

void
Link::Transmit (const std::string& nodeId, const uint8_t* data, std::size_t length)
{
  std::string msg (reinterpret_cast<const char*> (data), length);
  std::cout << "[Link::Transmit] src = " << nodeId << ": " << msg << std::endl;

  // Transmit to other nodes on the link according to link attribute matrix
  std::map<std::string, boost::shared_ptr<LinkAttribute> >& neighbors = m_linkMatrix[nodeId];
  std::map<std::string, boost::shared_ptr<LinkAttribute> >::iterator it;
  for (it = neighbors.begin (); it != neighbors.end (); it++)
    {
      std::cout << "[Link::Transmit] dst = " << it->first
                << ", IsConnected = " << it->second->IsConnected () << std::endl;
      if (it->second->IsConnected ())
        {
          m_nodeTable[it->first]->HandleLinkMessage (m_id, data, length);
        }
    }
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
                    << ", IsConnected = " << inner->second->IsConnected () << std::endl;
        }
    }
}

} // namespace emulator
