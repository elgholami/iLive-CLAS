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
 * SixLowPanMesh
 */

// TODO: MESH is still to be completed

NS_OBJECT_ENSURE_REGISTERED(SixLowPanMesh);

SixLowPanMesh::SixLowPanMesh()
{
  m_meshDispatch = 0x80;
}

SixLowPanMesh::SixLowPanMesh(uint8_t meshDispatch)
{
  m_meshDispatch = meshDispatch;
}

TypeId SixLowPanMesh::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::sixlowpan::SixLowPanMesh").SetParent<Header>().AddConstructor<SixLowPanMesh>();
  return tid;
}

TypeId SixLowPanMesh::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

void SixLowPanMesh::Print(std::ostream & os) const
{
  // os << "Sequence Number " << m_sequenceNumber;
}

uint32_t SixLowPanMesh::GetSerializedSize() const
{
  return 0;
}

void SixLowPanMesh::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  if( m_meshDispatch & 0x20 )
    {
      i.WriteU64(m_originLongAddr);
    }
  else
    {
      i.WriteU16(m_originShortAddr);
    }

  if( m_meshDispatch & 0x10 )
    {
      i.WriteU64(m_destLongAddr);
    }
  else
    {
      i.WriteU16(m_destShortAddr);
    }

  if( (m_meshDispatch & 0xF) == 0xF )
    {
      i.WriteU8(m_hopsLeft);
    }
}

uint32_t SixLowPanMesh::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_serializedSize = 0;

  if( m_meshDispatch & 0x20 )
    {
      m_originLongAddr = i.ReadU64();
      m_serializedSize += 8;
    }
  else
    {
      m_originShortAddr = i.ReadU16();
      m_serializedSize += 2;
    }

  if( m_meshDispatch & 0x10 )
    {
      m_destLongAddr = i.ReadU64();
      m_serializedSize += 8;
    }
  else
    {
      m_destShortAddr = i.ReadU16();
      m_serializedSize += 2;
    }
  if( (m_meshDispatch & 0xF) == 0xF)
    {
      m_hopsLeft = i.ReadU8();
      m_serializedSize ++;
    }
  return GetSerializedSize();
}

SixLowPanDispatch::Dispatch_e
SixLowPanMesh::GetDispatchType (void) const
{
  return LOWPAN_MESH;
}

void SixLowPanMesh::SetOriginShortAddress (uint16_t originAddress)
{
  m_originShortAddr = originAddress;
  m_meshDispatch |= 0x20;
}

void SixLowPanMesh::SetOriginLongAddress (uint64_t originAddress)
{
  m_originLongAddr = originAddress;
  m_meshDispatch &= 0x20;
}

void SixLowPanMesh::SetDestinationShortAddress (uint16_t destinationAddress)
{
  m_destShortAddr = destinationAddress;
  m_meshDispatch |= 0x10;
}

void SixLowPanMesh::SetDestinationLongAddress (uint64_t destinationAddress)
{
  m_destLongAddr = destinationAddress;
  m_meshDispatch &= 0x10;
}

uint16_t SixLowPanMesh::GetOriginShortAddress () const
{
  return m_originShortAddr;
}

uint64_t SixLowPanMesh::GetOriginLongAddress () const
{
  return m_originLongAddr;
}

uint16_t SixLowPanMesh::GetDestinationShortAddress () const
{
  return m_destShortAddr;
}

uint64_t SixLowPanMesh::GetDestinationLongAddress () const
{
  return m_destLongAddr;
}

void SixLowPanMesh::SetHopsLeft(uint8_t hopsLeft)
{
  m_hopsLeft = hopsLeft;
  m_meshDispatch &= 0xF;
  if( m_hopsLeft >= 0xF )
    {
      m_meshDispatch |= 0xF;
    }
  else
    {
      m_meshDispatch |= hopsLeft;
    }
}

uint8_t SixLowPanMesh::GetHopsLeft () const
{
  return m_hopsLeft;
}


std::ostream & operator << (std::ostream & os, const SixLowPanMesh & h)
{
  h.Print(os);
  return os;
}


/*
 * SixLowPanBc0
 */

// TODO: BC0 is still to be completed

NS_OBJECT_ENSURE_REGISTERED(SixLowPanBc0);

 SixLowPanBc0::SixLowPanBc0()
{
}

TypeId SixLowPanBc0::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::sixlowpan::SixLowPanBc0").SetParent<Header>().AddConstructor<SixLowPanBc0>();
  return tid;
}

TypeId SixLowPanBc0::GetInstanceTypeId(void) const
{
  return GetTypeId();
}

SixLowPanDispatch::Dispatch_e
SixLowPanBc0::GetDispatchType (void) const
{
  return LOWPAN_BC0;
}

void SixLowPanBc0::SetSequenceNumber(uint8_t sequenceNumber)
{
  m_sequenceNumber = sequenceNumber;
}

uint8_t SixLowPanBc0::GetSequenceNumber() const
{
  return m_sequenceNumber;
}

void SixLowPanBc0::Print(std::ostream & os) const
{
  os << "Sequence Number " << m_sequenceNumber;
}

uint32_t SixLowPanBc0::GetSerializedSize() const
{
  return 2;
}

void SixLowPanBc0::Serialize(Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8(LOWPAN_BC0);
  i.WriteU8(m_sequenceNumber);
}

uint32_t SixLowPanBc0::Deserialize(Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  i.Next();
  m_sequenceNumber = i.ReadU8();
  return GetSerializedSize();
}

std::ostream & operator << (std::ostream & os, const SixLowPanBc0 & h)
{
  h.Print(os);
  return os;
}


}
}
