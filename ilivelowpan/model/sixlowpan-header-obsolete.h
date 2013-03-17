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

namespace ns3 {
namespace sixlowpan {


/**
* \ingroup sixlowpan
* \brief   LOWPAN_MESH header
*
*  The MESH header defines ways to support the hop-by-hop typical of WSN.
  \verbatim
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |1 0|V|F|HopsLft| originator address, final address
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class SixLowPanMesh : public SixLowPanDispatch
{
public:

  SixLowPanMesh ();
  SixLowPanMesh (uint8_t meshDispatch);

  static TypeId GetTypeId (void);

  /**
   * \brief Return the instance type identifier.
   * \return instance type ID
   */
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  /**
   * \brief Get the serialized size of the packet.
   * \return size
   */
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Serialize the packet.
   * \param start Buffer iterator
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * \brief Deserialize the packet.
   * \param start Buffer iterator
   * \return size of the packet
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * \brief Get the Dispatch type.
   * \return the Dispatch type
   */
  virtual Dispatch_e GetDispatchType (void) const;

  /**
   * \brief Set the Origin Address.
   * \param originAddress the Origin Address
   */
  void SetOriginShortAddress (uint16_t originAddress);
  /**
   * \brief Set the Origin Address.
   * \param originAddress the Origin Address
   */
  void SetOriginLongAddress (uint64_t originAddress);
  /**
   * \brief Set the Destination Address.
   * \param destinationAddress the Destination Address
   */
  void SetDestinationShortAddress (uint16_t destinationAddress);
  /**
   * \brief Set the destinationAddress Address.
   * \param destinationAddress the Destination Address
   */
  void SetDestinationLongAddress (uint64_t destinationAddress);

  /**
   * \brief Get the Origin Short Address.
   * \return the Origin Short Address (or zero id not set)
   */
  uint16_t GetOriginShortAddress () const;
  /**
   * \brief Get the Origin Long Address.
   * \return the Origin Long Address (or zero id not set)
   */
  uint64_t GetOriginLongAddress () const;
  /**
   * \brief Get the Destination Short Address.
   * \return the Destination Short Address (or zero id not set)
   */
  uint16_t GetDestinationShortAddress () const;
  /**
   * \brief Get the destinationAddress Address.
   * \return the Destination Long Address (or zero id not set)
   */
  uint64_t GetDestinationLongAddress () const;

  /**
   * \brief Set the Hops Left number.
   * \param hopsLeft the number of Hops Left for the packet
   */
  void SetHopsLeft(uint8_t hopsLeft);
  /**
   * \brief Get the Hops Left number.
   * \return the number of Hops Left for the packet
   */
  uint8_t GetHopsLeft () const;

private:

  uint8_t m_serializedSize;
  uint8_t m_meshDispatch;
  uint16_t m_originShortAddr;
  uint64_t m_originLongAddr;
  uint16_t m_destShortAddr;
  uint64_t m_destLongAddr;
  uint64_t m_hopsLeft;

};

std::ostream & operator<< (std::ostream & os, SixLowPanMesh const & h);

/**
* \ingroup sixlowpan
* \brief   LOWPAN_BC0 header
*
*  A broadcast header consists of a LOWPAN_BC0 dispatch followed by a
*  sequence number field.  The sequence number is used to detect
*  duplicate packets (and hopefully suppress them).
  \verbatim
                       1
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |0|1|LOWPAN_BC0 |Sequence Number|
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class SixLowPanBc0 : public SixLowPanDispatch
{
public:

  SixLowPanBc0 (void);

  static TypeId GetTypeId (void);

  /**
   * \brief Return the instance type identifier.
   * \return instance type ID
   */
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  /**
   * \brief Get the Dispatch type.
   * \return the Dispatch type
   */
  virtual Dispatch_e GetDispatchType (void) const;

  /**
   * \brief Get the serialized size of the packet.
   * \return size
   */
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Serialize the packet.
   * \param start Buffer iterator
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * \brief Deserialize the packet.
   * \param start Buffer iterator
   * \return size of the packet
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * \brief Set the Sequence Number.
   * \param sequenceNumber the Sequence Number
   */
  void SetSequenceNumber (uint8_t sequenceNumber);

  /**
   * \brief Get the Sequence Number.
   * \return the Sequence Number.
   */
  uint8_t GetSequenceNumber (void) const;

private:
  uint8_t m_sequenceNumber;

};

std::ostream & operator<< (std::ostream & os, SixLowPanBc0 const & h);



}
}

#endif /* SIXLOWPANHEADER_H_ */
