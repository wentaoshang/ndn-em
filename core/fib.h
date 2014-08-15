/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __FIB_H__
#define __FIB_H__

#include <ndn-cxx/name.hpp>
#include <boost/unordered_map.hpp>
#include <set>
#include <iostream>

#include "ndn-name-hash.h"

namespace emulator {
namespace node {

class Fib {
public:
  explicit
  Fib (const std::string& nodeId)
    : m_nodeId (nodeId)
  {
  }

  typedef boost::unordered_map<ndn::Name, std::set<int>, ndn_name_hash> fib_type;

  void
  AddRoute (const ndn::Name& prefix, const int faceId)
  {
    // Research question: Currently we use broadcast when sending packet on
    // the local link. If we want to use L2 unicast, we need to include L2
    // addresses in the L3 routing table. Is that necessary for sensor networks,
    // given that the wireless channel is already broadcast in nature?

    m_fib[prefix].insert (faceId);

    //TODO: inherit from parent prefixes
  }

  void
  CleanUp (const int faceId)
  {
    fib_type::iterator it = m_fib.begin ();
    while (it != m_fib.end ())
      {
        it->second.erase (faceId);
        if (it->second.empty ())
          {
            it = m_fib.erase (it);
          }
        else
          {
            it++;
          }
      }
  }

  void
  LookUp (const ndn::Name&, std::set<int>&);

  void
  Print (const std::string& = "");

private:
  const std::string& m_nodeId;
  //TODO: support cost for each route
  boost::unordered_map<ndn::Name, std::set<int>, ndn_name_hash> m_fib;
};

} // namespace node
} // namespace emulator

#endif // __FIB_H__
