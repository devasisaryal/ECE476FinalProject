#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#include <iomanip>
#include <iostream>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BlueAqmExample");
uint32_t checkTimes; //!< Number of times the queues have been checked.
double avgQueueSize; //!< Average Queue size.
std::stringstream filePlotQueue;    //!< Output file name for queue size.
std::stringstream filePlotQueueAvg; //!< Output file name for queue average.
std::stringstream fileBlueMarkingProbability; // Output file name for Blue Marking Probability


/**
 * Check the queue size and write its stats to the output files.
 *
 * \param queue The queue to check.
 */
void
CheckQueueSize(Ptr<QueueDisc> queue)
{
    uint32_t qSize = queue->GetCurrentSize().GetValue();

    avgQueueSize += qSize;
    checkTimes++;

    // check queue size every 1/100 of a second
    Simulator::Schedule(Seconds(0.01), &CheckQueueSize, queue);


// Write to instaneous queue size data file
    std::ofstream fPlotQueue(filePlotQueue.str(), std::ios::out | std::ios::app);
    fPlotQueue << Simulator::Now().GetSeconds() << " " << qSize << std::endl;
    fPlotQueue.close();

    
// Write to avg queue size data file
    std::ofstream fPlotQueueAvg(filePlotQueueAvg.str(), std::ios::out | std::ios::app);
    fPlotQueueAvg << Simulator::Now().GetSeconds() << " " << avgQueueSize / checkTimes << std::endl;
    fPlotQueueAvg.close();
}

void
CheckMarkingProbability(Ptr<QueueDisc> queue)
{
    
    
    // check queue size every 1/100 of a second
    Simulator::Schedule(Seconds(0.01), &CheckMarkingProbability, queue);
    Ptr<BlueQueueDisc> blueQueue = DynamicCast<BlueQueueDisc>(queue);
    double dropProbability = blueQueue->GetDropProbability();

    // write to file that contains marking probability each 1/100 of a second
    std::ofstream fBlueMarkingProbability(fileBlueMarkingProbability.str(), std::ios::out | std::ios::app);
    fBlueMarkingProbability << Simulator::Now().GetSeconds() << " " << dropProbability << std::endl;
    fBlueMarkingProbability.close();
}




int main(int argc, char* argv[]) {
    // Setup simulation parameters and queue types
    // [your existing code]


    // enable logging for the Blue class
    LogComponentEnable("BlueAqmExample", LOG_LEVEL_INFO);
    LogComponentEnable("BlueQueueDisc", LOG_LEVEL_INFO);

    // file paths
    std::string QueueStatsPathOut;
    std::string FlowMonitorPathOut;
    std::string BlueMarketProbPathOut;

    // Default simulation parameters
    uint32_t nLeaf = 10; 
    uint32_t maxPackets = 100; // Max packets in device queue
    bool modeBytes = false; //Queue mode: packets or bytes
    uint32_t queueDiscLimitPackets = 1000; // Max packets in queue disc
    double minTh = 5; // RED min
    double maxTh = 15; // RED max
    uint32_t pktSize = 512;
    std::string appDataRate = "10Mbps";
    std::string queueDiscType = "RED"; // default queue discpline
    uint16_t port = 5001;
    std::string bottleNeckLinkBw = "1Mbps";
    std::string bottleNeckLinkDelay = "50ms";
    double blueIncrement = 0.02; // d1
    double blueDecrement = 0.002; // d2
    double blueFreezeTime = 0.1; 
    double checkQueueInterval = 0.01; // Default value of 0.01 seconds
    double checkBlueProbMarkingInterval = 0.01; // Default value of 0.01 seconds
  
    QueueStatsPathOut = "."; // Current directory
    FlowMonitorPathOut = "."; // Current Directory
    BlueMarketProbPathOut = '.'; //curent directory

    // Command-line argument parsing
    CommandLine cmd(__FILE__);
    cmd.AddValue("nLeaf", "Number of left and right side leaf nodes", nLeaf);
    cmd.AddValue("maxPackets", "Max Packets allowed in the device queue", maxPackets);
    cmd.AddValue("queueDiscLimitPackets",
                 "Max Packets allowed in the queue disc",
                 queueDiscLimitPackets);
    cmd.AddValue("queueDiscType", "Set Queue disc type to RED or ARED or Blue", queueDiscType);
    cmd.AddValue("appPktSize", "Set OnOff App Packet Size", pktSize);
    cmd.AddValue("appDataRate", "Set OnOff App DataRate", appDataRate);
    cmd.AddValue("modeBytes", "Set Queue disc mode to Packets (false) or bytes (true)", modeBytes);
    cmd.AddValue("redMinTh", "RED queue minimum threshold", minTh);
    cmd.AddValue("redMaxTh", "RED queue maximum threshold", maxTh);
    cmd.AddValue("blueIncrement", "Increment value for BlueQueueDisc marking probability", blueIncrement);
    cmd.AddValue("blueDecrement", "Decrement value for BlueQueueDisc marking probability", blueDecrement);
    cmd.AddValue("blueFreezeTime", "Freeze time before changing marking probability in BlueQueueDisc", blueFreezeTime);
    cmd.AddValue("QueueStatsPathOut", "Queue size and avg queue size at specific timestamp", QueueStatsPathOut);
    cmd.AddValue("FlowMonitorPathOut", "Flow Monitor Stats for flows in a simulation", FlowMonitorPathOut);
    cmd.AddValue("BlueMarketProbPathOut", "Blue Marking Probability at a specific timestamp", BlueMarketProbPathOut);
    cmd.AddValue("checkQueueInterval", "Interval for checking queue size", checkQueueInterval);
    cmd.AddValue("checkBlueProbMarkingInterval", "Interval for checking Blue's Marking Probability", checkBlueProbMarkingInterval);
    cmd.Parse(argc, argv);

     // Enable debug logs
    //LogComponentEnableAll(LOG_LEVEL_INFO);  // Add this line


// Validate queue disc type
    if ((queueDiscType != "RED") && (queueDiscType != "ARED") && (queueDiscType != "Blue"))
    {
        std::cout << "Invalid queue disc type: Use --queueDiscType=RED or --queueDiscType=ARED or --queueDiscType=Blue"
                  << std::endl;
        exit(1);
    }

    // Configure default settings for applications and queues
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(pktSize));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(appDataRate));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize",
                       StringValue(std::to_string(maxPackets) + "p"));
    
    // Configure queue disciplines
    if (queueDiscType == "RED" || queueDiscType == "ARED") {
        if (!modeBytes)
        {
            Config::SetDefault(
                "ns3::RedQueueDisc::MaxSize",
                QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
        }
        else
        {
            Config::SetDefault(
                "ns3::RedQueueDisc::MaxSize",
                QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));
            minTh *= pktSize;
            maxTh *= pktSize;
        }    
    
        Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(minTh));
        Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(maxTh));
        Config::SetDefault("ns3::RedQueueDisc::LinkBandwidth", StringValue(bottleNeckLinkBw));
        Config::SetDefault("ns3::RedQueueDisc::LinkDelay", StringValue(bottleNeckLinkDelay));
        Config::SetDefault("ns3::RedQueueDisc::MeanPktSize", UintegerValue(pktSize));

        if (queueDiscType == "ARED")
        {
            // Turn on ARED
            Config::SetDefault("ns3::RedQueueDisc::ARED", BooleanValue(true));
            Config::SetDefault("ns3::RedQueueDisc::LInterm", DoubleValue(10.0));
        }


    } else if (queueDiscType == "Blue") {

        if (!modeBytes)
        {
            Config::SetDefault(
                "ns3::BlueQueueDisc::MaxSize",
                QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queueDiscLimitPackets)));
        }
        else
        {
            Config::SetDefault(
                "ns3::BlueQueueDisc::MaxSize",
                QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, queueDiscLimitPackets * pktSize)));

        }
        Config::SetDefault("ns3::BlueQueueDisc::Increment", DoubleValue(blueIncrement));
        Config::SetDefault("ns3::BlueQueueDisc::Decrement", DoubleValue(blueDecrement));
        Config::SetDefault("ns3::BlueQueueDisc::FreezeTime", TimeValue(Seconds(blueFreezeTime)));
    }

    // Configure network topology
    PointToPointHelper bottleNeckLink;
    bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckLinkBw));
    bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckLinkDelay));

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointDumbbellHelper d(nLeaf, pointToPointLeaf, nLeaf, pointToPointLeaf, bottleNeckLink);

     // Install Internet stack
    InternetStackHelper stack;
    for (uint32_t i = 0; i < d.LeftCount(); ++i)
    {
        stack.Install(d.GetLeft(i));
    }
    for (uint32_t i = 0; i < d.RightCount(); ++i)
    {
        stack.Install(d.GetRight(i));
    }

    stack.Install(d.GetLeft());
    stack.Install(d.GetRight());

    // Configure Traffic Control Helper
    TrafficControlHelper tchBottleneck;
    QueueDiscContainer queueDiscs;

    if (queueDiscType == "RED" || queueDiscType == "ARED")
    {
        tchBottleneck.SetRootQueueDisc("ns3::RedQueueDisc");
    }
    else if (queueDiscType == "Blue")
    {
        tchBottleneck.SetRootQueueDisc("ns3::BlueQueueDisc");
    }
    tchBottleneck.Install(d.GetLeft()->GetDevice(0));
    queueDiscs = tchBottleneck.Install(d.GetRight()->GetDevice(0));

    // assign IP addresses
    d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));


     // Configure applications (OnOff traffic generator and packet sink)
    OnOffHelper clientHelper("ns3::TcpSocketFactory", Address());
    clientHelper.SetAttribute("OnTime", StringValue("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < d.LeftCount(); ++i)
    {
        sinkApps.Add(packetSinkHelper.Install(d.GetLeft(i)));
    }
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(30.0));

    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < d.RightCount(); ++i)
    {
        AddressValue remoteAddress(InetSocketAddress(d.GetLeftIpv4Address(i), port));
        clientHelper.SetAttribute("Remote", remoteAddress);
        clientApps.Add(clientHelper.Install(d.GetRight(i)));
    }
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(15.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    // set paths 
    filePlotQueue << QueueStatsPathOut << "/"
                    << "queue_size.plotme";
    filePlotQueueAvg << QueueStatsPathOut << "/"
                        << "queue_avg_size.plotme";
    fileBlueMarkingProbability << BlueMarketProbPathOut << "/"
                    << "Blue_marking_prob.plotme";


    // add update of inst/avg queue size
    Ptr<QueueDisc> queue = queueDiscs.Get(0);
    Simulator::ScheduleNow(&CheckQueueSize, queue);
    // Use DynamicCast to check if it's a BlueQueueDisc
    Ptr<BlueQueueDisc> blueQueue = DynamicCast<BlueQueueDisc>(queue);
    if (blueQueue != nullptr)  // Ensure the cast is valid
    {
        NS_LOG_INFO("The queue is a BlueQueueDisc.");
        Simulator::ScheduleNow(&CheckMarkingProbability, queue); // Add marking probability at specific timestamp to file
        
    }
    else
    {
        NS_LOG_WARN("The queue is NOT a BlueQueueDisc.");
    }
        
    // Setup flow monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(30));
    std::cout << "Starting the simulation" << std::endl;
    // Start simulation
    Simulator::Run();
   
    // Grab queue stats
    QueueDisc::Stats st = queueDiscs.Get(0)->GetStats();

    // Depending queue dispcline, change logging output slightly but display dropped packets
    if (queueDiscType == "RED" || queueDiscType == "ARED"){
        if (st.GetNDroppedPackets(RedQueueDisc::UNFORCED_DROP) == 0)
        {
            std::cout << "There should be some unforced drops" << std::endl;
            exit(1);
        }
       }
        if (st.GetNDroppedPackets(BlueQueueDisc::FORCED_DROP) == 0 &&
           st.GetNDroppedPackets(BlueQueueDisc::PROB_DROP) == 0)
       {
           std::cout << "There should be some drops (either forced or probabilistic)" << std::endl;
           exit(1);
       }
    
        if (st.GetNDroppedPackets(QueueDisc::INTERNAL_QUEUE_DROP) != 0)
        {
            std::cout << "There should be zero drops due to queue full" << std::endl;
            exit(1);
        }

    // Flow monitoring and stats output
    // [your existing code]
    /*
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
        NS_LOG_INFO("Flow " << it->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
        double throughput = (it->second.rxBytes * 8.0) / (it->second.timeLastRxPacket.GetSeconds() - it->second.timeFirstTxPacket.GetSeconds()) / 1e6;
        double latency = it->second.delaySum.GetSeconds() / it->second.rxPackets;
        NS_LOG_INFO("  Throughput: " << throughput << " Mbps");
        NS_LOG_INFO("  Latency: " << latency * 1000 << " ms");

    }
    */
   
    // output flow monitor data to file
    std::stringstream stmp;
    stmp << FlowMonitorPathOut << "/queue.flowmon";
    monitor->SerializeToXmlFile(stmp.str(), false, false);
    
        
    std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;
    std::cout << st << std::endl;

    std::cout << "Destroying the simulation" << std::endl;
    Simulator::Destroy();
    return 0;
}
