/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __PACKET_H__
#define __PACKET_H__

#include <boost/shared_ptr.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

namespace emulator {

// Wrapper classes for Interest and Data objects.
// Used by face sending routines.

/*
 * Provide a common base class for NDN packet wrappers
 */
class Packet {
protected:
  Packet (const uint64_t dst, const uint64_t src,
          const ndn::Block& wire)
    : m_dst (dst)
    , m_src (src)
    , m_wire (wire)
  {
  }

  virtual
  ~Packet ()
  {
  }

public:
  uint64_t
  GetDstMac () const
  {
    return m_dst;
  }

  uint64_t
  GetSrcMac () const
  {
    return m_src;
  }

  const ndn::Block&
  GetBlock () const
  {
    return m_wire;
  }

  uint32_t
  GetType () const
  {
    return m_wire.type ();
  }

  const uint8_t*
  GetBytes () const
  {
    return m_wire.wire ();
  }

  std::size_t
  GetLength () const
  {
    return m_wire.size ();
  }

protected:
  const uint64_t m_dst;
  const uint64_t m_src;
  const ndn::Block& m_wire;
};

/*
 * Wrapper for Interest packet
 */
class InterestPacket : public Packet {
public:
  InterestPacket (const uint64_t dst, const uint64_t src,
                  const boost::shared_ptr<ndn::Interest>& i)
    : Packet (dst, src, i->wireEncode ())
    , m_i (i)
  {
  }

  const boost::shared_ptr<ndn::Interest>&
  GetInterest () const
  {
    return m_i;
  }

private:
  const boost::shared_ptr<ndn::Interest> m_i;
};

/*
 * Wrapper for Data packet
 */
class DataPacket : public Packet {
public:
  DataPacket (const uint64_t dst, const uint64_t src,
              const boost::shared_ptr<ndn::Data>& d)
    : Packet (dst, src, d->wireEncode ())
    , m_d (d)
  {
  }

  const boost::shared_ptr<ndn::Data>&
  GetData () const
  {
    return m_d;
  }

private:
  const boost::shared_ptr<ndn::Data> m_d;
};

} // namespace emulator

#endif // __PACKET_H__
