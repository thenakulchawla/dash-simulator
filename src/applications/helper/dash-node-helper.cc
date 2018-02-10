/**
 * This file contains the definitions of the functions declared in dash-node-helper.h
 */

#include "dash-node-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "../model/dash-node.h"

namespace ns3 {

DashNodeHelper::DashNodeHelper (std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                                      std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                      nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) 
{
  m_factory.SetTypeId ("ns3::DashNode");
  commonConstructor (protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
}

DashNodeHelper::DashNodeHelper (void)
{
}

void 
DashNodeHelper::commonConstructor(std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                                     std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                     nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) 
{

  m_protocol = protocol;
  m_address = address;
  m_peersAddresses = peers;
  m_peersDownloadSpeeds = peersDownloadSpeeds;
  m_peersUploadSpeeds = peersUploadSpeeds;
  m_internetSpeeds = internetSpeeds;
  m_nodeStats = stats;
  m_protocolType = STANDARD_PROTOCOL;
	m_nodeType = FULL_NODE;
  
  m_factory.Set ("Protocol", StringValue (m_protocol));
  m_factory.Set ("Local", AddressValue (m_address));

}

void 
DashNodeHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DashNodeHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DashNodeHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DashNodeHelper::Install (NodeContainer c)
{ 

  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
  {
    apps.Add (InstallPriv (*i));
  }

  return apps;
}

Ptr<Application>
DashNodeHelper::InstallPriv (Ptr<Node> node)
{
  Ptr<DashNode> app = m_factory.Create<DashNode> ();
  app->SetPeersAddresses(m_peersAddresses);
  app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
  app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
  app->SetNodeInternetSpeeds(m_internetSpeeds);
  app->SetNodeStats(m_nodeStats);
  app->SetProtocolType(m_protocolType);
  app->SetNodeType(m_nodeType);

  node->AddApplication (app);

  return app;
}

void 
DashNodeHelper::SetPeersAddresses (std::vector<Ipv4Address> &peersAddresses)
{
  m_peersAddresses = peersAddresses;	
}

void 
DashNodeHelper::SetPeersDownloadSpeeds (std::map<Ipv4Address, double> &peersDownloadSpeeds)
{
  m_peersDownloadSpeeds = peersDownloadSpeeds;
}

void 
DashNodeHelper::SetPeersUploadSpeeds (std::map<Ipv4Address, double> &peersUploadSpeeds)
{
  m_peersUploadSpeeds = peersUploadSpeeds;
}

void 
DashNodeHelper::SetNodeInternetSpeeds (nodeInternetSpeeds &internetSpeeds)
{
  m_internetSpeeds = internetSpeeds;	
}

void 
DashNodeHelper::SetNodeStats (nodeStatistics *nodeStats)
{
  m_nodeStats = nodeStats;
}

void 
DashNodeHelper::SetProtocolType (enum ProtocolType protocolType)
{
  m_protocolType = protocolType;
}

void 
DashNodeHelper::SetNodeType (enum NodeType nodeType)
{
  m_nodeType = nodeType;
}

} // namespace ns3
