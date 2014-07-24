/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include "emulator.h"

namespace emulator {

int
main (int argc, char* argv[])
{
  Emulator em;
  em.ReadNodeConfig ("node.txt");
  em.ReadLinkConfig ("link.txt");

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
      std::cerr << e.what () << std::endl;
    }
  catch (boost::system::error_code& e)
    {
      std::cerr << e.message () << std::endl;
    }

  return 0;
}
