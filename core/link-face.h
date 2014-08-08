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
            const boost::function<void (const int, const ndn::Block&)>&
            nodeMessageCallback,
            boost::shared_ptr<Link> link,
            boost::asio::io_service& ioService);

  const std::string&
  GetLinkId ();

  void
  HandleLinkMessage (const uint8_t* data, std::size_t length);

  virtual void
  Send (const uint8_t* data, std::size_t length);

private:
  const std::string& m_linkId;
  boost::shared_ptr<Link> m_link;
  boost::asio::io_service& m_ioService;
};

} // namespace emulator

#endif // __LINK_FACE_H__
