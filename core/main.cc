/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include "emulator.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

namespace emulator {

namespace po = boost::program_options;

int
run (int argc, char* argv[])
{
  std::string log_level;
  po::options_description desc ("Allowed options");
  desc.add_options ()
    ("help,h", "print help message")
    ("log-level,l", po::value<std::string>
     (&log_level)->default_value ("info"),
     "logging level (trace, debug, info, warning, error, fatal)")
    ("config-file,c", po::value<std::string> (),
     "configuration file path")
    ;

  po::variables_map vm;
  po::store (po::parse_command_line (argc, argv, desc), vm);
  po::notify (vm);

  if (vm.count ("help") || !vm.count ("config-file"))
    {
      std::cerr << "Usage: " << argv[0] << " [options]" << std::endl;
      std::cerr << desc << std::endl;
      exit (1);
    }

  __NDNEM_LOG_LEVEL__ = GetLogLevelFromString (log_level);

  Emulator em;
  em.ReadNetworkConfig (vm["config-file"].as<std::string> ());

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
