/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <cstring>
#include "in-memory-repo.h"

using namespace ndn;

namespace ndnsensor {

void
Repo::HandleData (const Interest& interest, Data& data)
{
  const Name& dName = data.getName ();
  std::cout << "[HandleData] <<D: " << dName.toUri () << std::endl;
  assert (dName.size () == 4);

  const name::Component& seq = dName.get (3);
  int seqNum = boost::lexical_cast<int> (seq.toUri ());
  std::cout << "[HandleData] seq: " << seqNum << ", m_sequence: " << m_sequence << std::endl;
  if (m_store.find (seqNum) != m_store.end ())
    {
      std::cerr << "[HandleData] data already in store" << std::endl;
      return;
    }

  // Package sensor data and sign it with repo's key
  Name repoDataName ("/wsn/data/temperature");
  repoDataName.append (dName.get (3));
  shared_ptr<Data> repoData (make_shared<Data> ());
  repoData->setName (repoDataName);
  repoData->setFreshnessPeriod (time::milliseconds(5000));
  repoData->setContent (data.getContent ());

  // Sign Data packet with default identity
  m_keyChain.sign (*repoData);

  // Return Data packet to the requester
  std::cout << "[HandleData] store: " << repoData->getName () << std::endl;

  m_store[seqNum] = repoData;

  m_sequence = seqNum + 1;
}

void
Repo::HandlePushInterest (const Name& name, const Interest& interest)
{
  const Name& iName = interest.getName ();
  std::cout << "[PushInterest] <<I: " << iName << std::endl;
  assert (iName.size () == 5);

  const name::Component& seq = iName.get (3);
  int seqNum = boost::lexical_cast<int> (seq.toUri ());
  std::cout << "[PushInterest] seq: " << seqNum << ", m_sequence: " << m_sequence << std::endl;
  if (m_store.find (seqNum) != m_store.end ())
    {
      std::cerr << "[PushInterest] data already in store" << std::endl;
      return;
    }

  const Block& info = iName.get (4).wireEncode ();

  // Package sensor data and sign it with repo's key
  Name repoDataName ("/wsn/data/temperature");
  repoDataName.append (seq);
  shared_ptr<Data> repoData (make_shared<Data> ());
  repoData->setName (repoDataName);
  repoData->setFreshnessPeriod (time::milliseconds(5000));
  repoData->setContent (info);

  // Sign Data packet with default identity
  m_keyChain.sign (*repoData);

  // Return Data packet to the requester
  std::cout << "[PushInterest] store: " << repoData->getName () << std::endl;

  m_store[seqNum] = repoData;

  m_sequence = seqNum + 1;

  Name ackName (iName);
  ackName.append ("ack");
  shared_ptr<Data> ack (make_shared<Data> ());
  ack->setName (ackName);
  ack->setFreshnessPeriod (time::milliseconds(100));
  ack->setContent (info);
  m_keyChain.sign (*ack);

  // Research question: do we still need ack when there are multiple repos?
  // Interests will be multicasted to all of them and it is not a good
  // idea to have acks storm.
  std::cout << "[PushInterest] >>D: " << ack->getName () << std::endl;
  m_face.put (*ack);
}

void
Repo::HandleNotifyInterest (const Name& name, const Interest& interest)
{
  const Name& iName = interest.getName ();
  std::cout << "[NotifyInterest] <<I: " << iName << std::endl;
  assert (iName.size () == 4);

  const name::Component& seq = iName.get (3);
  std::cout << "[NotifyInterest] seq: " << seq << std::endl;

  ndn::Name data_prefix ("/wsn/thermometer/read");
  data_prefix.append (seq);

  ndn::Interest i (data_prefix);
  i.setInterestLifetime (ndn::time::milliseconds (2000));
  i.setMustBeFresh (true);

  std::cout << "[NotifyInterest] >>I: " << i << std::endl;

  m_face.expressInterest (i,
			  ndn::bind (&Repo::HandleData, this, _1, _2),
			  ndn::bind (&Repo::HandleTimeout, this, _1));
}

void
Repo::HandleUserInterest (const Name& name, const Interest& interest)
{
  std::cout << "[UserInterest] <<I: " << interest.getName () << std::endl;
}

void
Repo::PollData ()
{
  ndn::Name data_prefix ("/wsn/thermometer/read");
  if (m_sequence != -1)
    data_prefix.append (boost::lexical_cast<std::string> (m_sequence));

  ndn::Interest i (data_prefix);
  i.setInterestLifetime (ndn::time::milliseconds (2000));
  i.setMustBeFresh (true);

  std::cout << "[PollData] >>I: " << i << std::endl;

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
  else if (m_mode == PUSH)
    // Sensors send push interest to the repo using the repo prefix
    m_face.setInterestFilter ("/wsn/repo/push",
			      ndn::bind (&Repo::HandlePushInterest, this, _1, _2),
			      RegisterPrefixSuccessCallback (),
			      ndn::bind (&Repo::HandleRegisterFailed, this, _2));
  else if (m_mode == NOTIFY)
    m_face.setInterestFilter ("/wsn/repo/notify",
			      ndn::bind (&Repo::HandleNotifyInterest, this, _1, _2),
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
  else if (strcmp (argv[1], "notify") == 0)
    mode = Repo::NOTIFY;
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
