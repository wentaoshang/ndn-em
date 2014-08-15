/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "cache-manager.h"

namespace emulator {
namespace node {

const boost::posix_time::time_duration CacheManager::CACHE_PURGE_INTERVAL =
  boost::posix_time::milliseconds (3600000);  // 1 h

bool
CacheManager::FindMatchingData (const boost::shared_ptr<ndn::Interest>& i,
                                boost::shared_ptr<ndn::Data>& out)
{
  boost::chrono::system_clock::time_point now =
    boost::chrono::system_clock::now ();

  cache_type::iterator it;
  for (it = m_queue.begin (); it != m_queue.end (); it++)
    {
      bool mustBeFresh = i->getMustBeFresh ();
      // For now, ignore other selectors
      if (i->getName ().isPrefixOf (it->data->getName ())
	  && (!mustBeFresh || (mustBeFresh && it->expire > now)))
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
      NDNEM_LOG_ERROR ("[CacheManager::Insert] (" << m_nodeId
                       << ") data packet too big: " << d->getName ());
      return;
    }

  // Cache replacement policy (FIFO)
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
      NDNEM_LOG_ERROR ("[CacheManager::CleanUp] error = " << error.message ());
      return;
    }

  boost::chrono::system_clock::time_point now =
    boost::chrono::system_clock::now ();

  cache_type::iterator it = m_queue.begin ();
  while (it != m_queue.end ())
    {
      if (it->expire < now)
        {
          NDNEM_LOG_TRACE ("[CacheManager::CleanUp] (" << m_nodeId
                           << ") remove data: " << it->data->getName ());
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
