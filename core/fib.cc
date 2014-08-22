/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <sstream>
#include "logging.h"
#include "fib.h"

namespace emulator {
namespace node {

void
Fib::LookUp (const ndn::Name& name, std::set<int>& out)
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
          std::stringstream ss;
	  ss << "[Fib::LookUp] (" << m_nodeId << ") " << name << " ->";
	  std::set<int>::iterator fit;
	  for (fit = faces.begin (); fit != faces.end (); fit++)
	    {
	      ss << " " << *fit;
	    }
          NDNEM_LOG_TRACE (ss.str ());
	  out.insert (faces.begin (), faces.end ());
	}
    }
}

void
Fib::Print (const std::string& pad)
{
  fib_type::iterator it;
  for (it = m_fib.begin (); it != m_fib.end (); it++)
    {
      std::cout << pad << it->first << " -> faces:";
      std::set<int>& faces = it->second;
      std::set<int>::iterator fit;
      for (fit = faces.begin (); fit != faces.end (); fit++)
	{
	  std::cout << " " << *fit;
	}
      std::cout << std::endl;
    }
}

} // namespace node
} // namespace emulator
