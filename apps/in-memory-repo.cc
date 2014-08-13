/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <cstring>
#include "in-memory-repo.h"

using namespace ndn;

namespace ndnsensor {

void
Repo::HandleData (const Interest& interest, Data& data)
{
  std::cout << "I: " << interest.getName ().toUri () << std::endl;
  std::cout << "D: " << data.getName ().toUri () << std::endl;
  assert (data.getName ().size () == 4);
  int seqNum = boost::lexical_cast<int> (data.getName ().get (3).toUri ());
  std::cout << "Seq: " << seqNum << ", m_seq: " << m_sequence << std::endl;
  if (m_store.find (seqNum) == m_store.end ())
    m_store[seqNum] = ndn::make_shared<ndn::Data> (data);
  else
    std::cerr << "Data is already in store" << std::endl;
  m_sequence = seqNum + 1;
}

void
Repo::HandleSensorInterest (const Name& name, const Interest& interest)
{
  std::cout << "<< I: " << interest << std::endl;
}

void
Repo::HandleUserInterest (const Name& name, const Interest& interest)
{
  std::cout << "<< I: " << interest << std::endl;
}

void
Repo::PollData ()
{
  ndn::Name data_prefix ("/wsn/thermometer/read");
  if (m_sequence != -1)
    data_prefix.append (boost::lexical_cast<std::string> (m_sequence));

  ndn::Interest i (data_prefix);
  i.setScope (1);
  i.setInterestLifetime (ndn::time::milliseconds (2000));
  i.setMustBeFresh (true);

  std::cout << ">> I: " << i.toUri () << std::endl;

  m_face.expressInterest (i,
			  ndn::bind (&Repo::HandleData, this, _1, _2),
			  ndn::bind (&Repo::HandleTimeout, this, _1));

  m_scheduler.scheduleEvent (ndn::time::milliseconds (5000),  // fetch data every 5 sec
			     ndn::bind (&Repo::PollData, this));
}

void
Repo::Start ()
{
  // Listen on sensor data prefix
  m_face.setInterestFilter ("/wsn/data/temperature",
			    ndn::bind (&Repo::HandleUserInterest, this, _1, _2),
			    RegisterPrefixSuccessCallback (),
			    ndn::bind (&Repo::HandleRegisterFailed, this, _2));

  if (m_mode == POLL)
    this->PollData ();
  else
    // Sensors send interest to the repo using the repo prefix
    m_face.setInterestFilter ("/wsn/repo",
			      ndn::bind (&Repo::HandleSensorInterest, this, _1, _2),
			      RegisterPrefixSuccessCallback (),
			      ndn::bind (&Repo::HandleRegisterFailed, this, _2));

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

  Repo::OptMode mode;
  if (strcmp (argv[1], "poll") == 0)
    mode = Repo::POLL;
  else if (strcmp (argv[1], "push") == 0)
    mode = Repo::PUSH;
  else if (strcmp (argv[2], "phonehome") == 0)
    mode = Repo::PHONE_HOME;
  else
    {
      std::cerr << "Unknown operation mode: " << argv[1] << std::endl;
      exit (1);
    }

  try
    {
      Repo repo (mode, argv[2]);
      repo.Start ();
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
