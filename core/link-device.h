/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_DEVICE_H__
#define __LINK_DEVICE_H__

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <deque>

#include "packet.h"

namespace emulator {

class Link;
class Node;
class LinkFace;

class LinkDevice : public boost::enable_shared_from_this<LinkDevice>, boost::noncopyable {
public:
  LinkDevice (const std::string& id,
              const uint64_t macAddr,
	      boost::shared_ptr<Link>& link,
	      boost::shared_ptr<Node>& node,			
	      boost::asio::io_service& ioService,
	      const std::size_t txLimit = 5);

  // CSMA/CA constants
  static const int SYMBOL_TIME;
  static const int BACKOFF_PERIOD;
  static const int MIN_BE;
  static const int MAX_BE;
  static const int MAX_CSMA_BACKOFFS;

  enum PhyState {
    IDLE = 0,
    TX,
    RX,
    RX_COLLIDE,
    SLEEP = 254,
    FAILURE = 255
  };

  static const std::string
  PhyStateToString (PhyState s)
  {
    switch (s)
      {
      case IDLE:
        return "IDLE";
      case TX:
        return "TX";
      case RX:
        return "RX";
      case RX_COLLIDE:
        return "RX_COLLIDE";
      case SLEEP:
        return "SLEEP";
      case FAILURE:
        return "FAILURE";
      default:
        return "";
      }
  }

  boost::shared_ptr<Link>
  GetLink () const
  {
    return m_link;
  }

  uint64_t
  GetMacAddr () const
  {
    return m_macAddr;
  }

  boost::optional<boost::shared_ptr<LinkFace> >
  GetLinkFace (uint64_t remoteMac)
  {
    std::map<uint64_t, boost::shared_ptr<LinkFace> >::iterator it =
      m_faces.find (remoteMac);
    if (it != m_faces.end ())
      return it->second;
    else
      return boost::none;
  }

  void
  AddBroadcastFace ();

  void
  StartRx (const boost::shared_ptr<Packet>&);

  void
  StartTx (boost::shared_ptr<Packet>&);

private:
  void
  PostRx (const boost::system::error_code&);

  void
  StartCsma ();

  void
  DoCsma (int, int, const boost::system::error_code&);

private:
  const std::string m_id;
  const uint64_t m_macAddr;
  const std::string& m_nodeId;
  boost::shared_ptr<Link> m_link;
  boost::shared_ptr<Node> m_node;

  boost::asio::io_service& m_ioService;
  boost::asio::deadline_timer m_rxTimer;  // emulating transmission delay
  boost::asio::deadline_timer m_csmaTimer; // implementing CSMA algorithm
  PhyState m_state;
  boost::shared_ptr<Packet> m_pendingRx;
  std::deque<boost::shared_ptr<Packet> > m_txQueue;  // FIFO queue
  const std::size_t m_txQueueLimit;
  boost::random::mt19937 m_engine;

  std::map<uint64_t, boost::shared_ptr<LinkFace> > m_faces;
};

} // namespace emulator

#endif // __LINK_DEVICE_H__
