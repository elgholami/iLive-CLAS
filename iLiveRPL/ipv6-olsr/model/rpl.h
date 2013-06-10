/*
 *	Public API declarations for ContikiRPL.
 */

#ifndef RPL_H
#define RPL_H

#include "ns3/ipv6-address.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/timer.h"
#include "rpl-conf.h"
#include "list.h"

namespace ns3 {

namespace rpl {

/*---------------------------------------------------------------------------*/
/* The amount of parents that this node has in a particular DAG. */
#define RPL_PARENT_COUNT(dag)   list_length((dag)->parents)
/*---------------------------------------------------------------------------*/
typedef uint16_t rpl_rank_t;
typedef uint16_t rpl_ocp_t;
/*---------------------------------------------------------------------------*/
/* DAG Metric Container Object Types, to be confirmed by IANA. */
#define RPL_DAG_MC_NONE			0 /* Local identifier for empty MC */
#define RPL_DAG_MC_NSA                  1 /* Node State and Attributes */
#define RPL_DAG_MC_ENERGY               2 /* Node Energy */
#define RPL_DAG_MC_HOPCOUNT             3 /* Hop Count */
#define RPL_DAG_MC_THROUGHPUT           4 /* Throughput */
#define RPL_DAG_MC_LATENCY              5 /* Latency */
#define RPL_DAG_MC_LQL                  6 /* Link Quality Level */
#define RPL_DAG_MC_ETX                  7 /* Expected Transmission Count */
#define RPL_DAG_MC_LC                   8 /* Link Color */

/* DAG Metric Container flags. */
#define RPL_DAG_MC_FLAG_P               0x8
#define RPL_DAG_MC_FLAG_C               0x4
#define RPL_DAG_MC_FLAG_O               0x2
#define RPL_DAG_MC_FLAG_R               0x1

/* DAG Metric Container aggregation mode. */
#define RPL_DAG_MC_AGGR_ADDITIVE        0
#define RPL_DAG_MC_AGGR_MAXIMUM         1
#define RPL_DAG_MC_AGGR_MINIMUM         2
#define RPL_DAG_MC_AGGR_MULTIPLICATIVE  3

/* The bit index within the flags field of
 the rpl_metric_object_energy structure. */
#define RPL_DAG_MC_ENERGY_INCLUDED	3
#define RPL_DAG_MC_ENERGY_TYPE		1
#define RPL_DAG_MC_ENERGY_ESTIMATION	0

#define RPL_DAG_MC_ENERGY_TYPE_MAINS		0
#define RPL_DAG_MC_ENERGY_TYPE_BATTERY		1
#define RPL_DAG_MC_ENERGY_TYPE_SCAVENGING	2

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
	Ipv6Prefix prefix;
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
	struct rpl_instance *instance;LIST_STRUCT(parents);
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
	struct ctimer dio_timer;
	struct ctimer dao_timer;
};

/*---------------------------------------------------------------------------*/
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

}
}  // namespace ns3
#endif /* RPL_H */
