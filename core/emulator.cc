/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>

#include "emulator.h"

namespace emulator {

void
Emulator::ReadNetworkConfig (const char* path)
{
  NDNEM_LOG_INFO ("[Emulator::ReadNetworkConfig] from file: " << path);

  using boost::property_tree::ptree;
  ptree config;
  boost::property_tree::xml_parser::read_xml (path, config);

  ptree& links = config.get_child ("Config.Links");
  BOOST_FOREACH (ptree::value_type& v, links)
    {
      BOOST_ASSERT (v.first == "Link");
      const std::string linkId = v.second.get<std::string> ("Id");
      std::map<std::string, boost::shared_ptr<Link> >::iterator it = m_linkTable.find (linkId);
      if (it == m_linkTable.end ())
        m_linkTable[linkId] = boost::make_shared<Link> (linkId);
      else
        throw std::runtime_error ("[Emulator::ReadNetworkConfig] duplicate link id " + linkId);
    }

  ptree& nodes = config.get_child ("Config.Nodes");
  BOOST_FOREACH (ptree::value_type& v, nodes)
    {
      BOOST_ASSERT (v.first == "Node");
      ptree& node = v.second;
      const std::string nodeId = node.get<std::string> ("Id");
      const std::string path = node.get<std::string> ("Path");
      std::map<std::string, boost::shared_ptr<Node> >::iterator it = m_nodeTable.find (nodeId);
      if (it == m_nodeTable.end ())
        {
          boost::shared_ptr<Node> pnode
            (boost::make_shared<Node> (nodeId, path, boost::ref (m_ioService)));

          BOOST_FOREACH (ptree::value_type& v, node.get_child ("Links"))
            {
              BOOST_ASSERT (v.first == "LinkId");
              const std::string linkId = v.second.data ();
              std::map<std::string, boost::shared_ptr<Link> >::iterator it =
                m_linkTable.find (linkId);
              if (it != m_linkTable.end ())
                {
                  const boost::shared_ptr<Link>& link = it->second;
                  boost::shared_ptr<LinkFace> face;
                  if (pnode->AddLink (link, face))
                    link->AddNode (face);
                  else
                    NDNEM_LOG_ERROR ("[Emulator::ReadNetworkConfig] duplicate link id "
                                     << linkId << " for node " << nodeId);
                }
              else
                throw std::runtime_error ("[Emulator::ReadNetworkConfig] unkown link id " + linkId
                                          + " for node " + nodeId);
            }

          // Static routes are optional
          boost::optional<ptree&> routes = node.get_child_optional ("Routes");
          if (routes)
            {
              BOOST_FOREACH (ptree::value_type& v, *routes)
                {
                  BOOST_ASSERT (v.first == "Route");
                  ptree& rt = v.second;
                  const std::string prefix = rt.get<std::string> ("Prefix");
                  const std::string outFace = rt.get<std::string> ("OutFace");
                  pnode->AddRoute (prefix, outFace);
                }
            }

          m_nodeTable[nodeId] = pnode;
        }
      else
        throw std::runtime_error ("[Emulator::ReadNetworkConfig] duplicate node id " + nodeId);
    }
  this->PrintNodes();

  ptree& matrices = config.get_child ("Config.Matrices");
  BOOST_FOREACH (ptree::value_type& v, matrices)
    {
      BOOST_ASSERT (v.first == "Matrix");
      ptree& matrix = v.second;
      const std::string linkId = matrix.get<std::string> ("LinkId");
      std::map<std::string, boost::shared_ptr<Link> >::iterator it =
        m_linkTable.find (linkId);
      if (it != m_linkTable.end ())
        {
          const boost::shared_ptr<Link>& link = it->second;
          ptree& connections = matrix.get_child ("Connections");
          BOOST_FOREACH (ptree::value_type& v, connections)
            {
              BOOST_ASSERT (v.first == "Connection");
              ptree& con = v.second;
              const std::string from = con.get<std::string> ("From");
              const std::string to = con.get<std::string> ("To");
              const double rate = con.get<double> ("LossRate");

              // For now, only support packet loss rate
              boost::shared_ptr<LinkAttribute> attr =
                boost::make_shared<LinkAttribute> (rate);

              link->AddConnection (from, to, attr);
            }
        }
      else
        throw std::runtime_error ("[Emulator::ReadNetworkConfig] unkown matrix link id " + linkId);
    }
  this->PrintLinks ();
}

void
Emulator::PrintNodes ()
{
  std::cout << "[Emulator::PrintNodes] nodes summary:" << std::endl;
  std::map<std::string, boost::shared_ptr<Node> >::iterator it;
  for (it = m_nodeTable.begin (); it != m_nodeTable.end (); it++)
    {
      it->second->PrintInfo ();
    }
}

void
Emulator::PrintLinks ()
{
  std::cout << "[Emulator::PrintLinks] links summary:" << std::endl;
  std::map<std::string, boost::shared_ptr<Link> >::iterator it;
  for (it = m_linkTable.begin (); it != m_linkTable.end (); it++)
    {
      it->second->PrintInfo ();
    }
}

void
Emulator::Start ()
{
  std::map<std::string, boost::shared_ptr<Node> >::iterator it;
  for (it = m_nodeTable.begin (); it != m_nodeTable.end (); it++)
    {
      it->second->Start ();
    }

  m_ioService.run (); // This call will block
}

} // namespace emulator
