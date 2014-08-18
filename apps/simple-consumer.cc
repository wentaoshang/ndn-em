/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "simple-consumer.h"
#include <boost/lexical_cast.hpp>

using namespace ndn;

int
main(int argc, char* argv[])
{
  if (argc != 4)
    {
      std::cerr << "Usage: " << argv[0]
                << " [delay] [ndn-name] [nfd-path]" << std::endl;
      exit (1);
    }

  try {
    long delay = boost::lexical_cast<long> (std::string (argv[1]));
    SimpleConsumer consumer (delay, argv[2], argv[3]);

    consumer.Start ();
  }
  catch(std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
