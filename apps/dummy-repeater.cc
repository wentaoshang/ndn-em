/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "dummy-repeater.h"

void
runtime (int argc, char* argv[])
{
  std::string interval (argv[1]);
  boost::asio::io_service ioService;

  DummyRepeater repeator (boost::lexical_cast<long> (interval), argv[2], ioService);

  ioService.run ();
}


/**
 * Usage: ./dummy [interval] [unix-path]
 *
 */
int
main (int argc, char* argv[])
{
  try
    {
      runtime (argc, argv);
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
