/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "pit.h"

namespace emulator {
namespace node {

bool
PitEntry::AddNonce (const uint32_t nonce, const int faceId,
		    boost::chrono::system_clock::time_point& expire)
{
  std::map<uint32_t, FaceRecord>::iterator it;
  it = m_nonceTable.find (nonce);
  if (it == m_nonceTable.end ())
    {
      // Record nonce, incoming face id and expire time
      m_nonceTable.insert (std::make_pair<uint32_t, FaceRecord> (nonce, FaceRecord (faceId, expire)));
      return true;
    }
  else
    return false;
}

bool
Pit::AddInterest (const int faceId, const boost::shared_ptr<ndn::Interest>& i)
{
  boost::chrono::system_clock::time_point expire =
    boost::chrono::system_clock::now () + i->getInterestLifetime ();

  pit_type::iterator it = m_pit.find (i->getName ());
  if (it == m_pit.end ())
    {
      // No interest with the same name is in table yet
      boost::shared_ptr<PitEntry> entry = boost::make_shared<PitEntry> (i);
      entry->AddNonce (i->getNonce (), faceId, expire);
      m_pit.insert (std::make_pair<ndn::Name, boost::shared_ptr<PitEntry> > (i->getName (), entry));
      return true;
    }
  else
    {
      // Interest with the same name already exists
      boost::shared_ptr<PitEntry>& entry = it->second;
      return entry->AddNonce (i->getNonce (), faceId, expire);
    }
}

void
Pit::ConsumeInterestWithDataName (const ndn::Name& name, std::set<int>& out)
{
  boost::chrono::system_clock::time_point now =
    boost::chrono::system_clock::now ();

  pit_type::iterator it = m_pit.begin ();
  while (it != m_pit.end ())
    {
      if (it->first.isPrefixOf (name))
	{
	  // TODO: check selectors

	  // Record incoming faces
	  std::map<uint32_t, FaceRecord>::iterator nit;
	  for (nit = it->second->m_nonceTable.begin ();
	       nit != it->second->m_nonceTable.end (); nit++)
	    {
	      if (nit->second.expire > now)
		out.insert (nit->second.faceId);
	    }

	  // Remove this entry from pit
	  it = m_pit.erase (it); // this will return iterator for the next element
	}
      else
	it++;
    }
}

void
Pit::Print ()
{
  if (m_pit.empty ())
    {
      std::cout << "[Pit::Print] empty table" << std::endl;
      return;
    }

  std::cout << "[Pit::Print]" << std::endl;
  pit_type::iterator it;
  for (it = m_pit.begin (); it != m_pit.end (); it++)
    {
      std::cout << "  Name = " << it->first << std::endl;
      std::map<uint32_t, FaceRecord>::iterator nit;
      for (nit = it->second->m_nonceTable.begin ();
	   nit != it->second->m_nonceTable.end (); nit++)
	{
	  std::cout << "    Nonce = " << nit->first
		    << ", FaceId = " << nit->second.faceId
		    << ", Expire = " << nit->second.expire << std::endl;
	}
    }
}

void
Pit::CleanUp (const boost::system::error_code& error)
{
  if (error)
    {
      std::cerr << "[Pit::CleanUp] error = " << error.message () << std::endl;
      return;
    }

  boost::chrono::system_clock::time_point now =
    boost::chrono::system_clock::now ();

  pit_type::iterator it = m_pit.begin ();
  while (it != m_pit.end ())
    {
      std::map<uint32_t, FaceRecord>::iterator nit
	= it->second->m_nonceTable.begin ();
      while (nit != it->second->m_nonceTable.end ())
	{
	  if (nit->second.expire < now)
	    {
	      // Already expired
	      nit = it->second->m_nonceTable.erase (nit);
	    }
	  else
	    nit++;
	}

      if (it->second->m_nonceTable.empty ())
	{
	  // Nonce table is empty now. Remove pending interest
	  it = m_pit.erase (it);
	}
      else
	it++;
    }

  // Schedule the next cleanup
  ScheduleCleanUp ();
}

} // namespace node
} // namespace emulator
