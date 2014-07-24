/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include "emulator.h"

namespace emulator {

int
main (int argc, char* argv[])
{
  Emulator em;
  em.ReadNodeConfig (argv[1]);
  em.ReadLinkConfig (argv[2]);

  em.Start ();

  return 0;
}

} // emulator

int
main (int argc, char* argv[])
{
  try
    {
      emulator::main (argc, argv);
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
