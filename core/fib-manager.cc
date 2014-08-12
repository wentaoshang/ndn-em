/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include "fib-manager.h"

namespace emulator {
namespace node {

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

void
FibManager::ProcessCommand (const int faceId, const boost::shared_ptr<ndn::Interest>& request)
{
  const ndn::Name& command = request->getName ();
  const ndn::Name::Component& verb = command[m_fibCmdPrefix.size ()];
  const ndn::Name::Component& parameterComponent = command[m_fibCmdPrefix.size () + 1];

  if (verb == ndn::Name::Component ("register"))
    {
      ControlParameters parameters;
      if (!extractParameters (parameterComponent, parameters))
	{
	  NDNEM_LOG_ERROR ("[FibManager::ProcessCommand] ignore invalid command");
	  //TODO: send back error response
	  return;
	}

      if (!parameters.hasFaceId() || parameters.getFaceId () == 0)
	{
	  parameters.setFaceId (static_cast<uint64_t> (faceId));
	}

      ControlResponse response;
      AddNextHop (parameters, response);
      SendResponse (command, response);
    }
}

void
FibManager::AddNextHop(ControlParameters& parameters,
		       ControlResponse& response)
{
  ndn::nfd::RibRegisterCommand command;

  if (!validateParameters (command, parameters))
    {
      setResponse (response, 400, "Malformed command");
      return;
    }

  const ndn::Name& prefix = parameters.getName ();
  uint64_t faceId = parameters.getFaceId ();
  uint64_t cost = parameters.getCost(); // ignore cost for now

  NDNEM_LOG_DEBUG ("[FibManager::AddNextHop] prefix = " << prefix
                   << ", faceid = " << faceId
                   << ", cost = " << cost);

  //TODO: validate face id

  m_fib.AddRoute (prefix, faceId);

  setResponse (response, 200, "Success", parameters.wireEncode ());
}

void
FibManager::SendResponse(const ndn::Name& name,
			 const ControlResponse& response)
{
  const ndn::Block& encodedControl = response.wireEncode ();

  boost::shared_ptr<ndn::Data> responseData (boost::make_shared<ndn::Data> (name));
  responseData->setContent (encodedControl);

  m_keyChain.sign (*responseData);
  m_nodeMessageCallback (0, responseData);
}


} // namespace node
} // namespace emulator
