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

#include "BbrGlobal.h"

#include "../ISA100/typedef.h"
#include "../ISA100/porting.h" 
#include "../ISA100/slme.h"
#include "../ISA100/nlme.h"
#include "../ISA100/Ccm.h"

#include "BackBoneApp.h"
#include "BbrTypes.h"



bool isSMipv6( const uint8_t* p_pIPv6  )
{
	return ( ! memcmp( p_pIPv6, g_pApp->m_cfg.m_oSMIPv6.m_pu8RawAddress, IPv6_ADDR_LENGTH ) );	
}


bool isTRipv6( const uint8_t* p_pIPv6 )
{
	return ( ! memcmp( p_pIPv6,  g_pApp->m_cfg.m_oBBIPv6.m_pu8RawAddress, IPv6_ADDR_LENGTH ) );	
}

int getShortForIPv6 (const uint8_t* p_pu8Address, const uint8_t* p_pu8Default /*= NULL*/) 
{
	NLME_ADDR_TRANS_ATTRIBUTES * pAtt = NLME_FindATT (p_pu8Address);
	uint16_t u16ShortAddr = 0;

	if (pAtt)
	{
		memcpy(&u16ShortAddr, pAtt->m_aShortAddress, 2);
		return u16ShortAddr;
	}

	if (p_pu8Default && memcmp(p_pu8Address,p_pu8Default, IPv6_ADDR_LENGTH) == 0)
	{	return 0;
	}

	WARN("getShortForIPv6: Unknown IPv6 Source address. %s", GetHex(p_pu8Address,IPv6_ADDR_LENGTH)) ;		
	return -1;
}


//for address 0 -> SM IPv6 on dest and BBR on SRC
const uint8_t* getIpv6ForShort (uint16_t p_pu16Short, const uint8_t* p_pu8DefaultIPv6)
{
	NLME_ADDR_TRANS_ATTRIBUTES * pAtt =  NLME_FindATTByShortAddr ( (uint8_t*)&p_pu16Short );

	if (pAtt)
	{
		return pAtt->m_aIPV6Addr;
	}
	if (p_pu16Short == 0)
	{
		return p_pu8DefaultIPv6;
	}

	WARN("getIpv6ForShort: no ATT entry for %04X.", ntohs(p_pu16Short) );		
	return NULL ;			
}

bool Get6LowPanAddrs (IPv6Packet* p_pIPv6Packet, int p_nLen,  uint16_t& p_u16SrcAddr, uint16_t& p_u16DstAddr)
{
	if (p_nLen < (int)offsetof(IPv6Packet,m_stIPPayload.m_aUDPPayload) + 1 ) //the payload should have at least 1b
	{
		WARN("Get6LowPanAddrs: IPv6 size too low %db", p_nLen); //just in case should not happen
		return false;
	}
	
	if (memcmp(p_pIPv6Packet->m_stIPHeader.m_ucDstAddr, g_pApp->m_cfg.getBB_IPv6(), IPv6_ADDR_LENGTH) == 0)
	{	
		LOG("Get6LowPanAddrs: message for BBR");
		p_u16SrcAddr = 0;	
		p_u16DstAddr = 0;			
		return true;
	}
	int nAddr;

	nAddr = getShortForIPv6(p_pIPv6Packet->m_stIPHeader.m_ucDstAddr);

	bool bRet = true;
	if (nAddr < 0)
	{
		//send request to SM;
		bRet = false;
	}

	p_u16DstAddr = nAddr;

	nAddr = getShortForIPv6(p_pIPv6Packet->m_stIPHeader.m_ucSrcAddr);

	if (nAddr < 0)
	{
		//send request to SM;
		bRet = false;
	}
	p_u16SrcAddr = nAddr;

	return bRet;
}

// return true - the IPv6 are known
bool GetIPv6Addrs (ProtocolPacket* p_pPkt, const uint8_t*& p_pSrcIpv6,  const uint8_t*& p_pDstIpv6 )
{	
	if( p_pPkt->DataLen() < sizeof(NetMsgTrToBbrHeader) + 1 ) //at least dispatch octet
	{
		WARN("LowPANTunnel.lowPanToIPv6:Invalid Packet Size.");		
		return true; // discard later
	}

	NetMsgTrToBbrHeader* pNetMsgTrToBbr = (NetMsgTrToBbrHeader*)p_pPkt->Data();



	if (pNetMsgTrToBbr->m_u16DstAddr == 0 && pNetMsgTrToBbr->m_u16SrcAddr == 0)
	{	
		return true;
	}

	p_pDstIpv6 = getIpv6ForShort(pNetMsgTrToBbr->m_u16DstAddr, g_pApp->m_cfg.getSM_IPv6());
	if (!p_pDstIpv6)
	{	//[TODO] send request
		return false;
	}

	p_pSrcIpv6 = getIpv6ForShort(pNetMsgTrToBbr->m_u16SrcAddr, g_pApp->m_cfg.getBB_IPv6());
	if (!p_pSrcIpv6)
	{	//[TODO] send request
		return false;
	}
	
	return true;
}

