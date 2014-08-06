/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "cache-manager.h"

namespace emulator {
namespace node {

bool
CacheManager::FindMatchingData (const boost::shared_ptr<ndn::Interest>& i,
                                boost::shared_ptr<ndn::Data>& out)
{
  boost::chrono::system_clock::time_point now =
    boost::chrono::system_clock::now ();

  cache_type::iterator it;
  for (it = m_queue.begin (); it != m_queue.end (); it++)
    {
      // For now, ignore selectors
      if (i->getName ().isPrefixOf (it->data->getName ())
	  && it->expire > now)
	{
	  // Return the first match
	  out = it->data;
          return true;
	}
    }

  return false;
}

void
CacheManager::Insert (const boost::shared_ptr<ndn::Data>& d)
{
  boost::chrono::system_clock::time_point expire =
    boost::chrono::system_clock::now () + d->getFreshnessPeriod ();

  //TODO: check for duplicate data already in cache

  size_t sz = d->wireEncode ().size ();
  if (sz > m_limit)
    {
      std::cerr << "[CacheManager::Insert] Data packet too big: "
                << d->getName () << std::endl;
      return;
    }

  while (m_count + sz > m_limit)
    {
      size_t front_sz = m_queue.front ().size;
      m_queue.pop_front ();
      m_count -= front_sz;
      assert (m_count >= 0);
    }

  CacheEntry entry (d, expire);
  m_queue.push_back (entry);
  m_count += entry.size;
}

void
CacheManager::CleanUp (const boost::system::error_code& error)
{
  if (error)
    {
      std::cerr << "[CacheManager::CleanUp] error = " << error.message () << std::endl;
      return;
    }

  boost::chrono::system_clock::time_point now =
    boost::chrono::system_clock::now ();

  cache_type::iterator it = m_queue.begin ();
  while (it != m_queue.end ())
    {
      if (it->expire < now)
        {
          std::cout << "[CacheManager::CleanUp] remove data: " << it->data->getName () << std::endl;
          size_t sz = it->size;
          it = m_queue.erase (it);
          m_count -= sz;
        }
      else
	it++;
    }

  // Schedule the next cleanup
  ScheduleCleanUp ();
}

} // namespace node
} // namespace emulator
