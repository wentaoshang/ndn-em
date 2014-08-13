/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef __SENSOR_INFO_H__
#define __SENSOR_INFO_H__

#include <stdint.h>

namespace ndnsensor {

struct SensorInfo {
  enum Code {
    REPORT,
    NOTIFY
  };

  uint16_t code;
  uint16_t value;
  uint32_t timestamp;  // in seconds
};

} // ndnsensor

#endif // __SENSOR_INFO_H__
