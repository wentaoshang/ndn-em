/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <map>

#include "link.h"
#include "node.h"

namespace emulator {

class Emulator {
public:
  void
  ReadNodeConfig (const char* path);

  void
  ReadLinkConfig (const char* path);

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
