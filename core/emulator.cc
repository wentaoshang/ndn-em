/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "emulator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <boost/lexical_cast.hpp>

namespace emulator {

/**
 * Node config file is in the following space-separated format:
 *   [id] [unix-socket-path] [app-file-path]
 *
 * The line started with a ';' is treated as comment and ignored.
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

      // For now, ignore app file path
      m_nodeTable[nodeId] =
	boost::make_shared<Node> (nodeId, socketPath, boost::ref (m_ioService),
                                  boost::bind (&Emulator::DispatchMessage, this, _1, _2, _3));
    }
}

/**
 * Link config file is in the following space-separated format:
 *   [nodeID1] [nodeID2] [link-paramters] ...
 * which represents a link from nodeID1 to nodeID2.
 *
 * The line started with a ';' is treated as comment and ignored.
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
      std::string from, to, param;
      iss >> from >> to >> param;

      // For now, only support m_connected flag
      m_linkMatrix[from][to] = boost::make_shared<Link> (boost::lexical_cast<bool> (param));
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

void
Emulator::DispatchMessage (const std::string& nodeId, const uint8_t* data, std::size_t length)
{
  std::string msg (reinterpret_cast<const char*> (data), length);
  std::cout << "[Emulator::DispatchMessage] Node = " << nodeId << ": " << msg << std::endl;

  // For now, implement broadcast link
  std::map<std::string, boost::shared_ptr<Link> >& neighbors = m_linkMatrix[nodeId];
  std::map<std::string, boost::shared_ptr<Link> >::iterator it;
  for (it = neighbors.begin (); it != neighbors.end (); it++)
    {
      if (it->second->IsConnected ())
        {
          std::cout << "[Emulator::DoispatchMessage] forward to Node " << it->first << std::endl;
          m_nodeTable[it->first]->Forward (data, length);
        }
    }
}

} // namespace emulator
