/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */

#include <exception>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>

#include "emulator.h"

namespace emulator {

void
Emulator::ReadNetworkConfig (const std::string& path)
{
  NDNEM_LOG_INFO ("[Emulator::ReadNetworkConfig] from file: " << path);

  using boost::property_tree::ptree;
  ptree config;
  boost::property_tree::xml_parser::read_xml (path, config);

  ptree& links = config.get_child ("Config.Links");
  BOOST_FOREACH (ptree::value_type& v, links)
    {
      BOOST_ASSERT (v.first == "Link");
      ptree& link = v.second;
      const std::string linkId = link.get<std::string> ("Id");
      boost::optional<double> txRate = link.get_optional<double> ("TxRate");
      if (!txRate)
        txRate = boost::optional<double> (40.0);  // default tx rate is 40 kbits/s
      boost::optional<int> mtu = link.get_optional<int> ("Mtu");
      if (!mtu)
        mtu = boost::optional<int> (8800);  // default MTU is 8800 bytes

      std::map<std::string, boost::shared_ptr<Link> >::iterator it = m_linkTable.find (linkId);
      if (it == m_linkTable.end ())
        m_linkTable[linkId] = boost::make_shared<Link> (linkId, *txRate, *mtu);
      else
        throw std::runtime_error ("[Emulator::ReadNetworkConfig] duplicate link id " + linkId);
    }

  uint64_t globalMacAssigner = 0x0001; // ensures we allocate globally unique mac addresses
  ptree& nodes = config.get_child ("Config.Nodes");
  // First iteration will create all the nodes & devices
  BOOST_FOREACH (ptree::value_type& v, nodes)
    {
      BOOST_ASSERT (v.first == "Node");
      ptree& node = v.second;
      const std::string nodeId = node.get<std::string> ("Id");
      const std::string path = node.get<std::string> ("Path");
      boost::optional<int> cacheLimit = node.get_optional<int> ("CacheLimit");
      if (!cacheLimit)
        cacheLimit = boost::optional<int> (100);  // default cache size is 100 KB

      std::map<std::string, boost::shared_ptr<Node> >::iterator it = m_nodeTable.find (nodeId);
      if (it == m_nodeTable.end ())
        {
          boost::shared_ptr<Node> pnode
            (boost::make_shared<Node> (nodeId, path, (*cacheLimit << 10),
                                       boost::ref (m_ioService)));

          BOOST_FOREACH (ptree::value_type& v, node.get_child ("Devices"))
            {
              BOOST_ASSERT (v.first == "Device");
              ptree& dev = v.second;
              const std::string devId = dev.get<std::string> ("DeviceId");
              const std::string linkId = dev.get<std::string> ("LinkId");
              uint64_t macAddr = globalMacAssigner++;

              std::map<std::string, boost::shared_ptr<Link> >::iterator it =
                m_linkTable.find (linkId);
              if (it != m_linkTable.end ())
                {
                  boost::shared_ptr<Link>& link = it->second;
                  boost::shared_ptr<LinkDevice> ldev = pnode->AddDevice (devId, macAddr, link);
                  link->AddNodeDevice (nodeId, ldev);
                }
              else
                throw std::runtime_error ("[Emulator::ReadNetworkConfig] unkown link " + linkId
                                          + " on node " + nodeId);
            }

          m_nodeTable[nodeId] = pnode;
        }
      else
        throw std::runtime_error ("[Emulator::ReadNetworkConfig] duplicate node id " + nodeId);
    }

  // Second iteration will create routes
  BOOST_FOREACH (ptree::value_type& v, nodes)
    {
      BOOST_ASSERT (v.first == "Node");
      ptree& node = v.second;
      const std::string nodeId = node.get<std::string> ("Id");
      boost::shared_ptr<Node> pnode = m_nodeTable[nodeId];

      // Static routes are optional
      boost::optional<ptree&> routes = node.get_child_optional ("Routes");
      if (routes)
        {
          BOOST_FOREACH (ptree::value_type& v, *routes)
            {
              BOOST_ASSERT (v.first == "Route");
              ptree& rt = v.second;
              const std::string prefix = rt.get<std::string> ("Prefix");
              const std::string devId = rt.get<std::string> ("Interface");
              boost::optional<std::string> snexthop = rt.get_optional<std::string> ("Nexthop");
              uint64_t nexthop;
              if (snexthop)
                {
                  boost::shared_ptr<Link> l = pnode->GetDevice (devId)->GetLink ();
                  nexthop = l->GetNodeDevice (*snexthop)->GetMacAddr ();
                }
              else
                nexthop = 0xffff;  // broadcast by default
              pnode->AddRoute (prefix, devId, nexthop);
            }
        }
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
