/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#ifndef __FIB_MANAGER_H__
#define __FIB_MANAGER_H__

#include <boost/function.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>
#include <iostream>
#include <map>

#include "fib.h"

namespace emulator {

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

class FibManager {
public:
  FibManager (const int faceId, const std::string& nodeId, node::Fib& fib,
              const std::map<int, boost::shared_ptr<Face> >& faceTable,
              const boost::function<void (const int, const boost::shared_ptr<ndn::Data>&)>& callback)
    : m_id (faceId)
    , m_nodeId (nodeId)
    , m_fib (fib)
    , m_faceTable (faceTable)
    , m_nodeMessageCallback (callback)
    , m_fibCmdPrefix ("/localhost/nfd/rib")
  {
    m_fib.AddRoute (m_fibCmdPrefix, m_id);  // register fib command prefix in FIB
  }

  void
  ProcessCommand (const int faceId, const boost::shared_ptr<ndn::Interest>& request)
  {
    const ndn::Name& command = request->getName ();
    const ndn::Name::Component& verb = command[m_fibCmdPrefix.size ()];
    const ndn::Name::Component& parameterComponent = command[m_fibCmdPrefix.size () + 1];

    if (verb == ndn::Name::Component ("register"))
      {
        ControlParameters parameters;
        if (!extractParameters (parameterComponent, parameters))
          {
            std::cerr << "[FibManager::ProcessCommand] ignore invalid command" << std::endl;
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

  /*
   * Remove the face id from all relevant fib entries
   */
  void
  CleanUpFib (const int faceId)
  {
    m_fib.CleanUp (faceId);
  }

private:
  void
  AddNextHop(ControlParameters& parameters,
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

    std::cout << "[FibManager::AddNextHop] prefix = " << prefix
              << ", faceid = " << faceId
              << ", cost = " << cost << std::endl;

    //TODO: validate face id

    m_fib.AddRoute (prefix, faceId);

    setResponse (response, 200, "Success", parameters.wireEncode ());
  }

  void
  SendResponse(const ndn::Name& name,
               const ControlResponse& response)
  {
    const ndn::Block& encodedControl = response.wireEncode ();

    boost::shared_ptr<ndn::Data> responseData (boost::make_shared<ndn::Data> (name));
    responseData->setContent (encodedControl);

    m_keyChain.sign (*responseData);
    m_nodeMessageCallback (0, responseData);
  }

  void
  setResponse(ControlResponse& response,
              uint32_t code,
              const std::string& text)
  {
    response.setCode (code);
    response.setText (text);
  }

  void
  setResponse(ControlResponse& response,
              uint32_t code,
              const std::string& text,
              const ndn::Block& body)
  {
    setResponse (response, code, text);
    response.setBody (body);
  }

  bool
  validateParameters (const ControlCommand& command,
                      ControlParameters& parameters)
  {
    try
      {
        command.validateRequest (parameters);
      }
    catch (const ControlCommand::ArgumentError& error)
      {
        return false;
      }

    command.applyDefaultsToRequest (parameters);

    return true;
  }

  bool
  extractParameters (const ndn::Name::Component& parameterComponent,
                     ControlParameters& extractedParameters)
  {
    try
      {
        ndn::Block rawParameters = parameterComponent.blockFromValue ();
        extractedParameters.wireDecode (rawParameters);
      }
    catch (const ndn::Tlv::Error& e)
      {
        return false;
      }

    return true;
  }

private:
  const int m_id;
  const std::string& m_nodeId;
  // Reference to the node's fib table
  node::Fib& m_fib;
  // Reference to the nodes's face table
  const std::map<int, boost::shared_ptr<Face> >& m_faceTable;
  // Callback to send message to node
  const boost::function<void (const int, const boost::shared_ptr<ndn::Data>&)> m_nodeMessageCallback;
  const ndn::Name m_fibCmdPrefix;
  ndn::KeyChain m_keyChain;
};

} // namespace emulator

#endif // __FIB_MANAGER_H__
