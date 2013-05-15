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

#ifndef Bbr_global_h__
#define Bbr_global_h__

#include <stdint.h>
#include <stdlib.h>

#include "../ISA100/porting.h"

bool isSMipv6 (const uint8_t* p_pIPv6);

bool isTRipv6 (const uint8_t* p_pIPv6);

int getShortForIPv6 (const uint8_t* p_pu8Address, const uint8_t* p_pu8Default = NULL) ;

//for address 0 -> SM IPv6 on dest and BBR on SRC
const uint8_t* getIpv6ForShort (uint16_t p_pu16Short, const uint8_t* p_pu8DefaultIPv6);

bool Get6LowPanAddrs (IPv6Packet* p_pIPv6Packet, int p_nLen,  uint16_t& p_u16SrcAddr, uint16_t& p_u16DstAddr);

class ProtocolPacket;
bool GetIPv6Addrs (ProtocolPacket* p_pPkt, const uint8_t*& p_pSrcIpv6,  const uint8_t*& p_pDstIpv6 );

#endif // global_h__

