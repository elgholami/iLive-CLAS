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

NS_LOG_COMPONENT_DEFINE("ExampleSixlowpan");

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

	LogComponentEnable("ExampleSixlowpan", LOG_LEVEL_ALL);
	LogComponentEnable("SixLowPanHelper", LOG_LEVEL_ALL);
	LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_DEBUG);


	CommandLine cmd;
	cmd.Parse(argc, argv);

	Packet::EnablePrinting();
	Packet::EnableChecking();

	StackHelper stackHelper;

	NS_LOG_INFO ("Create nodes.");
	Ptr<Node> n0 = CreateObject<Node>();
	Ptr<Node> n1 = CreateObject<Node>();
	Ptr<Node> r = CreateObject<Node>();

	NodeContainer net1(n0, n1, r);
	NodeContainer all;
	all.Add(n0);
	all.Add(n1);
	all.Add(r);

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
	Ipv6AddressHelper ipv6;
	ipv6.SetBase(Ipv6Address("2013:1::"), Ipv6Prefix(64));
	Ipv6InterfaceContainer i1 = ipv6.Assign(six1);
	i1.SetRouter(1, true);

	stackHelper.PrintRoutingTable(r);
	stackHelper.PrintRoutingTable(n0);
	stackHelper.PrintRoutingTable(n1);

	/* Create a Ping6 application to send ICMPv6 echo request from n0 to n1 via r */
	uint32_t packetSize = 100;
	//uint32_t packetSize = 100;
	uint32_t maxPacketCount = 50;
	Time interPacketInterval = Seconds(1.);
	Ping6Helper ping6;

	//ping6.SetLocal(i1.GetAddress(1, 1));
	ping6.SetLocal(i1.GetAddress(0, 1));

	std::cout << "     - Packet now net interface index1-adress index1: " << i1.GetAddress(1, 1)
			<< std::endl;
	std::cout << "     - Packet now net interface index2-adress index1: " << i1.GetAddress(2, 1)
			<< std::endl;
	std::cout << "     - Packet now net interface index0-adress index1: " << i1.GetAddress(0, 1)
			<< std::endl;

	//ping6.SetRemote(i1.GetAddress(2, 1));
	ping6.SetRemote(i1.GetAddress(1, 1));

	//ping6.SetIfIndex (i1.GetInterfaceIndex (0));
	//ping6.SetRemote (Ipv6Address::GetAllNodesMulticast ());

	ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
	ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
	//ApplicationContainer apps = ping6.Install(net1.Get(1));
	ApplicationContainer apps = ping6.Install(net1.Get(0));

	std::cout << "      - Packet now: " << net1.Get(0) << std::endl;
	apps.Start(Seconds(5.0));
	apps.Stop(Seconds(6.0));

	AsciiTraceHelper ascii;
	csma.EnableAsciiAll(ascii.CreateFileStream("example-sixlowpan.tr"));
	csma.EnablePcapAll(std::string("example-sixlowpan"), true);

	Simulator::Stop(Seconds(100));
	NS_LOG_INFO ("Run Simulation.");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO ("Done.");
}

