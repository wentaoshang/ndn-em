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

#include "logging.h"
#include "link-attribute.h"

namespace emulator {

class LinkDevice;

class Link : boost::noncopyable {
public:
  Link (const std::string& id, double rate, std::size_t mtu)
    : m_id (id)
    , m_txRate (rate)
    , m_mtu (mtu)
  {
  }

  const std::string&
  GetId () const
  {
    return m_id;
  }

  double
  GetTxRate () const
  {
    return m_txRate;
  }

  std::size_t
  GetMtu () const
  {
    return m_mtu;
  }

  void
  AddNodeDevice (const std::string& nodeId, boost::shared_ptr<LinkDevice>& dev)
  {
    m_nodeTable[nodeId] = dev;
  }

  boost::shared_ptr<LinkDevice>
  GetNodeDevice (const std::string& nodeId)
  {
    std::map<std::string, boost::shared_ptr<LinkDevice> >::iterator it
      = m_nodeTable.find (nodeId);
    if (it == m_nodeTable.end ())
      throw std::runtime_error ("[Link::GetNodeDevice] unkown node "
                                + nodeId + " on link " + m_id);
    else
      return it->second;
  }

  void
  AddConnection (const std::string& from, const std::string& to,
                 boost::shared_ptr<LinkAttribute>& attr)
  {
    if (from == to)
      throw std::runtime_error ("[Link::AddConnection] src & dst equal to " + from);

    if (m_nodeTable.find (from) == m_nodeTable.end ())
      throw std::runtime_error ("[Link::AddConnection] unknown node "
                                + from + " on link " + m_id);

    if (m_nodeTable.find (to) == m_nodeTable.end ())
      throw std::runtime_error ("[LinkAddConnection] unknown node "
                                + to + " on link " + m_id);

    m_linkMatrix[from][to] = attr;
  }

  void
  Transmit (const std::string&, const boost::shared_ptr<Packet>&);

  void
  PrintLinkMatrix (const std::string& = "");

  void
  PrintInfo ()
  {
    std::cout << "Link id: " << m_id << std::endl;
    std::cout << "  LinkMatrix: " << std::endl;
    this->PrintLinkMatrix ("    ");
  }

private:
  const std::string m_id; // link id
  const double m_txRate; // in kbits/s
  const std::size_t m_mtu;  // in bytes
  std::map<std::string, boost::shared_ptr<LinkDevice> > m_nodeTable; // nodes on the link
  std::map<std::string, std::map<std::string, boost::shared_ptr<LinkAttribute> > > m_linkMatrix;
};

} // namespace emulator

#endif // __LINK_H__
