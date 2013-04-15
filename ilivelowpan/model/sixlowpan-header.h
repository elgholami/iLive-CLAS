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

#ifndef SIXLOWPANHEADER_H_
#define SIXLOWPANHEADER_H_

#include "ns3/header.h"
#include "ns3/ipv6-address.h"
#include "ns3/packet.h"
#include "ns3/internet-module.h"

#define UIP_HTONS(n) (uint16_t)((((uint16_t) (n)) << 8) | (((uint16_t) (n)) >> 8))

namespace ns3 {
namespace sixlowpan {

/**
 * \ingroup sixlowpan
 * \brief   Dispatch header. This is a virtual class.
 *
 * The dispatch type is defined by a zero bit as the first bit and a one
 *  bit as the second bit.
 \verbatim
 1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |0 1| Dispatch  |  type-specific header
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 \endverbatim
 */
class SixLowPanDispatch: public Header {
public:

	/**
	 *  \brief Dispatch values, as defined in RFC4944 and RFC6282
	 \verbatim
	 Pattern    Header Type
	 +------------+------------------------------------------------+
	 | 00  xxxxxx | NALP        - Not a LoWPAN frame               |
	 | 01  000000 | ESC         - Additional Dispatch byte follows |
	 | 01  000001 | IPv6        - Uncompressed IPv6 Addresses      |
	 | 01  000010 | LOWPAN_HC1  - LOWPAN_HC1 compressed IPv6       |
	 | 01  000011 | reserved    - Reserved for future use          |
	 |   ...      | reserved    - Reserved for future use          |
	 | 01  001111 | reserved    - Reserved for future use          |
	 | 01  010000 | LOWPAN_BC0  - LOWPAN_BC0 broadcast             |
	 | 01  010001 | reserved    - Reserved for future use          |
	 |   ...      | reserved    - Reserved for future use          |
	 | 01  1xxxxx | LOWPAN_IPHC - LOWPAN_IPHC compressed IPv6      |
	 | 10  xxxxxx | MESH        - Mesh Header                      |
	 | 11  000xxx | FRAG1       - Fragmentation Header (first)     |
	 | 11  001000 | reserved    - Reserved for future use          |
	 |   ...      | reserved    - Reserved for future use          |
	 | 11  011111 | reserved    - Reserved for future use          |
	 | 11  100xxx | FRAGN       - Fragmentation Header (subsequent)|
	 | 11  101000 | reserved    - Reserved for future use          |
	 |   ...      | reserved    - Reserved for future use          |
	 | 11  111111 | reserved    - Reserved for future use          |
	 +------------+------------------------------------------------+
	 \endverbatim
	 */
	typedef enum {
		LOWPAN_NALP = 0x0,
		LOWPAN_NALP_N = 0x3F,
		LOWPAN_NOTCOMPRESSED = 0x41,
		LOWPAN_HC1 = 0x42,
		LOWPAN_BC0 = 0x50,
		LOWPAN_IPHC = 0x60,
		LOWPAN_IPHC_N = 0x7F,
		LOWPAN_MESH = 0x80,
		LOWPAN_MESH_N = 0xBF,
		LOWPAN_FRAG1 = 0xC0,
		LOWPAN_FRAG1_N = 0xC7,
		LOWPAN_FRAGN = 0xE0,
		LOWPAN_FRAGN_N = 0xE7,
		LOWPAN_UNSUPPORTED = 0xFF
	} Dispatch_e;

	SixLowPanDispatch(void);
	SixLowPanDispatch(Dispatch_e dispatch);

	static TypeId GetTypeId(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	virtual TypeId GetInstanceTypeId(void) const;

	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size of the packet.
	 * \return size
	 */
	virtual uint32_t GetSerializedSize(void) const;

	/**
	 * \brief Serialize the packet.
	 * \param start Buffer iterator
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start Buffer iterator
	 * \return size of the packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	/**
	 * \brief Get the Dispatch type.
	 * \return the Dispatch type
	 */
	virtual Dispatch_e GetDispatchType(void) const;

	/**
	 * \brief Get the Dispatch type.
	 * \param dispatch the dispatch value
	 * \return the Dispatch type
	 */
	static Dispatch_e GetDispatchType(uint8_t dispatch);

};

std::ostream & operator<<(std::ostream & os, SixLowPanDispatch const & h);

class SixLowPanHc1: public SixLowPanDispatch {
public:

	/**
	 * \brief Kind of address compression.
	 *
	 * The address compression is handled in 4 bits and might mean:
	 * PI: Prefix inline, PC: Prefix Compressed,
	 * II: Interface Identifier, Inline, IC: Interface Identifier Compressed
	 */
	typedef enum {
		HC1_PIII = 0x00, HC1_PIIC = 0x01, HC1_PCII = 0x02, HC1_PCIC = 0x03
	} LowPanHc1Addr_e;

	/**
	 * \brief Next header information.
	 *
	 * The Next header compression is handled in 4 bits and might mean:
	 * NC: Not Compressed, UDP, ICMP or TCP.
	 */
	typedef enum {
		HC1_NC = 0x00, HC1_UDP = 0x01, HC1_ICMP = 0x03, HC1_TCP = 0x02
	} LowPanHc1NextHeader_e;

	SixLowPanHc1(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	static TypeId GetTypeId(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */

	virtual TypeId GetInstanceTypeId(void) const;

	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size of the packet.
	 * \return size
	 */
	virtual uint32_t GetSerializedSize(void) const;

	/**
	 * \brief Serialize the packet.
	 * \param start Buffer iterator
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start Buffer iterator
	 * \return size of the packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	/**
	 * \brief Get the Dispatch type.
	 * \return the Dispatch type
	 */
	virtual Dispatch_e GetDispatchType(void) const;

	/**
	 * \brief Set the "Hop limit" field (TTL).
	 * \param limit the hop limit value
	 */
	void SetHopLimit(uint8_t limit);

	/**
	 * \brief Get the "Hop limit" field (TTL).
	 * \return the hop limit value
	 */
	uint8_t GetHopLimit(void) const;

	LowPanHc1Addr_e GetDstCompression() const;
	const uint8_t* GetDstInterface() const;
	const uint8_t* GetDstPrefix() const;

	uint32_t GetFlowLabel() const;
	uint8_t GetNextHeader() const;

	LowPanHc1Addr_e GetSrcCompression() const;
	const uint8_t* GetSrcInterface() const;
	const uint8_t* GetSrcPrefix() const;

	uint8_t GetTrafficClass() const;

	bool IsTcflCompression() const;
	bool IsHc2HeaderPresent() const;

	void SetDstCompression(LowPanHc1Addr_e dstCompression);
	void SetDstInterface(const uint8_t* dstInterface);
	void SetDstPrefix(const uint8_t* dstPrefix);
	void SetFlowLabel(uint32_t flowLabel);
	void SetNextHeader(uint8_t nextHeader);
	void SetSrcCompression(LowPanHc1Addr_e srcCompression);
	void SetSrcInterface(const uint8_t* srcInterface);
	void SetSrcPrefix(const uint8_t* srcPrefix);
	void SetTcflCompression(bool tcflCompression);
	void SetTrafficClass(uint8_t trafficClass);
	void SetHc2HeaderPresent(bool hc2HeaderPresent);
	void SetTtl(uint8_t ttl);
	void SetHc1Encoding(uint8_t hc1Encoding);
	void SetUdpEncoding(uint8_t UdpEncoding);
	void SetUdpSrcPort(uint16_t UdpSrcPort);
	uint16_t GetUdpSrcPort() const;
	void SetUdpDstPort(uint16_t UdpDstPort);
	uint16_t GetUdpDstPort() const;
	void SetUdpLength(uint16_t UdpLength);
	void SetUdpChecksum(uint16_t UdpChecksum);

private:
//  uint8_t m_serializedSize;
//  uint8_t m_encoding;
	uint8_t m_hopLimit;
	uint8_t m_srcPrefix[8];
	uint8_t m_srcInterface[8];
	uint8_t m_dstPrefix[8];
	uint8_t m_dstInterface[8];
	uint8_t m_trafficClass;
	uint32_t m_flowLabel;
	uint8_t m_nextHeader;
	LowPanHc1Addr_e m_srcCompression;
	LowPanHc1Addr_e m_dstCompression;
	bool m_tcflCompression;
	LowPanHc1NextHeader_e m_nextHeaderCompression;
	bool m_hc2HeaderPresent;
	uint8_t m_ttl;
	uint8_t m_hc1Encoding;

	uint8_t m_hcUdpEncoding;
	uint16_t m_hcUdpSrcPort;
	uint16_t m_hcUdpDstPort;
	uint16_t m_hcUdpLength;
	uint16_t m_hcUdpChecksum;
};

std::ostream & operator<<(std::ostream & os, SixLowPanHc1 const & h);

class SixLowPanFrag1: public SixLowPanDispatch {
public:

	SixLowPanFrag1(void);

	static TypeId GetTypeId(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	virtual TypeId GetInstanceTypeId(void) const;

	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size of the packet.
	 * \return size
	 */
	virtual uint32_t GetSerializedSize(void) const;

	/**
	 * \brief Serialize the packet.
	 * \param start Buffer iterator
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start Buffer iterator
	 * \return size of the packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	/**
	 * \brief Get the Dispatch type.
	 * \return the Dispatch type
	 */
	virtual Dispatch_e GetDispatchType(void) const;

	void SetDatagramSize(uint16_t datagramsize);

	uint16_t GetDatagramSize(void) const;

	void SetDatagramTag(uint16_t datagramtag);

	uint16_t GetDatagramTag(void) const;

private:
	uint16_t m_datagramSize;
	uint16_t m_datagramTag;

};

std::ostream & operator<<(std::ostream & os, SixLowPanFrag1 const & h);

class SixLowPanFragN: public SixLowPanDispatch {
public:

	SixLowPanFragN(void);

	static TypeId GetTypeId(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	virtual TypeId GetInstanceTypeId(void) const;

	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size of the packet.
	 * \return size
	 */
	virtual uint32_t GetSerializedSize(void) const;

	/**
	 * \brief Serialize the packet.
	 * \param start Buffer iterator
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start Buffer iterator
	 * \return size of the packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	/**
	 * \brief Get the Dispatch type.
	 * \return the Dispatch type
	 */
	virtual Dispatch_e GetDispatchType(void) const;

	void SetDatagramSize(uint16_t datagramsize);

	uint16_t GetDatagramSize(void) const;

	void SetDatagramTag(uint16_t datagramtag);

	uint16_t GetDatagramTag(void) const;

	void SetDatagramOffset(uint8_t datagramoffset);

	uint8_t GetDatagramOffset(void) const;

private:
	uint16_t m_datagramSize;
	uint16_t m_datagramTag;
	uint8_t m_datagramOffset;

};

std::ostream & operator<<(std::ostream & os, SixLowPanFragN const & h);

/**
 * \ingroup sixlowpan
 * \brief   LOWPAN_IPHC base Encoding
 \verbatim
 0                                       1
 0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5
 +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 | 0 | 1 | 1 |  TF   |NH | HLIM  |CID|SAC|  SAM  | M |DAC|  DAM  |
 +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 \endverbatim
 */
class SixLowPanIphc: public SixLowPanDispatch {
public:
	/**
	 *  \brief TF: Traffic Class, Flow Label
	 *
	 *  00:  ECN + DSCP + 4-bit Pad + Flow Label (4 bytes)
	 *  01:  ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided.
	 *  10:  ECN + DSCP (1 byte), Flow Label is elided.
	 *  11:  Traffic Class and Flow Label are elided.
	 *
	 */
	enum TrafficClassFlowLabel_e {
		TF_FULL = 0, TF_DSCP_ELIDED, TF_FL_ELIDED, TF_ELIDED
	};

	/**
	 *  \brief HLIM: Hop Limit
	 *
	 *  00:  The Hop Limit field is carried in-line.
	 *  01:  The Hop Limit field is compressed and the hop limit is 1.
	 *  10:  The Hop Limit field is compressed and the hop limit is 64.
	 *  11:  The Hop Limit field is compressed and the hop limit is 255.
	 */
	enum Hlim_e {
		HLIM_INLINE = 0, HLIM_COMPR_1, HLIM_COMPR_64, HLIM_COMPR_255
	};

	/**
	 *  \brief Source or Destination Address Mode
	 *
	 *  00:  128 bits.
	 *  01:  64 bits (or 48 bits if multicast).
	 *  10:  16 bits (or 32 bits if multicast).
	 *  11:  Fully elided (or 8 bits if multicast).
	 */
	enum HeaderCompression_e {
		HC_INLINE = 0, HC_COMPR_64, HC_COMPR_16, HC_COMPR_0
	};

	SixLowPanIphc(void);
	SixLowPanIphc(uint8_t dispatch);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	static TypeId GetTypeId(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	virtual TypeId GetInstanceTypeId(void) const;

	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size of the packet.
	 * \return size
	 */
	virtual uint32_t GetSerializedSize(void) const;

	/**
	 * \brief Serialize the packet.
	 * \param start Buffer iterator
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start Buffer iterator
	 * \return size of the packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	/**
	 * \brief Get the Dispatch type.
	 * \return the Dispatch type
	 */
	virtual Dispatch_e GetDispatchType(void) const;

	/**
	 * \brief Set the TF (Traffic Class, Flow Label) compression.
	 * \param tfField ECN, DSCP, Flow Label compression type
	 */
	void SetTf(TrafficClassFlowLabel_e tfField);

	/**
	 * \brief Get the TF (Traffic Class, Flow Label) compression.
	 * \return the ECN, DSCP, Flow Label compression type
	 */
	TrafficClassFlowLabel_e GetTf(void) const;

	/**
	 * \brief Set the NH (Next Header) compression.
	 * \param nhField false (Next Header carried in-line), true (compressed NH)
	 */
	void SetNh(bool nhField);

	/**
	 * \brief Get the TF (Traffic Class, Flow Label) compression.
	 * \return false (Next Header carried in-line), true (compressed NH)
	 */
	bool GetNh(void) const;

	/**
	 * \brief Set the HLIM (Hop Limit) compression.
	 * \param hlimField Hop Limit compression type
	 */
	void SetHlim(Hlim_e hlimField);

	/**
	 * \brief Get the HLIM (Hop Limit) compression.
	 * \return Hop Limit compression type
	 */
	Hlim_e GetHlim(void) const;

	/**
	 * \brief Set the CID (Context Identifier Extension) compression.
	 * \param cidField false (no CID present), true (CID follows)
	 */
	void SetCid(bool cidField);

	/**
	 * \brief Get the TF (Traffic Class, Flow Label) compression.
	 * \return false (no CID present), true (CID follows)
	 */
	bool GetCid(void) const;

	/**
	 * \brief Set the SAC (Source Address Compression) compression.
	 * \param sacField false (stateful), true (stateless)
	 */
	void SetSac(bool sacField);

	/**
	 * \brief Get the SAC (Source Address Compression) compression.
	 * \return false (stareful), true (stateless)
	 */
	bool GetSac(void) const;

	/**
	 * \brief Set the SAM (Source Address Mode) compression.
	 * \param samField - depends on the SAC
	 */
	void SetSam(HeaderCompression_e samField);

	/**
	 * \brief Get the SAM (Source Address Mode) compression.
	 * \return depends on the SAC field
	 */
	HeaderCompression_e GetSam(void) const;

	/**
	 * \brief Set the M (Multicast) compression.
	 * \param mField true if destination is multicast
	 */
	void SetM(bool mField);

	/**
	 * \brief Get the M (Multicast) compression.
	 * \return true if destination is multicast
	 */
	bool GetM(void) const;

	/**
	 * \brief Set the DAC (Destination Address Compression) compression.
	 * \param dacField false (stateful), true (stateless)
	 */
	void SetDac(bool dacField);

	/**
	 * \brief Get the DAC (Destination Address Compression) compression.
	 * \return false (stareful), true (stateless)
	 */
	bool GetDac(void) const;

	/**
	 * \brief Set the DAM (Destination Address Mode) compression.
	 * \param damField - depends on the DAC and M fields
	 */
	void SetDam(HeaderCompression_e damField);

	/**
	 * \brief Get the DAM (Destination Address Mode) compression.
	 * \return depends on the DAC and M fields
	 */
	HeaderCompression_e GetDam(void) const;

	/**
	 * \brief Set the SrcContextId.
	 * \param srcContextId - valid values are [0:15]
	 */
	void SetSrcContextId(uint8_t srcContextId);

	/**
	 * \brief Get the SrcContextId.
	 * \return the SrcContextId
	 */
	uint8_t GetSrcContextId(void) const;

	/**
	 * \brief Set the DstContextId.
	 * \param dstContextId - valid values are [0:15]
	 */
	void SetDstContextId(uint8_t dstContextId);

	/**
	 * \brief Get the DstContextId.
	 * \return the DstContextId
	 */
	uint8_t GetDstContextId(void) const;

	/**
	 * \brief Set the ECN (2bits).
	 * \param ecn - valid values are [0:3]
	 */
	void SetEcn(uint8_t ecn);

	/**
	 * \brief Get the ECN.
	 * \return the ECN
	 */
	uint8_t GetEcn(void) const;

	/**
	 * \brief Set the DSCP (6bits).
	 * \param dscp - valid values are [0:63]
	 */
	void SetDscp(uint8_t dscp);

	/**
	 * \brief Get the DSCP.
	 * \return the DSCP
	 */
	uint8_t GetDscp(void) const;

	/**
	 * \brief Set the Flow Label (20bits).
	 * \param flowLabel - valid values are 20 bits long.
	 */
	void SetFlowLabel(uint32_t flowLabel);

	/**
	 * \brief Get the Flow Label.
	 * \return the Flow Label
	 */
	uint32_t GetFlowLabel(void) const;

	/**
	 * \brief Set the Next Header field.
	 * \param nextHeader Next Header field.
	 */
	void SetNextHeader(uint8_t nextHeader);

	/**
	 * \brief Get the Next Header field.
	 * \return the Next Header field.
	 */
	uint8_t GetNextHeader(void) const;

	/**
	 * \brief Set the Hop Limit field.
	 * \param hopLimit Hop Limit field.
	 */
	void SetHopLimit(uint8_t hopLimit);

	/**
	 * \brief Get the Hop Limit field.
	 * \return the Hop Limit field.
	 */
	uint8_t GetHopLimit(void) const;

	/**
	 * \brief Set the Source Address.
	 * \param srcAddress the Source Address.
	 */
	void SetSrcAddress(Ipv6Address srcAddress);

	/**
	 * \brief Get the Source Address.
	 * \return the Source Address.
	 */
	Ipv6Address GetSrcAddress() const;

	/**
	 * \brief Set the Destination Address.
	 * \param dstAddress the Destination Address.
	 */
	void SetDstAddress(Ipv6Address dstAddress);

	/**
	 * \brief Get the Destination Address.
	 * \return the Destination Address.
	 */
	Ipv6Address GetDstAddress() const;

private:
	uint16_t m_baseFormat;
	uint8_t m_srcdstContextId;
	uint8_t m_ecn :2;
	uint8_t m_dscp :6;
	uint32_t m_flowLabel :20;
	uint8_t m_nextHeader;
	uint8_t m_hopLimit;
	Ipv6Address m_srcAddress;
	Ipv6Address m_dstAddress;

	void PostProcessSac();
	void PostProcessDac();
};

std::ostream & operator<<(std::ostream & os, SixLowPanIphc const &);

/**
 * \ingroup sixlowpan
 * \brief   LOWPAN_NHC Extension Header Encoding
 \verbatim
 0   1   2   3   4   5   6   7
 +---+---+---+---+---+---+---+---+
 | 1 | 1 | 1 | 0 |    EID    |NH |
 +---+---+---+---+---+---+---+---+
 \endverbatim
 */
class SixLowPanNhcExtension: public SixLowPanDispatch {
public:
	/**
	 *  \brief EID: IPv6 Extension Header ID
	 *
	 *   EID: IPv6 Extension Header ID:
	 *      0: IPv6 Hop-by-Hop Options Header [RFC2460]
	 *      1: IPv6 Routing Header [RFC2460]
	 *      2: IPv6 Fragment Header [RFC2460]
	 *      3: IPv6 Destination Options Header [RFC2460]
	 *      4: IPv6 Mobility Header [RFC6275]
	 *      5: Reserved
	 *      6: Reserved
	 *      7: IPv6 Header
	 */
	enum Eid_e {
		EID_HOPBYHOP_OPTIONS_H = 0,
		EID_ROUTING_H,
		EID_FRAGMENTATION_H,
		EID_DESTINATION_OPTIONS_H,
		EID_MOBILITY_H,
		EID_IPv6_H = 7
	};

	SixLowPanNhcExtension(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	static TypeId GetTypeId(void);

	/**
	 * \brief Return the instance type identifier.
	 * \return instance type ID
	 */
	virtual TypeId GetInstanceTypeId(void) const;

	virtual void Print(std::ostream& os) const;

	/**
	 * \brief Get the serialized size of the packet.
	 * \return size
	 */
	virtual uint32_t GetSerializedSize(void) const;

	/**
	 * \brief Serialize the packet.
	 * \param start Buffer iterator
	 */
	virtual void Serialize(Buffer::Iterator start) const;

	/**
	 * \brief Deserialize the packet.
	 * \param start Buffer iterator
	 * \return size of the packet
	 */
	virtual uint32_t Deserialize(Buffer::Iterator start);

	/**
	 * \brief Get the Dispatch type.
	 * \return the Dispatch type
	 */
	virtual Dispatch_e GetDispatchType(void) const;

	/**
	 * \brief Set the Extension Header Type.
	 * \param extensionHeaderType the Extension Header Type
	 */
	void SetEid(Eid_e extensionHeaderType);

	/**
	 * \brief Get the Extension Header Type.
	 * \return the Extension Header Type
	 */
	Eid_e GetEid(void) const;

	/**
	 * \brief Set the Next Header field values.
	 * \param nextHeader the Next Header field value
	 */
	void SetNextHeader(uint8_t nextHeader);

	/**
	 * \brief Get the Next Header field value.
	 * \return the Next Header field value
	 */
	uint8_t GetNextHeader(void) const;

private:
	uint8_t m_nhcExtensionHeader;
	uint8_t m_nhcNextHeader;

};

std::ostream & operator<<(std::ostream & os, SixLowPanNhcExtension const &);

}
}

#endif /* SIXLOWPANHEADER_H_ */
