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

#ifndef GwUtil_h__
#define GwUtil_h__

#include <stdint.h>
#include <arpa/inet.h>

#include "../../Shared/Common.h"
#include "../ISA100/typedef.h"

/// The gateway AL identity
//#define GW_DMAP 0
#define ISA100_GW_UAP		2
#define TUNNEL_EXTRA_UAP	3

inline bool IsUAPLocalValid( int p_nUapId)
{
	switch(p_nUapId)
	{
	case 0: // in GSAP requests UapId==0 means do not care about what UAP
	
	case ISA100_GW_UAP:
	case TUNNEL_EXTRA_UAP:
		return true;	    
	default:
	    return false;
	}
	return false;
}

// move the method inside the only caller: ClientServer.cpp
#if 0
template <class SizeType>
uint8_t* UnpackNetworkVariableData ( uint8_t* p_pData, int p_nDataLen, SizeType* p_pnOutLen, const char* p_szErrorTag = "" )
{
	SizeType dataSize; 

	if ( (int)sizeof(dataSize) > p_nDataLen )
	{
		LOG("UnpackNetworkVariableData: ERROR: %s: can not extract size",p_szErrorTag);
		return NULL;
	}

	memcpy (&dataSize, p_pData, sizeof(dataSize));

	switch(sizeof(SizeType))
	{
	case 2:
		dataSize = (SizeType)ntohs((uint16_t)dataSize);
		break;
	case 4:
		dataSize = (SizeType)ntohl((uint32_t)dataSize);
		break;
	default:
		break;
	}

	p_pData += sizeof (SizeType);
	p_nDataLen -= sizeof (SizeType);

	if (dataSize >  p_nDataLen)
	{
		LOG("UnpackNetworkVariableData: ERROR: %s: pack size %d > % buffer size", p_szErrorTag, dataSize, p_nDataLen);
		return NULL;
	}

	*p_pnOutLen = dataSize;
	//p_poBuffer->Set(0, p_pData, dataSize);

	return p_pData + dataSize;
}
#endif
const char* GetAlertCategoryName (unsigned int p_nCategory);

/// GSAP service name, without direction (REQ/IND/CNF/RSP)
const char * getGSAPServiceNameNoDir( uint8 p_ucServiceType );

/// GSAP service name, including direction (REQ/IND/CNF/RSP)
const char * getGSAPServiceName( uint8 p_ucServiceType );

/// GSAP status name
const char * getGSAPStatusName( uint8_t p_ucProtoVersion, uint8_t p_ucServiceType, uint8_t p_ucStatus );

/// ISA100 service name (ASL_SERVICE_TYPE)
const char * getISA100SrvcTypeName( uint8 p_ucServiceType );

/// ISA100 device capabilities
const char * getCapabilitiesText( uint16 p_ushDeviceType );
		
/// device join status 
const char * getJoinStatusText( byte p_unJoinStatus );

/// JOIN Fail ALERT reason
const char* getJoinFailAlertReasonText (uint8_t p_u8Reason);

/// JOIN Leave ALERT reason
const char* getJoinLeaveAlertReasonText (uint8_t p_u8Reason);

/// UDO Progress End ALERT error code
const char* getUDOTransferEndAlertReasonText (uint8_t p_u8ErrCode );

#endif // GwUtil_h__
