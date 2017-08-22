/**
 * This file contains the definitions of the functions declared in dash-miner-helper.h
 */

#include "ns3/dash-miner-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/dash-miner.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

DashMinerHelper::DashMinerHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                        std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds, 
                                        nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats, double hashRate, double averageBlockGenIntervalSeconds) : 
                                        DashNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD),
                                        m_secureBlocks (6), m_blockGenBinSize (-1), m_blockGenParameter (-1)
{
  m_factory.SetTypeId ("ns3::DashMiner");
  commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
  
  m_noMiners = noMiners;
  m_hashRate = hashRate;
  m_averageBlockGenIntervalSeconds = averageBlockGenIntervalSeconds;
  
  m_factory.Set ("NumberOfMiners", UintegerValue(m_noMiners));
  m_factory.Set ("HashRate", DoubleValue(m_hashRate));
  m_factory.Set ("AverageBlockGenIntervalSeconds", DoubleValue(m_averageBlockGenIntervalSeconds));

}

Ptr<Application>
DashMinerHelper::InstallPriv (Ptr<Node> node) //FIX ME
{

   switch (m_minerType) 
   {
      case NORMAL_MINER: 
      {
        Ptr<DashMiner> app = m_factory.Create<DashMiner> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SIMPLE_ATTACKER: 
      {
        Ptr<DashSimpleAttacker> app = m_factory.Create<DashSimpleAttacker> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SELFISH_MINER: 
      {
        Ptr<DashSelfishMiner> app = m_factory.Create<DashSelfishMiner> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
      case SELFISH_MINER_TRIALS: 
      {
        Ptr<DashSelfishMinerTrials> app = m_factory.Create<DashSelfishMinerTrials> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetBlockBroadcastType(m_blockBroadcastType);
        app->SetProtocolType(m_protocolType);

        node->AddApplication (app);
        return app;
      }
   }
   
}

enum MinerType 
DashMinerHelper::GetMinerType(void)
{
  return m_minerType;
}

void 
DashMinerHelper::SetMinerType (enum MinerType m)  //FIX ME
{
   m_minerType = m;
  
   switch (m) 
   {
      case NORMAL_MINER: 
      {
        m_factory.SetTypeId ("ns3::DashMiner");
        SetFactoryAttributes();
        break;
      }
      case SIMPLE_ATTACKER:  
      {
        m_factory.SetTypeId ("ns3::DashSimpleAttacker");
        SetFactoryAttributes();
        m_factory.Set ("SecureBlocks", UintegerValue(m_secureBlocks));

        break;
      }
      case SELFISH_MINER:  
      {
        m_factory.SetTypeId ("ns3::DashSelfishMiner");
        SetFactoryAttributes();

        break;
      }
      case SELFISH_MINER_TRIALS:  
      {
        m_factory.SetTypeId ("ns3::DashSelfishMinerTrials");
        SetFactoryAttributes();
        m_factory.Set ("SecureBlocks", UintegerValue(m_secureBlocks));

        break;
      }
   }
}


void 
DashMinerHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
{
  m_blockBroadcastType = m;
}


void 
DashMinerHelper::SetFactoryAttributes (void)
{
  m_factory.Set ("Protocol", StringValue (m_protocol));
  m_factory.Set ("Local", AddressValue (m_address));
  m_factory.Set ("NumberOfMiners", UintegerValue(m_noMiners));
  m_factory.Set ("HashRate", DoubleValue(m_hashRate));
  m_factory.Set ("AverageBlockGenIntervalSeconds", DoubleValue(m_averageBlockGenIntervalSeconds));
  
  if (m_blockGenBinSize > 0 && m_blockGenParameter)
  {
    m_factory.Set ("BlockGenBinSize", DoubleValue(m_blockGenBinSize));
    m_factory.Set ("BlockGenParameter", DoubleValue(m_blockGenParameter));
  }
}

} // namespace ns3
