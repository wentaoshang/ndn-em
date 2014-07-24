/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <map>
#include <exception>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>

#include "node.h"
#include "link.h"

namespace emulator {

class Emulator : boost::noncopyable {
public:
  void
  ReadNodeConfig (const char* path);

  void
  ReadLinkConfig (const char* path);

  Node&
  GetNode (const std::string& id)
  {
    std::map<std::string, boost::shared_ptr<Node> >::iterator it = m_nodeTable.find (id);

    if (it != m_nodeTable.end ())
      return *(it->second);
    else
      throw std::invalid_argument ("Node id not found");
  }

  void
  Start ();

  void
  Stop ()
  {
    m_ioService.stop ();
  }

private:
  void
  DispatchMessage (const std::string&, const uint8_t*, std::size_t);

private:
  std::map<std::string, boost::shared_ptr<Node> > m_nodeTable;
  std::map<std::string, std::map<std::string, boost::shared_ptr<Link> > > m_linkMatrix;

  boost::asio::io_service m_ioService;
};

} // namespace emulator

#endif // __EMULATOR_H__
