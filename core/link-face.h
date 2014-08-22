/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_FACE_H__
#define __LINK_FACE_H__

#include <boost/asio.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <deque>

#include "face.h"

namespace emulator {

class LinkDevice;

class LinkFace : public Face {
public:
  LinkFace (const int faceId, boost::shared_ptr<Node> node,
            boost::asio::io_service& ioService,
            const uint64_t remoteMac,
            boost::shared_ptr<LinkDevice>& dev);

  void
  HandleReceive (const ndn::Block& blk)
  {
    this->Dispatch (blk);
  }

  virtual void
  Send (boost::shared_ptr<Packet>& pkt);

private:
  const uint64_t m_remoteMac;
  boost::shared_ptr<LinkDevice> m_device;
};

} // namespace emulator

#endif // __LINK_FACE_H__
