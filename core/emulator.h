/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <map>

#include "logging.h"
#include "link-face.h"
#include "link.h"
#include "node.h"

namespace emulator {

class Emulator {
public:
  void
  ReadNetworkConfig (const std::string& path);

  Node&
  GetNode (const std::string& nodeId)
  {
    std::map<std::string, boost::shared_ptr<Node> >::iterator it = m_nodeTable.find (nodeId);

    if (it != m_nodeTable.end ())
      return *(it->second);
    else
      throw std::invalid_argument ("Node not found");
  }

  void
  PrintLinks ();

  void
  PrintNodes ();

  void
  Start ();

  void
  Stop ()
  {
    m_ioService.stop ();
  }

private:
  boost::asio::io_service m_ioService;
  std::map<std::string, boost::shared_ptr<Node> > m_nodeTable; // all emulated nodes
  std::map<std::string, boost::shared_ptr<Link> > m_linkTable; // all emulated links
};

} // namespace emulator

#endif // __EMULATOR_H__
