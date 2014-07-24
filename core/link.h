/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_H__
#define __LINK_H__

#include <map>
#include <exception>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>

#include "link-attribute.h"

namespace emulator {

class Node;

class Link : boost::noncopyable {
public:
  Link (const std::string& id, boost::asio::io_service& ioService)
    : m_id (id)
    , m_ioService (ioService)
  {
  }

  const std::string&
  GetId ()
  {
    return m_id;
  }

  void
  AddNode (const std::string& id, boost::shared_ptr<Node>& node)
  {
    m_nodeTable[id] = node;
  }

  void
  AddConnection (const std::string& from, const std::string& to, boost::shared_ptr<LinkAttribute>& attr)
  {
    m_linkMatrix[from][to] = attr;
  }

  void
  Transmit (const std::string&, const uint8_t*, std::size_t);

  void
  PrintLinkMatrix ();

private:
  const std::string m_id; // link id
  std::map<std::string, boost::shared_ptr<Node> > m_nodeTable; // nodes on the link
  std::map<std::string, std::map<std::string, boost::shared_ptr<LinkAttribute> > > m_linkMatrix;

  boost::asio::io_service& m_ioService;
};

} // namespace emulator

#endif // __LINK_H__
