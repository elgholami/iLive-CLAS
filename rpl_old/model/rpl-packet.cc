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
#include "rpl-packet.h"
#include "rpl-conf.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace rpl {

NS_OBJECT_ENSURE_REGISTERED(RplHeader);

RplHeader::RplHeader(Ipv6Address dst, uint32_t hopCount, uint32_t dstSeqNo) :
		m_dst(dst), m_hopCount(hopCount), m_dstSeqNo(dstSeqNo) {
}

RplHeader::~RplHeader() {
}

TypeId RplHeader::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::dsdv::RplHeader").SetParent<Header>().AddConstructor<
					RplHeader>();
	return tid;
}

TypeId RplHeader::GetInstanceTypeId() const {
	return GetTypeId();
}

uint32_t RplHeader::GetSerializedSize() const {
	return 12;
}

void RplHeader::Serialize(Buffer::Iterator i) const {
	WriteTo(i, m_dst);
	i.WriteHtonU32(m_hopCount);
	i.WriteHtonU32(m_dstSeqNo);

}

uint32_t RplHeader::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator i = start;

	ReadFrom(i, m_dst);
	m_hopCount = i.ReadNtohU32();
	m_dstSeqNo = i.ReadNtohU32();

	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize ());
	return dist;
}

void RplHeader::Print(std::ostream &os) const {
	os << "DestinationIpv6: " << m_dst << " Hopcount: " << m_hopCount
			<< " SequenceNumber: " << m_dstSeqNo;
}

/**
 * DIO Packet of RPL
 */

NS_OBJECT_ENSURE_REGISTERED(DIOPacket);

TypeId DIOPacket::GetTypeId() {
	static TypeId tid =
			TypeId("ns3::DIOPacket").SetParent<Icmpv6Header>().AddConstructor<
					DIOPacket>();
	return tid;
}

TypeId DIOPacket::GetInstanceTypeId() const {

	return GetTypeId();
}

DIOPacket::DIOPacket() {

	SetType(155);
	SetCode(1);
	SetReserved(0);
	SetFlagR(0);
	SetFlagS(0);
	SetFlagO(0);
	m_checksum = 0;
}

DIOPacket::~DIOPacket() {

}

uint32_t DIOPacket::GetReserved() const {
	return m_reserved;
}

void DIOPacket::SetReserved(uint32_t reserved) {
	m_reserved = reserved;
}

Ipv6Address DIOPacket::GetIpv6Target() const {
	return m_target;
}

bool DIOPacket::GetFlagR() const {
	return m_flagR;
}

void DIOPacket::SetFlagR(bool r) {
	m_flagR = r;
}

bool DIOPacket::GetFlagS() const {
	return m_flagS;
}

void DIOPacket::SetFlagS(bool s) {
	m_flagS = s;
}

bool DIOPacket::GetFlagO() const {
	return m_flagO;
}

void DIOPacket::SetFlagO(bool o) {

	m_flagO = o;
}

void DIOPacket::SetIpv6Target(Ipv6Address target) {

	m_target = target;
}

void DIOPacket::Print(std::ostream& os) const {

	os << "( type = " << (uint32_t) GetType() << " (NA) code = "
			<< (uint32_t) GetCode() << " checksum = "
			<< (uint32_t) GetChecksum() << ")";
}

uint32_t DIOPacket::GetSerializedSize() const {

	return 24;
}

void DIOPacket::Serialize(Buffer::Iterator start) const {

	uint8_t buff_target[16];
	uint8_t Dag_ID[16];
	uint16_t checksum = 0;
	Buffer::Iterator i = start;
	uint32_t reserved = m_reserved;

	i.WriteU8(GetType());
	i.WriteU8(GetCode());
	i.WriteU16(0);

	if (m_flagR) {
		reserved |= (uint32_t) (1 << 31);
	}

	if (m_flagS) {
		reserved |= (uint32_t) (1 << 30);
	}

	if (m_flagO) {
		reserved |= (uint32_t) (1 << 29);
	}

	/*
	 * Start of DIO message
	 * */
	rpl_dag_t *dag;
	Ipv6Address uc_addr = new Ipv6Address("aaaa::ff:fe00:1");
//	dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &uc_addr);
	rpl_instance_t *instance = NULL;
	instance->used = 0;
	memset(instance, 0, sizeof(*instance));
	instance->instance_id = RPL_DEFAULT_INSTANCE;
	instance->def_route = NULL;
	instance->used = 1;

	Buffer::Iterator pos = i;
	dag = instance->current_dag;

	Ipv6Address addr;

	/* DAG Information Object */
	pos.WriteU8(instance->instance_id);
	pos.WriteU8(dag->version);

	pos.WriteU16(dag->rank);

	/*|G|0| MOP | Prf | */
	uint8_t temp = 0;
	temp |= 0x80;
	temp |= instance->mop << 3;
	temp |= dag->preference & 0x70;
	pos.WriteU8(temp);

	pos.WriteU8(instance->dtsn_out);

	/* always request new DAO to refresh route */
	RPL_LOLLIPOP_INCREMENT(instance->dtsn_out);

	/* reserved 2 bytes */
	pos.WriteU8(0); /* flags */
	pos.WriteU8(0); /* reserved */

	dag->dag_id.Serialize(Dag_ID);
	pos.Write(Dag_ID, 16);

	if (instance->mc.type != RPL_DAG_MC_NONE) {
		instance->of->update_metric_container(instance);

		pos.WriteU8(2);
		pos.WriteU8(6);
		pos.WriteU8(instance->mc.type);
		pos.WriteU8(instance->mc.flags >> 1);
		pos.WriteU8(
				((instance->mc.flags & 1) << 7)
						| ((instance->mc.aggr << 4) | instance->mc.prec));

		if (instance->mc.type == RPL_DAG_MC_ETX) {
			pos.WriteU8(2);
			pos.WriteU16(instance->mc.obj.etx);
		} else if (instance->mc.type == RPL_DAG_MC_ENERGY) {
			pos.WriteU8(2);
			pos.WriteU8(instance->mc.obj.energy.flags);
			pos.WriteU8(instance->mc.obj.energy.energy_est);
		} else {
			std::cout
					<< "RPL: Unable to send DIO because of unhandled DAG MC type %u\n"
					<< (unsigned) instance->mc.type << std::endl;
			return;
		}
	}

	/* Always add a DAG configuration option. */
	pos.WriteU8(4);
	pos.WriteU8(14);
	pos.WriteU8(0); /* No Auth, PCS = 0 */
	pos.WriteU8(instance->dio_intdoubl);
	pos.WriteU8(instance->dio_intmin);
	pos.WriteU8(instance->dio_redundancy);
	pos.WriteU16(instance->max_rankinc);
	pos.WriteU16(instance->min_hoprankinc);

	/* OCP is in the DAG_CONF option */
	pos.WriteU16(instance->of->ocp);
	pos.WriteU8(0); /* reserved */
	pos.WriteU8(instance->default_lifetime);
	pos.WriteU16(instance->lifetime_unit);

	/* Check if we have a prefix to send also. */
	if (dag->prefix_info.length > 0) {
		pos.WriteU8(8);
		pos.WriteU8(30); /* always 30 bytes + 2 long */
		pos.WriteU8(dag->prefix_info.length);
		pos.WriteU8(dag->prefix_info.flags);
		pos.WriteHtonU32(dag->prefix_info.lifetime);
		pos.WriteHtonU32(dag->prefix_info.lifetime);
		pos.WriteHtonU32(0); // Not for sure

		dag->prefix_info.prefix.Serialize(Dag_ID);
		pos.Write(Dag_ID, 16);

		std::cout << "RPL: Sending prefix info in DIO for " << std::endl;
		std::cout << "&dag->prefix_info.prefix" << std::endl;
	} else {
		std::cout << "RPL: No prefix to announce (len %d)\n"
				<< dag->prefix_info.length << std::endl;
	}

	/*
	 * End of DIO message
	 * */

	i.WriteHtonU32(reserved);
	m_target.Serialize(buff_target);
	i.Write(buff_target, 16);

	if (m_calcChecksum) {
		i = start;
		checksum = i.CalculateIpChecksum(i.GetSize(), GetChecksum());
		i = start;
		i.Next(2);
		i.WriteU16(checksum);
	}
}

uint32_t DIOPacket::Deserialize(Buffer::Iterator start) {

	uint8_t buf[16];
	Buffer::Iterator i = start;

	SetType(i.ReadU8());
	SetCode(i.ReadU8());
	m_checksum = i.ReadU16();
	m_reserved = i.ReadNtohU32();

	m_flagR = false;
	m_flagS = false;
	m_flagO = false;

	if (m_reserved & (1 << 31)) {
		m_flagR = true;
	}

	if (m_reserved & (1 << 30)) {
		m_flagS = true;
	}

	if (m_reserved & (1 << 29)) {
		m_flagO = true;
	}

	i.Read(buf, 16);
	m_target.Set(buf);

	return GetSerializedSize();
}
}
}
