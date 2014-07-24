/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LINK_H__
#define __LINK_H__

#include <iostream>

namespace emulator {

class Link {
public:
  Link ()
    : m_connected (true)
  {
  }

  Link (bool connected)
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
operator<< (std::ostream& os, const Link& link)
{
  os << "Link::connected = " << link.IsConnected ();
  return os;
}

} // namespace emulator

#endif // __LINK_H__
