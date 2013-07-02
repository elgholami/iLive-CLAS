/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hemanth Narra
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
 * Author: Hemanth Narra <hemanth@ittc.ku.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#ifndef RPL_PACKET_H
#define RPL_PACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/icmpv6-header.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "rpl-conf.h"

namespace ns3 {
namespace rpl {
/**
 * \ingroup dsdv
 * \brief DSDV Update Packet Format
 * \verbatim
 |      0        |      1        |      2        |       3       |
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                      Destination Address                      |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                            HopCount                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                       Sequence Number                         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 */

class RplHeader: public Header {
public:
	RplHeader(Ipv6Address dst = Ipv6Address(), uint32_t hopcount = 0,
			uint32_t dstSeqNo = 0);
	virtual ~RplHeader();
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize() const;
	virtual void Serialize(Buffer::Iterator start) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual void Print(std::ostream &os) const;

	void SetDst(Ipv6Address destination) {
		m_dst = destination;
	}
	Ipv6Address GetDst() const {
		return m_dst;
	}
	void SetHopCount(uint32_t hopCount) {
		m_hopCount = hopCount;
	}
	uint32_t GetHopCount() const {
		return m_hopCount;
	}
	void SetDstSeqno(uint32_t sequenceNumber) {
		m_dstSeqNo = sequenceNumber;
	}
	uint32_t GetDstSeqno() const {
		return m_dstSeqNo;
	}
private:
	Ipv6Address m_dst; ///< Destination IP Address
	uint32_t m_hopCount; ///< Number of Hops
	uint32_t m_dstSeqNo; ///< Destination Sequence Number
};

/**************************************************************/
struct rpl_metric_object_energy {
	uint8_t flags;
	uint8_t energy_est;
};

/* Logical representation of a DAG Metric Container. */
struct rpl_metric_container {
	uint8_t type;
	uint8_t flags;
	uint8_t aggr;
	uint8_t prec;
	uint8_t length;
	union metric_object {
		struct rpl_metric_object_energy energy;
		uint16_t etx;
	} obj;
};
typedef struct rpl_metric_container rpl_metric_container_t;
/*---------------------------------------------------------------------------*/
struct rpl_instance;
struct rpl_dag;
/*---------------------------------------------------------------------------*/
struct rpl_parent {
	struct rpl_parent *next;
	struct rpl_dag *dag;
	rpl_metric_container_t mc;
	Ipv6Address addr;
	rpl_rank_t rank;
	uint8_t link_metric;
	uint8_t dtsn;
	uint8_t updated;
};
typedef struct rpl_parent rpl_parent_t;
/*---------------------------------------------------------------------------*/
/* RPL DIO prefix suboption */
struct rpl_prefix {
	Ipv6Address prefix;
	uint32_t lifetime;
	uint8_t length;
	uint8_t flags;
};
typedef struct rpl_prefix rpl_prefix_t;
/*---------------------------------------------------------------------------*/
/* Directed Acyclic Graph */
struct rpl_dag {
	Ipv6Address dag_id;
	rpl_rank_t min_rank; /* should be reset per DAG iteration! */
	uint8_t version;
	uint8_t grounded;
	uint8_t preference;
	uint8_t used;
	/* live data for the DAG */
	uint8_t joined;
	rpl_parent_t *preferred_parent;
	rpl_rank_t rank;
	struct rpl_instance *instance;
	//		LIST_STRUCT(parents);
	rpl_prefix_t prefix_info;
};
typedef struct rpl_dag rpl_dag_t;
typedef struct rpl_instance rpl_instance_t;
/*---------------------------------------------------------------------------*/
/*
 * API for RPL objective functions (OF)
 *
 * reset(dag)
 *
 *  Resets the objective function state for a specific DAG. This function is
 *  called when doing a global repair on the DAG.
 *
 * parent_state_callback(parent, known, etx)
 *
 *  Receives link-layer neighbor information. The parameter "known" is set
 *  either to 0 or 1. The "etx" parameter specifies the current
 *  ETX(estimated transmissions) for the neighbor.
 *
 * best_parent(parent1, parent2)
 *
 *  Compares two parents and returns the best one, according to the OF.
 *
 * best_dag(dag1, dag2)
 *
 *  Compares two DAGs and returns the best one, according to the OF.
 *
 * calculate_rank(parent, base_rank)
 *
 *  Calculates a rank value using the parent rank and a base rank.
 *  If "parent" is NULL, the objective function selects a default increment
 *  that is adds to the "base_rank". Otherwise, the OF uses information known
 *  about "parent" to select an increment to the "base_rank".
 *
 * update_metric_container(dag)
 *
 *  Updates the metric container for outgoing DIOs in a certain DAG.
 *  If the objective function of the DAG does not use metric containers,
 *  the function should set the object type to RPL_DAG_MC_NONE.
 */

struct rpl_of {
	void (*reset)(struct rpl_dag *);
	void (*parent_state_callback)(rpl_parent_t *, int, int);
	rpl_parent_t *(*best_parent)(rpl_parent_t *, rpl_parent_t *);
	rpl_dag_t *(*best_dag)(rpl_dag_t *, rpl_dag_t *);
	rpl_rank_t (*calculate_rank)(rpl_parent_t *, rpl_rank_t);
	void (*update_metric_container)(rpl_instance_t *);
	rpl_ocp_t ocp;
};
typedef struct rpl_of rpl_of_t;

/*---------------------------------------------------------------------------*/
/* Instance */
struct rpl_instance {
	/* DAG configuration */
	rpl_metric_container_t mc;
	rpl_of_t *of;
	rpl_dag_t *current_dag;
	rpl_dag_t dag_table[RPL_MAX_DAG_PER_INSTANCE];
	/* The current default router - used for routing "upwards" */
	Ipv6RoutingTableEntry *def_route;
	uint8_t instance_id;
	uint8_t used;
	uint8_t dtsn_out;
	uint8_t mop;
	uint8_t dio_intdoubl;
	uint8_t dio_intmin;
	uint8_t dio_redundancy;
	uint8_t default_lifetime;
	uint8_t dio_intcurrent;
	uint8_t dio_send; /* for keeping track of which mode the timer is in */
	uint8_t dio_counter;
	rpl_rank_t max_rankinc;
	rpl_rank_t min_hoprankinc;
	uint16_t lifetime_unit; /* lifetime in seconds = l_u * d_l */
#if RPL_CONF_STATS
	uint16_t dio_totint;
	uint16_t dio_totsend;
	uint16_t dio_totrecv;
#endif /* RPL_CONF_STATS */
	Timer dio_next_delay; /* delay for completion of dio interval */
	Timer dio_timer;
	Timer dao_timer;
};

/*---------------------------------------------------------------------------*/
/* Logical representation of a DAG Information Object (DIO.) */
struct rpl_dio {
	Ipv6Address dag_id;
	rpl_ocp_t ocp;
	rpl_rank_t rank;
	uint8_t grounded;
	uint8_t mop;
	uint8_t preference;
	uint8_t version;
	uint8_t instance_id;
	uint8_t dtsn;
	uint8_t dag_intdoubl;
	uint8_t dag_intmin;
	uint8_t dag_redund;
	uint8_t default_lifetime;
	uint16_t lifetime_unit;
	rpl_rank_t dag_max_rankinc;
	rpl_rank_t dag_min_hoprankinc;
	rpl_prefix_t destination_prefix;
	rpl_prefix_t prefix_info;
	struct rpl_metric_container mc;
};
typedef struct rpl_dio rpl_dio_t;

#if RPL_CONF_STATS
/* Statistics for fault management. */
struct rpl_stats {
	uint16_t mem_overflows;
	uint16_t local_repairs;
	uint16_t global_repairs;
	uint16_t malformed_msgs;
	uint16_t resets;
	uint16_t parent_switch;
};
typedef struct rpl_stats rpl_stats_t;

extern rpl_stats_t rpl_stats;
#endif
/*
static void reset(rpl_dag_t *);
static void parent_state_callback(rpl_parent_t *, int, int);
static rpl_parent_t *best_parent(rpl_parent_t *, rpl_parent_t *);
static rpl_dag_t *best_dag(rpl_dag_t *, rpl_dag_t *);
static rpl_rank_t calculate_rank(rpl_parent_t *, rpl_rank_t);
static void update_metric_container(rpl_instance_t *);

rpl_of_t rpl_of_etx = {
	  reset,
	  parent_state_callback,
	  best_parent,
	  best_dag,
	  calculate_rank,
	  update_metric_container,
	  1
};
*/
/*---------------------------------------------------------------------------*/
/* RPL macros. */
/***************************************************************/

/**
 * DIO packet.
 */
class DIOPacket: public Icmpv6Header {
public:
	/**
	 * \brief Constructor.
	 */
	DIOPacket();

	/**
	 * \brief Destructor.
	 */
	virtual ~DIOPacket();

	/**
	 * \brief Get the UID of this class.
	 * \return UID
	 */
	static TypeId GetTypeId();

	/**
	 * \brief Get the instance type ID.
	 * \return instance type ID
	 */
	virtual TypeId GetInstanceTypeId() const;

	/**
	 * \brief Get the reserved field.
	 * \return reserved value
	 */
	uint32_t GetReserved() const;

	/**
	 * \brief Set the reserved field.
	 * \param reserved the reserved value
	 */
	void SetReserved(uint32_t reserved);

	/**
	 * \brief Get the IPv6 target field.
	 * \return IPv6 address
	 */
	Ipv6Address GetIpv6Target() const;

	/**
	 * \brief Set the IPv6 target field.
	 * \param target IPv6 address
	 */
	void SetIpv6Target(Ipv6Address target);

	/**
	 * \brief Get the R flag.
	 * \return R flag
	 */
	bool GetFlagR() const;

	/**
	 * \brief Set the R flag.
	 * \param r value
	 */
	void SetFlagR(bool r);

	/**
	 * \brief Get the S flag.
	 * \return S flag
	 */
	bool GetFlagS() const;

	/**
	 * \brief Set the S flag.
	 * \param s value
	 */
	void SetFlagS(bool s);

	/**
	 * \brief Get the O flag.
	 * \return O flag
	 */
	bool GetFlagO() const;

	/**
	 * \brief Set the O flag.
	 * \param o value
	 */
	void SetFlagO(bool o);

	/**
	 * \brief Print informations.
	 * \param os output stream
	 */
	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size.
	 * \return serialized size
	 */
	virtual uint32_t GetSerializedSize() const;

	/**
	 * \brief Serialize the packet.
	 * \param start start offset
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start start offset
	 * \return length of packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	uint8_t GetInstanceID() const;
	void SetInstanceID(uint8_t IntanceID);

	uint8_t GetDagVersion() const;
	void SetDagVersion(uint8_t DagVersion);

	uint16_t GetRank() const;
	void SetRank(uint16_t Rank);

	uint8_t GetGrounded() const;
	void SetGrounded(uint8_t Grounded);

	uint8_t GetMOP() const;
	void SetMOP(uint8_t MOP);

	uint8_t GetPreference() const;
	void SetPreference(uint8_t Preference);

	Ipv6Address GetDagID() const;
	void SetDagID(Ipv6Address DagID);

	rpl_instance_t *m_instance;
	rpl_dag_t *m_dag;


private:
	uint8_t IntanceID;
	uint8_t DagVersion;
	uint16_t Rank;
	uint8_t Grounded;
	uint8_t MOP;
	uint8_t Preference;
	Ipv6Address DagID;

	/**
	 * \brief The R flag.
	 */
	bool m_flagR;

	/**
	 * \brief The O flag.
	 */
	bool m_flagS;

	/**
	 * \brief The M flag.
	 */
	bool m_flagO;

	/**
	 * \brief The reserved value.
	 */
	uint32_t m_reserved;

	/**
	 * \brief The IPv6 target address.
	 */
	Ipv6Address m_target;
};

static inline std::ostream & operator<<(std::ostream& os,
		const RplHeader & packet) {
	packet.Print(os);
	return os;
}
}
}

#endif /* DSDV_PACKET_H */
