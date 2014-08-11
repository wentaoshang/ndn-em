/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_FACE_H__
#define __LINK_FACE_H__

#include <boost/asio.hpp>
#include <boost/function.hpp>

#include "face.h"

namespace emulator {

class Link;

class LinkFace : public Face {
public:
  LinkFace (const int faceId, const std::string& nodeId,
            boost::asio::io_service& ioService,
            const boost::function<void (const int, const ndn::Block&)>&
            nodeMessageCallback,
            boost::shared_ptr<Link> link);

  const std::string&
  GetLinkId () const;

  void
  HandleLinkMessage (const boost::shared_ptr<Packet>& pkt)
  {
    this->StartRx (pkt);
  }

  virtual void
  Send (const boost::shared_ptr<Packet>& pkt)
  {
    this->StartTx (pkt);
  }

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

private:
  void
  StartRx (const boost::shared_ptr<Packet>&);

  void
  PostRx (const boost::system::error_code&);

  void
  StartTx (const boost::shared_ptr<Packet>&);

  void
  PostTx (const boost::system::error_code&);

private:
  boost::shared_ptr<Link> m_link;
  boost::asio::deadline_timer m_delayTimer;
  PhyState m_state;
  boost::shared_ptr<Packet> m_pendingRx;
};

} // namespace emulator

#endif // __LINK_FACE_H__
