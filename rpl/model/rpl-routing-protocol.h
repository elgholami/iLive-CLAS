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

#ifndef RPL_ROUTING_PROTOCOL_H
#define RPL_ROUTING_PROTOCOL_H

#include "rpl-rtable.h"
#include "rpl-packet-queue.h"
#include "rpl-packet.h"
#include "rpl-conf.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/timer.h"

namespace ns3 {
namespace rpl {

/**
 * \ingroup rpl
 * \brief rpl routing protocol.
 */
class RoutingProtocol: public Ipv6RoutingProtocol {
public:
	static TypeId
	GetTypeId(void);
	static const uint32_t RPL_PORT;

	/// c-tor
	RoutingProtocol();
	virtual
	~RoutingProtocol();
	virtual void
	DoDispose();

	///\name From Ipv6RoutingProtocol
	// \{
	Ptr<Ipv6Route> RouteOutput(Ptr<Packet> p, const Ipv6Header &header,
			Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	bool RouteInput(Ptr<const Packet> p, const Ipv6Header &header,
			Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
			MulticastForwardCallback mcb, LocalDeliverCallback lcb,
			ErrorCallback ecb);
	virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const;
	virtual void NotifyInterfaceUp(uint32_t interface);
	virtual void NotifyInterfaceDown(uint32_t interface);
	virtual void NotifyAddAddress(uint32_t interface,
			Ipv6InterfaceAddress address);
	virtual void NotifyRemoveAddress(uint32_t interface,
			Ipv6InterfaceAddress address);
	virtual void NotifyAddRoute(Ipv6Address dst, Ipv6Prefix mask,
			Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse =
					Ipv6Address::GetZero());
	virtual void NotifyRemoveRoute(Ipv6Address dst, Ipv6Prefix mask,
			Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse =
					Ipv6Address::GetZero());

	virtual void SetIpv6(Ptr<Ipv6> ipv6);
	// \}
	///\name Methods to handle protocol parameters
	// \{
	void SetEnableBufferFlag(bool f);
	bool GetEnableBufferFlag() const;
	void SetWSTFlag(bool f);
	bool GetWSTFlag() const;
	void SetEnableRAFlag(bool f);
	bool GetEnableRAFlag() const;
	// \}

	/**
	 * Assign a fixed random variable stream number to the random variables
	 * used by this model.  Return the number of streams (possibly zero) that
	 * have been assigned.
	 *
	 * \param stream first stream index to use
	 * \return the number of stream indices assigned by this model
	 */
	int64_t AssignStreams(int64_t stream);

	/*
	 * RPL Stuff in all Public
	 *
	 */

	/* Public RPL functions. */
	void rpl_init(void);
	void uip_rpl_input(void);
	rpl_dag_t *rpl_set_root(uint8_t instance_id, Ipv6Address * dag_id);
	int rpl_set_prefix(rpl_dag_t *dag, Ipv6Prefix *prefix, unsigned len);
	int rpl_repair_root(uint8_t instance_id);
	int rpl_set_default_route(rpl_instance_t *instance, Ipv6Address *from);
	rpl_dag_t *rpl_get_any_dag(void);
	rpl_instance_t *rpl_get_instance(uint8_t instance_id);
	void rpl_update_header_empty(void);
	int rpl_update_header_final(Ipv6Address *addr);
	int rpl_verify_header(int);
	void rpl_remove_header(void);
	uint8_t rpl_invert_header(void);
	/*---------------------------------------------------------------------------*/

#if RPL_CONF_STATS
#define RPL_STAT(code)	(code)
#else
#define RPL_STAT(code)
#endif /* RPL_CONF_STATS */
	/*---------------------------------------------------------------------------*/
	/* Instances */
	rpl_instance_t instance_table[];
	rpl_instance_t *default_instance;

	/* ICMPv6 functions for RPL. */
	void dis_output(Ipv6Address *addr);
	void dio_output(rpl_instance_t *, Ipv6Address *uc_addr);
	void dao_output(rpl_parent_t *, uint8_t lifetime);
	void dao_ack_output(rpl_instance_t *, Ipv6Address *, uint8_t);

	/* RPL logic functions. */
	void rpl_join_dag(Ipv6Address *from, rpl_dio_t *dio);
	void rpl_join_instance(Ipv6Address *from, rpl_dio_t *dio);
	void rpl_local_repair(rpl_instance_t *instance);
	void rpl_process_dio(Ipv6Address *, rpl_dio_t *);
	int rpl_process_parent_event(rpl_instance_t *, rpl_parent_t *);

	/* DAG object management. */
	rpl_dag_t *rpl_alloc_dag(uint8_t, Ipv6Address);
	rpl_instance_t *rpl_alloc_instance(uint8_t);
	void rpl_free_dag(rpl_dag_t *);
	void rpl_free_instance(rpl_instance_t *);

	/* DAG parent management function. */
	rpl_parent_t *rpl_add_parent(rpl_dag_t *, rpl_dio_t *dio, Ipv6Address *);
	rpl_parent_t *rpl_find_parent(rpl_dag_t *, Ipv6Address *);
	rpl_parent_t *rpl_find_parent_any_dag(rpl_instance_t *instance,
			Ipv6Address *addr);
	void rpl_nullify_parent(rpl_dag_t *, rpl_parent_t *);
	void rpl_remove_parent(rpl_dag_t *, rpl_parent_t *);
	void rpl_move_parent(rpl_dag_t *dag_src, rpl_dag_t *dag_dst,
			rpl_parent_t *parent);
	rpl_parent_t *rpl_select_parent(rpl_dag_t *dag);
	rpl_dag_t *rpl_select_dag(rpl_instance_t *instance, rpl_parent_t *parent);
	void rpl_recalculate_ranks(void);

	/* RPL routing table functions. */
	void rpl_remove_routes(rpl_dag_t *dag);
	void rpl_remove_routes_by_nexthop(Ipv6Address *nexthop, rpl_dag_t *dag);
	Ipv6RoutingTableEntry *rpl_add_route(rpl_dag_t *dag, Ipv6Prefix *prefix,
			int prefix_len, Ipv6Address *next_hop);
	void rpl_purge_routes(void);

	/* Objective function. */
	rpl_of_t *rpl_find_of(rpl_ocp_t);

	/* Timer functions. */
	void rpl_schedule_dao(rpl_instance_t *);
	void rpl_reset_dio_timer(rpl_instance_t *);
	void rpl_reset_periodic_timer(void);

	/* Route poisoning. */
	void rpl_poison_routes(rpl_dag_t *, rpl_parent_t *);

	rpl_dag_t *get_dag(uint8_t instance_id, Ipv6Address *dag_id);

	/*
	 * RPL Stuff all Public
	 *
	 */

private:
	///\name Protocol parameters.
	// \{
	/// Holdtimes is the multiplicative factor of PeriodicUpdateInterval for which the node waits since the last update
	/// before flushing a route from the routing table. If PeriodicUpdateInterval is 8s and Holdtimes is 3, the node
	/// waits for 24s since the last update to flush this route from its routing table.
	uint32_t Holdtimes;
	/// PeriodicUpdateInterval specifies the periodic time interval between which the a node broadcasts
	/// its entire routing table.
	Time m_periodicUpdateInterval;
	/// SettlingTime specifies the time for which a node waits before propagating an update.
	/// It waits for this time interval in hope of receiving an update with a better metric.
	Time m_settlingTime;
	/// Nodes IP address
	Ipv6Address m_mainAddress;
	/// IP protocol
	Ptr<Ipv6> m_ipv6;
	/// Raw socket per each IP interface, map socket -> iface address (IP + mask)
	std::map<Ptr<Socket>, Ipv6InterfaceAddress> m_socketAddresses;
	/// Loopback device used to defer route requests until a route is found
	Ptr<NetDevice> m_lo;
	/// Main Routing table for the node
	RoutingTable m_routingTable;
	/// Advertised Routing table for the node
	RoutingTable m_advRoutingTable;
	/// The maximum number of packets that we allow a routing protocol to buffer.
	uint32_t m_maxQueueLen;
	/// The maximum number of packets that we allow per destination to buffer.
	uint32_t m_maxQueuedPacketsPerDst;
	/// The maximum period of time that a routing protocol is allowed to buffer a packet for.
	Time m_maxQueueTime;
	/// A "drop front on full" queue used by the routing layer to buffer packets to which it does not have a route.
	PacketQueue m_queue;
	/// Flag that is used to enable or disable buffering
	bool EnableBuffering;
	/// Flag that is used to enable or disable Weighted Settling Time
	bool EnableWST;
	/// This is the wighted factor to determine the weighted settling time
	double m_weightedFactor;
	/// This is a flag to enable route aggregation. Route aggregation will aggregate all routes for
	/// 'RouteAggregationTime' from the time an update is received by a node and sends them as a single update .
	bool EnableRouteAggregation;
	/// Parameter that holds the route aggregation time interval
	Time m_routeAggregationTime;
	/// Unicast callback for own packets
	UnicastForwardCallback m_scb;
	/// Error callback for own packets
	ErrorCallback m_ecb;
	// \}

private:
	/// Start protocol operation
	void
	Start();
	/// Queue packet untill we find a route
	void
	DeferredRouteOutput(Ptr<const Packet> p, const Ipv6Header & header,
			UnicastForwardCallback ucb, ErrorCallback ecb);
	/// Look for any queued packets to send them out
	void
	LookForQueuedPackets(void);
	/**
	 * Send packet from queue
	 * \param dst - destination address to which we are sending the packet to
	 * \param route - route identified for this packet
	 */
	void
	SendPacketFromQueue(Ipv6Address dst, Ptr<Ipv6Route> route);
	/// Find socket with local interface address iface
	Ptr<Socket>
	FindSocketWithInterfaceAddress(Ipv6InterfaceAddress iface) const;
	///\name Receive rpl control packets
	// \{
	/// Receive and process rpl control packet
	void
	Recvrpl(Ptr<Socket> socket);
	// \}
	void
	Send(Ptr<Ipv6Route>, Ptr<const Packet>, const Ipv6Header &);
	void
	RPLSend(Ptr<Ipv6Route>, Ptr<const Packet>, const Ipv6Header &);
	/// Create loopback route for given header
	Ptr<Ipv6Route>
	LoopbackRoute(const Ipv6Header & header, Ptr<NetDevice> oif) const;
	/**
	 * Get settlingTime for a destination
	 * \param dst - destination address
	 * \return settlingTime for the destination if found
	 */
	Time
	GetSettlingTime(Ipv6Address dst);
	/// Sends trigger update from a node
	void
	SendTriggeredUpdate();
	/// Broadcasts the entire routing table for every PeriodicUpdateInterval
	void
	SendPeriodicUpdate();
	/// Multicast DIO controlling messages in PeriodicUpdateInterval
	void
	SendPeriodicDIOPacket(Ipv6Address src, Ipv6Address dst,
			Address* hardwareAddress, uint8_t flags, rpl_instance_t *instance);
	void
	MergeTriggerPeriodicUpdates();
	/// Notify that packet is dropped for some reason
	void
	Drop(Ptr<const Packet>, const Ipv6Header &, Socket::SocketErrno);
	/// Timer to trigger periodic updates from a node
	Timer m_periodicUpdateTimer;
	/// Timer used by the trigger updates in case of Weighted Settling Time is used
	Timer m_triggeredExpireTimer;

	/// Provides uniform random variables.
	Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

} /* namespace rpl */
} /* namespace ns3 */

#endif /* IPV6_STATIC_ROUTING_H */

