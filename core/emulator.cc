/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>

#include "emulator.h"

namespace emulator {

/**
 * Node config file is in the following space-separated format:
 *   [id] [unix-socket-path] [app-file-path]
 *
 * The line started with a ';' is treated as comment and ignored.
 *
 * This function will create the nodes without adding links.
 */
void
Emulator::ReadNodeConfig (const char* path)
{
  std::ifstream is (path);
  if (!is.is_open ())
    throw std::invalid_argument ("Error opening node config file");

  std::cout << "[Emulator::ReadNodeConfig] " << path << std::endl;

  std::string line;
  while (std::getline (is, line))
    {
      if (line[0] == ';')  // ignore comment line
	continue;

      std::istringstream iss (line);
      std::string nodeId, socketPath, appPath;
      iss >> nodeId >> socketPath >> appPath;

      //TODO: sanitation check

      // For now, ignore app file path
      m_nodeTable[nodeId] =
	boost::make_shared<Node> (nodeId, socketPath, boost::ref (m_ioService));
    }
}

/**
 * Link config file is in the following space-separated format:
 *   [srcID] [dstID] [linkID] [link-paramters] ...
 * which represents a link from srcID to dstID.
 *
 * The line started with a ';' is treated as comment and ignored.
 *
 * This function will add links to nodes and connection between nodes on the link.
 */
void
Emulator::ReadLinkConfig (const char* path)
{
  std::ifstream is (path);
  if (!is.is_open ())
    throw std::invalid_argument ("Error opening link config file");

  std::cout << "[Emulator::ReadLinkConfig] " << path << std::endl;

  std::string line;
  while (std::getline (is, line))
    {
      if (line[0] == ';')  // ignore comment line
	continue;

      std::istringstream iss (line);
      std::string from, to, linkId, param;
      iss >> from >> to >> linkId >> param;

      boost::shared_ptr<Link> link;
      std::map<std::string, boost::shared_ptr<Link> >::iterator it = m_linkTable.find (linkId);
      if (it == m_linkTable.end ())
        {
          link = boost::make_shared<Link> (linkId, boost::ref (m_ioService));
          m_linkTable[linkId] = link;
        }
      else
        link = it->second;

      // For now, only support packet loss rate
      boost::shared_ptr<LinkAttribute> attr = boost::make_shared<LinkAttribute> (boost::lexical_cast<double> (param));

      //TODO: sanitation check

      // Add nodes & connections to link
      link->AddNode (from, m_nodeTable[from]);
      link->AddNode (to, m_nodeTable[to]);
      link->AddConnection (from, to, attr);

      // Add links to nodes
      m_nodeTable[from]->AddLink (linkId, link);
      m_nodeTable[to]->AddLink (linkId, link);
    }

  std::map<std::string, boost::shared_ptr<Link> >::iterator it0;
  for (it0 = m_linkTable.begin (); it0 != m_linkTable.end (); it0++)
    {
      std::cout << "[Emulator::ReadLinkConfig] LinkMatrix for " << it0->first << ":" << std::endl;
      it0->second->PrintLinkMatrix ();
    }
}

void
Emulator::Start ()
{
  std::map<std::string, boost::shared_ptr<Node> >::iterator it;
  for (it = m_nodeTable.begin (); it != m_nodeTable.end (); it++)
    {
      it->second->Start ();
    }

  m_ioService.run (); // This call will block
}

} // namespace emulator
