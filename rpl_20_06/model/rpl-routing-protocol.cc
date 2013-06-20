/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007-2009 Strasbourg University
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
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "rpl-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("RplRoutingProtocol");

namespace ns3 {
namespace rpl {

NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);

/// UDP Port for rpl control traffic
const uint32_t RoutingProtocol::RPL_PORT = 269;

/// Tag used by rpl implementation
struct DeferredRouteOutputTag: public Tag {
	/// Positive if output device is fixed in RouteOutput
	int32_t oif;

	DeferredRouteOutputTag(int32_t o = -1) :
			Tag(), oif(o) {
	}

	static TypeId GetTypeId() {
		static TypeId tid =
				TypeId("ns3::rpl::DeferredRouteOutputTag").SetParent<Tag>();
		return tid;
	}

	TypeId GetInstanceTypeId() const {
		return GetTypeId();
	}

	uint32_t GetSerializedSize() const {
		return sizeof(int32_t);
	}

	void Serialize(TagBuffer i) const {
		i.WriteU32(oif);
	}

	void Deserialize(TagBuffer i) {
		oif = i.ReadU32();
	}

	void Print(std::ostream &os) const {
		os << "DeferredRouteOutputTag: output interface = " << oif;
	}
};

TypeId RoutingProtocol::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::rpl::RoutingProtocol")
			.SetParent<Ipv6RoutingProtocol>()
			.AddConstructor<RoutingProtocol>()
			.AddAttribute("PeriodicUpdateInterval",
					"Periodic interval between exchange of full routing tables among nodes. ",TimeValue(Seconds(15)),
					MakeTimeAccessor(&RoutingProtocol::m_periodicUpdateInterval),MakeTimeChecker())
			.AddAttribute("SettlingTime",
					"Minimum time an update is to be stored in adv table before sending out"
							"in case of change in metric (in seconds)",TimeValue(Seconds(5)),
					MakeTimeAccessor(&RoutingProtocol::m_settlingTime),MakeTimeChecker())
			.AddAttribute("MaxQueueLen",
					"Maximum number of packets that we allow a routing protocol to buffer.",
					UintegerValue(500 /*assuming maximum nodes in simulation is 100*/),
					MakeUintegerAccessor(&RoutingProtocol::m_maxQueueLen),
					MakeUintegerChecker<uint32_t>())
			.AddAttribute(
					"MaxQueuedPacketsPerDst",
					"Maximum number of packets that we allow per destination to buffer.",
					UintegerValue(5),
					MakeUintegerAccessor(
							&RoutingProtocol::m_maxQueuedPacketsPerDst),
					MakeUintegerChecker<uint32_t>())
			.AddAttribute(
					"MaxQueueTime",
					"Maximum time packets can be queued (in seconds)",
					TimeValue(Seconds(30)),
					MakeTimeAccessor(&RoutingProtocol::m_maxQueueTime),
					MakeTimeChecker())
			.AddAttribute("EnableBuffering",
					"Enables buffering of data packets if no route to destination is available",
					BooleanValue(true),
					MakeBooleanAccessor(&RoutingProtocol::SetEnableBufferFlag,
							&RoutingProtocol::GetEnableBufferFlag),
					MakeBooleanChecker())
			.AddAttribute("EnableWST",
					"Enables Weighted Settling Time for the updates before advertising",
					BooleanValue(true),
					MakeBooleanAccessor(&RoutingProtocol::SetWSTFlag,
							&RoutingProtocol::GetWSTFlag), MakeBooleanChecker())
			.AddAttribute(
					"Holdtimes",
					"Times the forwarding Interval to purge the route.",
					UintegerValue(3),
					MakeUintegerAccessor(&RoutingProtocol::Holdtimes),
					MakeUintegerChecker<uint32_t>())
			.AddAttribute(
					"WeightedFactor",
					"WeightedFactor for the settling time if Weighted Settling Time is enabled",
					DoubleValue(0.875),
					MakeDoubleAccessor(&RoutingProtocol::m_weightedFactor),
					MakeDoubleChecker<double>())
			.AddAttribute(
					"EnableRouteAggregation",
					"Enables Weighted Settling Time for the updates before advertising",
					BooleanValue(false),
					MakeBooleanAccessor(&RoutingProtocol::SetEnableRAFlag,
							&RoutingProtocol::GetEnableRAFlag),
					MakeBooleanChecker())
			.AddAttribute("RouteAggregationTime",
					"Time to aggregate updates before sending them out (in seconds)",
					TimeValue(Seconds(1)),
					MakeTimeAccessor(&RoutingProtocol::m_routeAggregationTime),
					MakeTimeChecker());
	return tid;
}

void RoutingProtocol::SetEnableBufferFlag(bool f) {
	EnableBuffering = f;
}
bool RoutingProtocol::GetEnableBufferFlag() const {
	return EnableBuffering;
}
void RoutingProtocol::SetWSTFlag(bool f) {
	EnableWST = f;
}
bool RoutingProtocol::GetWSTFlag() const {
	return EnableWST;
}
void RoutingProtocol::SetEnableRAFlag(bool f) {
	EnableRouteAggregation = f;
}
bool RoutingProtocol::GetEnableRAFlag() const {
	return EnableRouteAggregation;
}

int64_t RoutingProtocol::AssignStreams(int64_t stream) {
	NS_LOG_FUNCTION (this << stream);
	m_uniformRandomVariable->SetStream(stream);
	return 1;
}

RoutingProtocol::RoutingProtocol() :
		m_routingTable(), m_advRoutingTable(), m_queue(), m_periodicUpdateTimer(
				Timer::CANCEL_ON_DESTROY) {
	m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
}

RoutingProtocol::~RoutingProtocol() {
}

void RoutingProtocol::DoDispose() {
	m_ipv6 = 0;
	for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::iterator iter =
			m_socketAddresses.begin(); iter != m_socketAddresses.end();
			iter++) {
		iter->first->Close();
	}
	m_socketAddresses.clear();
	Ipv6RoutingProtocol::DoDispose();
}

void RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const {
	*stream->GetStream() << "Node: " << m_ipv6->GetObject<Node>()->GetId()
			<< " Time: " << Simulator::Now().GetSeconds() << "s ";
	m_routingTable.Print(stream);
}

void RoutingProtocol::Start() {
	m_queue.SetMaxPacketsPerDst(m_maxQueuedPacketsPerDst);
	m_queue.SetMaxQueueLen(m_maxQueueLen);
	m_queue.SetQueueTimeout(m_maxQueueTime);
	m_routingTable.Setholddowntime(Time(Holdtimes * m_periodicUpdateInterval));
	m_advRoutingTable.Setholddowntime(
			Time(Holdtimes * m_periodicUpdateInterval));
	m_scb = MakeCallback(&RoutingProtocol::Send, this);
	m_mcb = MakeCallback(&RoutingProtocol::RPLSend, this); //RPL DIO/DAO/DIS sending
	m_ecb = MakeCallback(&RoutingProtocol::Drop, this);
//	m_periodicUpdateTimer.SetFunction(&RoutingProtocol::SendPeriodicUpdate, this);
	m_periodicUpdateTimer.SetFunction(&RoutingProtocol::SendPeriodicDIOPacket, this);
	Time t_update_root = MicroSeconds(m_uniformRandomVariable->GetInteger(0, 1000));
	Time t_update_leaf = MicroSeconds(m_uniformRandomVariable->GetInteger(2000, 2500));

	if(m_ipv6->GetObject<Node>()->GetId() == 0){
		std::cout << "RPL: This is the root node" << std::endl;
		Ipv6Address addr = new Ipv6Address ("2001:1::1");
		m_dag = this->rpl_set_root(RPL_DEFAULT_INSTANCE, &addr);
		m_periodicUpdateTimer.Schedule(t_update_root);
	}else{
		std::cout << "RPL: This is not the root node, let it have some delay to start" << std::endl;
		m_periodicUpdateTimer.Schedule(t_update_leaf);
	}
}

Ptr<Ipv6Route> RoutingProtocol::RouteOutput(Ptr<Packet> p,
		const Ipv6Header &header, Ptr<NetDevice> oif,
		Socket::SocketErrno &sockerr) {
	NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));

	if (!p) {
		return LoopbackRoute(header, oif);
	}
	if (m_socketAddresses.empty()) {
		sockerr = Socket::ERROR_NOROUTETOHOST;
		NS_LOG_LOGIC ("No rpl interfaces");
		Ptr<Ipv6Route> route;
		return route;
	}
	std::map<Ipv6Address, RoutingTableEntry> removedAddresses;
	sockerr = Socket::ERROR_NOTERROR;
	Ptr<Ipv6Route> route;
	Ipv6Address dst = header.GetDestinationAddress();
	NS_LOG_DEBUG ("Packet Size: " << p->GetSize ()
			<< ", Packet id: " << p->GetUid () << ", Destination address in Packet: " << dst);
	RoutingTableEntry rt;
	m_routingTable.Purge(removedAddresses);
	for (std::map<Ipv6Address, RoutingTableEntry>::iterator rmItr =
			removedAddresses.begin(); rmItr != removedAddresses.end();
			++rmItr) {
		rmItr->second.SetEntriesChanged(true);
		rmItr->second.SetSeqNo(rmItr->second.GetSeqNo() + 1);
		m_advRoutingTable.AddRoute(rmItr->second);
	}
	if (!removedAddresses.empty()) {
		Simulator::Schedule(
				MicroSeconds(m_uniformRandomVariable->GetInteger(0, 1000)),
				&RoutingProtocol::SendTriggeredUpdate, this);
	}
	if (m_routingTable.LookupRoute(dst, rt)) {
		if (EnableBuffering) {
			LookForQueuedPackets();
		}
		if (rt.GetHop() == 1) {
			route = rt.GetRoute();
			NS_ASSERT(route != 0);
			NS_LOG_DEBUG ("A route exists from " << route->GetSource ()
					<< " to neighboring destination "
					<< route->GetDestination ());
			if (oif != 0 && route->GetOutputDevice() != oif) {
				NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
				sockerr = Socket::ERROR_NOROUTETOHOST;
				return Ptr<Ipv6Route>();
			}
			return route;
		} else {
			RoutingTableEntry newrt;
			if (m_routingTable.LookupRoute(rt.GetNextHop(), newrt)) {
				route = newrt.GetRoute();
				NS_ASSERT(route != 0);
				NS_LOG_DEBUG ("A route exists from " << route->GetSource ()
						<< " to destination " << dst << " via "
						<< rt.GetNextHop ());
				if (oif != 0 && route->GetOutputDevice() != oif) {
					NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
					sockerr = Socket::ERROR_NOROUTETOHOST;
					return Ptr<Ipv6Route>();
				}
				return route;
			}
		}
	}

	if (EnableBuffering) {
		uint32_t iif = (oif ? m_ipv6->GetInterfaceForDevice(oif) : -1);
		DeferredRouteOutputTag tag(iif);
		if (!p->PeekPacketTag(tag)) {
			p->AddPacketTag(tag);
		}
	}
	return LoopbackRoute(header, oif);
}

void RoutingProtocol::DeferredRouteOutput(Ptr<const Packet> p,
		const Ipv6Header & header, UnicastForwardCallback ucb,
		ErrorCallback ecb) {
	NS_LOG_FUNCTION (this << p << header);
	NS_ASSERT(p != 0 && p != Ptr<Packet> ());
	QueueEntry newEntry(p, header, ucb, ecb);
	bool result = m_queue.Enqueue(newEntry);
	if (result) {
		NS_LOG_DEBUG ("Added packet " << p->GetUid () << " to queue.");
	}
}

bool RoutingProtocol::RouteInput(Ptr<const Packet> p, const Ipv6Header &header,
		Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
		MulticastForwardCallback mcb, LocalDeliverCallback lcb,
		ErrorCallback ecb) {
	NS_LOG_FUNCTION (m_mainAddress << " received packet " << p->GetUid ()
			<< " from " << header.GetSourceAddress()
			<< " on interface " << idev->GetAddress ()
			<< " to destination " << header.GetDestinationAddress());
	if (m_socketAddresses.empty()) {
		NS_LOG_DEBUG ("No rpl interfaces");
		return false;
	}
	NS_ASSERT(m_ipv6 != 0);
	// Check if input device supports IP
	NS_ASSERT(m_ipv6->GetInterfaceForDevice (idev) >= 0);
	int32_t iif = m_ipv6->GetInterfaceForDevice(idev);

	Ipv6Address dst = header.GetDestinationAddress();
	Ipv6Address origin = header.GetSourceAddress();

	// rpl is not a multicast routing protocol
	if (dst.IsMulticast()) {
		return false;
	}

	// Deferred route request
	if (EnableBuffering == true && idev == m_lo) {
		DeferredRouteOutputTag tag;
		if (p->PeekPacketTag(tag)) {
			DeferredRouteOutput(p, header, ucb, ecb);
			return true;
		}
	}
	for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
		Ipv6InterfaceAddress iface = j->second;
		if (origin == iface.GetAddress()) {
			return true;
		}
	}
	// LOCAL DELIVARY TO rpl INTERFACES
	for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
		Ipv6InterfaceAddress iface = j->second;
		if (m_ipv6->GetInterfaceForAddress(iface.GetAddress()) == iif) {
			if (dst == iface.GetAddress() || dst.IsAllNodesMulticast()) {
				Ptr<Packet> packet = p->Copy();
				if (lcb.IsNull() == false) {
					NS_LOG_LOGIC ("Broadcast local delivery to " << iface.GetAddress());
					lcb(p, header, iif);
					// Fall through to additional processing
				} else {
					NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
					ecb(p, header, Socket::ERROR_NOROUTETOHOST);
				}
				if (header.GetHopLimit() > 1) {
					NS_LOG_LOGIC ("Forward broadcast. TTL " << (uint16_t) header.GetHopLimit());
					RoutingTableEntry toBroadcast;
					if (m_routingTable.LookupRoute(dst, toBroadcast, true)) {
						Ptr<Ipv6Route> route = toBroadcast.GetRoute();
						ucb(route, packet, header);
					} else {
						NS_LOG_DEBUG ("No route to forward. Drop packet " << p->GetUid ());
					}
				}
				return true;
			}
		}
	}

	if (m_ipv6->IsForwarding(iif)) {
		if (lcb.IsNull() == false) {
			NS_LOG_LOGIC ("Unicast local delivery to " << dst);
			lcb(p, header, iif);
		} else {
			NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
			ecb(p, header, Socket::ERROR_NOROUTETOHOST);
		}
		return true;
	}
	RoutingTableEntry toDst;
	if (m_routingTable.LookupRoute(dst, toDst)) {
		RoutingTableEntry ne;
		if (m_routingTable.LookupRoute(toDst.GetNextHop(), ne)) {
			Ptr<Ipv6Route> route = ne.GetRoute();
			NS_LOG_LOGIC (m_mainAddress << " is forwarding packet " << p->GetUid ()
					<< " to " << dst
					<< " from " << header.GetSourceAddress()
					<< " via nexthop neighbor " << toDst.GetNextHop ());
			ucb(route, p, header);
			return true;
		}
	}NS_LOG_LOGIC ("Drop packet " << p->GetUid ()
			<< " as there is no route to forward it.");
	return false;
}

Ptr<Ipv6Route> RoutingProtocol::LoopbackRoute(const Ipv6Header & hdr,
		Ptr<NetDevice> oif) const {
	NS_ASSERT(m_lo != 0);
	Ptr<Ipv6Route> rt = Create<Ipv6Route>();
	rt->SetDestination(hdr.GetDestinationAddress());
	// rt->SetSource (hdr.GetSource ());
	//
	// Source address selection here is tricky.  The loopback route is
	// returned when rpl does not have a route; this causes the packet
	// to be looped back and handled (cached) in RouteInput() method
	// while a route is found. However, connection-oriented protocols
	// like TCP need to create an endpoint four-tuple (src, src port,
	// dst, dst port) and create a pseudo-header for checksumming.  So,
	// rpl needs to guess correctly what the eventual source address
	// will be.
	//
	// For single interface, single address nodes, this is not a problem.
	// When there are possibly multiple outgoing interfaces, the policy
	// implemented here is to pick the first available rpl interface.
	// If RouteOutput() caller specified an outgoing interface, that
	// further constrains the selection of source address
	//
	std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin();
	if (oif) {
		// Iterate to find an address on the oif device
		for (j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
			Ipv6Address addr = j->second.GetAddress();
			int32_t interface = m_ipv6->GetInterfaceForAddress(addr);
			if (oif == m_ipv6->GetNetDevice(static_cast<uint32_t>(interface))) {
				rt->SetSource(addr);
				break;
			}
		}
	} else {
		rt->SetSource(j->second.GetAddress());
	}
	NS_ASSERT_MSG(rt->GetSource () != Ipv6Address (),
			"Valid rpl source address not found");
	rt->SetGateway(Ipv6Address("127.0.0.1"));
	rt->SetOutputDevice(m_lo);
	return rt;
}

void RoutingProtocol::Recvrpl(Ptr<Socket> socket) {
	Address sourceAddress;
	Ptr<Packet> advpacket = Create<Packet>();
	Ptr<Packet> packet = socket->RecvFrom(sourceAddress);
	Inet6SocketAddress inetSourceAddr = Inet6SocketAddress::ConvertFrom(
			sourceAddress);
	Ipv6Address sender = inetSourceAddr.GetIpv6();
	Ipv6Address receiver = m_socketAddresses[socket].GetAddress();
	Ptr<NetDevice> dev = m_ipv6->GetNetDevice(
			m_ipv6->GetInterfaceForAddress(receiver));
	uint32_t packetSize = packet->GetSize();
	NS_LOG_FUNCTION (m_mainAddress << " received rpl packet of size: " << packetSize
			<< " and packet id: " << packet->GetUid ());
	uint32_t count = 0;
	for (; packetSize > 0; packetSize = packetSize - 12) {
		count = 0;
		RplHeader rplHeader, temprplHeader;
		packet->RemoveHeader(rplHeader);
		NS_LOG_DEBUG ("Processing new update for " << rplHeader.GetDst ());
		/*Verifying if the packets sent by me were returned back to me. If yes, discarding them!*/
		for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
				m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
			Ipv6InterfaceAddress interface = j->second;
			if (rplHeader.GetDst() == interface.GetAddress()) {
				if (rplHeader.GetDstSeqno() % 2 == 1) {
					NS_LOG_DEBUG ("Sent rpl update back to the same Destination, "
							"with infinite metric. Time left to send fwd update: "
							<< m_periodicUpdateTimer.GetDelayLeft ());
					count++;
				} else {
					NS_LOG_DEBUG ("Received update for my address. Discarding this.");
					count++;
				}
			}
		}
		if (count > 0) {
			continue;
		}NS_LOG_DEBUG ("Received a rpl packet from "
				<< sender << " to " << receiver << ". Details are: Destination: " << rplHeader.GetDst () << ", Seq No: "
				<< rplHeader.GetDstSeqno () << ", HopCount: " << rplHeader.GetHopCount ());
		RoutingTableEntry fwdTableEntry, advTableEntry;
		EventId event;
		bool permanentTableVerifier = m_routingTable.LookupRoute(
				rplHeader.GetDst(), fwdTableEntry);
		if (permanentTableVerifier == false) {
			if (rplHeader.GetDstSeqno() % 2 != 1) {
				NS_LOG_DEBUG ("Received New Route!");
				RoutingTableEntry newEntry(
				/*device=*/dev, /*dst=*/
				rplHeader.GetDst(), /*seqno=*/
				rplHeader.GetDstSeqno(),
						/*iface=*/m_ipv6->GetAddress(
								m_ipv6->GetInterfaceForAddress(receiver), 0),
						/*hops=*/rplHeader.GetHopCount(), /*next hop=*/
						sender, /*lifetime=*/
						Simulator::Now(), /*settlingTime*/
						m_settlingTime, /*entries changed*/
						true);
				newEntry.SetFlag(VALID);
				m_routingTable.AddRoute(newEntry);
				NS_LOG_DEBUG ("New Route added to both tables");
				m_advRoutingTable.AddRoute(newEntry);
			} else {
				// received update not present in main routing table and also with infinite metric
				NS_LOG_DEBUG ("Discarding this update as this route is not present in "
						"main routing table and received with infinite metric");
			}
		} else {
			if (!m_advRoutingTable.LookupRoute(rplHeader.GetDst(),
					advTableEntry)) {
				RoutingTableEntry tr;
				std::map<Ipv6Address, RoutingTableEntry> allRoutes;
				m_advRoutingTable.GetListOfAllRoutes(allRoutes);
				for (std::map<Ipv6Address, RoutingTableEntry>::const_iterator i =
						allRoutes.begin(); i != allRoutes.end(); ++i) {
					NS_LOG_DEBUG ("ADV table routes are:" << i->second.GetDestination ());
				}
				// present in fwd table and not in advtable
				m_advRoutingTable.AddRoute(fwdTableEntry);
				m_advRoutingTable.LookupRoute(rplHeader.GetDst(),
						advTableEntry);
			}
			if (rplHeader.GetDstSeqno() % 2 != 1) {
				if (rplHeader.GetDstSeqno() > advTableEntry.GetSeqNo()) {
					// Received update with better seq number. Clear any old events that are running
					if (m_advRoutingTable.ForceDeleteIpv6Event(
							rplHeader.GetDst())) {
						NS_LOG_DEBUG ("Canceling the timer to update route with better seq number");
					}
					// if its a changed metric *nomatter* where the update came from, wait  for WST
					if (rplHeader.GetHopCount() != advTableEntry.GetHop()) {
						advTableEntry.SetSeqNo(rplHeader.GetDstSeqno());
						advTableEntry.SetLifeTime(Simulator::Now());
						advTableEntry.SetFlag(VALID);
						advTableEntry.SetEntriesChanged(true);
						advTableEntry.SetNextHop(sender);
						advTableEntry.SetHop(rplHeader.GetHopCount());
						NS_LOG_DEBUG ("Received update with better sequence number and changed metric.Waiting for WST");
						Time tempSettlingtime = GetSettlingTime(
								rplHeader.GetDst());
						advTableEntry.SetSettlingTime(tempSettlingtime);
						NS_LOG_DEBUG ("Added Settling Time:" << tempSettlingtime.GetSeconds ()
								<< "s as there is no event running for this route");
						event = Simulator::Schedule(tempSettlingtime,
								&RoutingProtocol::SendTriggeredUpdate, this);
						m_advRoutingTable.AddIpv6Event(rplHeader.GetDst(),
								event);
						NS_LOG_DEBUG ("EventCreated EventUID: " << event.GetUid ());
						// if received changed metric, use it but adv it only after wst
						m_routingTable.Update(advTableEntry);
						m_advRoutingTable.Update(advTableEntry);
					} else {
						// Received update with better seq number and same metric.
						advTableEntry.SetSeqNo(rplHeader.GetDstSeqno());
						advTableEntry.SetLifeTime(Simulator::Now());
						advTableEntry.SetFlag(VALID);
						advTableEntry.SetEntriesChanged(true);
						advTableEntry.SetNextHop(sender);
						advTableEntry.SetHop(rplHeader.GetHopCount());
						m_advRoutingTable.Update(advTableEntry);
						NS_LOG_DEBUG ("Route with better sequence number and same metric received. Advertised without WST");
					}
				} else if (rplHeader.GetDstSeqno()
						== advTableEntry.GetSeqNo()) {
					if (rplHeader.GetHopCount() < advTableEntry.GetHop()) {
						/*Received update with same seq number and better hop count.
						 * As the metric is changed, we will have to wait for WST before sending out this update.
						 */
						NS_LOG_DEBUG ("Canceling any existing timer to update route with same sequence number "
								"and better hop count");
						m_advRoutingTable.ForceDeleteIpv6Event(
								rplHeader.GetDst());
						advTableEntry.SetSeqNo(rplHeader.GetDstSeqno());
						advTableEntry.SetLifeTime(Simulator::Now());
						advTableEntry.SetFlag(VALID);
						advTableEntry.SetEntriesChanged(true);
						advTableEntry.SetNextHop(sender);
						advTableEntry.SetHop(rplHeader.GetHopCount());
						Time tempSettlingtime = GetSettlingTime(
								rplHeader.GetDst());
						advTableEntry.SetSettlingTime(tempSettlingtime);
						NS_LOG_DEBUG ("Added Settling Time," << tempSettlingtime.GetSeconds ()
								<< " as there is no current event running for this route");
						event = Simulator::Schedule(tempSettlingtime,
								&RoutingProtocol::SendTriggeredUpdate, this);
						m_advRoutingTable.AddIpv6Event(rplHeader.GetDst(),
								event);
						NS_LOG_DEBUG ("EventCreated EventUID: " << event.GetUid ());
						// if received changed metric, use it but adv it only after wst
						m_routingTable.Update(advTableEntry);
						m_advRoutingTable.Update(advTableEntry);
					} else {
						/*Received update with same seq number but with same or greater hop count.
						 * Discard that update.
						 */
						if (not m_advRoutingTable.AnyRunningEvent(
								rplHeader.GetDst())) {
							/*update the timer only if nexthop address matches thus discarding
							 * updates to that destination from other nodes.
							 */
							if (advTableEntry.GetNextHop() == sender) {
								advTableEntry.SetLifeTime(Simulator::Now());
								m_routingTable.Update(advTableEntry);
							}
							m_advRoutingTable.DeleteRoute(rplHeader.GetDst());
						}NS_LOG_DEBUG ("Received update with same seq number and "
								"same/worst metric for, " << rplHeader.GetDst () << ". Discarding the update.");
					}
				} else {
					// Received update with an old sequence number. Discard the update
					if (not m_advRoutingTable.AnyRunningEvent(
							rplHeader.GetDst())) {
						m_advRoutingTable.DeleteRoute(rplHeader.GetDst());
					}NS_LOG_DEBUG (rplHeader.GetDst () << " : Received update with old seq number. Discarding the update.");
				}
			} else {
				NS_LOG_DEBUG ("Route with infinite metric received for "
						<< rplHeader.GetDst () << " from " << sender);
				// Delete route only if update was received from my nexthop neighbor
				if (sender == advTableEntry.GetNextHop()) {
					NS_LOG_DEBUG ("Triggering an update for this unreachable route:");
					std::map<Ipv6Address, RoutingTableEntry> dstsWithNextHopSrc;
					m_routingTable.GetListOfDestinationWithNextHop(
							rplHeader.GetDst(), dstsWithNextHopSrc);
					m_routingTable.DeleteRoute(rplHeader.GetDst());
					advTableEntry.SetSeqNo(rplHeader.GetDstSeqno());
					advTableEntry.SetEntriesChanged(true);
					m_advRoutingTable.Update(advTableEntry);
					for (std::map<Ipv6Address, RoutingTableEntry>::iterator i =
							dstsWithNextHopSrc.begin();
							i != dstsWithNextHopSrc.end(); ++i) {
						i->second.SetSeqNo(i->second.GetSeqNo() + 1);
						i->second.SetEntriesChanged(true);
						m_advRoutingTable.AddRoute(i->second);
						m_routingTable.DeleteRoute(i->second.GetDestination());
					}
				} else {
					if (not m_advRoutingTable.AnyRunningEvent(
							rplHeader.GetDst())) {
						m_advRoutingTable.DeleteRoute(rplHeader.GetDst());
					}NS_LOG_DEBUG (rplHeader.GetDst () <<
							" : Discard this link break update as it was received from a different neighbor "
							"and I can reach the destination");
				}
			}
		}
	}
	std::map<Ipv6Address, RoutingTableEntry> allRoutes;
	m_advRoutingTable.GetListOfAllRoutes(allRoutes);
	if (EnableRouteAggregation && allRoutes.size() > 0) {
		Simulator::Schedule(m_routeAggregationTime,
				&RoutingProtocol::SendTriggeredUpdate, this);
	} else {
		Simulator::Schedule(
				MicroSeconds(m_uniformRandomVariable->GetInteger(0, 1000)),
				&RoutingProtocol::SendTriggeredUpdate, this);
	}
}

void RoutingProtocol::SendTriggeredUpdate() {
	NS_LOG_FUNCTION (m_mainAddress << " is sending a triggered update");
	std::map<Ipv6Address, RoutingTableEntry> allRoutes;
	m_advRoutingTable.GetListOfAllRoutes(allRoutes);
	for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
		RplHeader rplHeader;
		Ptr<Socket> socket = j->first;
		Ipv6InterfaceAddress iface = j->second;
		Ptr<Packet> packet = Create<Packet>();
		for (std::map<Ipv6Address, RoutingTableEntry>::const_iterator i =
				allRoutes.begin(); i != allRoutes.end(); ++i) {
			NS_LOG_LOGIC ("Destination: " << i->second.GetDestination ()
					<< " SeqNo:" << i->second.GetSeqNo () << " HopCount:"
					<< i->second.GetHop () + 1);
			RoutingTableEntry temp = i->second;
			if ((i->second.GetEntriesChanged() == true)
					&& (not m_advRoutingTable.AnyRunningEvent(
							temp.GetDestination()))) {
				rplHeader.SetDst(i->second.GetDestination());
				rplHeader.SetDstSeqno(i->second.GetSeqNo());
				rplHeader.SetHopCount(i->second.GetHop() + 1);
				temp.SetFlag(VALID);
				temp.SetEntriesChanged(false);
				m_advRoutingTable.DeleteIpv6Event(temp.GetDestination());
				if (!(temp.GetSeqNo() % 2)) {
					m_routingTable.Update(temp);
				}
				packet->AddHeader(rplHeader);
				m_advRoutingTable.DeleteRoute(temp.GetDestination());
				NS_LOG_DEBUG ("Deleted this route from the advertised table");
			} else {
				EventId event = m_advRoutingTable.GetEventId(
						temp.GetDestination());
				NS_ASSERT(event.GetUid () != 0);
				NS_LOG_DEBUG ("EventID " << event.GetUid () << " associated with "
						<< temp.GetDestination () << " has not expired, waiting in adv table");
			}
		}
		if (packet->GetSize() >= 12) {
			RoutingTableEntry temp2;
			m_routingTable.LookupRoute(m_ipv6->GetAddress(1, 0).GetAddress(),
					temp2);
			rplHeader.SetDst(m_ipv6->GetAddress(1, 0).GetAddress());
			rplHeader.SetDstSeqno(temp2.GetSeqNo());
			rplHeader.SetHopCount(temp2.GetHop() + 1);
			NS_LOG_DEBUG ("Adding my update as well to the packet");
			packet->AddHeader(rplHeader);
			// Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
			Ipv6Address destination;
			if (iface.GetPrefix() == Ipv6Prefix::GetOnes()) {
				destination = Ipv6Address("255.255.255.255");
			} else {
				destination = iface.GetAddress();
			}
			socket->SendTo(packet, 0,
					Inet6SocketAddress(destination, RPL_PORT));
			NS_LOG_FUNCTION ("Sent Triggered Update from "
					<< rplHeader.GetDst ()
					<< " with packet id : " << packet->GetUid () << " and packet Size: " << packet->GetSize ());
		} else {
			NS_LOG_FUNCTION ("Update not sent as there are no updates to be triggered");
		}
	}
}

void RoutingProtocol::SendPeriodicUpdate() {
	std::map<Ipv6Address, RoutingTableEntry> removedAddresses, allRoutes;
	m_routingTable.Purge(removedAddresses); // Purge = clean
	MergeTriggerPeriodicUpdates();
	m_routingTable.GetListOfAllRoutes(allRoutes);
	if (allRoutes.empty()) {
		return;
	}NS_LOG_FUNCTION (m_mainAddress << " is sending out its periodic update");

	for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
		Ptr<Socket> socket = j->first;
		Ipv6InterfaceAddress iface = j->second;
		Ptr<Packet> packet = Create<Packet>();
		for (std::map<Ipv6Address, RoutingTableEntry>::const_iterator i =
				allRoutes.begin(); i != allRoutes.end(); ++i) {
			RplHeader rplHeader;
			if (i->second.GetHop() == 0) {
				RoutingTableEntry ownEntry;
				rplHeader.SetDst(m_ipv6->GetAddress(1, 0).GetAddress());
				rplHeader.SetDstSeqno(i->second.GetSeqNo() + 2);
				rplHeader.SetHopCount(i->second.GetHop() + 1);
				m_routingTable.LookupRoute(
						m_ipv6->GetAddress(1, 0).GetAddress(), ownEntry);
				ownEntry.SetSeqNo(rplHeader.GetDstSeqno());
				m_routingTable.Update(ownEntry);
				packet->AddHeader(rplHeader);
			} else {
				rplHeader.SetDst(i->second.GetDestination());
				rplHeader.SetDstSeqno((i->second.GetSeqNo()));
				rplHeader.SetHopCount(i->second.GetHop() + 1);
				packet->AddHeader(rplHeader);
			}NS_LOG_DEBUG ("Forwarding the update for " << i->first);NS_LOG_DEBUG ("Forwarding details are, Destination: " << rplHeader.GetDst ()
					<< ", SeqNo:" << rplHeader.GetDstSeqno ()
					<< ", HopCount:" << rplHeader.GetHopCount ()
					<< ", LifeTime: " << i->second.GetLifeTime ().GetSeconds ());
		}
		for (std::map<Ipv6Address, RoutingTableEntry>::const_iterator rmItr =
				removedAddresses.begin(); rmItr != removedAddresses.end();
				++rmItr) {
			RplHeader removedHeader;
			removedHeader.SetDst(rmItr->second.GetDestination());
			removedHeader.SetDstSeqno(rmItr->second.GetSeqNo() + 1);
			removedHeader.SetHopCount(rmItr->second.GetHop() + 1);
			packet->AddHeader(removedHeader);
			NS_LOG_DEBUG ("Update for removed record is: Destination: " << removedHeader.GetDst ()
					<< " SeqNo:" << removedHeader.GetDstSeqno ()
					<< " HopCount:" << removedHeader.GetHopCount ());
		}
		socket->Send(packet);
		// Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
		Ipv6Address destination;
		if (iface.GetPrefix() == Ipv6Prefix::GetOnes()) {
			destination = Ipv6Address("255.255.255.255");
		} else {
			destination = iface.GetAddress();
		}
		socket->SendTo(packet, 0, Inet6SocketAddress(destination, RPL_PORT));
		NS_LOG_FUNCTION ("PeriodicUpdate Packet UID is : " << packet->GetUid ());
	}
	m_periodicUpdateTimer.Schedule(
			m_periodicUpdateInterval
					+ MicroSeconds(
							25 * m_uniformRandomVariable->GetInteger(0, 1000)));
}

void RoutingProtocol::SendPeriodicDIOPacket(Ipv6Address src, Ipv6Address dst, Address* hardwareAddress, uint8_t flags, rpl_instance_t *instance) {
	NS_LOG_FUNCTION (m_mainAddress << " is sending out its periodic update");

	NS_LOG_FUNCTION (this << src << dst << hardwareAddress << static_cast<uint32_t> (flags));
	Ptr<Packet> p = Create<Packet>();
	DIOPacket dio;

	std::cout << "RPL: Sending Period DIO !!!" << std::endl;

	dst = dst.GetAllNodesMulticast();
	instance = default_instance;
	std::cout << "Send DIO ( from " << src << " to " << dst << " target " << src << ")"<< std::endl;
	dio.SetIpv6Target(Ipv6Address::GetAllNodesMulticast());

	if ((flags & 1)) {
		dio.SetFlagO(true);
	}
	if ((flags & 2) && src != Ipv6Address::GetAny()) {
		dio.SetFlagS(true);
	}
	if ((flags & 4)) {
		dio.SetFlagR(true);
	}

	dio.CalculatePseudoHeaderChecksum(src, dst,
			p->GetSize() + dio.GetSerializedSize(), 58);

	dio.m_instance = this->default_instance;
	dio.m_dag = this->m_dag;

	p->AddHeader(dio);

//    SendMessage(p, src, dst, 255);

	std::cout << "RPL: Finish adding DIO Header" << std::endl;

	/* Unicast requests get unicast replies! */
	if (dst.IsAllNodesMulticast()) {
		std::cout << "RPL: Sending a multicast-DIO with rank = "
				<< (unsigned) instance->current_dag->rank << std::endl;

//		uip_create_linklocal_rplnodes_mcast(&addr);
		//uip_icmp6_send(&addr, ICMP6_RPL, RPL_CODE_DIO, pos);
	} else {
		NS_LOG_FUNCTION (this << "RPL: Sending unicast-DIO with rank %u to " <<
				(unsigned) instance->current_dag->rank);
		NS_LOG_FUNCTION (dst);
	}

	m_periodicUpdateTimer.Schedule(
			m_periodicUpdateInterval
					+ MicroSeconds(
							25 * m_uniformRandomVariable->GetInteger(0, 1000)));
}

void RoutingProtocol::SetIpv6(Ptr<Ipv6> ipv6) {
	NS_ASSERT(ipv6 != 0);
	NS_ASSERT(m_ipv6 == 0);
	m_ipv6 = ipv6;
	// Create lo route. It is asserted that the only one interface up for now is loopback
//	NS_ASSERT(m_ipv6->GetNInterfaces () == 1 && m_ipv6->GetAddress (0, 0) == Ipv6Address ("fe80::1"));
	m_lo = m_ipv6->GetNetDevice(0);
	NS_ASSERT(m_lo != 0);
	// Remember lo route
	RoutingTableEntry rt(
			/*device=*/m_lo,
			/*dst=*/Ipv6Address::GetLoopback(),
			/*seqno=*/0,
			/*iface=*/Ipv6InterfaceAddress(Ipv6Address::GetLoopback(),Ipv6Prefix("fe80::")),
			/*hops=*/0,
			/*next hop=*/Ipv6Address::GetLoopback(),
			/*lifetime=*/Simulator::GetMaximumSimulationTime());
	rt.SetFlag(INVALID);
	rt.SetEntriesChanged(false);
	m_routingTable.AddRoute(rt);
	Simulator::ScheduleNow(&RoutingProtocol::Start, this);
}

void RoutingProtocol::NotifyInterfaceUp(uint32_t i) {
	NS_LOG_FUNCTION (this << m_ipv6->GetAddress (i, 0).GetAddress()
			<< " interface is up");
	Ptr<Ipv6L3Protocol> l3 = m_ipv6->GetObject<Ipv6L3Protocol>();
	Ipv6InterfaceAddress iface = l3->GetAddress(i, 0);
	if (iface.GetAddress() == Ipv6Address("::1")) {
		return;
	}
	// Create a socket to listen only on this interface
	Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
	NS_ASSERT(socket != 0);
	socket->SetRecvCallback(MakeCallback(&RoutingProtocol::Recvrpl, this));
	socket->BindToNetDevice(l3->GetNetDevice(i));
	socket->Bind(Inet6SocketAddress(Ipv6Address::GetAny(), RPL_PORT));
	socket->SetAllowBroadcast(true);
	socket->SetAttribute("IpTtl", UintegerValue(1));
	m_socketAddresses.insert(std::make_pair(socket, iface));
	// Add local broadcast record to the routing table
	Ptr<NetDevice> dev = m_ipv6->GetNetDevice(
			m_ipv6->GetInterfaceForAddress(iface.GetAddress()));
	RoutingTableEntry rt(/*device=*/dev, /*dst=*/iface.GetAddress(), /*seqno=*/
	0,/*iface=*/iface,/*hops=*/0,
	/*next hop=*/iface.GetAddress(), /*lifetime=*/
	Simulator::GetMaximumSimulationTime());
	m_routingTable.AddRoute(rt);
	if (m_mainAddress == Ipv6Address()) {
		m_mainAddress = iface.GetAddress();
	}
	NS_ASSERT(m_mainAddress != Ipv6Address ());
}

void RoutingProtocol::NotifyInterfaceDown(uint32_t i) {
	Ptr<Ipv6L3Protocol> l3 = m_ipv6->GetObject<Ipv6L3Protocol>();
	Ptr<NetDevice> dev = l3->GetNetDevice(i);
	Ptr<Socket> socket = FindSocketWithInterfaceAddress(
			m_ipv6->GetAddress(i, 0));
	NS_ASSERT(socket);
	socket->Close();
	m_socketAddresses.erase(socket);
	if (m_socketAddresses.empty()) {
		NS_LOG_LOGIC ("No rpl interfaces");
		m_routingTable.Clear();
		return;
	}
	m_routingTable.DeleteAllRoutesFromInterface(m_ipv6->GetAddress(i, 0));
	m_advRoutingTable.DeleteAllRoutesFromInterface(m_ipv6->GetAddress(i, 0));
}

void RoutingProtocol::NotifyAddAddress(uint32_t i,
		Ipv6InterfaceAddress address) {
	NS_LOG_FUNCTION (this << " interface " << i << " address " << address);
	Ptr<Ipv6L3Protocol> l3 = m_ipv6->GetObject<Ipv6L3Protocol>();
	if (!l3->IsUp(i)) {
		return;
	}
	Ipv6InterfaceAddress iface = l3->GetAddress(i, 0);
	Ptr<Socket> socket = FindSocketWithInterfaceAddress(iface);
	if (!socket) {
		if (iface.GetAddress() == Ipv6Address("127.0.0.1")) {
			return;
		}
		Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(),
				UdpSocketFactory::GetTypeId());
		NS_ASSERT(socket != 0);
		socket->SetRecvCallback(MakeCallback(&RoutingProtocol::Recvrpl, this));
		socket->BindToNetDevice(l3->GetNetDevice(i));
		// Bind to any IP address so that broadcasts can be received
		socket->Bind(Inet6SocketAddress(Ipv6Address::GetAny(), RPL_PORT));
		socket->SetAllowBroadcast(true);
		m_socketAddresses.insert(std::make_pair(socket, iface));
		Ptr<NetDevice> dev = m_ipv6->GetNetDevice(
				m_ipv6->GetInterfaceForAddress(iface.GetAddress()));
		RoutingTableEntry rt(/*device=*/dev, /*dst=*/iface.GetAddress(),/*seqno=*/
		0, /*iface=*/iface,/*hops=*/0,
		/*next hop=*/iface.GetAddress(), /*lifetime=*/
		Simulator::GetMaximumSimulationTime());
		m_routingTable.AddRoute(rt);
	}
}

void RoutingProtocol::NotifyRemoveAddress(uint32_t i,
		Ipv6InterfaceAddress address) {
	Ptr<Socket> socket = FindSocketWithInterfaceAddress(address);
	if (socket) {
		m_socketAddresses.erase(socket);
		Ptr<Ipv6L3Protocol> l3 = m_ipv6->GetObject<Ipv6L3Protocol>();
		if (l3->GetNAddresses(i)) {
			Ipv6InterfaceAddress iface = l3->GetAddress(i, 0);
			// Create a socket to listen only on this interface
			Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(),
					UdpSocketFactory::GetTypeId());
			NS_ASSERT(socket != 0);
			socket->SetRecvCallback(
					MakeCallback(&RoutingProtocol::Recvrpl, this));
			// Bind to any IP address so that broadcasts can be received
			socket->Bind(Inet6SocketAddress(Ipv6Address::GetAny(), RPL_PORT));
			socket->SetAllowBroadcast(true);
			m_socketAddresses.insert(std::make_pair(socket, iface));
		}
	}
}

void RoutingProtocol::NotifyAddRoute(Ipv6Address dst, Ipv6Prefix mask,
		Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse) {
	NS_LOG_INFO (this << dst << mask << nextHop << interface << prefixToUse);

}

void RoutingProtocol::NotifyRemoveRoute(Ipv6Address dst, Ipv6Prefix mask,
		Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse) {
	NS_LOG_FUNCTION (this << dst << mask << nextHop << interface);

}

Ptr<Socket> RoutingProtocol::FindSocketWithInterfaceAddress(
		Ipv6InterfaceAddress addr) const {
	for (std::map<Ptr<Socket>, Ipv6InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
		Ptr<Socket> socket = j->first;
		Ipv6InterfaceAddress iface = j->second;
		if (iface == addr) {
			return socket;
		}
	}
	Ptr<Socket> socket;
	return socket;
}

void RoutingProtocol::Send(Ptr<Ipv6Route> route, Ptr<const Packet> packet,
		const Ipv6Header & header) {
	Ptr<Ipv6L3Protocol> l3 = m_ipv6->GetObject<Ipv6L3Protocol>();
	NS_ASSERT(l3 != 0);
	Ptr<Packet> p = packet->Copy();
	l3->Send(p, route->GetSource(), header.GetDestinationAddress(),
			header.GetTrafficClass(), route);
}

void RoutingProtocol::RPLSend(Ptr<Ipv6MulticastRoute> m_route, Ptr<const Packet> packet,
		const Ipv6Header & header) {
//	Ptr<Icmpv6L4Protocol> rpl = m_ipv6->GetObject<Icmpv6L4Protocol>();
//	NS_ASSERT(rpl != 0);
//	Ptr<Packet> p = packet->Copy();
//	rpl->SendMessage();
}

void RoutingProtocol::Drop(Ptr<const Packet> packet, const Ipv6Header & header,
		Socket::SocketErrno err) {
	NS_LOG_DEBUG (m_mainAddress << " drop packet " << packet->GetUid () << " to "
			<< header.GetDestinationAddress() << " from queue. Error " << err);
}

void RoutingProtocol::LookForQueuedPackets() {
	NS_LOG_FUNCTION (this);
	Ptr<Ipv6Route> route;
	std::map<Ipv6Address, RoutingTableEntry> allRoutes;
	m_routingTable.GetListOfAllRoutes(allRoutes);
	for (std::map<Ipv6Address, RoutingTableEntry>::const_iterator i =
			allRoutes.begin(); i != allRoutes.end(); ++i) {
		RoutingTableEntry rt;
		rt = i->second;
		if (m_queue.Find(rt.GetDestination())) {
			if (rt.GetHop() == 1) {
				route = rt.GetRoute();
				NS_LOG_LOGIC ("A route exists from " << route->GetSource ()
						<< " to neighboring destination "
						<< route->GetDestination ());
				NS_ASSERT(route != 0);
			} else {
				RoutingTableEntry newrt;
				m_routingTable.LookupRoute(rt.GetNextHop(), newrt);
				route = newrt.GetRoute();
				NS_LOG_LOGIC ("A route exists from " << route->GetSource ()
						<< " to destination " << route->GetDestination () << " via "
						<< rt.GetNextHop ());
				NS_ASSERT(route != 0);
			}
			SendPacketFromQueue(rt.GetDestination(), route);
		}
	}
}

void RoutingProtocol::SendPacketFromQueue(Ipv6Address dst,
		Ptr<Ipv6Route> route) {
	NS_LOG_DEBUG (m_mainAddress << " is sending a queued packet to destination " << dst);
	QueueEntry queueEntry;
	if (m_queue.Dequeue(dst, queueEntry)) {
		DeferredRouteOutputTag tag;
		Ptr<Packet> p = ConstCast<Packet>(queueEntry.GetPacket());
		if (p->RemovePacketTag(tag)) {
			if (tag.oif != -1
					&& tag.oif
							!= m_ipv6->GetInterfaceForDevice(
									route->GetOutputDevice())) {
				NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
				return;
			}
		}
		UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback();
		Ipv6Header header = queueEntry.GetIpv6Header();
		header.SetSourceAddress(route->GetSource());
		header.SetHopLimit(header.GetHopLimit() + 1); // compensate extra TTL decrement by fake loopback routing
		ucb(route, p, header);
		if (m_queue.GetSize() != 0 && m_queue.Find(dst)) {
			Simulator::Schedule(
					MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
					&RoutingProtocol::SendPacketFromQueue, this, dst, route);
		}
	}
}

Time RoutingProtocol::GetSettlingTime(Ipv6Address address) {
	NS_LOG_FUNCTION ("Calculating the settling time for " << address);
	RoutingTableEntry mainrt;
	Time weightedTime;
	m_routingTable.LookupRoute(address, mainrt);
	if (EnableWST) {
		if (mainrt.GetSettlingTime() == Seconds(0)) {
			return Seconds(0);
		} else {
			NS_LOG_DEBUG ("Route SettlingTime: " << mainrt.GetSettlingTime ().GetSeconds ()
					<< " and LifeTime:" << mainrt.GetLifeTime ().GetSeconds ());
			weightedTime = Time(
					m_weightedFactor * mainrt.GetSettlingTime().GetSeconds()
							+ (1.0 - m_weightedFactor)
									* mainrt.GetLifeTime().GetSeconds());
			NS_LOG_DEBUG ("Calculated weightedTime:" << weightedTime.GetSeconds ());
			return weightedTime;
		}
	}
	return mainrt.GetSettlingTime();
}

void RoutingProtocol::MergeTriggerPeriodicUpdates() {
	NS_LOG_FUNCTION ("Merging advertised table changes with main table before sending out periodic update");
	std::map<Ipv6Address, RoutingTableEntry> allRoutes;
	m_advRoutingTable.GetListOfAllRoutes(allRoutes);
	if (allRoutes.size() > 0) {
		for (std::map<Ipv6Address, RoutingTableEntry>::const_iterator i =
				allRoutes.begin(); i != allRoutes.end(); ++i) {
			RoutingTableEntry advEntry = i->second;
			if ((advEntry.GetEntriesChanged() == true)
					&& (not m_advRoutingTable.AnyRunningEvent(
							advEntry.GetDestination()))) {
				if (!(advEntry.GetSeqNo() % 2)) {
					advEntry.SetFlag(VALID);
					advEntry.SetEntriesChanged(false);
					m_routingTable.Update(advEntry);
					NS_LOG_DEBUG ("Merged update for " << advEntry.GetDestination () << " with main routing Table");
				}
				m_advRoutingTable.DeleteRoute(advEntry.GetDestination());
			} else {
				NS_LOG_DEBUG ("Event currently running. Cannot Merge Routing Tables");
			}
		}
	}
}

/*----------Code from Contiki-----------------*/
/*---------------------------------------------------------------------------*/
rpl_instance_t *
RoutingProtocol::rpl_get_instance(uint8_t instance_id) {
	int i;

	for (i = 0; i < RPL_MAX_INSTANCES; ++i) {
		if (instance_table[i].used
				&& instance_table[i].instance_id == instance_id) {
			return &instance_table[i];
		}
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
RoutingProtocol::get_dag(uint8_t instance_id, Ipv6Address dag_id) {
	rpl_instance_t *instance;
	rpl_dag_t *dag;
	int i;

	instance = this->rpl_get_instance(instance_id);
	if (instance == NULL) {
		return NULL;
	}

	for (i = 0; i < RPL_MAX_DAG_PER_INSTANCE; ++i) {
		dag = &instance->dag_table[i];
		if (dag->used && (dag->dag_id == dag_id)) {
			return dag;
		}
	}

	return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_instance_t *
RoutingProtocol::rpl_alloc_instance(uint8_t instance_id) {
	rpl_instance_t *instance, *end;

	for (instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES;
			instance < end; ++instance) {
		if (instance->used == 0) {
			memset(instance, 0, sizeof(*instance));
			instance->instance_id = instance_id;
			instance->def_route = NULL;
			instance->used = 1;
			std::cout << "RPL: Return a allocated instance." << std::endl;
			return instance;
		}
	}
	return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
RoutingProtocol::rpl_alloc_dag(uint8_t instance_id, Ipv6Address dag_id) {
	rpl_dag_t *dag, *end;
	rpl_instance_t *instance;


	instance = rpl_get_instance(instance_id);
	if (instance == NULL) {
		instance = rpl_alloc_instance(instance_id);
		if (instance == NULL) {
			RPL_STAT(rpl_stats.mem_overflows++);
			std::cout << "RPL: NULL." << std::endl;
			return NULL;
		}
	}

	for (dag = &instance->dag_table[0], end = dag + RPL_MAX_DAG_PER_INSTANCE; dag < end; ++dag)
	{
		if (!dag->used) {
			memset(dag, 0, sizeof(*dag));
			parents_list.push_back(*dag);
			dag->used = 1;
			dag->rank = INFINITE_RANK;
			dag->min_rank = INFINITE_RANK;
			dag->instance = instance;
			std::cout << "RPL: Return a DAG." << std::endl;
			return dag;
		}
	}

	RPL_STAT(rpl_stats.mem_overflows++);
//  rpl_free_instance(instance);
	return NULL;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
rpl_dag_t *
RoutingProtocol::rpl_set_root(uint8_t instance_id, Ipv6Address dag_id) {
	rpl_dag_t *dag;
	rpl_instance_t *instance;
	uint8_t version;

	version = RPL_LOLLIPOP_INIT;
//	dag = get_dag(instance_id, dag_id);

	dag = rpl_alloc_dag(instance_id, dag_id);
	if (dag == NULL) {
		std::cout << "RPL: Failed to allocate a DAG." << std::endl;
		return NULL;
	}

	instance = dag->instance;

	dag->version = version;
	dag->joined = 1;
	dag->grounded = 0;
	instance->mop = RPL_MOP_DEFAULT;
//	instance->of = &RPL_OF;
	instance->of = NULL; // pretty bad
	dag->preferred_parent = NULL;

	dag->dag_id = dag_id;

	instance->dio_intdoubl = RPL_DIO_INTERVAL_DOUBLINGS;
	instance->dio_intmin = RPL_DIO_INTERVAL_MIN;
	/* The current interval must differ from the minimum interval in order to
	 trigger a DIO timer reset. */
	instance->dio_intcurrent = RPL_DIO_INTERVAL_MIN + RPL_DIO_INTERVAL_DOUBLINGS;
	instance->dio_redundancy = RPL_DIO_REDUNDANCY;
	instance->max_rankinc = RPL_MAX_RANKINC;
	instance->min_hoprankinc = RPL_MIN_HOPRANKINC;
	instance->default_lifetime = RPL_DEFAULT_LIFETIME;
	instance->lifetime_unit = RPL_DEFAULT_LIFETIME_UNIT;

	dag->rank = ROOT_RANK(instance);

	if (instance->current_dag != dag && instance->current_dag != NULL) {
		/* Remove routes installed by DAOs. */
//		rpl_remove_routes(instance->current_dag);
		std::cout << "RPL: Enter into a strange place." << std::endl;
		instance->current_dag->joined = 0;
	}

	instance->current_dag = dag;
	instance->dtsn_out = RPL_LOLLIPOP_INIT;
//	instance->of->update_metric_container(instance);
	default_instance = instance;

	std::cout << "RPL: Node set to be a DAG root with DAG ID: " << std::endl;
	std::cout << dag->dag_id << std::endl;

//	rpl_reset_dio_timer(instance);

	return dag;
}
/*---------------------------------------------------------------------------*/
/*----------Code from Contiki-----------------*/

}
}

