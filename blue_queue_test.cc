/*
 * Copyright (c) 2015 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

 #include "ns3/applications-module.h"
 #include "ns3/core-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/network-module.h"
 #include "ns3/point-to-point-layout-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/traffic-control-module.h"
 
 #include <iomanip>
 #include <iostream>
 #include <map>
 
 using namespace ns3;
 
 int
 main(int argc, char* argv[])
 {
     uint32_t nLeaf = 10;
     uint32_t maxPackets = 100;
     bool modeBytes = false;
     uint32_t queueDiscLimitPackets = 1000;
     double minTh = 5;
     double maxTh = 15;
     uint32_t pktSize = 512;
     std::string appDataRate = "10Mbps";
     std::string queueDiscType = "RED";
     uint16_t port = 5001;
     std::string bottleNeckLinkBw = "1Mbps";
     std::string bottleNeckLinkDelay = "50ms";
     // Add new command-line arguments for BlueQueueDisc parameters
     double blueIncrement = 0.02;
     double blueDecrement = 0.002;
     double blueFreezeTime = 0.1;

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
      // Add command-line values for BlueQueueDisc parameters
     cmd.AddValue("blueIncrement", "Increment value for BlueQueueDisc marking probability", blueIncrement);
     cmd.AddValue("blueDecrement", "Decrement value for BlueQueueDisc marking probability", blueDecrement);
     cmd.AddValue("blueFreezeTime", "Freeze time before changing marking probability in BlueQueueDisc", blueFreezeTime);
     cmd.Parse(argc, argv);
 
     if ((queueDiscType != "RED") && (queueDiscType != "ARED") && (queueDiscType != "Blue"))
     {
         std::cout << "Invalid queue disc type: Use --queueDiscType=RED or --queueDiscType=ARED or --queueDiscType=Blue"
                   << std::endl;
         exit(1);
     }
 
     Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(pktSize));
     Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(appDataRate));
 
     Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize",
                        StringValue(std::to_string(maxPackets) + "p"));
     if (queueDiscType == "RED" || queueDiscType == "ARED"){
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
         // add blue logic that corresponds to this condition with bytes
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
     }
     else if (queueDiscType == "Blue")
     {
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
        Config::SetDefault("ns3::BlueQueueDisc::Increment", DoubleValue(blueIncrement));  // Increase probability of marking
        Config::SetDefault("ns3::BlueQueueDisc::Decrement", DoubleValue(blueDecrement));  // Decrease probability of marking
        Config::SetDefault("ns3::BlueQueueDisc::FreezeTime", TimeValue(Seconds(blueFreezeTime))); // Time before probability change
    }


     // Create the point-to-point link helpers
     PointToPointHelper bottleNeckLink;
     bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckLinkBw));
     bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckLinkDelay));
 
     PointToPointHelper pointToPointLeaf;
     pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
     pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));
 
     PointToPointDumbbellHelper d(nLeaf, pointToPointLeaf, nLeaf, pointToPointLeaf, bottleNeckLink);
 
     // Install Stack
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
 
     // Assign IP Addresses
     d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                           Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                           Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));
 
     // Install on/off app on all right side nodes
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
         // Create an on/off app sending packets to the left side
         AddressValue remoteAddress(InetSocketAddress(d.GetLeftIpv4Address(i), port));
         clientHelper.SetAttribute("Remote", remoteAddress);
         clientApps.Add(clientHelper.Install(d.GetRight(i)));
     }
     clientApps.Start(Seconds(1.0)); // Start 1 second after sink
     clientApps.Stop(Seconds(15.0)); // Stop before the sink
 
     Ipv4GlobalRoutingHelper::PopulateRoutingTables();
 
     std::cout << "Running the simulation" << std::endl;
     Simulator::Run();
 
     QueueDisc::Stats st = queueDiscs.Get(0)->GetStats();
     
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
 
     std::cout << "*** Stats from the bottleneck queue disc ***" << std::endl;
     std::cout << st << std::endl;
     std::cout << "Destroying the simulation" << std::endl;
 
     Simulator::Destroy();
     return 0;
 }
 