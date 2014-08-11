/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_H__
#define __LINK_H__

#include <map>
#include <exception>
#include <boost/asio.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility.hpp>

#include "link-attribute.h"

namespace emulator {

const std::size_t LINK_MTU = 8800;  // unrealistic assumption

class LinkFace;

class Link : boost::noncopyable {
public:
  Link (const std::string& id)
    : m_id (id)
  {
  }

  const std::string&
  GetId ()
  {
    return m_id;
  }

  void
  AddNode (boost::shared_ptr<LinkFace>&);

  void
  AddConnection (const std::string& from, const std::string& to,
                 boost::shared_ptr<LinkAttribute>& attr)
  {
    m_linkMatrix[from][to] = attr;
  }

  void
  Transmit (const std::string&, const boost::shared_ptr<Packet>&);

  void
  PrintLinkMatrix ();

  static const double TX_RATE; // in kbit/s

private:
  const std::string m_id; // link id
  std::map<std::string, boost::shared_ptr<LinkFace> > m_nodeTable; // nodes on the link
  std::map<std::string, std::map<std::string, boost::shared_ptr<LinkAttribute> > > m_linkMatrix;
};

} // namespace emulator

#endif // __LINK_H__
