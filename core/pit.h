/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __PIT_H__
#define __PIT_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <ndn-cxx/interest.hpp>
#include <map>
#include <set>
#include <iostream>

#include "ndn-name-hash.h"

namespace emulator {
namespace node {

class FaceRecord {
public:
  FaceRecord (int id, boost::chrono::system_clock::time_point& e)
    : faceId (id)
    , expire (e)
  {
  }

public:
  // The id of the face where the interest comes from
  int faceId;
  // The time when the interest from this face will expire
  boost::chrono::system_clock::time_point expire;
};

class PitEntry {
public:
  PitEntry (const boost::shared_ptr<ndn::Interest>& i)
    : m_interest (i)
  {
  }

  // Return false if the nonce already exists
  // Otherwise insert the nonce into nonce table and return true
  bool
  AddNonce (const uint32_t, const int,
            boost::chrono::system_clock::time_point&);

  friend class Pit;

private:
  const boost::shared_ptr<ndn::Interest> m_interest;
  std::map<uint32_t, FaceRecord> m_nonceTable;
};

class Pit {
public:
  Pit (long interval, boost::asio::io_service& ioService)
    : m_cleanupInterval (boost::posix_time::milliseconds (interval))
    , m_cleanupTimer (ioService)
  {
  }

  typedef boost::unordered_map<ndn::Name, boost::shared_ptr<PitEntry>, ndn_name_hash> pit_type;

  bool
  AddInterest (const int, const boost::shared_ptr<ndn::Interest>&);

  void
  ConsumeInterestWithDataName (const ndn::Name&, std::set<int>&);

  void
  Print ();

  void
  ScheduleCleanUp ()
  {
    m_cleanupTimer.expires_from_now (m_cleanupInterval);
    m_cleanupTimer.async_wait (boost::bind (&Pit::CleanUp, this, _1));
  }

private:
  void
  CleanUp (const boost::system::error_code&);

private:
  boost::unordered_map<ndn::Name, boost::shared_ptr<PitEntry>, ndn_name_hash> m_pit;
  boost::posix_time::time_duration m_cleanupInterval;
  boost::asio::deadline_timer m_cleanupTimer;
};

} // namespace node
} // namespace emulator

#endif // __PIT_H__
