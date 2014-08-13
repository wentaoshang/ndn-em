/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ctime>
#include <cstring>
#include "dummy-sensor.h"

using namespace ndn;

namespace ndnsensor {

void
Sensor::HandleInterest (const Name& name, const Interest& interest)
{
  std::cout << "<< I: " << interest.getName ().toUri () << std::endl;

  // Create new name, based on Interest's name
  Name dataName (interest.getName ());
  if (dataName.size () == 3)
    dataName.append(boost::lexical_cast<std::string> (m_sequence));
  else if (dataName.size () == 4)
    {
      int seqNum = boost::lexical_cast<int> (dataName.get (3).toUri ());
      if (seqNum != m_sequence)
        {
          std::cerr << "Sequence number out of sync. Disgard interest" << std::endl;
          return;
        }
    }

  struct SensorInfo info;
  info.code = SensorInfo::REPORT;
  info.value = m_reading;
  info.timestamp = ::time (NULL);

  // Create Data packet
  shared_ptr<Data> data (make_shared<Data> ());
  data->setName (dataName);
  data->setFreshnessPeriod (time::milliseconds(5000));
  data->setContent (reinterpret_cast<const uint8_t*> (&info), sizeof (info));

  // Sign Data packet with default identity
  m_keyChain.sign (*data);

  // Return Data packet to the requester
  std::cout << ">> D: " << *data;
  m_face.put (*data);
}

void
Sensor::SchedulePush ()
{
}

void
Sensor::SchedulePhoneHome ()
{
}

void
Sensor::ScheduleNewReading ()
{
  m_sequence++;
  // m_value = ...
  std::cout << "Generate new sequence number: " << m_sequence << std::endl;

  // Generate "new" reading every 5 sec
  m_scheduler.scheduleEvent (ndn::time::milliseconds (5000),
			     ndn::bind (&Sensor::ScheduleNewReading, this));
}

void
Sensor::Start ()
{
  this->ScheduleNewReading ();

  // Always listen on sensor prefix
  m_face.setInterestFilter ("/wsn/thermometer",
			    ndn::bind (&Sensor::HandleInterest, this, _1, _2),
			    RegisterPrefixSuccessCallback (),
			    ndn::bind (&Sensor::HandleRegisterFailed, this, _2));

  if (m_mode == Sensor::PUSH)
    this->SchedulePush ();
  else if (m_mode == Sensor::PHONE_HOME)
    this->SchedulePhoneHome ();

  m_ioService.run ();
}

int
main (int argc, char* argv[])
{
  if (argc != 3)
    {
      std::cerr << "Usage: " << argv[0] << " [mode] [path]" << std::endl;
      exit (1);
    }

  Sensor::OptMode mode;
  if (strcmp (argv[1], "poll") == 0)
    mode = Sensor::POLL;
  else if (strcmp (argv[1], "push") == 0)
    mode = Sensor::PUSH;
  else if (strcmp (argv[2], "phonehome") == 0)
    mode = Sensor::PHONE_HOME;
  else
    {
      std::cerr << "Unknown operation mode: " << argv[1] << std::endl;
      exit (1);
    }

  try
    {
      Sensor sensor (mode, argv[2]);
      sensor.Start ();
    }
  catch (std::exception& e)
    {
      std::cerr << "ERROR: " << e.what () << std::endl;
    }

  return 0;
}

} // namespace ndnsensor

int
main (int argc, char* argv[])
{
  return ndnsensor::main (argc, argv);
}
