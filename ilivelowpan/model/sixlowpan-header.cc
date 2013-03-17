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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/assert.h"
#include "ns3/log.h"

#include "ns3/address-utils.h"
#include "sixlowpan-header.h"

namespace ns3 {
namespace sixlowpan {

/*
 * SixLowPanDispatch
 */

NS_OBJECT_ENSURE_REGISTERED(SixLowPanDispatch);

SixLowPanDispatch::SixLowPanDispatch() {
}

SixLowPanDispatch::SixLowPanDispatch(SixLowPanDispatch::Dispatch_e dispatch) {
}

TypeId SixLowPanDispatch::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::sixlowpan::SixLowPanDispatch").SetParent<
			Header>().AddConstructor<SixLowPanDispatch>();
	return tid;
}

TypeId SixLowPanDispatch::GetInstanceTypeId(void) const {
	return GetTypeId();
}

SixLowPanDispatch::Dispatch_e SixLowPanDispatch::GetDispatchType(void) const {
	return LOWPAN_NALP;
}

SixLowPanDispatch::Dispatch_e SixLowPanDispatch::GetDispatchType(
		uint8_t dispatch) {
	if (dispatch <= LOWPAN_NALP_N) {
		return LOWPAN_NALP;
	} else if (dispatch == LOWPAN_NOTCOMPRESSED) {
		return LOWPAN_NOTCOMPRESSED;
	} else if (dispatch == LOWPAN_HC1) {
		return LOWPAN_HC1;
	} else if (dispatch == LOWPAN_BC0) {
		return LOWPAN_BC0;
	} else if ((dispatch >= LOWPAN_IPHC) && (dispatch <= LOWPAN_IPHC_N)) {
		return LOWPAN_IPHC;
	} else if ((dispatch >= LOWPAN_MESH) && (dispatch <= LOWPAN_MESH_N)) {
		return LOWPAN_MESH;
	} else if ((dispatch >= LOWPAN_FRAG1) && (dispatch <= LOWPAN_FRAG1_N)) {
		return LOWPAN_FRAG1;
	} else if ((dispatch >= LOWPAN_FRAGN) && (dispatch <= LOWPAN_FRAGN_N)) {
		return LOWPAN_FRAGN;
	}
	return LOWPAN_UNSUPPORTED;
}

void SixLowPanDispatch::Print(std::ostream & os) const {
}

uint32_t SixLowPanDispatch::GetSerializedSize() const {
	return 0;
}

void SixLowPanDispatch::Serialize(Buffer::Iterator start) const {
}

uint32_t SixLowPanDispatch::Deserialize(Buffer::Iterator start) {
	return GetSerializedSize();
}

std::ostream & operator <<(std::ostream & os, const SixLowPanDispatch & h) {
	h.Print(os);
	return os;
}

/*
 * SixLowPanHc1
 */
NS_OBJECT_ENSURE_REGISTERED(SixLowPanHc1);

SixLowPanHc1::SixLowPanHc1() :
		m_hopLimit(0) {
}

TypeId SixLowPanHc1::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::sixlowpan::SixLowPanHc1").SetParent<Header>().AddConstructor<
					SixLowPanHc1>();
	return tid;
}

TypeId SixLowPanHc1::GetInstanceTypeId(void) const {
	return GetTypeId();
}

void SixLowPanHc1::Print(std::ostream & os) const {
	uint8_t encoding;
	encoding = m_srcCompression;
	encoding <<= 2;
	encoding |= m_dstCompression;
	encoding <<= 1;
	encoding |= m_tcflCompression;
	encoding <<= 2;
	encoding |= m_nextHeaderCompression;
	encoding <<= 1;
	encoding |= m_hc2HeaderPresent;

	os << "encoding " << int(encoding) << ", hopLimit " << int(m_hopLimit);
}

uint32_t SixLowPanHc1::GetSerializedSize() const {
	uint32_t serializedSize = 3;

	switch (m_srcCompression) {
	case HC1_PIII:
		serializedSize += 16;
		break;
	case HC1_PIIC:
		serializedSize += 8;
		break;
	case HC1_PCII:
		serializedSize += 8;
		break;
	case HC1_PCIC:
		break;
	}
	switch (m_dstCompression) {
	case HC1_PIII:
		serializedSize += 16;
		break;
	case HC1_PIIC:
		serializedSize += 8;
		break;
	case HC1_PCII:
		serializedSize += 8;
		break;
	case HC1_PCIC:
		break;
	}

	if (m_tcflCompression == false) {
		serializedSize += 4;
	}

	if (m_nextHeaderCompression == HC1_NC) {
		serializedSize++;
	}

	return serializedSize;
}

void SixLowPanHc1::Serialize(Buffer::Iterator start) const {
	Buffer::Iterator i = start;
	uint8_t encoding;
	encoding = m_srcCompression;
	encoding <<= 2;
	encoding |= m_dstCompression;
	encoding <<= 1;
	encoding |= m_tcflCompression;
	encoding <<= 2;
	encoding |= m_nextHeaderCompression;
	encoding <<= 1;
	encoding |= m_hc2HeaderPresent;

	i.WriteU8(LOWPAN_HC1);
	i.WriteU8(encoding);
	i.WriteU8(m_hopLimit);
	switch (m_srcCompression) {
	case HC1_PIII:
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_srcPrefix[j]);
		}
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_srcInterface[j]);
		}
		break;
	case HC1_PIIC:
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_srcPrefix[j]);
		}
		break;
	case HC1_PCII:
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_srcInterface[j]);
		}
		break;
	case HC1_PCIC:
		break;
	}
	switch (m_dstCompression) {
	case HC1_PIII:
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_dstPrefix[j]);
		}
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_dstInterface[j]);
		}
		break;
	case HC1_PIIC:
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_dstPrefix[j]);
		}
		break;
	case HC1_PCII:
		for (int j = 0; j < 8; j++) {
			i.WriteU8(m_dstInterface[j]);
		}
		break;
	case HC1_PCIC:
		break;
	}

	if (m_tcflCompression == false) {
		i.WriteU8(m_trafficClass);
		uint8_t temp[3];
		temp[0] = uint8_t(m_flowLabel & 0xff);
		temp[1] = uint8_t((m_flowLabel >> 8) & 0xff);
		temp[2] = uint8_t((m_flowLabel >> 16) & 0xff);
		i.Write(temp, 3);
	}

	if (m_nextHeaderCompression == HC1_NC) {
		i.WriteU8(m_nextHeader);
	}

	// TODO: HC2 is not yet supported. Should be.
	NS_ASSERT_MSG( m_hc2HeaderPresent != true,
			"Can not compress HC2, exiting. Very sorry.");
}

uint32_t SixLowPanHc1::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator i = start;
	uint32_t serializedSize = 3;

	i.Next();
	uint8_t encoding = i.ReadU8();
	m_hopLimit = i.ReadU8();

	m_srcCompression = LowPanHc1Addr_e(encoding >> 6);
	m_dstCompression = LowPanHc1Addr_e((encoding >> 4) & 0x3);
	m_tcflCompression = (encoding >> 3) & 0x1;
	m_nextHeaderCompression = LowPanHc1NextHeader_e((encoding >> 1) & 0x3);
	m_hc2HeaderPresent = encoding & 0x1;

	switch (m_srcCompression) {
	case HC1_PIII:
		for (int j = 0; j < 8; j++) {
			m_srcPrefix[j] = i.ReadU8();
		}
		for (int j = 0; j < 8; j++) {
			m_srcInterface[j] = i.ReadU8();
		}
		serializedSize += 16;
		break;
	case HC1_PIIC:
		for (int j = 0; j < 8; j++) {
			m_srcPrefix[j] = i.ReadU8();
		}
		serializedSize += 8;
		break;
	case HC1_PCII:
		for (int j = 0; j < 8; j++) {
			m_srcInterface[j] = i.ReadU8();
		}
		serializedSize += 8;
		break;
	case HC1_PCIC:
		break;
	}
	switch (m_dstCompression) {
	case HC1_PIII:
		for (int j = 0; j < 8; j++) {
			m_dstPrefix[j] = i.ReadU8();
		}
		for (int j = 0; j < 8; j++) {
			m_dstInterface[j] = i.ReadU8();
		}
		serializedSize += 16;
		break;
	case HC1_PIIC:
		for (int j = 0; j < 8; j++) {
			m_dstPrefix[j] = i.ReadU8();
		}
		serializedSize += 8;
		break;
	case HC1_PCII:
		for (int j = 0; j < 8; j++) {
			m_dstInterface[j] = i.ReadU8();
		}
		serializedSize += 8;
		break;
	case HC1_PCIC:
		break;
	}

	if (m_tcflCompression == false) {
		m_trafficClass = i.ReadU8();
		uint8_t temp[3];
		i.Read(temp, 3);
		m_flowLabel = temp[2];
		m_flowLabel = (m_flowLabel << 8) | temp[1];
		m_flowLabel = (m_flowLabel << 8) | temp[0];
		serializedSize += 4;
	}

	switch (m_nextHeaderCompression) {
	case HC1_NC:
		m_nextHeader = i.ReadU8();
		serializedSize++;
		break;
	case HC1_TCP:
		m_nextHeader = Ipv6Header::IPV6_TCP;
		break;
	case HC1_UDP:
		m_nextHeader = Ipv6Header::IPV6_UDP;
		break;
	case HC1_ICMP:
		m_nextHeader = Ipv6Header::IPV6_ICMPV6;
		break;
	}
	// TODO end this crap
	if (m_nextHeaderCompression == HC1_NC) {
	} else if (m_nextHeaderCompression == HC1_UDP) {
		m_nextHeader = UdpL4Protocol::PROT_NUMBER;
	} else {
		NS_ASSERT_MSG( m_hc2HeaderPresent != true,
				"Can not compress HC2, exiting. Very sorry.");
	}

	return GetSerializedSize();
}

SixLowPanDispatch::Dispatch_e SixLowPanHc1::GetDispatchType(void) const {
	return LOWPAN_HC1;
}

void SixLowPanHc1::SetHopLimit(uint8_t limit) {
	m_hopLimit = limit;
}

uint8_t SixLowPanHc1::GetHopLimit() const {
	return m_hopLimit;
}

SixLowPanHc1::LowPanHc1Addr_e SixLowPanHc1::GetDstCompression() const {
	return m_dstCompression;
}

const uint8_t* SixLowPanHc1::GetDstInterface() const {
	return m_dstInterface;
}

const uint8_t* SixLowPanHc1::GetDstPrefix() const {
	return m_dstPrefix;
}

uint32_t SixLowPanHc1::GetFlowLabel() const {
	return m_flowLabel;
}

uint8_t SixLowPanHc1::GetNextHeader() const {
	return m_nextHeader;
}

SixLowPanHc1::LowPanHc1Addr_e SixLowPanHc1::GetSrcCompression() const {
	return m_srcCompression;
}

const uint8_t* SixLowPanHc1::GetSrcInterface() const {
	return m_srcInterface;
}

const uint8_t* SixLowPanHc1::GetSrcPrefix() const {
	return m_srcPrefix;
}

uint8_t SixLowPanHc1::GetTrafficClass() const {
	return m_trafficClass;
}

bool SixLowPanHc1::IsTcflCompression() const {
	return m_tcflCompression;
}

bool SixLowPanHc1::IsHc2HeaderPresent() const {
	return m_hc2HeaderPresent;
}

void SixLowPanHc1::SetDstCompression(LowPanHc1Addr_e dstCompression) {
	m_dstCompression = dstCompression;
}

void SixLowPanHc1::SetDstInterface(const uint8_t* dstInterface) {
	for (int i = 0; i < 8; i++) {
		m_dstInterface[i] = dstInterface[i];
	}
}

void SixLowPanHc1::SetDstPrefix(const uint8_t* dstPrefix) {
	for (int i = 0; i < 8; i++) {
		m_dstPrefix[i] = dstPrefix[i];
	}
}

void SixLowPanHc1::SetFlowLabel(uint32_t flowLabel) {
	m_flowLabel = flowLabel;
}

void SixLowPanHc1::SetNextHeader(uint8_t nextHeader) {
	m_nextHeader = nextHeader;

	switch (m_nextHeader) {
	case Ipv6Header::IPV6_UDP:
		m_nextHeaderCompression = HC1_UDP;
		break;
	case Ipv6Header::IPV6_TCP:
		m_nextHeaderCompression = HC1_TCP;
		break;
	case Ipv6Header::IPV6_ICMPV6:
		m_nextHeaderCompression = HC1_ICMP;
		break;
	default:
		m_nextHeaderCompression = HC1_NC;
		break;
	}
}

void SixLowPanHc1::SetSrcCompression(LowPanHc1Addr_e srcCompression) {
	m_srcCompression = srcCompression;
}

void SixLowPanHc1::SetSrcInterface(const uint8_t* srcInterface) {
	for (int i = 0; i < 8; i++) {
		m_srcInterface[i] = srcInterface[i];
	}
}

void SixLowPanHc1::SetSrcPrefix(const uint8_t* srcPrefix) {
	for (int i = 0; i < 8; i++) {
		m_srcPrefix[i] = srcPrefix[i];
	}
}

void SixLowPanHc1::SetTcflCompression(bool tcflCompression) {
	m_tcflCompression = tcflCompression;
}

void SixLowPanHc1::SetTrafficClass(uint8_t trafficClass) {
	m_trafficClass = trafficClass;
}

void SixLowPanHc1::SetHc2HeaderPresent(bool hc2HeaderPresent) {
	m_hc2HeaderPresent = hc2HeaderPresent;
}

std::ostream & operator <<(std::ostream & os, const SixLowPanHc1 & h) {
	h.Print(os);
	return os;
}

/*
 * SixLowPanFrag1
 */
NS_OBJECT_ENSURE_REGISTERED(SixLowPanFrag1);

SixLowPanFrag1::SixLowPanFrag1() :
		m_datagramSize(0), m_datagramTag(0) {
}

TypeId SixLowPanFrag1::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::sixlowpan::SixLowPanFrag1").SetParent<
			Header>().AddConstructor<SixLowPanFrag1>();
	return tid;
}

TypeId SixLowPanFrag1::GetInstanceTypeId(void) const {
	return GetTypeId();
}

SixLowPanDispatch::Dispatch_e SixLowPanFrag1::GetDispatchType(void) const {
	return LOWPAN_FRAG1;
}

void SixLowPanFrag1::Print(std::ostream & os) const {
	os << "datagram size " << m_datagramSize;
	os << "datagram tag " << m_datagramTag;
}

uint32_t SixLowPanFrag1::GetSerializedSize() const {
	return 4;
}

void SixLowPanFrag1::Serialize(Buffer::Iterator start) const {
	Buffer::Iterator i = start;

	uint16_t temp = m_datagramSize | (uint16_t(LOWPAN_FRAG1) << 8);
	i.WriteU16(temp);
	i.WriteU16(m_datagramTag);
}

uint32_t SixLowPanFrag1::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator i = start;
	m_datagramSize = i.ReadU16() & 0x7FF;
	m_datagramTag = i.ReadU16();
	return GetSerializedSize();
}

void SixLowPanFrag1::SetDatagramSize(uint16_t datagramSize) {
	m_datagramSize = datagramSize & 0x7FF;
}

uint16_t SixLowPanFrag1::GetDatagramSize() const {
	return m_datagramSize & 0x7FF;
}

void SixLowPanFrag1::SetDatagramTag(uint16_t datagramTag) {
	m_datagramTag = datagramTag;
}

uint16_t SixLowPanFrag1::GetDatagramTag() const {
	return m_datagramTag;
}

std::ostream & operator <<(std::ostream & os, const SixLowPanFrag1 & h) {
	h.Print(os);
	return os;
}

/*
 * SixLowPanFragN
 */

NS_OBJECT_ENSURE_REGISTERED(SixLowPanFragN);

SixLowPanFragN::SixLowPanFragN() :
		m_datagramSize(0), m_datagramTag(0), m_datagramOffset(0) {
}
/*
 * SixLowPanFragmentOffset
 */
TypeId SixLowPanFragN::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::sixlowpan::SixLowPanFragN").SetParent<
			Header>().AddConstructor<SixLowPanFragN>();
	return tid;
}

TypeId SixLowPanFragN::GetInstanceTypeId(void) const {
	return GetTypeId();
}

void SixLowPanFragN::Print(std::ostream & os) const {
	os << "datagram size " << m_datagramSize;
	os << "datagram tag " << m_datagramTag;
	os << "datagram offset " << m_datagramOffset;
}

uint32_t SixLowPanFragN::GetSerializedSize() const {
	return 5;
}

void SixLowPanFragN::Serialize(Buffer::Iterator start) const {
	Buffer::Iterator i = start;

	uint16_t temp = m_datagramSize | (uint16_t(LOWPAN_FRAGN) << 8);
	i.WriteU16(temp);
	i.WriteU16(m_datagramTag);
	i.WriteU8(m_datagramOffset);
}

uint32_t SixLowPanFragN::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator i = start;

	m_datagramSize = i.ReadU16();
	m_datagramTag = i.ReadU16();
	m_datagramOffset = i.ReadU8();

	return GetSerializedSize();
}

SixLowPanDispatch::Dispatch_e SixLowPanFragN::GetDispatchType(void) const {
	return LOWPAN_FRAGN;
}

void SixLowPanFragN::SetDatagramSize(uint16_t datagramSize) {
	m_datagramSize = datagramSize & 0x7FF;
}

uint16_t SixLowPanFragN::GetDatagramSize() const {
	return m_datagramSize & 0x7FF;
}

void SixLowPanFragN::SetDatagramTag(uint16_t datagramTag) {
	m_datagramTag = datagramTag;
}

uint16_t SixLowPanFragN::GetDatagramTag() const {
	return m_datagramTag;
}

void SixLowPanFragN::SetDatagramOffset(uint8_t datagramOffset) {
	m_datagramOffset = datagramOffset;
}

uint8_t SixLowPanFragN::GetDatagramOffset() const {
	return m_datagramOffset;
}

std::ostream & operator <<(std::ostream & os, const SixLowPanFragN & h) {
	h.Print(os);
	return os;
}

/*
 * SixLowPanIphcHeader
 */
NS_OBJECT_ENSURE_REGISTERED(SixLowPanIphc);

SixLowPanIphc::SixLowPanIphc() {
	// 011x xxxx xxxx xxxx
	m_baseFormat = 0x6000;
}

SixLowPanIphc::SixLowPanIphc(uint8_t dispatch) {
	// 011x xxxx xxxx xxxx
	m_baseFormat = dispatch;
	m_baseFormat <<= 8;
}

TypeId SixLowPanIphc::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::sixlowpan::SixLowPanIphc").SetParent<Header>().AddConstructor<
					SixLowPanIphc>();
	return tid;
}

TypeId SixLowPanIphc::GetInstanceTypeId(void) const {
	return GetTypeId();
}

void SixLowPanIphc::Print(std::ostream & os) const {
	os << "Compression kind: " << m_baseFormat;
}

uint32_t SixLowPanIphc::GetSerializedSize() const {
	uint32_t serializedSize = 1;

	if (GetCid()) {
		serializedSize++;
	}
	switch (GetTf()) {
	case TF_FULL:
		serializedSize += 4;
		break;
	case TF_DSCP_ELIDED:
		serializedSize += 3;
		break;
	case TF_FL_ELIDED:
		serializedSize++;
		break;
	default:
		break;
	}
	if (GetNh() == false) {
		serializedSize++;
	}
	if (GetHlim() == HLIM_INLINE) {
		serializedSize++;
	}
	switch (GetSam()) {
	case HC_INLINE:
		if (GetSac() == false) {
			serializedSize += 16;
		}
		break;
	case HC_COMPR_64:
		serializedSize += 8;
		break;
	case HC_COMPR_16:
		serializedSize += 2;
		break;
	case HC_COMPR_0:
	default:
		break;
	}
	if (GetM() == false) {
		switch (GetDam()) {
		case HC_INLINE:
			if (GetDac() == false) {
				serializedSize += 16;
			}
			break;
		case HC_COMPR_64:
			serializedSize += 8;
			break;
		case HC_COMPR_16:
			serializedSize += 2;
			break;
		case HC_COMPR_0:
		default:
			break;
		}
	} else {
		switch (GetDam()) {
		case HC_INLINE:
			if (GetDac() == false) {
				serializedSize += 16;
			} else {
				serializedSize += 6;
			}
			break;
		case HC_COMPR_64:
			if (GetDac() == false) {
				serializedSize += 6;
			}
			break;
		case HC_COMPR_16:
			if (GetDac() == false) {
				serializedSize += 4;
			}
			break;
		case HC_COMPR_0:
		default:
			if (GetDac() == false) {
				serializedSize++;
			}
			break;
		}
	}

	return serializedSize;
}

void SixLowPanIphc::Serialize(Buffer::Iterator start) const {
	Buffer::Iterator i = start;
	uint8_t temp = uint8_t(m_baseFormat & 0xff);
	i.WriteU8(temp);
	if (GetCid()) {
		i.WriteU8(m_srcdstContextId);
	}
	// Traffic Class and Flow Label
	switch (GetTf()) {
	uint8_t temp;
case TF_FULL:
	temp = (m_ecn << 6) | m_dscp;
	i.WriteU8(temp);
	temp = m_flowLabel >> 16;
	i.WriteU8(temp);
	temp = (m_flowLabel >> 8) & 0xff;
	i.WriteU8(temp);
	temp = m_flowLabel & 0xff;
	i.WriteU8(temp);
	break;
case TF_DSCP_ELIDED:
	temp = (m_ecn << 6) | (m_flowLabel >> 16);
	i.WriteU8(temp);
	temp = (m_flowLabel >> 8) & 0xff;
	i.WriteU8(temp);
	temp = m_flowLabel & 0xff;
	i.WriteU8(temp);
	break;
case TF_FL_ELIDED:
	temp = (m_ecn << 6) | m_dscp;
	i.WriteU8(temp);
	break;
default:
	break;
	}
	// Next Header
	if (GetNh() == false) {
		i.WriteU8(m_nextHeader);
	}
	// Hop Limit
	if (GetHlim() == HLIM_INLINE) {
		i.WriteU8(m_hopLimit);
	}
	// Source Address
	switch (GetSam()) {
	uint8_t temp[16];
case HC_INLINE:
	if (GetSac() == false) {
		uint8_t temp[16];
		m_srcAddress.Serialize(temp);
		i.Write(temp, 16);
	}
	break;
case HC_COMPR_64:
	m_srcAddress.Serialize(temp);
	i.Write(temp + 8, 8);
	break;
case HC_COMPR_16:
	m_srcAddress.Serialize(temp);
	i.Write(temp + 14, 2);
	break;
case HC_COMPR_0:
default:
	break;
	}
	// Destination Address
	if (GetM() == false) {
		uint8_t temp[16];
		switch (GetDam()) {
		case HC_INLINE:
			if (GetDac() == false) {
				m_dstAddress.Serialize(temp);
				i.Write(temp, 16);
			}
			break;
		case HC_COMPR_64:
			m_dstAddress.Serialize(temp);
			i.Write(temp + 8, 8);
			break;
		case HC_COMPR_16:
			m_dstAddress.Serialize(temp);
			i.Write(temp + 14, 2);
			break;
		case HC_COMPR_0:
		default:
			break;
		}
	} else {
		switch (GetDam()) {
		uint8_t temp[16];
	case HC_INLINE:
		if (GetDac() == false) {
			m_dstAddress.Serialize(temp);
			i.Write(temp, 16);
		} else {
			m_dstAddress.Serialize(temp);
			i.Write(temp + 1, 2);
			i.Write(temp + 12, 4);
		}
		break;
	case HC_COMPR_64:
		if (GetDac() == false) {
			m_dstAddress.Serialize(temp);
			i.Write(temp + 1, 1);
			i.Write(temp + 11, 5);
		}
		break;
	case HC_COMPR_16:
		if (GetDac() == false) {
			m_dstAddress.Serialize(temp);
			i.Write(temp + 1, 1);
			i.Write(temp + 13, 3);
		}
		break;
	case HC_COMPR_0:
	default:
		if (GetDac() == false) {
			m_dstAddress.Serialize(temp);
			i.WriteU8(temp[15]);
		}
		break;
		}
	}
}

uint32_t SixLowPanIphc::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator i = start;
	uint8_t temp;
	temp = i.ReadU8();
	m_baseFormat |= uint16_t(temp);
	if (GetCid()) {
		m_srcdstContextId = i.ReadU8();
	}
	// Traffic Class and Flow Label
	switch (GetTf()) {
	uint8_t temp;
case TF_FULL:
	temp = i.ReadU8();
	m_ecn = temp >> 6;
	m_dscp = temp & 0x3F;
	temp = i.ReadU8();
	m_flowLabel = temp;
	temp = i.ReadU8();
	m_flowLabel = (m_flowLabel << 8) | temp;
	temp = i.ReadU8();
	m_flowLabel = (m_flowLabel << 8) | temp;
	break;
case TF_DSCP_ELIDED:
	temp = i.ReadU8();
	m_ecn = temp >> 6;
	m_flowLabel = temp & 0x3F;
	temp = i.ReadU8();
	m_flowLabel = (m_flowLabel << 8) | temp;
	temp = i.ReadU8();
	m_flowLabel = (m_flowLabel << 8) | temp;
	break;
case TF_FL_ELIDED:
	temp = i.ReadU8();
	m_ecn = temp >> 6;
	m_dscp = temp & 0x3F;
	break;
default:
	break;
	}
	// Next Header
	if (GetNh() == false) {
		m_nextHeader = i.ReadU8();
	}
	// Hop Limit
	switch (GetHlim()) {
	case HLIM_INLINE:
		m_hopLimit = i.ReadU8();
		break;
	case HLIM_COMPR_1:
		m_hopLimit = 1;
		break;
	case HLIM_COMPR_64:
		m_hopLimit = 64;
		break;
	case HLIM_COMPR_255:
	default:
		m_hopLimit = 255;
		break;

	}
	// Source Address
	switch (GetSam()) {
	uint8_t temp[16];
case HC_INLINE:
	if (GetSac() == false) {
		i.Read(temp, 16);
		m_srcAddress = Ipv6Address::Deserialize(temp);
	}
	break;
case HC_COMPR_64:
	i.Read(temp + 8, 8);
	temp[0] = 0xfe;
	temp[1] = 0x80;
	m_srcAddress = Ipv6Address::Deserialize(temp);
	break;
case HC_COMPR_16:
	i.Read(temp + 14, 2);
	temp[0] = 0xfe;
	temp[1] = 0x80;
	temp[11] = 0xff;
	temp[12] = 0xfe;
	m_srcAddress = Ipv6Address::Deserialize(temp);
	break;
case HC_COMPR_0:
default:
	break;
	}
	if (GetSac() == true) {
		PostProcessSac();
	}
	// Destination Address
	if (GetM() == false) {
		uint8_t temp[16];
		switch (GetDam()) {
		case HC_INLINE:
			if (GetDac() == false) {
				i.Read(temp, 16);
				m_dstAddress = Ipv6Address::Deserialize(temp);
			}
			break;
		case HC_COMPR_64:
			i.Read(temp + 8, 8);
			temp[0] = 0xfe;
			temp[1] = 0x80;
			m_dstAddress = Ipv6Address::Deserialize(temp);
			break;
		case HC_COMPR_16:
			i.Read(temp + 14, 2);
			temp[0] = 0xfe;
			temp[1] = 0x80;
			temp[11] = 0xff;
			temp[12] = 0xfe;
			m_dstAddress = Ipv6Address::Deserialize(temp);
			break;
		case HC_COMPR_0:
		default:
			break;
		}
	} else {
		switch (GetDam()) {
		uint8_t temp[16];
	case HC_INLINE:
		if (GetDac() == false) {
			i.Read(temp, 16);
			m_dstAddress = Ipv6Address::Deserialize(temp);
		} else {
			i.Read(temp + 1, 2);
			i.Read(temp + 12, 4);
			temp[0] = 0xff;
			m_dstAddress = Ipv6Address::Deserialize(temp);
		}
		break;
	case HC_COMPR_64:
		if (GetDac() == false) {
			i.Read(temp + 1, 1);
			i.Read(temp + 11, 5);
			temp[0] = 0xff;
			m_dstAddress = Ipv6Address::Deserialize(temp);
		}
		break;
	case HC_COMPR_16:
		if (GetDac() == false) {
			i.Read(temp + 1, 1);
			i.Read(temp + 13, 3);
			temp[0] = 0xff;
			m_dstAddress = Ipv6Address::Deserialize(temp);
		}
		break;
	case HC_COMPR_0:
	default:
		if (GetDac() == false) {
			temp[15] = i.ReadU8();
			temp[0] = 0xff;
			temp[1] = 0x02;
			m_dstAddress = Ipv6Address::Deserialize(temp);
		}
		break;
		}
	}
	if (GetDac() == true) {
		PostProcessDac();
	}
	return GetSerializedSize();
}

SixLowPanDispatch::Dispatch_e SixLowPanIphc::GetDispatchType(void) const {
	return LOWPAN_IPHC;
}

void SixLowPanIphc::SetTf(TrafficClassFlowLabel_e tfField) {
	uint16_t field = tfField;
	m_baseFormat |= (field << 11);
}

SixLowPanIphc::TrafficClassFlowLabel_e SixLowPanIphc::GetTf(void) const {
	return TrafficClassFlowLabel_e((m_baseFormat >> 11) & 0x3);
}

void SixLowPanIphc::SetNh(bool nhField) {
	uint16_t field = nhField;
	m_baseFormat |= (field << 10);
}

bool SixLowPanIphc::GetNh(void) const {
	return ((m_baseFormat >> 10) & 0x1);
}

void SixLowPanIphc::SetHlim(Hlim_e hlimField) {
	uint16_t field = hlimField;
	m_baseFormat |= (field << 8);
}

SixLowPanIphc::Hlim_e SixLowPanIphc::GetHlim(void) const {
	return Hlim_e((m_baseFormat >> 8) & 0x3);
}

void SixLowPanIphc::SetCid(bool cidField) {
	uint16_t field = cidField;
	m_baseFormat |= (field << 7);
}

bool SixLowPanIphc::GetCid(void) const {
	return ((m_baseFormat >> 7) & 0x1);
}

void SixLowPanIphc::SetSac(bool sacField) {
	uint16_t field = sacField;
	m_baseFormat |= (field << 6);
}

bool SixLowPanIphc::GetSac(void) const {
	return ((m_baseFormat >> 6) & 0x1);
}

void SixLowPanIphc::SetSam(HeaderCompression_e samField) {
	uint16_t field = samField;
	m_baseFormat |= (field << 4);
}

SixLowPanIphc::HeaderCompression_e SixLowPanIphc::GetSam(void) const {
	return HeaderCompression_e((m_baseFormat >> 4) & 0x3);
}

void SixLowPanIphc::SetM(bool mField) {
	uint16_t field = mField;
	m_baseFormat |= (field << 3);
}

bool SixLowPanIphc::GetM(void) const {
	return ((m_baseFormat >> 3) & 0x1);
}

void SixLowPanIphc::SetDac(bool dacField) {
	uint16_t field = dacField;
	m_baseFormat |= (field << 2);
}

bool SixLowPanIphc::GetDac(void) const {
	return ((m_baseFormat >> 2) & 0x1);
}

void SixLowPanIphc::SetDam(HeaderCompression_e damField) {
	uint16_t field = damField;
	m_baseFormat |= field;
}

SixLowPanIphc::HeaderCompression_e SixLowPanIphc::GetDam(void) const {
	return HeaderCompression_e(m_baseFormat & 0x3);
}

void SixLowPanIphc::SetSrcContextId(uint8_t srcContextId) {
	NS_ASSERT_MSG(srcContextId < 16, "Src Context ID too large");
	m_srcdstContextId |= srcContextId << 4;
}

uint8_t SixLowPanIphc::GetSrcContextId(void) const {
	return (m_srcdstContextId >> 4);
}

void SixLowPanIphc::SetDstContextId(uint8_t dstContextId) {
	NS_ASSERT_MSG(dstContextId < 16, "Dst Context ID too large");
	m_srcdstContextId |= (dstContextId & 0xF);
}

uint8_t SixLowPanIphc::GetDstContextId(void) const {
	return (m_srcdstContextId & 0xF);
}

void SixLowPanIphc::SetEcn(uint8_t ecn) {
	NS_ASSERT_MSG(ecn < 4, "ECN too large");
	m_ecn = ecn;
}

uint8_t SixLowPanIphc::GetEcn(void) const {
	return m_ecn;
}

void SixLowPanIphc::SetDscp(uint8_t dscp) {
	NS_ASSERT_MSG(dscp < 64, "DSCP too large");
	m_dscp = dscp;
}

uint8_t SixLowPanIphc::GetDscp(void) const {
	return m_dscp;
}

void SixLowPanIphc::SetFlowLabel(uint32_t flowLabel) {
	NS_ASSERT_MSG(flowLabel < 0x100000, "Flow Label too large");
	m_flowLabel = flowLabel;
}

uint32_t SixLowPanIphc::GetFlowLabel(void) const {
	return m_flowLabel;
}

void SixLowPanIphc::SetNextHeader(uint8_t nextHeader) {
	m_nextHeader = nextHeader;
}

uint8_t SixLowPanIphc::GetNextHeader(void) const {
	return m_nextHeader;
}

void SixLowPanIphc::SetHopLimit(uint8_t hopLimit) {
	m_hopLimit = hopLimit;
}

uint8_t SixLowPanIphc::GetHopLimit(void) const {
	return m_hopLimit;
}

void SixLowPanIphc::SetSrcAddress(Ipv6Address srcAddress) {
	m_srcAddress = srcAddress;
}

Ipv6Address SixLowPanIphc::GetSrcAddress() const {
	return m_srcAddress;
}

void SixLowPanIphc::SetDstAddress(Ipv6Address dstAddress) {
	m_dstAddress = dstAddress;
}

Ipv6Address SixLowPanIphc::GetDstAddress() const {
	return m_dstAddress;
}

void SixLowPanIphc::PostProcessSac() {
	return;
}

void SixLowPanIphc::PostProcessDac() {
	return;
}

std::ostream & operator <<(std::ostream & os, const SixLowPanIphc & h) {
	h.Print(os);
	return os;
}

/*
 * SixLowPanNhcExtensionHeader
 */
NS_OBJECT_ENSURE_REGISTERED(SixLowPanNhcExtension);

SixLowPanNhcExtension::SixLowPanNhcExtension() {
	// 1110 xxxx
	m_nhcExtensionHeader = 0xE0;
}

TypeId SixLowPanNhcExtension::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::sixlowpan::SixLowPanNhcExtension").SetParent<Header>().AddConstructor<
					SixLowPanNhcExtension>();
	return tid;
}

TypeId SixLowPanNhcExtension::GetInstanceTypeId(void) const {
	return GetTypeId();
}

void SixLowPanNhcExtension::Print(std::ostream & os) const {
	os << "Compression kind: " << m_nhcExtensionHeader;
}

uint32_t SixLowPanNhcExtension::GetSerializedSize() const {
	uint32_t serializedSize = 1;
	if (m_nhcExtensionHeader & 0x1) {
		serializedSize++;
	}
	return serializedSize;
}

void SixLowPanNhcExtension::Serialize(Buffer::Iterator start) const {
	Buffer::Iterator i = start;
	i.WriteU8(m_nhcExtensionHeader);
	if (m_nhcExtensionHeader & 0x1) {
		i.WriteU8(m_nhcNextHeader);
	}
}

uint32_t SixLowPanNhcExtension::Deserialize(Buffer::Iterator start) {
	Buffer::Iterator i = start;
	m_nhcExtensionHeader = i.ReadU8();
	if (m_nhcExtensionHeader & 0x1) {
		m_nhcNextHeader = i.ReadU8();
	}

	return GetSerializedSize();
}

SixLowPanDispatch::Dispatch_e SixLowPanNhcExtension::GetDispatchType(
		void) const {
	// TODO: fix it
	return LOWPAN_NALP;
}

}
}
