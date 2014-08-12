/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include "emulator.h"

namespace emulator {

int
run (int argc, char* argv[])
{
  if (argc < 2)
    {
      throw std::invalid_argument
        ("Not enough cmd parameters. Missing config file path.");
    }

  Emulator em;
  em.ReadNetworkConfig (argv[1]);

  NDNEM_LOG_INFO ("[::run] emulation start");
  em.Start ();

  return 0;
}

} // emulator

int
main (int argc, char* argv[])
{
  try
    {
      emulator::run (argc, argv);
    }
  catch (std::exception& e)
    {
      NDNEM_LOG_FATAL ("[main] std error = " << e.what ());
    }
  catch (boost::system::error_code& e)
    {
      NDNEM_LOG_FATAL ("[main] boost error = " << e.message ());
    }

  return 0;
}
