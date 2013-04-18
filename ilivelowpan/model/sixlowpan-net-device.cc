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

#include "sixlowpan-net-device.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/icmpv6-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/random-variable.h"
#include "sixlowpan-header.h"

#define YIBO

NS_LOG_COMPONENT_DEFINE("SixLowPanNetDevice");

namespace ns3 {
namespace sixlowpan {

NS_OBJECT_ENSURE_REGISTERED(SixLowPanNetDevice);

TypeId SixLowPanNetDevice::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::sixlowpan::SixLowPanNetDevice").SetParent<NetDevice>().AddConstructor<
					SixLowPanNetDevice>().AddAttribute(
					"FragmentReassemblyListSize",
					"The maximum size of the reassembly buffer (in packets). Zero meaning infinite.",
					UintegerValue(0),
					MakeUintegerAccessor(
							&SixLowPanNetDevice::m_fragmentReassemblyListSize),
					MakeUintegerChecker<uint16_t>()).AddAttribute(
					"FragmentExpirationTimeout",
					"When this timeout expires, the fragments will be cleared from the buffer.",
					TimeValue(Seconds(180)),
					MakeTimeAccessor(
							&SixLowPanNetDevice::m_fragmentExpirationTimeout),
					MakeTimeChecker()).AddTraceSource("Tx",
					"Send IPv6 packet to outgoing interface.",
					MakeTraceSourceAccessor(&SixLowPanNetDevice::m_txTrace)).AddTraceSource(
					"Rx", "Receive IPv6 packet from incoming interface.",
					MakeTraceSourceAccessor(&SixLowPanNetDevice::m_rxTrace)).AddTraceSource(
					"Drop", "Drop IPv6 packet",
					MakeTraceSourceAccessor(&SixLowPanNetDevice::m_dropTrace))
//					        .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
//					                       UintegerValue (102),
//					                       MakeUintegerAccessor (&SixLowPanNetDevice::SetMtu,
//					                                             &SixLowPanNetDevice::GetMtu),
//					                                             MakeUintegerChecker<uint16_t> ())
							;
	//YIBO: Mtu should be added or not ? Should ask Khalid.
	return tid;
}

SixLowPanNetDevice::SixLowPanNetDevice() :
		m_node(0), m_port(0), m_ifIndex(0)
//    m_channel (0),
//YIBO: channel issue give to CSMA module
{
	NS_LOG_FUNCTION_NOARGS ();
	m_port = 0;
	//YIBO: Use Port not interface index ? No. Use Set/GetIfindex.

//  overhead = 0;
//  ipv6_oh_diff = 0;
//  timer_overhead.SetFunction (&SixLowPanNetDevice::SaveOverhead, this);
//  timer_overhead.Schedule (Seconds (1));
	//YIBO: if 1 second is expired, the overhead will be thrown away.
	//YIBO: SaveOverHead is not used in this version.
//  enter = false;
}

//void
//SixLowPanNetDevice::SaveOverhead ()
//{
//  file_o.open ("overhead.csv",ios::out | ios::app);
//  file_diff.open("overhead-diff-ipv6.csv", ios::out | ios::app);
//
//   if (enter)
//   {
//      if (m_node->GetId() == 0)
//      {
//        file_o << "\n";
//        file_diff << "\n";
//      }
//      enter = false;
//   }else
//   {
//      file_o << overhead << ";";
//      file_diff << ipv6_oh_diff << ";";
//      overhead = 0;
//      ipv6_oh_diff = 0;
//      enter = true;
//   }

//  file_o.close();
//  file_diff.close();
//
//   timer_overhead.Schedule (Seconds (0.5));
//}

SixLowPanNetDevice::~SixLowPanNetDevice() {
	NS_LOG_FUNCTION_NOARGS ();
	//YIBO: do nothing for destructure.
}

Ptr<NetDevice> SixLowPanNetDevice::GetPort() const {
	NS_LOG_FUNCTION_NOARGS ();
	return m_port;
}

void SixLowPanNetDevice::SetPort(Ptr<NetDevice> port) {
	NS_LOG_FUNCTION_NOARGS ();
	m_port = port;

	NS_LOG_DEBUG ("RegisterProtocolHandler for " << port->GetInstanceTypeId ().GetName ());
	m_node->RegisterProtocolHandler(
			MakeCallback(&SixLowPanNetDevice::ReceiveFromDevice, this), 0, port,
			false);
}

void SixLowPanNetDevice::DoDispose() {
	NS_LOG_FUNCTION_NOARGS ();

	m_port = 0;
	//  m_channel = 0;
	//YIBO: don't need care channel in the 6lowpan
	m_node = 0;
	NetDevice::DoDispose();
}

void SixLowPanNetDevice::ReceiveFromDevice(Ptr<NetDevice> incomingPort,
		Ptr<const Packet> packet, uint16_t protocol, Address const &src,
		Address const &dst, PacketType packetType) {
	NS_LOG_FUNCTION_NOARGS ();NS_LOG_DEBUG ("UID is " << packet->GetUid ());

	uint8_t dispatchRawVal = 0;
	SixLowPanDispatch::Dispatch_e dispatchVal;
	Ptr<Packet> copyPkt = packet->Copy();
	Ptr<HeaderStorage> ipHeaders = Create<HeaderStorage>();
#ifdef YIBO
	std::cout << "***>>>>>>>>>>YIBO::Start of ReceiveFromDevice<<<<<<<<<<***"
			<< std::endl;
#endif
	NS_LOG_DEBUG("Received a packet::" << *copyPkt);

	copyPkt->CopyData(&dispatchRawVal, sizeof(dispatchRawVal));
	dispatchVal = SixLowPanDispatch::GetDispatchType(dispatchRawVal);
	bool isPktDecompressed = false;

	NS_LOG_DEBUG("dispatchVal = " << dispatchVal);

	//YIBO: Print the packet received and length, which is from csma layer right now.
//	NS_LOG_DEBUG ( "Packet received: " << *copyPkt );NS_LOG_DEBUG ( "Packet length:" << copyPkt->GetSize () );

	//      NALP = 0x0,
	//      NALP_N = 0x3F,
	//      NOTCOMPRESSED = 0x41,
	//      LOWPAN_HC1 = 0x42,
	//      LOWPAN_BC0 = 0x50,
	//      LOWPAN_IPHC = 0x60,
	//      LOWPAN_MESH = 0x80,
	//      LOWPAN_FRAG1 = 0xC0,
	//      LOWPAN_FRAGN = 0xE0,
	//      UNSUPPORTED = 0xFF
	//YIBO:: 6lowpan dispatches

	if (dispatchVal == SixLowPanDispatch::LOWPAN_MESH) {
		NS_FATAL_ERROR(
				"Unsupported 6LoWPAN encoding: MESH, exit due to error.");
	}

	if (dispatchVal == SixLowPanDispatch::LOWPAN_BC0) {
		NS_FATAL_ERROR("Unsupported 6LoWPAN encoding: BC0, exit due to error.");
	}

	if (((dispatchVal) & 0xf8) == SixLowPanDispatch::LOWPAN_FRAG1) {
		NS_LOG_DEBUG("CYB:: Receive a dispatch of LOWPAN_FRAG1");
		isPktDecompressed = ProcessFragment(copyPkt, src, dst, true, ipHeaders);
		NS_LOG_DEBUG("CYB:: Process LOWPAN_FRAG1 successfully!");
	} else if (dispatchVal == SixLowPanDispatch::LOWPAN_FRAGN) {
		NS_LOG_DEBUG("CYB:: Receive a dispatch of LOWPAN_FRAGN");
		isPktDecompressed = ProcessFragment(copyPkt, src, dst, false, ipHeaders);
		NS_LOG_DEBUG("CYB:: Process LOWPAN_FRAGN successfully!");
		NS_LOG_DEBUG ( "YIBO::Packet After FRAGNs:" << *copyPkt );
	} else {
		switch (dispatchVal) {
		case SixLowPanDispatch::LOWPAN_NOTCOMPRESSED:
			NS_LOG_DEBUG ( "Packet without compression:" << *copyPkt );
			NS_LOG_DEBUG ( "Packet length:" << copyPkt->GetSize () );
			//YIBO: without doing anything for the uncompressed package, just Print.
			//YIBO: TODO: Need to deliver the package to IPv6 network layer.
			break;
		case SixLowPanDispatch::LOWPAN_HC1:
			NS_LOG_DEBUG ( "YIBO::Packet with HC1 compression:" << *copyPkt );
			DecompressLowPanHc1(copyPkt, src, dst, ipHeaders);
#ifdef YIBO
			std::cout << "*****YIBO:" << src << std::endl;
#endif
			isPktDecompressed = true;
			break;
		case SixLowPanDispatch::LOWPAN_IPHC:
			NS_LOG_DEBUG ( "Unsupported");
			break;
		default:
#ifdef YIBO
			std::cout << "YIBO::problem Pkt:" << *copyPkt << std::endl;
#endif
			NS_FATAL_ERROR("Unsupported 6LoWPAN encoding, exit due to error.");
			break;
		}

	}

	if (!isPktDecompressed) {
#ifdef YIBO
		std::cout << "Pkt is not Decompressed!" << std::endl;
#endif
		return;
	}
#ifdef YIBO
	std::cout << "Pkt is Decompressed well!" << std::endl;
#endif

	FinalizePacketIp(copyPkt, ipHeaders);
#ifdef YIBO
	std::cout << "Let us see what we got =" << *copyPkt
			<< std::endl;
#endif


	//YIBO: Test the header storage of 6LoWPAN
	Address address = m_port->GetAddress();
#ifdef YIBO
	std::cout << "###***" << address << std::endl;
	std::cout << "###***" << dst << std::endl;
#endif

	Ipv6Header* hdr; // = (*dynamic_cast<Ipv6Header *> ipHeaders->GetHeader(Ipv6Header::GetTypeId()) );
	//YIBO: Force Header type to Ipv6Header type, so the IPv6 can process it.
	hdr = (Ipv6Header *) ipHeaders->GetHeader(Ipv6Header::GetTypeId());
//	if (hdr) {
//		packet->AddHeader(*dynamic_cast<Ipv6Header *>(hdr));
////		//YIBO: why need to add the header again?
////		packet->AddHeader(hdr);
//	}

	if (packetType == 0) {
#ifdef YIBO
		std::cout << "YIBO::packetType == 0" << std::endl;
#endif
		if (hdr->GetDestinationAddress().IsMulticast()) {
			packetType = PACKET_MULTICAST;
#ifdef YIBO
			std::cout << "YIBO::Get a PACKET_MULTICAST." << std::endl;
#endif
		} else if (dst == address) {
			packetType = PACKET_HOST;
#ifdef YIBO
			std::cout << "YIBO::Get a PACKET_HOST." << std::endl;
#endif
		} else {
			packetType = PACKET_OTHERHOST;
#ifdef YIBO
			std::cout << "YIBO::Get a PACKET_OTHERHOST." << std::endl;
#endif
		}
	}

	//YIBO: After the process functions above, the processed package need to be delivered to Ipv6L3Protocol.
	//YIBO: Notice these two Callbacks.
	if (!m_promiscRxCallback.IsNull()) {
#ifdef YIBO
		//YIBO: If m_promiscRxCallback of this net-device is not NULL.
		std::cout << "###YIBO::Throw this packet to Ipv6L3Protocol."
				<< std::endl;
#endif
		m_promiscRxCallback(this, copyPkt, Ipv6L3Protocol::PROT_NUMBER, src,
				dst, packetType);
	}
#ifdef YIBO
	std::cout << "YIBO::Throw this packet to Ipv6L3Protocol." << std::endl;
	std::cout << "###" << *copyPkt << std::endl;
#endif

	//YIBO: Check the packetType
	switch (packetType) {
	case PACKET_HOST:
		if (dst == address) {
#ifdef YIBO
			std::cout << "YIBO::Go to the PACKET_HOST case." << std::endl;
#endif
//			m_promiscRxCallback(this, copyPkt, Ipv6L3Protocol::PROT_NUMBER, hdr->GetSourceAddress(), hdr->GetDestinationAddress(), packetType);

			m_rxCallback(this, copyPkt, Ipv6L3Protocol::PROT_NUMBER, src);
		}
		break;

	case PACKET_BROADCAST:
	case PACKET_MULTICAST:
		m_rxCallback(this, copyPkt, Ipv6L3Protocol::PROT_NUMBER, src);

		break;

	case PACKET_OTHERHOST:
		if (dst == address) {
			m_rxCallback(this, copyPkt, Ipv6L3Protocol::PROT_NUMBER, src);
		}
		break;
	}
#ifdef YIBO
	std::cout << "***>>>>>>>>>>YIBO::END of ReceiveFromDevice<<<<<<<<<<***"
			<< std::endl;
#endif
	return;
}

void SixLowPanNetDevice::SetIfIndex(const uint32_t index) {
	NS_LOG_FUNCTION_NOARGS ();
	// NS_ASSERT_MSG ( m_port != 0, "Sixlowpan: can't find any lower-layer protocol " << m_port );
	m_ifIndex = index;
//	 m_port->SetIfIndex(index);
}

uint32_t SixLowPanNetDevice::GetIfIndex(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	// NS_ASSERT_MSG ( m_port != 0, "Sixlowpan: can't find any lower-layer protocol " << m_port );
	// return m_ifIndex;
	return m_port->GetIfIndex();
	//YIBO:: ??
}

Ptr<Channel> SixLowPanNetDevice::GetChannel(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->GetChannel();
}

void SixLowPanNetDevice::SetAddress(Address address) {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	m_port->SetAddress(address);
}

Address SixLowPanNetDevice::GetAddress(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->GetAddress();
}

bool SixLowPanNetDevice::SetMtu(const uint16_t mtu) {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->SetMtu(mtu);
}

uint16_t SixLowPanNetDevice::GetMtu(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->GetMtu();
}

bool SixLowPanNetDevice::IsLinkUp(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->IsLinkUp();
}

void SixLowPanNetDevice::AddLinkChangeCallback(Callback<void> callback) {
}

bool SixLowPanNetDevice::IsBroadcast(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->IsBroadcast();
}

Address SixLowPanNetDevice::GetBroadcast(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->GetBroadcast();
}

bool SixLowPanNetDevice::IsMulticast(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->IsMulticast();
}

Address SixLowPanNetDevice::GetMulticast(Ipv4Address multicastGroup) const {
	NS_LOG_FUNCTION (this << multicastGroup);
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->GetMulticast(multicastGroup);
}

Address SixLowPanNetDevice::GetMulticast(Ipv6Address addr) const {
	NS_LOG_FUNCTION (this << addr);
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->GetMulticast(addr);
}

bool SixLowPanNetDevice::IsPointToPoint(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->IsPointToPoint();
}

bool SixLowPanNetDevice::IsBridge(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	return m_port->IsBridge();
}
//YIBO:: ?? All of the functions above use the m_port variable.

bool SixLowPanNetDevice::Send(Ptr<Packet> packet, const Address& dest,
		uint16_t protocolNumber) {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);
	//YIBO:: m_port is the only used parameter in sixlowpan net device.

	uint32_t origHdrSize = 0;
//	uint32_t origPacketSize = packet->GetSerializedSize();
	uint32_t origPacketSize = packet->GetSize();
	Ptr<HeaderStorage> headersPre = Create<HeaderStorage>();
	Ptr<HeaderStorage> headersPost = Create<HeaderStorage>();
	bool ret = false;
	NS_LOG_DEBUG( "***Prepare to Send a 6LoWPAN packet*** " );NS_LOG_DEBUG( "***CYB:origPacketSize = " << origPacketSize);
	origHdrSize += CompressLowPanHc1(packet, m_port->GetAddress(), dest,
			headersPre);
	NS_LOG_DEBUG( "***CYB:origHdrSize = " << origHdrSize);
	//YIBO:: create new function compressLowPanHc6...here

	if (origPacketSize > 102) {
		//YIBO:: fragment is needed, Mtu of 802.15.4 is smaller than the packet. Test requested.
		std::list<Ptr<Packet> > fragmentList;
#ifdef YIBO
		std::cout
				<< "***YIBO: DoFragmentation of Send! Need Frag.*** origPacketSize ="
				<< origPacketSize << std::endl;
#endif

		DoFragmentation(packet, origPacketSize, origHdrSize, headersPre,
				headersPost, fragmentList);
		std::list<Ptr<Packet> >::iterator it;
		bool err = false;
		for (it = fragmentList.begin(); it != fragmentList.end(); it++) {
			NS_LOG_DEBUG("CYB:SixLowPanNetDevice::Send (Fragment) " << **it);
			// err |= !(m_port->Send(*it, dest, protocolNumber));
			err |= !(m_port->Send(*it, dest, 0x809a));
		}
		ret = !err;
	} else {
		NS_LOG_DEBUG( "***YIBO: No Need for Frag.***");
		FinalizePacketPreFrag(packet, headersPre);
		FinalizePacketPostFrag(packet, headersPost);

		NS_LOG_DEBUG( "CYB:SixLowPanNetDevice::Send " << m_node->GetId () << " " << *packet << " Dst:"<< dest);
		// ret = m_port->Send (packet, dest, protocolNumber);
		//YIBO:: Fix the protocolNumber to UIP_ETHTYPE_802154, like ravenusb. So Wireshark can work.
		ret = m_port->Send(packet, dest, 0x809a);
		NS_LOG_DEBUG ("Sending UID packet is " << packet->GetUid ());
//		ret = m_port->Send(packet, dest, 0x86dd);
	}NS_LOG_DEBUG( "***End of Sending a 6LoWPAN packet*** " );
	return ret;
}

bool SixLowPanNetDevice::SendFrom(Ptr<Packet> packet, const Address& src,
		const Address& dest, uint16_t protocolNumber) {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);

	uint32_t origHdrSize = 0;
	uint32_t origPacketSize = packet->GetSerializedSize();
	Ptr<HeaderStorage> headersPre = Create<HeaderStorage>();
	Ptr<HeaderStorage> headersPost = Create<HeaderStorage>();
	bool ret = false;

	origHdrSize += CompressLowPanHc1(packet, src, dest, headersPre);
	//YIBO:: create new function compressLowPanHc6...here

	if (packet->GetSerializedSize() > 102) {
		// fragment
		//YIBO:: Not test yet
		std::list<Ptr<Packet> > fragmentList;
		DoFragmentation(packet, origPacketSize, origHdrSize, headersPre,
				headersPost, fragmentList);
		std::list<Ptr<Packet> >::iterator it;
		bool err = false;
		for (it = fragmentList.begin(); it != fragmentList.end(); it++) {
			NS_LOG_DEBUG( "SixLowPanNetDevice::SendFrom (Fragment) " << **it );
			// err |= !(m_port->SendFrom(*it, src, dest, protocolNumber));
			err |= !(m_port->SendFrom(*it, src, dest, 0x809a));
		}
		ret = !err;
	} else {
		FinalizePacketPreFrag(packet, headersPre);
		FinalizePacketPostFrag(packet, headersPost);

		NS_LOG_DEBUG( "SixLowPanNetDevice::SendFrom " << *packet );
		// ret = m_port->SendFrom (packet, src, dest, protocolNumber);
		//YIBO:: Fix the protocolNumber to UIP_ETHTYPE_802154, like ravenusb. So Wireshark can work.
		ret = m_port->SendFrom(packet, src, dest, 0x809a);
	}

	return ret;
}

Ptr<Node> SixLowPanNetDevice::GetNode(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	return m_node;
}

void SixLowPanNetDevice::SetNode(Ptr<Node> node) {
	NS_LOG_FUNCTION_NOARGS ();
	m_node = node;
}

bool SixLowPanNetDevice::NeedsArp(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT_MSG( m_port != 0,
			"Sixlowpan: can't find any lower-layer protocol " << m_port);
	//YIBO:: never used.
	return m_port->NeedsArp();
}

void SixLowPanNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb) {
	NS_LOG_FUNCTION_NOARGS ();
	m_rxCallback = cb;
}

void SixLowPanNetDevice::SetPromiscReceiveCallback(
		NetDevice::PromiscReceiveCallback cb) {
	NS_LOG_FUNCTION_NOARGS ();
	m_promiscRxCallback = cb;
}

bool SixLowPanNetDevice::SupportsSendFrom() const {
	NS_LOG_FUNCTION_NOARGS ();
	//YIBO:: useless function.
	return true;
}

//YIBO:: ********* Incomplete header compress 01-02 module *********
uint32_t SixLowPanNetDevice::CompressLowPanHc1(Ptr<Packet> packet,
		Address const &src, Address const &dst, Ptr<HeaderStorage> headers) {
	NS_LOG_FUNCTION (this << *packet << src << dst);

	Ipv6Header ipHeader;
	SixLowPanHc1* hc1Header = new SixLowPanHc1;
	uint32_t size = 0;
	NS_LOG_DEBUG( "SixLowPanNetDevice::CompressLowPanHc1_B " << *packet << ",src:" << src << ",dsr:" << dst );

	if (packet->PeekHeader(ipHeader) != 0) {
		packet->RemoveHeader(ipHeader);
		size += ipHeader.GetSerializedSize();

#ifdef YIBO
		std::cout << "***YIBO: original header size = " << size << std::endl;
#endif

		hc1Header->SetHopLimit(ipHeader.GetHopLimit());
		NS_LOG_DEBUG( "SixLowPanNetDevice::HopLimit/LLT = "<< int(hc1Header->GetHopLimit()));

		uint8_t bufOne[16];
		uint8_t bufTwo[16];
		Ipv6Address srcAddr = ipHeader.GetSourceAddress();
		srcAddr.GetBytes(bufOne); //YIBO::Put srcAddr into bufOne.
		Ipv6Address mySrcAddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(
				Mac48Address::ConvertFrom(src)); //YIBO:: process src to Ipv6Address mySrcAddr.
		mySrcAddr.GetBytes(bufTwo); //YIBO:: process src to Ipv6Address mySrcAddr. Saved in bufTwo.
		bool isSrcSrc = (memcmp(bufOne + 8, bufTwo + 8, 8) == 0);
		//YIBO:: if AddrSrc in packet and src are equal. isSrcSrc = true.
		if (srcAddr.IsLinkLocal() && isSrcSrc) {
			hc1Header->SetSrcCompression(SixLowPanHc1::HC1_PCIC);
			NS_LOG_DEBUG( " -SrcCompression:HC1_PCIC::0x11- ");
		} else if (srcAddr.IsLinkLocal()) {
			hc1Header->SetSrcCompression(SixLowPanHc1::HC1_PCII);
			hc1Header->SetSrcInterface(bufOne + 8);
			NS_LOG_DEBUG( " -SrcCompression:HC1_PCII::0x10- ");
		} else if (isSrcSrc) {
			hc1Header->SetSrcCompression(SixLowPanHc1::HC1_PIIC);
			hc1Header->SetSrcPrefix(bufOne);
			NS_LOG_DEBUG( " -SrcCompression:HC1_PIIC::0x01- ");
		} else {
			hc1Header->SetSrcCompression(SixLowPanHc1::HC1_PIII);
			hc1Header->SetSrcInterface(bufOne + 8);
			hc1Header->SetSrcPrefix(bufOne);
			NS_LOG_DEBUG( " -SrcCompression:HC1_PIII::0x00- ");
		}

		Ipv6Address dstAddr = ipHeader.GetDestinationAddress();
		dstAddr.GetBytes(bufOne);
		Ipv6Address myDstAddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(
				Mac48Address::ConvertFrom(dst));
		myDstAddr.GetBytes(bufTwo);
		bool isDstDst = (memcmp(bufOne + 8, bufTwo + 8, 8) == 0);

		if (dstAddr.IsLinkLocal() && isDstDst) {
			hc1Header->SetDstCompression(SixLowPanHc1::HC1_PCIC);
			NS_LOG_DEBUG( " -DstCompression:HC1_PCIC::0x11- ");
		} else if (dstAddr.IsLinkLocal()) {
			hc1Header->SetDstCompression(SixLowPanHc1::HC1_PCII);
			hc1Header->SetDstInterface(bufOne + 8);
			NS_LOG_DEBUG( " -DstCompression:HC1_PCII::0x10- ");
		} else if (isDstDst) {
			hc1Header->SetDstCompression(SixLowPanHc1::HC1_PIIC);
			hc1Header->SetDstPrefix(bufOne);
			NS_LOG_DEBUG( " -DstCompression:HC1_PIIC::0x01- ");
		} else {
			hc1Header->SetDstCompression(SixLowPanHc1::HC1_PIII);
			hc1Header->SetDstInterface(bufOne + 8);
			hc1Header->SetDstPrefix(bufOne);
			NS_LOG_DEBUG( " -DstCompression:HC1_PIII::0x00- ");
		}

		if ((ipHeader.GetFlowLabel() == 0)
				&& (ipHeader.GetTrafficClass() == 0)) {
			hc1Header->SetTcflCompression(true); //YIBO:: tcfl Traffic class field - 8 bit of ipv6 header
			NS_LOG_DEBUG( " -HC1 Compression:tcfl::0x1- ");
		} else {
			hc1Header->SetTcflCompression(false);
			hc1Header->SetTrafficClass(ipHeader.GetTrafficClass());
			hc1Header->SetFlowLabel(ipHeader.GetFlowLabel());
			NS_LOG_DEBUG( " -HC1 Compression:tcfl::0x0- ");
		}

		uint8_t nextHeader = ipHeader.GetNextHeader(); //YIBO:: Return a next header number.
		hc1Header->SetNextHeader(nextHeader); //YIBO:: UDP;TCP;ICMPv6
		NS_LOG_DEBUG( "------YIBO: nextHeader = "<< (uint16_t)nextHeader);

		//YIBO:: Add the proper getter/setters to UdpHeader and finalize this.
		//YIBO:: We do only full compression.
		//YIBO:: This is feasible if both src and dest ports are between
		//YIBO:: SIXLOWPAN_UDP_PORT_MIN and SIXLOWPAN_UDP_PORT_MIN + 15
		if (nextHeader == Ipv6Header::IPV6_UDP) {
#ifdef YIBO
			std::cout << "-------YIBO: I am compressing a IPV6_UDP Header "
					<< std::endl;
#endif
			NS_LOG_DEBUG( " -HC1 Compression:last 3 bits:011- ");
			hc1Header->SetHc2HeaderPresent(true);

			UdpHeader udpHeader;
			packet->RemoveHeader(udpHeader);
			size += udpHeader.GetSerializedSize();

			hc1Header->SetHc1Encoding(0xFB);
			hc1Header->SetUdpEncoding(0xE0);
			hc1Header->SetHopLimit(ipHeader.GetHopLimit());
			hc1Header->SetUdpSrcPort(udpHeader.GetSourcePort());
			hc1Header->SetUdpDstPort(udpHeader.GetDestinationPort());
			hc1Header->SetUdpLength(udpHeader.GetSerializedSize());
			hc1Header->SetUdpChecksum(udpHeader.GetChecksums());

			size += 7;

		} else if (nextHeader == Ipv6Header::IPV6_ICMPV6) {
#ifdef YIBO
			std::cout << "------YIBO: I am compressing a IPV6_ICMPV6 Header!"
					<< std::endl;
#endif
			NS_LOG_DEBUG( " -HC1 Compression:last 3 bits:110- ");
//			hc1Header->SetHc1Encoding(0xFE);
			hc1Header->SetHc2HeaderPresent(false);
			hc1Header->SetTtl(ipHeader.GetHopLimit());
			size += 3;
		} else {
			hc1Header->SetHc2HeaderPresent(false);
		}

		size = hc1Header->GetSerializedSize();
		//Yibo:: Store the Hc1 Header to headersPre. Suspicious item.
		headers->StoreHeader(SixLowPanHc1::GetTypeId(), hc1Header);

#ifdef YIBO
		std::cout << "------YIBO: after HC1 compress, size = " << size
				<< std::endl;
#endif

		return size; //YIBO:: Only compress IPv6 header.
		//packet->AddHeader(*hc1Header);
//		packet->AddHeader(*dynamic_cast<SixLowPanHc1 *>(hc1Header));
	}

	return 0;
}

void SixLowPanNetDevice::DecompressLowPanHc1(Ptr<Packet> packet,
		Address const &src, Address const &dst, Ptr<HeaderStorage> headers) {
	NS_LOG_FUNCTION (this << *packet << src << dst);

	Ipv6Header* ipHeaderPtr = new Ipv6Header;
	SixLowPanHc1 encoding;
	uint32_t packetOrigBufferSize = packet->GetSize();
#ifdef YIBO
	std::cout << "<<<<<<<<---START of DecompressLowPanHc1--->>>>>>>>"
			<< std::endl;
#endif

	packet->RemoveHeader(encoding);
	ipHeaderPtr->SetHopLimit(encoding.GetHopLimit());
	uint32_t packetOrigHeaderSize = encoding.GetSerializedSize();

	switch (encoding.GetSrcCompression()) {

	const uint8_t* interface;
	const uint8_t* prefix;
	uint8_t address[16];

case SixLowPanHc1::HC1_PIII:
	prefix = encoding.GetSrcPrefix();
	interface = encoding.GetSrcInterface();
	for (int j = 0; j < 8; j++) {
		address[j + 8] = interface[j];
		address[j] = prefix[j];
	}
	ipHeaderPtr->SetSourceAddress(Ipv6Address(address));
	break;
case SixLowPanHc1::HC1_PIIC:
	prefix = encoding.GetSrcPrefix();
	for (int j = 0; j < 8; j++) {
		address[j + 8] = 0;
		address[j] = prefix[j];
	}

	ipHeaderPtr->SetSourceAddress(
			Ipv6Address::MakeAutoconfiguredAddress(
					Mac48Address::ConvertFrom(src), Ipv6Address(address)));
	break;
case SixLowPanHc1::HC1_PCII:
	interface = encoding.GetSrcInterface();
	address[0] = 0xfe;
	address[1] = 0x80;
	for (int j = 0; j < 8; j++) {
		address[j + 8] = interface[j];
	}
	ipHeaderPtr->SetSourceAddress(Ipv6Address(address));
	break;
case SixLowPanHc1::HC1_PCIC:
	ipHeaderPtr->SetSourceAddress(
			Ipv6Address::MakeAutoconfiguredLinkLocalAddress(
					Mac48Address::ConvertFrom(src)));
	break;
	}
//YIBO: End of SrcCompression.

	switch (encoding.GetDstCompression()) {
	const uint8_t* interface;
	const uint8_t* prefix;
	uint8_t address[16];

case SixLowPanHc1::HC1_PIII:
	prefix = encoding.GetDstPrefix();
	interface = encoding.GetDstInterface();
	for (int j = 0; j < 8; j++) {
		address[j + 8] = interface[j];
		address[j] = prefix[j];
	}
	ipHeaderPtr->SetDestinationAddress(Ipv6Address(address));
	break;
case SixLowPanHc1::HC1_PIIC:
	prefix = encoding.GetDstPrefix();
	for (int j = 0; j < 8; j++) {
		address[j + 8] = 0;
		address[j] = prefix[j];
	}

	ipHeaderPtr->SetDestinationAddress(
			Ipv6Address::MakeAutoconfiguredAddress(
					Mac48Address::ConvertFrom(dst), Ipv6Address(address)));
	break;
case SixLowPanHc1::HC1_PCII:
	interface = encoding.GetDstInterface();
	address[0] = 0xfe;
	address[1] = 0x80;
	for (int j = 0; j < 8; j++) {
		address[j + 8] = interface[j];
	}
	ipHeaderPtr->SetDestinationAddress(Ipv6Address(address));
	break;
case SixLowPanHc1::HC1_PCIC:
	ipHeaderPtr->SetDestinationAddress(
			Ipv6Address::MakeAutoconfiguredLinkLocalAddress(
					Mac48Address::ConvertFrom(dst)));
	break;
	}
	//YIBO: End of DstCompression.

	if (!encoding.IsTcflCompression()) {
		ipHeaderPtr->SetFlowLabel(encoding.GetFlowLabel());
		ipHeaderPtr->SetTrafficClass(encoding.GetTrafficClass());
	} else {
		ipHeaderPtr->SetFlowLabel(0);
		ipHeaderPtr->SetTrafficClass(0);
	}

	ipHeaderPtr->SetNextHeader(encoding.GetNextHeader());
	ipHeaderPtr->SetPayloadLength(
			packetOrigBufferSize - packetOrigHeaderSize + 8);

//	NS_ASSERT_MSG(
//			encoding.IsHc2HeaderPresent() && (encoding.GetNextHeader() != Ipv6Header::IPV6_UDP),
//			"6LoWPAN: error in decompressing HC1 encoding, unsupported L4 compressed header present.");

	headers->StoreHeader(Ipv6Header::GetTypeId(), ipHeaderPtr);

	NS_LOG_DEBUG("---------------------------------------------------------------------------------" );
	if (encoding.GetNextHeader() == Ipv6Header::IPV6_UDP) {
		UdpHeader* udpHeaderPtr = new UdpHeader;
		udpHeaderPtr->SetSourcePort(encoding.GetUdpSrcPort());
		udpHeaderPtr->SetDestinationPort(encoding.GetUdpDstPort());
		udpHeaderPtr->SetPayLoadSize(
				packetOrigBufferSize - packetOrigHeaderSize);
		udpHeaderPtr->EnableChecksums();

		headers->StoreHeader(UdpHeader::GetTypeId(), udpHeaderPtr);
		NS_LOG_DEBUG( "YIBO:Decompressed Rebuilt packet: " << *ipHeaderPtr << " " << *udpHeaderPtr << " " << *packet << ", PL Size " << packet->GetSize () );
	} else {
		NS_LOG_DEBUG( "YIBO:Decompressed Rebuilt packet: " << *ipHeaderPtr << " " << *packet << ", PL Size " << packet->GetSize () );
	}NS_LOG_DEBUG( "---------------------------------------------------------------------------------" );
#ifdef YIBO
	std::cout << "<<<<<<<<---END of DecompressLowPanHc1--->>>>>>>>"
			<< std::endl;
#endif
}

void SixLowPanNetDevice::FinalizePacketPreFrag(Ptr<Packet> packet,
		Ptr<HeaderStorage> headers) {

	if (headers->IsEmpty()) {
		//YIBO:TODO: add the IPV6 dispatch header, haven't used yet.
		//YIBO:IPV6 DISPATCH Something cannot be compressed, use IPV6 DISPATCH,compress nothing, copy IPv6 header in packet
		Header* hdr;
#ifdef YIBO
		std::cout << "-YIBO: HeaderStorage is Empty!-" << std::endl;
#endif

//		uint8_t dispatchRawValFrag1 = 0;
//		SixLowPanDispatch::Dispatch_e dispatchValFrag1;
//		SixLowPanDispatch* ipv6dispatch = new SixLowPanDispatch(SixLowPanDispatch::LOWPAN_NOTCOMPRESSED);

		hdr = headers->GetHeader(SixLowPanDispatch::GetTypeId());
		if (hdr) {
			packet->AddHeader(*dynamic_cast<SixLowPanHc1 *>(hdr));
		}
//	    rime_hdr_len += SICSLOWPAN_IPV6_HDR_LEN;
//	    memcpy(rime_ptr + rime_hdr_len, UIP_IP_BUF, UIP_IPH_LEN);
//	    rime_hdr_len += UIP_IPH_LEN;
//	    uncomp_hdr_len += UIP_IPH_LEN;
		return;
	}
#ifdef YIBO
	std::cout << "<<--YIBO: FinalizaPacketPreFrag-->>" << std::endl;
#endif
	Header* hdr;
	hdr = headers->GetHeader(SixLowPanHc1::GetTypeId());
	if (hdr) {
		packet->AddHeader(*dynamic_cast<SixLowPanHc1 *>(hdr));

//		//----------------------------------//
//		uint8_t dispatchRawValFrag1 = 0;
//		SixLowPanDispatch::Dispatch_e dispatchValFrag1;
//		packet->CopyData(&dispatchRawValFrag1, sizeof(dispatchRawValFrag1));
//		dispatchValFrag1 = SixLowPanDispatch::GetDispatchType(
//				dispatchRawValFrag1);
//		std::cout << "<<FinalizePacketPreFragn>>, dispatchValFrag1 = "
//				<< dispatchValFrag1 << std::endl;
//		//----------------------------------//
	}
	return;
}

void SixLowPanNetDevice::FinalizePacketPostFrag(Ptr<Packet> packet,
		Ptr<HeaderStorage> headers) {
	// MESH and BC0
	return;
}

void SixLowPanNetDevice::FinalizePacketIp(Ptr<Packet> packet,
		Ptr<HeaderStorage> headers) {
	if (headers->IsEmpty()) {
		//YIBO: What should do here?
#ifdef YIBO
		std::cout << "-headers is empty!!-" << std::endl;
#endif
		return;
	}

	Header* hdr;

	hdr = headers->GetHeader(UdpHeader::GetTypeId());
	if (hdr) {
#ifdef YIBO
		std::cout << "-YIBO: FinalizaPacketIp-UdpHeader" << std::endl;
#endif
		packet->AddHeader(*dynamic_cast<UdpHeader *>(hdr));
	}

	hdr = headers->GetHeader(TcpHeader::GetTypeId());
	if (hdr) {
		packet->AddHeader(*dynamic_cast<UdpHeader *>(hdr));
	}

	hdr = headers->GetHeader(Icmpv6Header::GetTypeId());
	if (hdr) {
#ifdef YIBO
		std::cout << "-YIBO: FinalizaPacketIp-Icmpv6Header" << std::endl;
#endif
		packet->AddHeader(*dynamic_cast<Icmpv6Header *>(hdr));
	}

	hdr = headers->GetHeader(Ipv6Header::GetTypeId());
	if (hdr) {
#ifdef YIBO
		std::cout << "-YIBO: FinalizaPacketIp-Ipv6Header" << std::endl;
#endif
		packet->AddHeader(*dynamic_cast<Ipv6Header *>(hdr));
	}
	return;
}

void SixLowPanNetDevice::DoFragmentation(Ptr<Packet> packet,
		uint32_t origPacketSizexxx, uint32_t origHdrSize,
		Ptr<HeaderStorage> headersPre, Ptr<HeaderStorage> headersPost,
		std::list<Ptr<Packet> >& listFragments) {

	Ptr<Packet> p = packet->Copy();

	uint16_t offsetData = 0;
	uint16_t offset = 0;
//	uint16_t l2Mtu = GetMtu();
	uint16_t l2Mtu = 102;
	uint32_t packetSize = packet->GetSerializedSize();
	//YIBO:: Use the Size of buffer not Serialized Size. The size of IPv6 packet payload
	packetSize = packet->GetSize();
	//YIBO: The total size of packet we need to do the fragmentations.
	uint32_t origPacketSize = packetSize + origHdrSize;
	uint32_t cmpHdrSizePre = headersPre->GetHeaderSize();
	uint32_t cmpHdrSizePost = headersPost->GetHeaderSize();
#ifdef YIBO
	std::cout << "packetSize - " << packetSize << ", cmpHdrSizePre -"
			<< cmpHdrSizePre << ", cmpHdrSizePost -" << cmpHdrSizePost
			<< std::endl;
	std::cout << "original packet size = " << origPacketSize << std::endl;
//	std::cout << "original packet = " << *p << std::endl;
#endif
	UniformVariable cd(0, 65535);
	uint16_t tag;
	tag = cd.GetValue();
#ifdef YIBO
	std::cout << "random tag " << tag << std::endl;
#endif

	// First fragment
	SixLowPanFrag1* frag1Hdr = new SixLowPanFrag1;
	frag1Hdr->SetDatagramTag(tag);

	// uint32_t size = (l2Mtu - frag1Hdr.GetSerializedSize()) & 0x7;
	uint32_t size;
	NS_ASSERT_MSG(
			l2Mtu > frag1Hdr->GetSerializedSize() + cmpHdrSizePre + cmpHdrSizePost,
			"6LoWPAN: can not fragment, 6LoWPAN headers are bigger than MTU");

	size = (l2Mtu - frag1Hdr->GetSerializedSize() - cmpHdrSizePre
			- cmpHdrSizePost) & 0xf8;
#ifdef YIBO
	std::cout << "First Frag payload size = " << size << std::endl;
#endif

	frag1Hdr->SetDatagramSize(origPacketSize);

	NS_LOG_LOGIC ("Fragment 1Hdr creation - " << offset << ", " << p->GetSerializedSize() );
	Ptr<Packet> fragment1 = p->CreateFragment(offsetData, size);
	offset += size + origHdrSize;
	offsetData += size;

	FinalizePacketPreFrag(fragment1, headersPre);
	fragment1->AddHeader(*frag1Hdr);
	FinalizePacketPostFrag(fragment1, headersPost);
	listFragments.push_back(fragment1);

#ifdef YIBO
	std::cout << "Fragment1 = " << *fragment1 << std::endl;
#endif

	bool moreFrag = true;
	do {
		SixLowPanFragN* fragNHdr = new SixLowPanFragN;
		fragNHdr->SetDatagramTag(tag);
		fragNHdr->SetDatagramSize(origPacketSize);
		fragNHdr->SetDatagramOffset((offset) >> 3);
//		fragNHdr->SetDatagramOffset(offset);

		size = (l2Mtu - fragNHdr->GetSerializedSize() - cmpHdrSizePost) & 0xf8;

		if ((offsetData + size) > packetSize) {
			size = packetSize - offsetData;
			moreFrag = false;
		}

		NS_LOG_LOGIC ("Fragment NHdr creation - " << offset << ", " << size << ", buffer size-" << p->GetSize() );
		Ptr<Packet> fragmentN = p->CreateFragment(offsetData, size);
		NS_LOG_LOGIC ("Fragment NHdr created - " << offset << ", " << fragmentN->GetSize () );

		offset += size;
		offsetData += size;

		fragmentN->AddHeader(*fragNHdr);
		FinalizePacketPostFrag(fragmentN, headersPost);
		listFragments.push_back(fragmentN);

	} while (moreFrag);

	return;
}

bool SixLowPanNetDevice::ProcessFragment(Ptr<Packet>& packet,
		Address const &src, Address const &dst, bool isFirst, Ptr<HeaderStorage> ipHeaders) {
	NS_LOG_FUNCTION ( this << *packet );
//	Ptr<HeaderStorage> ipHeaders = Create<HeaderStorage>();
	SixLowPanFrag1 frag1Header;
	SixLowPanFragN fragNHeader;
	FragmentKey key;
	key.first = std::pair<Address, Address>(src, dst);

	Ptr<Packet> p = packet->Copy();
	uint16_t offset = 0;

#ifdef YIBO
	std::cout << "~~~~~~~~~~START of ProcessFragment~~~~~~~~~~" << std::endl;
#endif

	if (isFirst) {
		uint8_t dispatchRawValFrag1 = 0;
		SixLowPanDispatch::Dispatch_e dispatchValFrag1;

		p->CopyData(&dispatchRawValFrag1, sizeof(dispatchRawValFrag1));
		dispatchValFrag1 = SixLowPanDispatch::GetDispatchType(
				dispatchRawValFrag1);
#ifdef YIBO
		std::cout << "<<ProcessFragment>>, dispatchValFrag1 = "
				<< dispatchValFrag1 << std::endl;
#endif
		p->RemoveHeader(frag1Header);
		p->CopyData(&dispatchRawValFrag1, sizeof(dispatchRawValFrag1));
		dispatchValFrag1 = SixLowPanDispatch::GetDispatchType(
				dispatchRawValFrag1);
#ifdef YIBO
		std::cout << "<<ProcessFragment>>, dispatchValFrag1 = "
				<< dispatchValFrag1 << std::endl;
#endif
		switch (dispatchValFrag1) {
		case SixLowPanDispatch::LOWPAN_NOTCOMPRESSED:
			NS_LOG_DEBUG ( "Packet without compression:" << *p );
			NS_LOG_DEBUG ( "Packet length:" << p->GetSize () );
			break;
		case SixLowPanDispatch::LOWPAN_HC1:
#ifdef YIBO
			std::cout << "YIBO::Processed Pkt:" << *p << std::endl;
#endif
			NS_LOG_DEBUG ( "Packet length:" << p->GetSize () );
			DecompressLowPanHc1(p, src, dst, ipHeaders);
			break;
		case SixLowPanDispatch::LOWPAN_IPHC:
			NS_LOG_DEBUG ( "Unsupported");
			break;
		default:

			NS_FATAL_ERROR("Unsupported 6LoWPAN encoding, exiting.");
			break;
		}
		FinalizePacketIp(p, ipHeaders);
#ifdef YIBO
		std::cout << "Frag1Size = " << frag1Header.GetDatagramSize()
				<< ", Frag1Tag = " << frag1Header.GetDatagramTag() << std::endl;
#endif
		key.second = std::pair<uint16_t, uint16_t>(
				frag1Header.GetDatagramSize(), frag1Header.GetDatagramTag());
	} else {
		p->RemoveHeader(fragNHeader);
		offset = fragNHeader.GetDatagramOffset() << 3;
		offset += 48;
#ifdef YIBO
		std::cout << "fragNHeader's offset = " << offset << std::endl;
#endif
		key.second = std::pair<uint16_t, uint16_t>(
				fragNHeader.GetDatagramSize(), fragNHeader.GetDatagramTag());
	}

	Ptr<Fragments> fragments;

	MapFragments_t::iterator it = m_fragments.find(key);
	if (it == m_fragments.end()) {
		// erase the oldest packet.
		if (m_fragmentReassemblyListSize
				&& (m_fragments.size() >= m_fragmentReassemblyListSize)) {
#ifdef YIBO
			std::cout << "Going to erase the oldest fragment! " << std::endl;
#endif
			MapFragmentsTimers_t::iterator iter;
			MapFragmentsTimers_t::iterator iterFound =
					m_fragmentsTimers.begin();
			for (iter = m_fragmentsTimers.begin();
					iter != m_fragmentsTimers.end(); iter++) {
				if (iter->second.GetTs() < iterFound->second.GetTs()) {
					iterFound = iter;
				}
			}
			FragmentKey oldestKey = iterFound->first;
			m_fragmentsTimers[oldestKey].Cancel();
			m_fragmentsTimers.erase(oldestKey);
			m_fragments[oldestKey] = 0;
			m_fragments.erase(oldestKey);
			m_dropTrace(DROP_FRAGMENT_BUFFERFULL,
					m_node->GetObject<SixLowPanNetDevice>(), GetIfIndex());
		}
#ifdef YIBO
		std::cout << "---###it == m_fragments.end()###---" << std::endl;
#endif
		fragments = Create<Fragments>();
		fragments->SetPacketSize(frag1Header.GetDatagramSize() - 7 + 48, ipHeaders);
		m_fragments.insert(std::make_pair(key, fragments));
		uint32_t ifIndex = GetIfIndex();
		m_fragmentsTimers[key] = Simulator::Schedule(
				m_fragmentExpirationTimeout,
				&SixLowPanNetDevice::HandleFragmentsTimeout, this, key,
				ifIndex);

	} else {
		fragments = it->second;
	}

	fragments->AddFragment(p, offset);

	if (fragments->IsEntire()) {
		packet = fragments->GetPacket();
		Ipv6Header ipHeaderPtr;
		Ipv6Header* ipHeaderPtr_2 = new Ipv6Header;
		packet->RemoveHeader(ipHeaderPtr);
		ipHeaderPtr_2->SetSourceAddress(ipHeaderPtr.GetSourceAddress());
		ipHeaderPtr_2->SetDestinationAddress(ipHeaderPtr.GetDestinationAddress());
		ipHeaderPtr_2->SetHopLimit(ipHeaderPtr.GetHopLimit());
		ipHeaderPtr_2->SetPayloadLength(ipHeaderPtr.GetPayloadLength() + p->GetSize());
		ipHeaderPtr_2->SetFlowLabel(ipHeaderPtr.GetFlowLabel());
		ipHeaderPtr_2->SetNextHeader(ipHeaderPtr.GetNextHeader());
		ipHeaderPtr_2->SetTrafficClass(ipHeaderPtr.GetTrafficClass());

		ipHeaders->StoreHeader(Ipv6Header::GetTypeId(), ipHeaderPtr_2);
		fragments = 0;
		m_fragments.erase(key);
		if (m_fragmentsTimers[key].IsRunning()) {
			NS_LOG_LOGIC ("Stopping 6LoWPAN WaitFragmentsTimer at " << Simulator::Now ().GetSeconds () << " due to complete packet");
			m_fragmentsTimers[key].Cancel();
		}
		m_fragmentsTimers.erase(key);
#ifdef YIBO
		std::cout << "~~~~~~~~~~assembly packet =" << *packet
						<< std::endl;
		std::cout << "~~~~~~~~~~END of ProcessFragment, return true~~~~~~~~~~"
				<< std::endl;
#endif
		return true;

	} else {

#ifdef YIBO
		std::cout << "~~~~~~~~~~END of ProcessFragment, return false~~~~~~~~~~"
				<< std::endl;
#endif
		return false;
	}
}

SixLowPanNetDevice::HeaderStorage::HeaderStorage() {
	hdrSize = 0;
}

SixLowPanNetDevice::HeaderStorage::~HeaderStorage() {
	for (HdrMap::iterator it = hdrMap.begin(); it != hdrMap.end(); it++) {
		if (it->second != 0) {
			delete it->second;
		}
	}
}

Header*
SixLowPanNetDevice::HeaderStorage::GetHeader(TypeId headerType) {
	HdrMap::iterator it;
	it = hdrMap.find(headerType);

	if (it == hdrMap.end()) {
		return 0;
	}

	return it->second;
}

void SixLowPanNetDevice::HeaderStorage::StoreHeader(TypeId headerType,
		Header* header) {
	hdrMap[headerType] = header;
	hdrSize += header->GetSerializedSize();
}

uint32_t SixLowPanNetDevice::HeaderStorage::GetHeaderSize() {
	return hdrSize;
}

bool SixLowPanNetDevice::HeaderStorage::IsEmpty() {
	return hdrMap.empty();
}

SixLowPanNetDevice::Fragments::Fragments() :
		lastAccess(Simulator::Now()) {
}

SixLowPanNetDevice::Fragments::~Fragments() {
}

void SixLowPanNetDevice::Fragments::AddFragment(Ptr<Packet> fragment,
		uint16_t fragmentOffset) {
	NS_LOG_DEBUG ("Fragment = " << *fragment << ", fragment Offset = " << fragmentOffset);

	std::list<std::pair<Ptr<Packet>, uint16_t> >::iterator it;

	for (it = m_fragments.begin(); it != m_fragments.end(); it++) {
		if (it->second > fragmentOffset) {
			NS_LOG_DEBUG ("it->second = " << it->second);
			break;
		}
	}

	lastAccess = Simulator::Now();
	m_fragments.insert(it,
			std::make_pair<Ptr<Packet>, uint16_t>(fragment, fragmentOffset));
}

bool SixLowPanNetDevice::Fragments::IsEntire() const {
	bool ret = m_fragments.size() > 0;
	uint16_t lastEndOffset = 0;

	if (ret) {
#ifdef YIBO
		std::cout << "$$$$$$$$ ret==real of Fragment is entire !! $$$$$$$$"
				<< std::endl;
#endif
//		uint16_t lastEndOffset = 0;

		for (std::list<std::pair<Ptr<Packet>, uint16_t> >::const_iterator it =
				m_fragments.begin(); it != m_fragments.end(); it++) {
			// overlapping fragments do exist
			NS_LOG_DEBUG ("Checking overlaps " << lastEndOffset << " - " << it->second );

			if (lastEndOffset < it->second) {
				ret = false;
#ifdef YIBO
				std::cout
						<< "$$$$$$$$ Set ret = false of Fragment is entire !! $$$$$$$$"
						<< std::endl;
#endif
				break;
			}
			// fragments might overlap in strange ways
			uint16_t fragmentEnd = it->first->GetSize() + it->second;
#ifdef YIBO
			std::cout << "it->first->GetSize() = " << it->first->GetSize()
					<< ", it->second = " << it->second << std::endl;
#endif
			lastEndOffset = std::max(lastEndOffset, fragmentEnd);
#ifdef YIBO
			std::cout << "fragmentEnd = " << fragmentEnd << ", lastEndOffset = "
					<< lastEndOffset << std::endl;
#endif
		}
	}

#ifdef YIBO
	std::cout << "$$$$$$$$m_packetSize = " << m_packetSize << ", ret = " << ret << std::endl;
#endif

	if (ret && (lastEndOffset == m_packetSize)) {
#ifdef YIBO
		std::cout << "$$$$$$$$ Return True of Fragment is entire !! $$$$$$$$"
				<< std::endl;
#endif
		return true;
	}
#ifdef YIBO
	std::cout << "$$$$$$$$ Return False of Fragment is entire !! $$$$$$$$"
			<< std::endl;
#endif
	return false;
}

Ptr<Packet> SixLowPanNetDevice::Fragments::GetPacket() const {
	NS_LOG_FUNCTION (this);

	std::list<std::pair<Ptr<Packet>, uint16_t> >::const_iterator it =
			m_fragments.begin();

	Ptr<Packet> p = Create<Packet>();
	uint16_t lastEndOffset = 0;

	for (it = m_fragments.begin(); it != m_fragments.end(); it++) {
		if (lastEndOffset > it->second) {
			// The fragments are overlapping.
			// We do not overwrite the "old" with the "new" because we do not know when each arrived.
			// This is different from what Linux does.
			// It is not possible to emulate a fragmentation attack.
			uint32_t newStart = lastEndOffset - it->second;
			if (it->first->GetSize() > newStart) {
				uint32_t newSize = it->first->GetSize() - newStart;
				Ptr<Packet> tempFragment = it->first->CreateFragment(newStart,
						newSize);
				p->AddAtEnd(tempFragment);
			}
		} else {
			NS_LOG_LOGIC ("Adding: " << *(it->first) );
			p->AddAtEnd(it->first);
		}
		lastEndOffset = p->GetSize();
	}

	return p;
}

void SixLowPanNetDevice::Fragments::SetPacketSize(uint32_t packetSize, Ptr<HeaderStorage> ipHeaders) {
	m_packetSize = packetSize;
	m_ipHeaders = ipHeaders;

}

void SixLowPanNetDevice::HandleFragmentsTimeout(FragmentKey key, uint32_t iif) {
	NS_LOG_FUNCTION (this);

	MapFragments_t::iterator it = m_fragments.find(key);

	m_dropTrace(DROP_FRAGMENT_TIMEOUT, m_node->GetObject<SixLowPanNetDevice>(),
			iif);

	// clear the buffers
	it->second = 0;

	m_fragments.erase(key);
	m_fragmentsTimers.erase(key);
}

}
}

// namespace ns3
