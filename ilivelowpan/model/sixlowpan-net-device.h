#ifndef SIXLOWPAN_NET_DEVICE_H
#define SIXLOWPAN_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/internet-module.h"
#include "ns3/nstime.h"
#include "sixlowpan-header.h"
#include <stdint.h>
#include <string>
#include <map>

namespace ns3 {
namespace sixlowpan {

//class ns3::Node;

class SixLowPanNetDevice: public NetDevice {
public:
	// same as IPv4 Drop Reasons, some does not apply
	enum DropReason {
		DROP_TTL_EXPIRED = 1, /**< Packet TTL has expired */
		DROP_NO_ROUTE, /**< No route to host */
		DROP_BAD_CHECKSUM, /**< Bad checksum */
		DROP_INTERFACE_DOWN, /**< Interface is down so can not send packet */
		DROP_ROUTE_ERROR, /**< Route error */
		DROP_FRAGMENT_TIMEOUT, /**< Fragment timeout exceeded */
		DROP_FRAGMENT_BUFFERFULL /**< Fragment buffer size exceeded */
	};

	static TypeId GetTypeId(void);
	SixLowPanNetDevice();
	virtual ~SixLowPanNetDevice();

	// inherited from NetDevice base class

	virtual void SetIfIndex(const uint32_t index);
	virtual uint32_t GetIfIndex(void) const;
	virtual Ptr<Channel> GetChannel(void) const;
	virtual void SetAddress(Address address);
	virtual Address GetAddress(void) const;
	virtual bool SetMtu(const uint16_t mtu);
	virtual uint16_t GetMtu(void) const;
	virtual bool IsLinkUp(void) const;
	virtual void AddLinkChangeCallback(Callback<void> callback);
	virtual bool IsBroadcast(void) const;
	virtual Address GetBroadcast(void) const;
	virtual bool IsMulticast(void) const;
	virtual Address GetMulticast(Ipv4Address multicastGroup) const;
	virtual bool IsPointToPoint(void) const;
	virtual bool IsBridge(void) const;
	virtual bool Send(Ptr<Packet> packet, const Address& dest,
			uint16_t protocolNumber);
	virtual bool SendFrom(Ptr<Packet> packet, const Address& source,
			const Address& dest, uint16_t protocolNumber);
	virtual Ptr<Node> GetNode(void) const;
	virtual void SetNode(Ptr<Node> node);
	//WTF, ARP for IPv6?? Not exist in the NetDevice base class anymore.
	virtual bool NeedsArp(void) const;
	//WTF, ARP for IPv6?? Not exist in the NetDevice base class anymore.
	virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);
	virtual void SetPromiscReceiveCallback(
			NetDevice::PromiscReceiveCallback cb);
	virtual bool SupportsSendFrom() const;
	virtual Address GetMulticast(Ipv6Address addr) const;

	Ptr<NetDevice> GetPort() const;
	void SetPort(Ptr<NetDevice> port);

protected:
	virtual void DoDispose(void);

	void ReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet> packet,
			uint16_t protocol, Address const &source,
			Address const &destination, PacketType packetType);

private:
	NetDevice::ReceiveCallback m_rxCallback;
	NetDevice::PromiscReceiveCallback m_promiscRxCallback;

	// The following two traces pass a packet with a 6LoWPAN header
	TracedCallback<Ptr<const Packet>, Ptr<SixLowPanNetDevice>, uint32_t> m_txTrace;
	TracedCallback<Ptr<const Packet>, Ptr<SixLowPanNetDevice>, uint32_t> m_rxTrace;
	// <ip-header, payload, reason, ifindex> (ifindex not valid if reason is DROP_NO_ROUTE)
	TracedCallback<DropReason, Ptr<SixLowPanNetDevice>, uint32_t> m_dropTrace;

	typedef std::map<TypeId, Header*> HdrMap;

	class HeaderStorage: public SimpleRefCount<HeaderStorage> {
	public:
		HeaderStorage();
		~HeaderStorage();

		Header* GetHeader(TypeId headerType);
		void StoreHeader(TypeId headerType, Header* header);
		uint32_t GetHeaderSize(void);
		bool IsEmpty();

	private:
		uint32_t hdrSize;
		HdrMap hdrMap;
	};

	uint32_t CompressLowPanHc1(Ptr<Packet> packet, Address const &src,
			Address const &dst, Ptr<HeaderStorage> headersPre);
	void DecompressLowPanHc1(Ptr<Packet> packet, Address const &src,
			Address const &dst, Ptr<HeaderStorage> headers);

	void FinalizePacketPreFrag(Ptr<Packet> packet, Ptr<HeaderStorage> headers);
	void FinalizePacketPostFrag(Ptr<Packet> packet, Ptr<HeaderStorage> headers);
	void FinalizePacketIp(Ptr<Packet> packet, Ptr<HeaderStorage> headers);

	typedef std::pair<std::pair<Address, Address>, std::pair<uint16_t, uint16_t> > FragmentKey;

	/**
	 * \class Fragments
	 * \brief A Set of Fragment
	 */
	class Fragments: public SimpleRefCount<Fragments> {
	public:
		/**
		 * \brief Constructor.
		 */
		Fragments();

		/**
		 * \brief Destructor.
		 */
		~Fragments();

		/**
		 * \brief Add a fragment.
		 * \param fragment the fragment
		 * \param fragmentOffset the offset of the fragment
		 */
		void AddFragment(Ptr<Packet> fragment, uint16_t fragmentOffset);

		/**
		 * \brief If all fragments have been added.
		 * \returns true if the packet is entire
		 */
		bool IsEntire() const;

		/**
		 * \brief Get the entire packet.
		 * \return the entire packet
		 */
		Ptr<Packet> GetPacket() const;

		/**
		 * \brief Set the reconstructed packet size.
		 */
		void SetPacketSize(uint32_t packetSize);

	private:

		/**
		 * \brief The size of the reconstructed packet.
		 */
		uint32_t m_packetSize;

		/**
		 * \brief The current fragments.
		 */
		std::list<std::pair<Ptr<Packet>, uint16_t> > m_fragments;

		Time lastAccess;
	};

	/**
	 * \brief Return the instance type identifier, a value of void ???.
	 * \param packet the packet to be fragmented (with headers already compressed with 6LoWPAN)
	 * \param origPacketSize the size of the IP packet before the 6LoWPAN header compression, including the IP/L4 headers
	 * \param listFragments a reference to the list of the resulting packets, all with the proper headers in place
	 */
	void DoFragmentation(Ptr<Packet> packet, uint32_t origPacketSize,
			uint32_t origHdrSize, Ptr<HeaderStorage> headersPre,
			Ptr<HeaderStorage> headersPost,
			std::list<Ptr<Packet> >& listFragments);

	/**
	 * \brief Process a packet fragment
	 * \param packet the packet
	 * \param fragmentSize the size of the fragment
	 * \param iif Input Interface
	 * \return true is the fragment completed the packet
	 */
	bool ProcessFragment(Ptr<Packet>& packet, Address const &src,
			Address const &dst, bool isFirst);

	/**
	 * \brief Process the timeout for packet fragments
	 * \param key representing the packet fragments
	 * \param iif Input Interface
	 */
	void HandleFragmentsTimeout(FragmentKey key, uint32_t iif);

	/**
	 * \brief Drops the oldest fragment set
	 */
	void DropOldestFragmentSet();

	typedef std::map<FragmentKey, Ptr<Fragments> > MapFragments_t;
	typedef std::map<FragmentKey, EventId> MapFragmentsTimers_t;

	MapFragments_t m_fragments;
	MapFragmentsTimers_t m_fragmentsTimers;
	Time m_fragmentExpirationTimeout;
	uint16_t m_fragmentReassemblyListSize;

	Ptr<Node> m_node;
	Ptr<NetDevice> m_port;
	uint32_t m_ifIndex;
};

} // namespace sixlowpan
} // namespace ns3

#endif /* SIXLOWPAN_NET_DEVICE_H */

