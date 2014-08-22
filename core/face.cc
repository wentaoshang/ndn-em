/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "face.h"
#include "node.h"

namespace emulator {

Face::Face (const int faceId, boost::shared_ptr<Node>& node,
            boost::asio::io_service& ioService)
  : m_id (faceId)
  , m_nodeId (node->GetId ())
  , m_node (node)
  , m_ioService (ioService)
{
  NDNEM_LOG_TRACE ("[Face::Face] (" << m_nodeId << ":" << m_id << ")");
}

void
Face::Dispatch (const ndn::Block& blk)
{
  NDNEM_LOG_TRACE ("[Face::Dispatch] (" << m_nodeId << ":" << m_id
                   << ") packet type = " << blk.type ());
  try
    {
      if (blk.type () == ndn::Tlv::Interest)
        {
          boost::shared_ptr<ndn::Interest> i (boost::make_shared<ndn::Interest> ());
          i->wireDecode (blk);
          m_node->HandleInterest (m_id, i);
        }
      else if (blk.type () == ndn::Tlv::Data)
        {
          boost::shared_ptr<ndn::Data> d (boost::make_shared<ndn::Data> ());
          d->wireDecode (blk);
          m_node->HandleData (m_id, d);
        }
      else
        throw std::runtime_error ("Unknown NDN packet type");
    }
  catch (ndn::Tlv::Error&)
    {
      throw std::runtime_error ("NDN packet decoding error");
    }
}

} // namespace emulator
