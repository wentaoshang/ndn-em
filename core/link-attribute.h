/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_ATTRIBUTE_H__
#define __LINK_ATTRIBUTE_H__

#include <iostream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/bernoulli_distribution.hpp>

namespace emulator {

class LinkAttribute {
public:
  LinkAttribute ()
    : m_random (0.0)
  {
  }

  LinkAttribute (double loss)
    : m_random (loss)
  {
  }

  double
  GetLossRate () const
  {
    return m_random.p ();
  }

  bool
  DropPacket ()
  {
    return m_random (m_engine);
  }

private:
  boost::random::mt19937 m_engine;
  boost::random::bernoulli_distribution<> m_random; // random generator with bernoulli distribution
};

inline std::ostream&
operator<< (std::ostream& os, const LinkAttribute& attr)
{
  os << "LinkAttribute: LossRate = " << attr.GetLossRate ();
  return os;
}

} // namespace emulator

#endif // __LINK_ATTRIBUTE_H__
