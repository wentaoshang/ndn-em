/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

enum {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARNING = 3,
  ERROR = 4,
  FATAL = 5
};

#define LOG_LEVEL TRACE

#define NDNEM_LOG_TRACE(stream)                                         \
if (LOG_LEVEL <= TRACE) {                                               \
  std::cout << boost::posix_time::microsec_clock::local_time()          \
            << " [TRACE] " << stream << std::endl;                      \
 } else ((void)0)

#define NDNEM_LOG_DEBUG(stream)                                         \
if (LOG_LEVEL <= DEBUG) {                                               \
  std::cout << boost::posix_time::microsec_clock::local_time()          \
            << " [DEBUG] " << stream << std::endl;                      \
 } else ((void)0)

#define NDNEM_LOG_INFO(stream)                                          \
if (LOG_LEVEL <= INFO) {                                                \
  std::cout << boost::posix_time::microsec_clock::local_time()          \
            << " [INFO] " << stream << std::endl;                       \
 } else ((void)0)

#define NDNEM_LOG_WARNING(stream)                                       \
if (LOG_LEVEL <= WARNING) {                                             \
  std::cerr << boost::posix_time::microsec_clock::local_time()          \
            << " [WARN] " << stream << std::endl;                       \
 } else ((void)0)

#define NDNEM_LOG_ERROR(stream)                                         \
if (LOG_LEVEL <= ERROR) {                                               \
  std::cerr << boost::posix_time::microsec_clock::local_time()          \
            << " [ERROR] " << stream << std::endl;                      \
 } else ((void)0)

#define NDNEM_LOG_FATAL(stream)                                         \
if (LOG_LEVEL <= FATAL) {                                               \
  std::cerr << boost::posix_time::microsec_clock::local_time()          \
            << " [FATAL] " << stream << std::endl;                      \
 } else ((void)0)

#endif // __LOGGING_H__
