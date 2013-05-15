/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef BbrTypes_h__
#define BbrTypes_h__



/// The lower limit for the UDP Port of 6lowPAN
#define LOWPAN_UDP_BASE 61616

#define NetDispatch_NALP		0
#define NetDispatch_IPv6		2
#define NetDispatch_IPHC		3
#define NetDispatch_FragFirst	6
#define NetDispatch_FragNext	7


struct NetMsgTrToBbrHeader
{
	uint8_t		m_u8TrafficClass; //priority,DE,ECN
	uint16_t	m_u16SrcAddr ;
	uint16_t	m_u16DstAddr ;
}__attribute__((__packed__));

struct NetMsgBbrToTrHeader
{
	uint16_t	m_u16ContractID;
	uint8_t		m_u8TrafficClass; //priority,DE,ECN
	uint16_t	m_u16SrcAddr ;
	uint16_t	m_u16DstAddr ;
}__attribute__((__packed__));


//////////////////////////////////////////////////////////////////////////////
/// @struct FragmHdr
/// @ingroup Backbone
/// @brief The 6lowPAN Fragmentation Header.
//////////////////////////////////////////////////////////////////////////////
struct FragmHdr {
	uint8_t  dgramSz[2] ;
	uint8_t  dgramTag[2] ;
} __attribute__((__packed__));



struct TProvisioningInfo                                                                                                                                         
{      
	uint16_t m_unFormatVersion;                   // 0x0080   
	uint8_t m_aBbrEUI64[8];
	//uint16 m_unSubnetID;                        // 0x0082                                                                                         
	uint16_t m_unFilterBitMask;                   // 0x0084                                                                                         
	uint16_t m_unFilterTargetID;                  // 0x0086                                                                                         
	uint8_t  m_aAppJoinKey[16];                   // 0x0088                                                                                         
	//uint8  m_aDllJoinKey[16];                   // 0x0098                                                                                         
	//uint8  m_aProvisionKey[16];                 // 0x00A8                                                                                         
	uint8_t  m_aSecMngrEUI64[8];                  // 0x00B8                                                                                         
	uint8_t  m_aSysMngrIPv6[16];                  // 0x00C0                                                                                         

	uint8_t  m_aIpv6BBR[16];                      // 0x00D0
	uint8_t  m_aBbrTag[16];     
	//uint8_t	 m_u8CCA_Limit;			
} __attribute__((__packed__));





#endif // BbrTypes_h__


