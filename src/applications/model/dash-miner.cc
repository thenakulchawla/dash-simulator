#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/dash-miner.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <array>
#include "uint256.h"
#include "hash.h"


static double GetWallTime();

uint256 GetHash(std::string my_data);

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DashMiner");

NS_OBJECT_ENSURE_REGISTERED (DashMiner);

TypeId 
    DashMiner::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::DashMiner")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<DashMiner> ()
            .AddAttribute ("Local",
                    "The Address on which to Bind the rx socket.",
                    AddressValue (),
                    MakeAddressAccessor (&DashMiner::m_local),
                    MakeAddressChecker ())
            .AddAttribute ("Protocol",
                    "The type id of the protocol to use for the rx socket.",
                    TypeIdValue (UdpSocketFactory::GetTypeId ()),
                    MakeTypeIdAccessor (&DashMiner::m_tid),
                    MakeTypeIdChecker ())
            .AddAttribute ("BlockTorrent",
                    "Enable the BlockTorrent protocol",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DashMiner::m_blockTorrent),
                    MakeBooleanChecker ())
            .AddAttribute ("SPV",
                    "Enable SPV Mechanism",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DashMiner::m_spv),
                    MakeBooleanChecker ())
            .AddAttribute ("raptor",
                    "Enable raptor Mechanism",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DashMiner::m_raptor),
                    MakeBooleanChecker ())
            .AddAttribute ("TargetNumberOfBlocks",
                    "Stop mining blocks after blockchain size is equal to target number of blocks",
                    UintegerValue (500),
                    MakeUintegerAccessor (&DashMiner::m_targetNumberOfBlocks),
                    MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("NumberOfMiners", 
                    "The number of miners",
                    UintegerValue (16),
                    MakeUintegerAccessor (&DashMiner::m_noMiners),
                    MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("FixedBlockSize", 
                    "The fixed size of the block",
                    UintegerValue (0),
                    MakeUintegerAccessor (&DashMiner::m_fixedBlockSize),
                    MakeUintegerChecker<uint32_t> ())				   
            .AddAttribute ("FixedBlockIntervalGeneration", 
                    "The fixed time to wait between two consecutive block generations",
                    DoubleValue (0),
                    MakeDoubleAccessor (&DashMiner::m_fixedBlockTimeGeneration),
                    MakeDoubleChecker<double> ())
            .AddAttribute ("InvTimeoutMinutes", 
                    "The timeout of inv messages in minutes",
                    TimeValue (Minutes (20)),
                    MakeTimeAccessor (&DashMiner::m_invTimeoutMinutes),
                    MakeTimeChecker())
            .AddAttribute ("HashRate", 
                    "The hash rate of the miner",
                    DoubleValue (0.2),
                    MakeDoubleAccessor (&DashMiner::m_hashRate),
                    MakeDoubleChecker<double> ())	
            .AddAttribute ("BlockGenBinSize", 
                    "The block generation bin size",
                    DoubleValue (-1),
                    MakeDoubleAccessor (&DashMiner::m_blockGenBinSize),
                    MakeDoubleChecker<double> ())	
            .AddAttribute ("BlockGenParameter", 
                    "The block generation distribution parameter",
                    DoubleValue (-1),
                    MakeDoubleAccessor (&DashMiner::m_blockGenParameter),
                    MakeDoubleChecker<double> ())	
            .AddAttribute ("AverageBlockGenIntervalSeconds", 
                    "The average block generation interval we aim at (in seconds)",
                    DoubleValue (2.5*60),
                    MakeDoubleAccessor (&DashMiner::m_averageBlockGenIntervalSeconds),
                    MakeDoubleChecker<double> ())
            .AddAttribute ("ChunkSize", 
                    "The fixed size of the block chunk",
                    UintegerValue (100000),
                    MakeUintegerAccessor (&DashMiner::m_chunkSize),
                    MakeUintegerChecker<uint32_t> ())
            .AddTraceSource ("Rx",
                    "A packet has been received",
                    MakeTraceSourceAccessor (&DashMiner::m_rxTrace),
                    "ns3::Packet::AddressTracedCallback")
            ;
        return tid;
    }

DashMiner::DashMiner () : DashNode(), m_realAverageBlockGenIntervalSeconds(2.5*m_secondsPerMin),
m_timeStart (0), m_timeFinish (0), m_fistToMine (false)
{
    m_minerAverageBlockGenInterval = 0;
    m_minerGeneratedBlocks = 0;
    m_previousBlockGenerationTime = 0;


    std::random_device rd;
    m_generator.seed(rd());

    if (m_fixedBlockTimeGeneration > 0)
        m_nextBlockTime = m_fixedBlockTimeGeneration;  
    else
        m_nextBlockTime = 0;

    if (m_fixedBlockSize > 0)
        m_nextBlockSize = m_fixedBlockSize;
    else
        m_nextBlockSize = 0;

    m_isMiner = true;
}


DashMiner::~DashMiner(void)
{
    // NS_LOG_FUNCTION (this);
}


void 
DashMiner::StartApplication ()    // Called at time specified by Start
{
    // NS_LOG_FUNCTION(this);
    DashNode::StartApplication ();
    // NS_LOG_WARN ("Miner " << GetNode()->GetId() << " m_noMiners = " << m_noMiners << "");
    // NS_LOG_WARN ("Miner " << GetNode()->GetId() << " m_realAverageBlockGenIntervalSeconds = " << m_realAverageBlockGenIntervalSeconds << "s");
    // NS_LOG_WARN ("Miner " << GetNode()->GetId() << " m_averageBlockGenIntervalSeconds = " << m_averageBlockGenIntervalSeconds << "s");
    // NS_LOG_WARN ("Miner " << GetNode()->GetId() << " m_fixedBlockTimeGeneration = " << m_fixedBlockTimeGeneration << "s");
    // NS_LOG_WARN ("Miner " << GetNode()->GetId() << " m_hashRate = " << m_hashRate );
    // NS_LOG_WARN ("Miner " << GetNode()->GetId() << " m_blockBroadcastType = " << getBlockBroadcastType(m_blockBroadcastType));

    if (m_blockGenBinSize < 0 && m_blockGenParameter < 0)
    {
        m_blockGenBinSize = 1./m_secondsPerMin/1000;
        m_blockGenParameter = 0.19 * m_blockGenBinSize / 2;
    }
    else
        m_blockGenParameter *= m_hashRate;

    if (m_fixedBlockTimeGeneration == 0)
        m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter)); 

    if (m_fixedBlockSize > 0)
        m_nextBlockSize = m_fixedBlockSize;
    else
    {
        std::array<double,201> intervals {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125, 
                                        130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200, 205, 210, 215, 220, 225, 230, 235, 
                                        240, 245, 250, 255, 260, 265, 270, 275, 280, 285, 290, 295, 300, 305, 310, 315, 320, 325, 330, 335, 340, 345, 
                                        350, 355, 360, 365, 370, 375, 380, 385, 390, 395, 400, 405, 410, 415, 420, 425, 430, 435, 440, 445, 450, 455, 
                                        460, 465, 470, 475, 480, 485, 490, 495, 500, 505, 510, 515, 520, 525, 530, 535, 540, 545, 550, 555, 560, 565, 
                                        570, 575, 580, 585, 590, 595, 600, 605, 610, 615, 620, 625, 630, 635, 640, 645, 650, 655, 660, 665, 670, 675, 
                                        680, 685, 690, 695, 700, 705, 710, 715, 720, 725, 730, 735, 740, 745, 750, 755, 760, 765, 770, 775, 780, 785, 
                                        790, 795, 800, 805, 810, 815, 820, 825, 830, 835, 840, 845, 850, 855, 860, 865, 870, 875, 880, 885, 890, 895, 
                                        900, 905, 910, 915, 920, 925, 930, 935, 940, 945, 950, 955, 960, 965, 970, 975, 980, 985, 990, 995, 1000};

        std::array<double,200> weights {4.96, 0.21, 0.17, 0.25, 0.27, 0.3, 0.34, 0.26, 0.26, 0.33, 0.35, 0.49, 0.42, 0.42, 0.48, 0.41, 0.46, 0.45, 
                                      0.58, 0.58, 0.57, 0.52, 0.54, 0.47, 0.53, 0.56, 0.5, 0.48, 0.53, 0.54, 0.49, 0.51, 0.56, 0.53, 0.56, 0.5, 
                                      0.47, 0.45, 0.52, 0.43, 0.46, 0.47, 0.6, 0.53, 0.42, 0.48, 0.55, 0.49, 0.63, 2.38, 0.47, 0.53, 0.43, 0.51, 
                                      0.44, 0.46, 0.44, 0.41, 0.47, 0.46, 0.45, 0.37, 0.49, 0.4, 0.41, 0.41, 0.41, 0.37, 0.43, 0.47, 0.48, 0.37, 
                                      0.4, 0.46, 0.34, 0.35, 0.37, 0.36, 0.37, 0.31, 0.35, 0.39, 0.34, 0.38, 0.29, 0.41, 0.37, 0.34, 0.36, 0.34, 
                                      0.29, 0.3, 0.36, 0.26, 0.29, 0.31, 0.3, 0.29, 0.35, 0.5, 0.28, 0.37, 0.31, 0.33, 0.32, 0.28, 0.34, 0.31, 
                                      0.26, 0.24, 0.22, 0.25, 0.24, 0.25, 0.26, 0.25, 0.24, 0.33, 0.24, 0.23, 0.2, 0.24, 0.26, 0.27, 0.27, 0.21, 
                                      0.22, 0.3, 0.25, 0.21, 0.26, 0.21, 0.21, 0.21, 0.23, 0.48, 0.2, 0.19, 0.21, 0.2, 0.17, 0.19, 0.21, 0.22, 
                                      0.24, 0.25, 0.23, 0.31, 0.46, 8.32, 0.22, 0.11, 0.13, 0.17, 0.12, 0.16, 0.15, 0.16, 0.19, 0.21, 0.18, 0.24, 
                                      0.19, 0.2, 0.16, 0.17, 0.19, 0.17, 0.22, 0.33, 0.17, 0.22, 0.25, 0.19, 0.2, 0.17, 0.28, 0.25, 0.24, 0.25, 0.3,
                                      0.34, 0.46, 0.49, 0.67, 3.13, 2.94, 0.14, 0.36, 3.88, 0.07, 0.11, 0.11, 0.11, 0.26, 0.12, 0.13, 0.88, 5.84, 4.11};
        m_blockSizeDistribution = std::piecewise_constant_distribution<double> (intervals.begin(), intervals.end(), weights.begin());

        //std::array<double,14> iCount{1000,1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300};
        //std::array<double,13> wCount{2,0,3,7,10,10,17,13,30,29,27,26,6};

    }

    // if (GetNode()->GetId() == 0)
    // {
    //   Block newBlock(1, 0, -1, 500000, 0, 0, Ipv4Address("0.0.0.0"));
    //   m_blockchain.AddBlock(newBlock); 
    // } 

    m_nodeStats->hashRate = m_hashRate;
    m_nodeStats->miner = 1;

    ScheduleNextMiningEvent ();
}

void 
DashMiner::StopApplication ()
{
    // NS_LOG_FUNCTION (this);
    DashNode::StopApplication ();  
    Simulator::Cancel (m_nextMiningEvent);

    NS_LOG_WARN ("The miner " << GetNode ()->GetId () << " with hash rate = " << m_hashRate << " generated " << m_minerGeneratedBlocks 
            << " blocks "<< "(" << 100. * m_minerGeneratedBlocks / (m_blockchain.GetTotalBlocks() - 1) 
            << "%) with average block generation time = " << m_minerAverageBlockGenInterval
            << "s or " << static_cast<int>(m_minerAverageBlockGenInterval) / m_secondsPerMin << "min and " 
            << m_minerAverageBlockGenInterval - static_cast<int>(m_minerAverageBlockGenInterval) / m_secondsPerMin * m_secondsPerMin << "s"
            << " and average size " << m_minerAverageBlockSize << " Bytes");


    m_nodeStats->minerGeneratedBlocks = m_minerGeneratedBlocks;
    m_nodeStats->minerAverageBlockGenInterval = m_minerAverageBlockGenInterval;
    m_nodeStats->minerAverageBlockSize = m_minerAverageBlockSize;

    if (m_fistToMine)
    {
        m_timeFinish = GetWallTime();
        std::cout << "Time/Block = " << (m_timeFinish - m_timeStart) / (m_blockchain.GetTotalBlocks() - 1) << "s\n";
    }
}

void 
DashMiner::DoDispose (void)
{
    // NS_LOG_FUNCTION(this);
    DashNode::DoDispose ();
}

double 
DashMiner::GetFixedBlockTimeGeneration(void) const
{
    return m_fixedBlockTimeGeneration;
}

void 
DashMiner::SetFixedBlockTimeGeneration(double fixedBlockTimeGeneration) 
{
    m_fixedBlockTimeGeneration = fixedBlockTimeGeneration;
}

uint32_t 
DashMiner::GetFixedBlockSize(void) const
{
    return m_fixedBlockSize;
}

void 
DashMiner::SetFixedBlockSize(uint32_t fixedBlockSize) 
{
    m_fixedBlockSize = fixedBlockSize;
}

double 
DashMiner::GetBlockGenBinSize(void) const
{
    return m_blockGenBinSize;	
}

void 
DashMiner::SetBlockGenBinSize (double blockGenBinSize)
{
    m_blockGenBinSize = blockGenBinSize;	
}

double 
DashMiner::GetBlockGenParameter(void) const
{
    return m_blockGenParameter;	
}

void 
DashMiner::SetBlockGenParameter (double blockGenParameter)
{
    // NS_LOG_FUNCTION (this);
    m_blockGenParameter = blockGenParameter;
    m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter));

}

double 
DashMiner::GetHashRate(void) const
{
    return m_hashRate;	
}

void 
DashMiner::SetHashRate (double hashRate)
{
    m_hashRate = hashRate;
}

void 
DashMiner::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
    m_blockBroadcastType = blockBroadcastType;
}

void
DashMiner::ScheduleNextMiningEvent (void)
{
    // NS_LOG_FUNCTION(this);

    m_nextBlockTime = m_blockGenTimeDistribution(m_generator)*m_blockGenBinSize*m_secondsPerMin
        *( m_averageBlockGenIntervalSeconds/m_realAverageBlockGenIntervalSeconds )/m_hashRate;

    NS_LOG_DEBUG("m_nextBlockTime = " << m_nextBlockTime << ", binsize = " << m_blockGenBinSize << ", m_blockGenParameter = " << m_blockGenParameter << ", hashrate = " << m_hashRate);

    m_nextMiningEvent = Simulator::Schedule (Seconds(m_nextBlockTime), &DashMiner::MineBlock, this);

    // NS_LOG_WARN ("Time " << Simulator::Now ().GetSeconds () << ": Miner " << GetNode ()->GetId () << " will generate a block in " 
    //         << m_nextBlockTime << "s or " << static_cast<int>(m_nextBlockTime) / m_secondsPerMin 
    //         << "  min and  " << static_cast<int>(m_nextBlockTime) % m_secondsPerMin 
    //         << "s using Geometric Block Time Generation with parameter = "<< m_blockGenParameter);

}

void 
DashMiner::MineBlock (void)
{
    // NS_LOG_FUNCTION(this);

    rapidjson::Document inv; 
    rapidjson::Document block; 

    int height =  m_blockchain.GetCurrentTopBlock()->GetBlockHeight() + 1;
    int minerId = GetNode ()->GetId ();
    int parentBlockMinerId = m_blockchain.GetCurrentTopBlock()->GetMinerId();
    double currentTime = Simulator::Now ().GetSeconds ();
    std::ostringstream stringStream;  
    std::string blockHash;

    stringStream << height << "/" << minerId;
    blockHash = stringStream.str();

    inv.SetObject();
    block.SetObject();

    if (height == 1)
    {
        m_fistToMine = true;
        m_timeStart = GetWallTime();
    }


    // //For attacks
    // if (GetNode ()->GetId () == 0)
    //   height = 2 - m_minerGeneratedBlocks; 
    //
    // if (GetNode ()->GetId () == 0)
    // {
    // if (height == 1)
    //    parentBlockMinerId = -1;
    //  else 
    //  parentBlockMinerId = 0;


    // reduce these conditions by thinking clear 
    if (m_fixedBlockSize > 0)
    {
        m_nextBlockSize = m_fixedBlockSize;
    }
    else
    {

        m_nextBlockSize = m_blockSizeDistribution(m_generator) * 1000;	// *1000 because the m_blockSizeDistribution returns KBytes

    }

    // The block size is linearly dependent on the averageBlockGenIntervalSeconds

    if(m_nextBlockSize < m_maxBlockSize - m_headersSizeBytes)
        m_nextBlockSize = m_nextBlockSize * m_averageBlockGenIntervalSeconds / m_realAverageBlockGenIntervalSeconds
            + m_headersSizeBytes;	
    else
        m_nextBlockSize = m_nextBlockSize * m_averageBlockGenIntervalSeconds / m_realAverageBlockGenIntervalSeconds;


    if (m_nextBlockSize < averageTransactionSize )
        m_nextBlockSize = averageTransactionSize + m_headersSizeBytes;


    Block newBlock (height, minerId, parentBlockMinerId, m_nextBlockSize,
            currentTime, currentTime, Ipv4Address("127.0.0.1"));

    // NS_LOG_INFO("New block mined");


    // NS_LOG_INFO("m_raptor "<< m_raptor<<std::endl);
    switch(m_blockBroadcastType)				  
    {
        case STANDARD:
            {

                if (m_protocolType == STANDARD_PROTOCOL)
                {

                    rapidjson::Value value;
                    rapidjson::Value array(rapidjson::kArrayType);
                    rapidjson::Value blockInfo(rapidjson::kObjectType);
                    rapidjson::Value raptorArray(rapidjson::kArrayType);
                    rapidjson::Value raptorInfo(rapidjson::kObjectType);

                    value.SetString("block"); //Remove
                    inv.AddMember("type", value, inv.GetAllocator());

                    if (!m_blockTorrent && !m_raptor)
                    {
                        value = INV;
                        inv.AddMember("message", value, inv.GetAllocator());

                        value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
                        array.PushBack(value, inv.GetAllocator());

                        inv.AddMember("inv", array, inv.GetAllocator()); 
                    }
                    else if (m_raptor)
                    {
                        // NS_LOG_INFO("m_raptor in Standard protocol "<< m_raptor<<std::endl);
                        // rapidjson::Value value;
                        // rapidjson::Value array(rapidjson::kArrayType);

                        inv.RemoveMember("type");

                        value.SetString("raptor"); //Remove
                        inv.AddMember("type", value, inv.GetAllocator());


                        value = RAPTORCODE;
                        inv.AddMember("message", value, inv.GetAllocator());

                        int symbolSize = static_cast<double>(m_nextBlockSize/10);
                        value = symbolSize;
                        raptorInfo.AddMember("symbolSize", value, inv.GetAllocator());

                        value = newBlock.GetBlockSizeBytes();
                        raptorInfo.AddMember("blockSize", value, inv.GetAllocator());

                        // value = m_nextBlockSize/symbolSize;
                        value = 10;
                        raptorInfo.AddMember("symbolCount", value, inv.GetAllocator());

                        value = static_cast<int>(m_nextBlockSize/symbolSize);
                        raptorInfo.AddMember("symbolsRequired", value, inv.GetAllocator());

                        value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
                        raptorInfo.AddMember("hash", value, inv.GetAllocator ());

                        value = newBlock.GetBlockHeight ();
                        raptorInfo.AddMember("height", value, inv.GetAllocator ());

                        value = newBlock.GetMinerId ();
                        raptorInfo.AddMember("minerId", value, inv.GetAllocator ());

                        value = newBlock.GetParentBlockMinerId ();
                        raptorInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

                        // value = newBlock.GetBlockSizeBytes ();
                        // raptorInfo.AddMember("size", value, inv.GetAllocator ());

                        value = newBlock.GetTimeCreated ();
                        raptorInfo.AddMember("timeCreated", value, inv.GetAllocator ());

                        value = newBlock.GetTimeReceived();
                        raptorInfo.AddMember("timeReceived", value, inv.GetAllocator ());

                        raptorArray.PushBack(raptorInfo, inv.GetAllocator());
                        inv.AddMember("raptors", raptorArray, inv.GetAllocator()); 

                        value = newBlock.GetBlockHeight ();
                        blockInfo.AddMember("height", value, inv.GetAllocator ());

                        value = newBlock.GetMinerId ();
                        blockInfo.AddMember("minerId", value, inv.GetAllocator ());

                        value = newBlock.GetParentBlockMinerId ();
                        blockInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

                        value = newBlock.GetBlockSizeBytes ();
                        blockInfo.AddMember("size", value, inv.GetAllocator ());

                        value = newBlock.GetTimeCreated ();
                        blockInfo.AddMember("timeCreated", value, inv.GetAllocator ());

                        value = newBlock.GetTimeReceived ();							
                        blockInfo.AddMember("timeReceived", value, inv.GetAllocator ());

                        array.PushBack(blockInfo, inv.GetAllocator());
                        inv.AddMember("blocks", array, inv.GetAllocator()); 


                    }
                    else
                    {
                        value = EXT_INV;
                        inv.AddMember("message", value, inv.GetAllocator());

                        value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
                        blockInfo.AddMember("hash", value, inv.GetAllocator ());

                        value = newBlock.GetBlockSizeBytes ();
                        blockInfo.AddMember("size", value, inv.GetAllocator ());

                        value = true;
                        blockInfo.AddMember("fullBlock", value, inv.GetAllocator ());

                        array.PushBack(blockInfo, inv.GetAllocator());
                        inv.AddMember("inv", array, inv.GetAllocator()); 
                    }
                }
                else if (m_protocolType == SENDHEADERS)
                {
                    rapidjson::Value value;
                    rapidjson::Value array(rapidjson::kArrayType);
                    rapidjson::Value blockInfo(rapidjson::kObjectType);

                    value.SetString("block"); //Remove
                    inv.AddMember("type", value, inv.GetAllocator());

                    value = newBlock.GetBlockHeight ();
                    blockInfo.AddMember("height", value, inv.GetAllocator ());

                    value = newBlock.GetMinerId ();
                    blockInfo.AddMember("minerId", value, inv.GetAllocator ());

                    value = newBlock.GetParentBlockMinerId ();
                    blockInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

                    value = newBlock.GetBlockSizeBytes ();
                    blockInfo.AddMember("size", value, inv.GetAllocator ());

                    value = newBlock.GetTimeCreated ();
                    blockInfo.AddMember("timeCreated", value, inv.GetAllocator ());

                    value = newBlock.GetTimeReceived ();							
                    blockInfo.AddMember("timeReceived", value, inv.GetAllocator ());

                    if (!m_blockTorrent)
                    {
                        value = HEADERS;
                        inv.AddMember("message", value, inv.GetAllocator()); 
                    }
                    else
                    {
                        value = EXT_HEADERS;
                        inv.AddMember("message", value, inv.GetAllocator());

                        value = true;
                        blockInfo.AddMember("fullBlock", value, inv.GetAllocator ());
                    }

                    array.PushBack(blockInfo, inv.GetAllocator());
                    inv.AddMember("blocks", array, inv.GetAllocator());      
                }	

                break;
            }
        case UNSOLICITED:
            {
                rapidjson::Value value (BLOCK);
                rapidjson::Value blockInfo(rapidjson::kObjectType);
                rapidjson::Value array(rapidjson::kArrayType);

                block.AddMember("message", value, block.GetAllocator());

                value.SetString("block"); //Remove
                block.AddMember("type", value, block.GetAllocator());

                value = newBlock.GetBlockHeight ();
                blockInfo.AddMember("height", value, block.GetAllocator ());

                value = newBlock.GetMinerId ();
                blockInfo.AddMember("minerId", value, block.GetAllocator ());

                value = newBlock.GetParentBlockMinerId ();
                blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

                value = newBlock.GetBlockSizeBytes ();
                blockInfo.AddMember("size", value, block.GetAllocator ());

                value = newBlock.GetTimeCreated ();
                blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

                value = newBlock.GetTimeReceived ();							
                blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

                array.PushBack(blockInfo, block.GetAllocator());
                block.AddMember("blocks", array, block.GetAllocator());

                break;
            }
        case RELAY_NETWORK:
            {
                rapidjson::Value value;
                rapidjson::Value headersInfo(rapidjson::kObjectType);
                rapidjson::Value chunkInfo(rapidjson::kObjectType);
                rapidjson::Value blockInfo(rapidjson::kObjectType);
                rapidjson::Value invArray(rapidjson::kArrayType);
                rapidjson::Value blockArray(rapidjson::kArrayType);

                value.SetString("block"); //Remove
                inv.AddMember("type", value, inv.GetAllocator());

                if (m_protocolType == STANDARD_PROTOCOL)
                {
                    if (!m_blockTorrent)
                    {
                        value = INV;
                        inv.AddMember("message", value, inv.GetAllocator());

                        value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
                        invArray.PushBack(value, inv.GetAllocator());

                        inv.AddMember("inv", invArray, inv.GetAllocator()); 
                    }
                    else
                    {
                        value = EXT_INV;
                        inv.AddMember("message", value, inv.GetAllocator());

                        value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
                        chunkInfo.AddMember("hash", value, inv.GetAllocator ());

                        value = newBlock.GetBlockSizeBytes ();
                        chunkInfo.AddMember("size", value, inv.GetAllocator ());

                        value = true;
                        chunkInfo.AddMember("fullBlock", value, inv.GetAllocator ());

                        invArray.PushBack(chunkInfo, inv.GetAllocator());
                        inv.AddMember("inv", invArray, inv.GetAllocator()); 
                    }
                }

                else if (m_protocolType == SENDHEADERS)
                {

                    value = newBlock.GetBlockHeight ();
                    headersInfo.AddMember("height", value, inv.GetAllocator ());

                    value = newBlock.GetMinerId ();
                    headersInfo.AddMember("minerId", value, inv.GetAllocator ());

                    value = newBlock.GetParentBlockMinerId ();
                    headersInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

                    value = newBlock.GetBlockSizeBytes ();
                    headersInfo.AddMember("size", value, inv.GetAllocator ());

                    value = newBlock.GetTimeCreated ();
                    headersInfo.AddMember("timeCreated", value, inv.GetAllocator ());

                    value = newBlock.GetTimeReceived ();							
                    headersInfo.AddMember("timeReceived", value, inv.GetAllocator ());

                    if (!m_blockTorrent)
                    {
                        value = HEADERS;
                        inv.AddMember("message", value, inv.GetAllocator()); 
                    }
                    else
                    {
                        value = EXT_HEADERS;
                        inv.AddMember("message", value, inv.GetAllocator());

                        value = true;
                        headersInfo.AddMember("fullBlock", value, inv.GetAllocator ());
                    }

                    invArray.PushBack(headersInfo, inv.GetAllocator());
                    inv.AddMember("blocks", invArray, inv.GetAllocator());      
                }	

                //Unsolicited for miners
                value = BLOCK;
                block.AddMember("message", value, block.GetAllocator());

                value.SetString("compressed-block"); //Remove
                block.AddMember("type", value, block.GetAllocator());

                value = newBlock.GetBlockHeight ();
                blockInfo.AddMember("height", value, block.GetAllocator ());

                value = newBlock.GetMinerId ();
                blockInfo.AddMember("minerId", value, block.GetAllocator ());

                value = newBlock.GetParentBlockMinerId ();
                blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

                value = newBlock.GetBlockSizeBytes ();
                blockInfo.AddMember("size", value, block.GetAllocator ());

                value = newBlock.GetTimeCreated ();
                blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

                value = newBlock.GetTimeReceived ();							
                blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

                blockArray.PushBack(blockInfo, block.GetAllocator());
                block.AddMember("blocks", blockArray, block.GetAllocator());

                break;
            }
        case UNSOLICITED_RELAY_NETWORK:
            {
                rapidjson::Value value;
                rapidjson::Value blockNodesInfo(rapidjson::kObjectType);
                rapidjson::Value blockInfo(rapidjson::kObjectType);
                rapidjson::Value invArray(rapidjson::kArrayType);
                rapidjson::Value blockArray(rapidjson::kArrayType);

                //Unsolicited for nodes
                value = BLOCK;
                inv.AddMember("message", value, inv.GetAllocator());

                value.SetString("block"); //Remove
                inv.AddMember("type", value, inv.GetAllocator());

                value = newBlock.GetBlockHeight ();
                blockNodesInfo.AddMember("height", value, inv.GetAllocator ());

                value = newBlock.GetMinerId ();
                blockNodesInfo.AddMember("minerId", value, inv.GetAllocator ());

                value = newBlock.GetParentBlockMinerId ();
                blockNodesInfo.AddMember("parentBlockMinerId", value, inv.GetAllocator ());

                value = newBlock.GetBlockSizeBytes ();
                blockNodesInfo.AddMember("size", value, inv.GetAllocator ());

                value = newBlock.GetTimeCreated ();
                blockNodesInfo.AddMember("timeCreated", value, inv.GetAllocator ());

                value = newBlock.GetTimeReceived ();							
                blockNodesInfo.AddMember("timeReceived", value, inv.GetAllocator ());

                invArray.PushBack(blockNodesInfo, inv.GetAllocator());
                inv.AddMember("blocks", invArray, inv.GetAllocator());


                //Unsolicited for miners
                value = BLOCK;
                block.AddMember("message", value, block.GetAllocator());

                value.SetString("compressed-block"); //Remove
                block.AddMember("type", value, block.GetAllocator());

                value = newBlock.GetBlockHeight ();
                blockInfo.AddMember("height", value, block.GetAllocator ());

                value = newBlock.GetMinerId ();
                blockInfo.AddMember("minerId", value, block.GetAllocator ());

                value = newBlock.GetParentBlockMinerId ();
                blockInfo.AddMember("parentBlockMinerId", value, block.GetAllocator ());

                value = newBlock.GetBlockSizeBytes ();
                blockInfo.AddMember("size", value, block.GetAllocator ());

                value = newBlock.GetTimeCreated ();
                blockInfo.AddMember("timeCreated", value, block.GetAllocator ());

                value = newBlock.GetTimeReceived ();							
                blockInfo.AddMember("timeReceived", value, block.GetAllocator ());

                blockArray.PushBack(blockInfo, block.GetAllocator());
                block.AddMember("blocks", blockArray, block.GetAllocator());

                break;
            }
    }


    /**
     * Update m_meanBlockReceiveTime with the timeCreated of the newly generated block
     */

    m_meanBlockReceiveTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockReceiveTime 
        + (currentTime - m_previousBlockReceiveTime)/(m_blockchain.GetTotalBlocks());

    m_previousBlockReceiveTime = currentTime;	

    m_meanBlockPropagationTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockPropagationTime;

    m_meanBlockSize = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockSize  
        + (m_nextBlockSize)/static_cast<double>(m_blockchain.GetTotalBlocks());

    // if(!m_blockchain.HasBlock(newBlock))
    // {
    //     std::cout<< "New Block Added to Blockchain\n";
    //     std::cout<< "Block count for miner : "<<GetNode()->GetId() << " is: " << m_blockchain.GetTotalBlocks() << "\n";
    //     // std::cout<<"Block size: " << m_nextBlockSize << "\n";
    //     NS_LOG_INFO("Block height: " << newBlock.GetBlockHeight() << " miner Id: " << newBlock.GetMinerId() );
    // }
    m_blockchain.AddBlock(newBlock);

    // m_mempool.DeleteTransactionsFromBegin(newBlock.GetTransactionCount());

    // Stringify the DOM
    rapidjson::StringBuffer invInfo;
    rapidjson::Writer<rapidjson::StringBuffer> invWriter(invInfo);
    inv.Accept(invWriter);

    rapidjson::StringBuffer blockInfo;
    rapidjson::Writer<rapidjson::StringBuffer> blockWriter(blockInfo);
    block.Accept(blockWriter);

    // uint256 toPrintInv = GetHash(invInfo.GetString());
    // uint256 toPrintBlock = GetHash(blockInfo.GetString());

    //std::cout<<"My uint256 cheaphash for Inv is : "<< toPrintInv.GetCheapHash()<<'\n';
    //std::cout<<"My uint256 cheaphash for Block is : "<< toPrintBlock.GetCheapHash()<<'\n';

    int count = 0;

    // NS_LOG_INFO("invInfo is :" <<invInfo.GetString());
    // NS_LOG_INFO("blockInfo is :" <<blockInfo.GetString());


    for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
    {

        const uint8_t delimiter[] = "#";

        switch(m_blockBroadcastType)				  
        {
            case STANDARD:
                {

                    if (m_protocolType == STANDARD_PROTOCOL && !m_blockTorrent && !m_raptor)
                    {
                        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
                        m_peersSockets[*i]->Send (delimiter, 1, 0);
                        m_nodeStats->invSentBytes += m_dashMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
                    }
                    else if (m_protocolType == SENDHEADERS && !m_blockTorrent)
                    {
                        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
                        m_peersSockets[*i]->Send (delimiter, 1, 0);
                        m_nodeStats->headersSentBytes += m_dashMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;

                    }
                    else if (m_protocolType == STANDARD_PROTOCOL && m_blockTorrent)
                    {
                        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
                        m_peersSockets[*i]->Send (delimiter, 1, 0);
                        m_nodeStats->extInvSentBytes += m_dashMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
                        for (int j=0; j<inv["inv"].Size(); j++)
                        {
                            m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
                            if (!inv["inv"][j]["fullBlock"].GetBool())
                                m_nodeStats->extInvSentBytes += inv["inv"][j]["availableChunks"].Size()*1;
                        }
                    }
                    else if (m_protocolType == SENDHEADERS && m_blockTorrent)
                    {
                        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
                        m_peersSockets[*i]->Send (delimiter, 1, 0);
                        m_nodeStats->extHeadersSentBytes += m_dashMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
                        for (int j=0; j<inv["blocks"].Size(); j++)
                        {
                            m_nodeStats->extHeadersSentBytes += 1;//fullBlock
                            if (!inv["blocks"][j]["fullBlock"].GetBool())
                                m_nodeStats->extHeadersSentBytes += inv["inv"][j]["availableChunks"].Size();
                        }	
                    }
                    else if (m_protocolType ==STANDARD_PROTOCOL &&  m_raptor)
                    {
                        int symbolCount =inv["raptors"][0]["symbolCount"].GetInt() ;
                        double symbolSize =inv["raptors"][0]["symbolSize"].GetDouble() ;

                        for (int j=0;j< symbolCount ; ++j)
                        {
                            double sendTime = symbolSize/m_uploadSpeed;
                            double eventTime = 0;
                            m_nodeStats->raptorSentBytes += symbolSize; 
                            if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
                            {
                                eventTime = 0; 
                            }
                            else
                            {
                                eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds(); 
                            }
                            m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
                            std::string packet = invInfo.GetString();
                            Simulator::Schedule (Seconds(eventTime), &DashMiner::SendRaptorSymbol, this, packet, m_peersSockets[*i]);
                            Simulator::Schedule (Seconds(eventTime + sendTime), &DashMiner::RemoveSendTime, this);

                        }
                    }

                    break;
                }
            case UNSOLICITED:
                {
                    m_nodeStats->blockSentBytes += m_dashMessageHeader + block["blocks"][0]["size"].GetInt();

                    double sendTime = m_nextBlockSize / m_uploadSpeed;
                    double eventTime;	

                    if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
                    {
                        eventTime = 0; 
                    }
                    else
                    {
                        eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds(); 
                    }
                    m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);


                    std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl;
                    NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i 
                            << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

                    std::string packet = blockInfo.GetString();
                    Simulator::Schedule (Seconds(eventTime), &DashMiner::SendBlock, this, packet, m_peersSockets[*i]);
                    Simulator::Schedule (Seconds(eventTime + sendTime), &DashMiner::RemoveSendTime, this);

                    break;
                }
            case RELAY_NETWORK:
                {
                    if(count < m_noMiners - 1)
                    {
			int transactionCount = static_cast<int>((m_nextBlockSize - m_blockHeadersSizeBytes)/ m_averageTransactionSize);
			long blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize * transactionCount;
                        double sendTime = blockSize / m_uploadSpeed;
                        double eventTime;

                        m_nodeStats->blockSentBytes += m_dashMessageHeader + blockSize;

                        /* 				std::cout << "Node " << GetNode()->GetId() << "-" << *i 
                                                        << " " << m_peersDownloadSpeeds[*i] << " Mbps , time = "
                                                        << Simulator::Now ().GetSeconds() << "s \n"; */

                        if (m_sendCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendCompressedBlockTimes.back())
                        {
                            eventTime = 0; 
                        }
                        else
                        {
                            //std::cout << "m_sendCompressedBlockTimes.back() = m_sendCompressedBlockTimes.back() = " << m_sendCompressedBlockTimes.back() << std::endl;
                            eventTime = m_sendCompressedBlockTimes.back() - Simulator::Now ().GetSeconds(); 
                        }
                        m_sendCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);

                        //std::cout << sendTime << " " << eventTime << " " << m_sendCompressedBlockTimes.size() << std::endl;
                        NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                                << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

                        //sendTime = blockSize / m_uploadSpeed * count;		  
                        //std::cout << sendTime << std::endl;

                        std::string packet = blockInfo.GetString();
                        Simulator::Schedule (Seconds(sendTime), &DashMiner::SendBlock, this, packet, m_peersSockets[*i]);
                        Simulator::Schedule (Seconds(eventTime + sendTime), &DashMiner::RemoveCompressedBlockSendTime, this);

                    }
                    else
                    {	    
                        m_peersSockets[*i]->Send (reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
                        m_peersSockets[*i]->Send (delimiter, 1, 0);

                        if (m_protocolType == STANDARD_PROTOCOL && !m_blockTorrent)
                            m_nodeStats->invSentBytes += m_dashMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
                        else if (m_protocolType == SENDHEADERS && !m_blockTorrent)
                            m_nodeStats->headersSentBytes += m_dashMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
                        else if (m_protocolType == STANDARD_PROTOCOL && m_blockTorrent)
                        {
                            m_nodeStats->extInvSentBytes += m_dashMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
                            for (int j=0; j<inv["inv"].Size(); j++)
                            {
                                m_nodeStats->extInvSentBytes += 5; //1Byte(fullBlock) + 4Bytes(numberOfChunks)
                                if (!inv["inv"][j]["fullBlock"].GetBool())
                                    m_nodeStats->extInvSentBytes += inv["inv"][j]["availableChunks"].Size()*1;
                            }
                        }
                        else if (m_protocolType == SENDHEADERS && m_blockTorrent)
                        {
                            m_nodeStats->extHeadersSentBytes += m_dashMessageHeader + m_countBytes + inv["blocks"].Size()*m_headersSizeBytes;
                            for (int j=0; j<inv["blocks"].Size(); j++)
                            {
                                m_nodeStats->extHeadersSentBytes += 1;//fullBlock
                                if (!inv["blocks"][j]["fullBlock"].GetBool())
                                    m_nodeStats->extHeadersSentBytes += inv["blocks"][j]["availableChunks"].Size()*1;
                            }	
                        }

                        NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                                << "s dash miner " << GetNode ()->GetId () 
                                << " sent a packet invInfo " 
                                << " to " << *i);
                    }
                    break;
                }
            case UNSOLICITED_RELAY_NETWORK:
                {
                    double sendTime;
                    double eventTime;
                    std::string packet;

                    std::cout << "Node " << GetNode()->GetId() << "-" << *i 
                        << " " << m_peersDownloadSpeeds[*i] << " Mbps , time = "
                        << Simulator::Now ().GetSeconds() << "s \n";

                    if(count < m_noMiners - 1)
                    {
			int transactionCount = static_cast<int>((m_nextBlockSize - m_blockHeadersSizeBytes)/ m_averageTransactionSize);
			long blockSize = m_blockHeadersSizeBytes + m_transactionIndexSize * transactionCount;
                        sendTime = blockSize / m_uploadSpeed;

                        m_nodeStats->blockSentBytes += m_dashMessageHeader + blockSize;

                        if (m_sendCompressedBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendCompressedBlockTimes.back())
                        {
                            eventTime = 0; 
                        }
                        else
                        {
                            //std::cout << "m_sendCompressedBlockTimes.back() = m_sendCompressedBlockTimes.back() = " << m_sendCompressedBlockTimes.back() << std::endl;
                            eventTime = m_sendCompressedBlockTimes.back() - Simulator::Now ().GetSeconds(); 
                        }
                        m_sendCompressedBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);

                        //std::cout << sendTime << " " << eventTime << " " << m_sendCompressedBlockTimes.size() << std::endl;
                        NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to " << *i
                                << " at " << Simulator::Now ().GetSeconds() + eventTime << "\n");

                        //sendTime = blockSize / m_uploadSpeed * count;		  
                        //std::cout << sendTime << std::endl;

                        std::string packet = blockInfo.GetString();
                        Simulator::Schedule (Seconds(sendTime), &DashMiner::SendBlock, this, packet, m_peersSockets[*i]);
                        Simulator::Schedule (Seconds(eventTime + sendTime), &DashMiner::RemoveCompressedBlockSendTime, this);
                    }
                    else
                    {
                        sendTime = m_nextBlockSize / m_uploadSpeed;
                        m_nodeStats->blockSentBytes += m_dashMessageHeader + m_nextBlockSize;
                        if (m_sendBlockTimes.size() == 0 || Simulator::Now ().GetSeconds() >  m_sendBlockTimes.back())
                        {
                            eventTime = 0; 
                        }
                        else
                        {
                            //std::cout << "m_sendBlockTimes.back() = m_sendBlockTimes.back() = " << m_sendBlockTimes.back() << std::endl;
                            eventTime = m_sendBlockTimes.back() - Simulator::Now ().GetSeconds(); 
                        }
                        m_sendBlockTimes.push_back(Simulator::Now ().GetSeconds() + eventTime + sendTime);
                        packet = invInfo.GetString();

                        /* std::cout << sendTime << " " << eventTime << " " << m_sendBlockTimes.size() << std::endl; */
                        NS_LOG_INFO("Node " << GetNode()->GetId() << " will send the block to " << *i 
                                << " at " << Simulator::Now ().GetSeconds() + eventTime << ", eventTime = " << eventTime  << "\n");

                        Simulator::Schedule (Seconds(eventTime), &DashMiner::SendBlock, this, packet, m_peersSockets[*i]);
                        Simulator::Schedule (Seconds(eventTime + sendTime), &DashMiner::RemoveSendTime, this);

                    }
                    break;
                }
        }


        /* 	//Send large packet
                int k;
                for (k = 0; k < 4; k++)
                {
                ns3TcpSocket->Send (reinterpret_cast<const uint8_t*>(packetInfo.GetString()), packetInfo.GetSize(), 0);
                ns3TcpSocket->Send (delimiter, 1, 0);
                } */


    }

    m_minerAverageBlockGenInterval = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockGenInterval 
        + (Simulator::Now ().GetSeconds () - m_previousBlockGenerationTime)/(m_minerGeneratedBlocks+1);

    m_minerAverageBlockSize = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockSize 
        + static_cast<double>(m_nextBlockSize)/(m_minerGeneratedBlocks+1);

    m_previousBlockGenerationTime = Simulator::Now ().GetSeconds ();

    //std::cout<<"Previous block generation time : " << m_previousBlockGenerationTime << "\n";

    m_minerGeneratedBlocks++;

    ScheduleNextMiningEvent ();
}

void 
DashMiner::ReceivedHigherBlock(const Block &newBlock)
{
    // NS_LOG_FUNCTION(this);
    // NS_LOG_WARN("Dash miner " << GetNode ()->GetId () << " added a new block in the m_blockchain with higher height: " << newBlock);
    Simulator::Cancel (m_nextMiningEvent);
    ScheduleNextMiningEvent ();
}

void 
DashMiner::SendBlock(std::string packetInfo, Ptr<Socket> to) 
{
    // NS_LOG_FUNCTION (this);

    NS_LOG_INFO ("SendBlock: At time " << Simulator::Now ().GetSeconds ()
    		<< "s dash miner " << GetNode ()->GetId () << " send " 
    		<< packetInfo << " to " << to);
rapidjson::Document d;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    d.Parse(packetInfo.c_str());  
    d.Accept(writer);

    SendMessage(NO_MESSAGE, BLOCK, d, to);
    m_nodeStats->blockSentBytes -= m_dashMessageHeader + d["blocks"][0]["size"].GetInt();
}

void
DashMiner::SendRaptorSymbol(std::string packetInfo, Ptr<Socket> to)
{
    NS_LOG_INFO ("SendRaptor: At time " << Simulator::Now ().GetSeconds ()
    		<< "s dash miner " << GetNode ()->GetId () << " send " 
    		<< packetInfo << " to " << to);
    rapidjson::Document d;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    d.Parse(packetInfo.c_str());  
    d.Accept(writer);

    SendMessage(NO_MESSAGE, RAPTORCODE, d, to);
    m_nodeStats->raptorSentBytes -= d["raptors"][0]["symbolSize"].GetDouble();

}

} // Namespace ns3

uint256 GetHash(std::string my_data)
{
    //const void* data = "Hello";
    const void* data = my_data.c_str();
    return HashX11(&data);
}

static double GetWallTime()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
