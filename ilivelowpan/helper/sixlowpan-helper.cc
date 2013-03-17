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

#include "sixlowpan-helper.h"
#include "ns3/log.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/node.h"
#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("SixLowPanHelper");

namespace ns3 {

SixLowPanHelper::SixLowPanHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.SetTypeId ("ns3::sixlowpan::SixLowPanNetDevice");
}

void SixLowPanHelper::SetDeviceAttribute (std::string n1,
                                          const AttributeValue &v1)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.Set (n1,
                       v1);
}

NetDeviceContainer SixLowPanHelper::Install (const NetDeviceContainer c)
{
  NS_LOG_FUNCTION_NOARGS ();

  NetDeviceContainer devs;

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);

      Ptr<Node> node = device->GetNode ();
      NS_LOG_LOGIC ("**** Install 6LoWPAN on node " << node->GetId ());

      Ptr<sixlowpan::SixLowPanNetDevice> dev = m_deviceFactory.Create<sixlowpan::SixLowPanNetDevice> ();
      devs.Add (dev);
      node->AddDevice (dev);
      dev->SetPort (device);
    }
  return devs;
}

} // namespace ns3
