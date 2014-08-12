/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include "emulator.h"

namespace emulator {

int
run (int argc, char* argv[])
{
  Emulator em;
  em.ReadNetworkConfig (argv[1]);

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
      std::cerr << "[main] std error = " << e.what () << std::endl;
    }
  catch (boost::system::error_code& e)
    {
      std::cerr << "[main] boost error = " << e.message () << std::endl;
    }

  return 0;
}
