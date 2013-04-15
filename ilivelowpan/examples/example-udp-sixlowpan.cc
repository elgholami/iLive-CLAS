/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Universita' di Firenze, Italy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Lorenzo Bellini (lorenzobellini84@gmail.com)
 * Author: Alberto Cappetti (albertocappetti@alice.it)
 */

// Network topology
// //
// //             n0   r    n1
// //             |    _    |
// //             ====|_|====
// //                router
// //
// // - Tracing of queues and packet receptions to file "example-sixlowpan.tr"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-static-routing-helper.h"

#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-header.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/sixlowpan-helper.h"
#include <ns3/nstime.h>
#include <ns3/error-model.h>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ExampleUdpSixlowpan");

/**
 * \class StackHelper
 * \brief Helper to set or get some IPv6 information about nodes.
 */
class StackHelper {
public:
	/**
	 * \brief Add an address to a IPv6 node.
	 * \param n node
	 * \param interface interface index
	 * \param address IPv6 address to add
	 */
	inline void AddAddress(Ptr<Node>& n, uint32_t interface,
			Ipv6Address address) {
		Ptr<Ipv6> ipv6 = n->GetObject<Ipv6>();
		ipv6->AddAddress(interface, address);
	}

	/**
	 * \brief Print the routing table.
	 * \param n the node
	 */
	inline void PrintRoutingTable(Ptr<Node>& n) {
		Ptr<Ipv6StaticRouting> routing = 0;
		Ipv6StaticRoutingHelper routingHelper;
		Ptr<Ipv6> ipv6 = n->GetObject<Ipv6>();
		uint32_t nbRoutes = 0;
		Ipv6RoutingTableEntry route;

		routing = routingHelper.GetStaticRouting(ipv6);

		std::cout << "Routing table of " << n << " : " << std::endl;
		std::cout << "Destination\t\t\t\t" << "Gateway\t\t\t\t\t"
				<< "Interface\t" << "Prefix to use" << std::endl;

		nbRoutes = routing->GetNRoutes();
		for (uint32_t i = 0; i < nbRoutes; i++) {
			route = routing->GetRoute(i);
			std::cout << route.GetDest() << "\t" << route.GetGateway() << "\t"
					<< route.GetInterface() << "\t" << route.GetPrefixToUse()
					<< "\t" << std::endl;
		}
	}
};

int main(int argc, char** argv) {
//#if 0
//LogComponentEnable ("Ipv6L3Protocol", LOG_LEVEL_ALL);
//LogComponentEnable ("Icmpv6L4Protocol", LOG_LEVEL_ALL);
//LogComponentEnable ("Ipv6StaticRouting", LOG_LEVEL_ALL);
//LogComponentEnable ("Ipv6Interface", LOG_LEVEL_ALL);
//LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
//LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
//#endif

	LogComponentEnable("ExampleUdpSixlowpan", LOG_LEVEL_ALL);
//	LogComponentEnable("SixLowPanHelper", LOG_LEVEL_ALL);
	LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_DEBUG);
	LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
	LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

	CommandLine cmd;
	cmd.Parse(argc, argv);

	Packet::EnablePrinting();
	Packet::EnableChecking();

	StackHelper stackHelper;

	NS_LOG_INFO ("Create nodes.");
	Ptr<Node> n0 = CreateObject<Node>();
	Ptr<Node> n1 = CreateObject<Node>();
//	Ptr<Node> r = CreateObject<Node>();

//	NodeContainer net1(n0, n1, r);
	NodeContainer net1(n0, n1);
	NodeContainer all;
	all.Add(n0);
	all.Add(n1);
//	all.Add(r);

	NS_LOG_INFO ("Create IPv6 Internet Stack");
	InternetStackHelper internetv6;
	internetv6.Install(all);

	NS_LOG_INFO ("Create channels.");
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
	NetDeviceContainer d1 = csma.Install(net1);

//  Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> (
//    "RanVar", RandomVariableValue (UniformVariable (0., 1.)),
//  "ErrorRate", DoubleValue (0.1));
//d1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

	SixLowPanHelper sixlowpan;
	NetDeviceContainer six1 = sixlowpan.Install(d1);
	//NetDeviceContainer six2 = sixlowpan.Install(d2);
	//NetDeviceContainer six3 = sixlowpan.Install(d3);
	//Why need to define three NetDeviceContainer valuables.

	NS_LOG_INFO ("Create networks and assign IPv6 Addresses.");
	Address serverAddress;
	Ipv6AddressHelper ipv6;
	ipv6.SetBase("fe80:0:0:1::", Ipv6Prefix(64));
//	ipv6.SetBase(Ipv6Address ("fe80::"), Ipv6Prefix (64), Ipv6Address ("::1"));
//	ipv6.SetBase(Ipv6Address ("fe80:0000:f00d:cafe::"), Ipv6Prefix (64));

	Ipv6InterfaceContainer i1 = ipv6.Assign(six1);
//	i1.SetRouter(2, true);


//	stackHelper.PrintRoutingTable(r);
	stackHelper.PrintRoutingTable(n0);
	stackHelper.PrintRoutingTable(n1);

	//YIBO:: Build UDP client-server application here.
	serverAddress = Address(i1.GetAddress(1, 1));
//	serverAddress = ipv6.NewAddress ();

	NS_LOG_INFO ("Create Applications." << serverAddress);
	//
	// Create one udpServer applications on node one.
	//
	uint16_t port = 61630;
	UdpServerHelper server(port);
	ApplicationContainer apps = server.Install(net1.Get(1));
	apps.Start(Seconds(20.0));
	apps.Stop(Seconds(60.0));

	//
	// Create one UdpClient application to send UDP datagrams from node zero to
	// node one.
	//
	uint32_t MaxPacketSize = 24;
	Time interPacketInterval = Seconds(2.00);
	uint32_t maxPacketCount = 320;
	UdpClientHelper client(serverAddress, port);
	client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	client.SetAttribute("Interval", TimeValue(interPacketInterval));
	client.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));
	apps = client.Install(net1.Get(0));
	apps.Start(Seconds(23.0));
	apps.Stop(Seconds(59.0));

	AsciiTraceHelper ascii;
	csma.EnableAsciiAll(ascii.CreateFileStream("example-udp-sixlowpan.tr"));
	csma.EnablePcapAll(std::string("example-udp-sixlowpan"), true);

	Simulator::Stop(Seconds(100));
	NS_LOG_INFO ("Run Simulation.");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO ("Done.");
}

