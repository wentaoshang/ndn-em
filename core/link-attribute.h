/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_ATTRIBUTE_H__
#define __LINK_ATTRIBUTE_H__

#include <iostream>

namespace emulator {

class LinkAttribute {
public:
  LinkAttribute ()
    : m_connected (true)
  {
  }

  LinkAttribute (bool connected)
    : m_connected (connected)
  {
  }

  bool
  IsConnected () const
  {
    return m_connected;
  }

private:
  bool m_connected;
};

inline std::ostream&
operator<< (std::ostream& os, const LinkAttribute& attr)
{
  os << "LinkAttribute: IsConnected = " << attr.IsConnected ();
  return os;
}

} // namespace emulator

#endif // __LINK_ATTRIBUTE_H__
