/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __CACHE_MANAGER_H__
#define __CACHE_MANAGER_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/shared_ptr.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <iostream>
#include <deque>

#include "logging.h"

namespace emulator {
namespace node {

class CacheEntry {
public:
  CacheEntry (const boost::shared_ptr<ndn::Data>& d,
	      boost::chrono::system_clock::time_point& e)
    : data (d)
    , expire (e)
  {
    size = d->wireEncode ().size ();
  }

  boost::shared_ptr<ndn::Data> data;
  size_t size;
  boost::chrono::system_clock::time_point expire;
};

class CacheManager {
public:
  static const boost::posix_time::time_duration CACHE_PURGE_INTERVAL;

  CacheManager (const std::string& nodeId, int limit,
		boost::asio::io_service& ioService)
    : m_nodeId (nodeId)
    , m_count (0)
    , m_limit (limit)
    , m_cleanupTimer (ioService)
  {
  }

  int
  GetLimit () const
  {
    return m_limit;
  }

  typedef std::deque<CacheEntry> cache_type;

  bool
  FindMatchingData (const boost::shared_ptr<ndn::Interest>&,
		    boost::shared_ptr<ndn::Data>&);

  void
  Insert (const boost::shared_ptr<ndn::Data>&);

  void
  ScheduleCleanUp ()
  {
    m_cleanupTimer.expires_from_now (CACHE_PURGE_INTERVAL);
    m_cleanupTimer.async_wait (boost::bind (&CacheManager::CleanUp, this, _1));
  }

private:
  void
  CleanUp (const boost::system::error_code&);

private:
  const std::string& m_nodeId;
  std::deque<CacheEntry> m_queue; // FIFO queue
  int m_count;
  const int m_limit;  // cache limit in # of bytes
  boost::asio::deadline_timer m_cleanupTimer;
};

} // namespace node
} // namespace emulator

#endif // __CACHE_MANAGER_H__
