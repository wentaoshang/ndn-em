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
  Link (const std::string& id, boost::asio::io_service& ioService)
    : m_id (id)
    , m_busy (false)
    , m_txRate (250.0) // 250 kbit/s
    , m_delayTimer (ioService)
  {
  }

  const std::string&
  GetId ()
  {
    return m_id;
  }

  bool
  IsBusy ()
  {
    return m_busy;
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
  Transmit (const std::string&, const uint8_t*, std::size_t);

  void
  PrintLinkMatrix ();

private:
  void
  PostTransmit (const std::string&, std::size_t, const boost::system::error_code&);

private:
  const std::string m_id; // link id
  uint8_t m_pipe[LINK_MTU]; // link pipe
  bool m_busy;
  double m_txRate; // in kbit/s
  //TODO: use multiple timers for different nodes and emulate per-node delay
  boost::asio::deadline_timer m_delayTimer;
  std::map<std::string, boost::shared_ptr<LinkFace> > m_nodeTable; // nodes on the link
  std::map<std::string, std::map<std::string, boost::shared_ptr<LinkAttribute> > > m_linkMatrix;
};

} // namespace emulator

#endif // __LINK_H__
