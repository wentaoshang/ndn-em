/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __NAME_HASH_H__
#define __NAME_HASH_H__

#include <boost/functional/hash.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/encoding/block.hpp>

namespace emulator {
namespace node {

struct ndn_name_hash
  : std::unary_function<ndn::Name, std::size_t>
{
  std::size_t operator() (const ndn::Name& n) const
  {
    const ndn::Block& b = n.wireEncode ();
    const uint8_t* wire = b.wire ();
    std::size_t size = b.size ();
    return boost::hash_range (wire, wire + size);
  }
};

} // namespace node
} // namespace emulator

#endif // __NAME_HASH_H__
