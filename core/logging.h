/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <exception>

enum {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARNING = 3,
  ERROR = 4,
  FATAL = 5
};

extern int __NDNEM_LOG_LEVEL__;

inline int
GetLogLevelFromString (std::string& level)
{
  if (level == "trace")
    return TRACE;
  else if (level == "debug")
    return DEBUG;
  else if (level == "info")
    return INFO;
  else if (level == "warning")
    return WARNING;
  else if (level == "error")
    return ERROR;
  else if (level == "fatal")
    return FATAL;
  else
    throw std::invalid_argument ("Unknown log level: " + level);
}

#define NDNEM_LOG_TRACE(stream)                                         \
  if (__NDNEM_LOG_LEVEL__ <= TRACE) {                                   \
    std::cout << boost::posix_time::microsec_clock::local_time ()       \
              << " [TRACE] " << stream << std::endl;                    \
  } else ((void)0)

#define NDNEM_LOG_DEBUG(stream)                                         \
  if (__NDNEM_LOG_LEVEL__ <= DEBUG) {                                   \
    std::cout << boost::posix_time::microsec_clock::local_time ()       \
              << " [DEBUG] " << stream << std::endl;                    \
  } else ((void)0)

#define NDNEM_LOG_INFO(stream)                                          \
  if (__NDNEM_LOG_LEVEL__ <= INFO) {                                    \
    std::cout << boost::posix_time::microsec_clock::local_time ()       \
              << " [INFO] " << stream << std::endl;                     \
  } else ((void)0)

#define NDNEM_LOG_WARNING(stream)                                       \
  if (__NDNEM_LOG_LEVEL__ <= WARNING) {                                 \
    std::cerr << boost::posix_time::microsec_clock::local_time ()       \
              << " [WARN] " << stream << std::endl;                     \
  } else ((void)0)

#define NDNEM_LOG_ERROR(stream)                                         \
  if (__NDNEM_LOG_LEVEL__ <= ERROR) {                                   \
    std::cerr << boost::posix_time::microsec_clock::local_time ()       \
              << " [ERROR] " << stream << std::endl;                    \
  } else ((void)0)

#define NDNEM_LOG_FATAL(stream)                                         \
  if (__NDNEM_LOG_LEVEL__ <= FATAL) {                                   \
    std::cerr << boost::posix_time::microsec_clock::local_time ()       \
              << " [FATAL] " << stream << std::endl;                    \
  } else ((void)0)

#endif // __LOGGING_H__
