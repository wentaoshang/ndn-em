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
  typedef boost::unordered_map<ndn::Name, std::set<int>, ndn_name_hash> fib_type;

  void
  AddRoute (const ndn::Name& prefix, int faceId)
  {
    m_fib[prefix].insert (faceId);

    //TODO: inherit from parent prefixes
  }

  void
  LookUp (const ndn::Name& name, std::set<int>& out)
  {
    // If we use longest prefix match, we need to inherit
    // outgoing faces from parent prefixes. Or we don't
    // inherit but use "match-all" semantics. This allows
    // simpler "add/delete route" operations but will
    // make lookup operation very slow. Therefore we
    // should use "LPM with inheritance" semantics for
    // faster lookup speed.

    for (int i = name.size (); i >= 0; i--)
      {
        ndn::Name prefix = name.getPrefix (i);
        fib_type::iterator it = m_fib.find (prefix);
        if (it != m_fib.end ())
          {
            // Found match, stop now and copy all faces to "out"
            std::set<int>& faces = it->second;
            out.insert (faces.begin (), faces.end ());
          }
      }
  }

private:
  //TODO: support cost for each route
  boost::unordered_map<ndn::Name, std::set<int>, ndn_name_hash> m_fib;
};

} // namespace node
} // namespace emulator

#endif // __FIB_H__
