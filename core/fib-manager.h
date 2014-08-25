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

#include "logging.h"
#include "face.h"
#include "fib.h"

namespace emulator {

class Node;

namespace node {

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

class FibManager {
public:
  FibManager (const int faceId, boost::shared_ptr<Node>& node, node::Fib& fib)
    : m_id (faceId)
    , m_node (node)
    , m_fib (fib)
    , m_fibCmdPrefix ("/localhost/nfd/rib")
  {
    m_fib.AddRoute (m_fibCmdPrefix, m_id);  // register fib command prefix in FIB
  }

  void
  ProcessCommand (const int, const boost::shared_ptr<ndn::Interest>&);

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
             ControlResponse& response);

  void
  SendResponse(const ndn::Name& name,
               const ControlResponse& response);

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
  boost::shared_ptr<Node> m_node;
  // Reference to the node's fib table
  node::Fib& m_fib;
  const ndn::Name m_fibCmdPrefix;
  ndn::KeyChain m_keyChain;
};

} // namespace node
} // namespace emulator

#endif // __FIB_MANAGER_H__
