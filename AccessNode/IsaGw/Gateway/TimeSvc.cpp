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

////////////////////////////////////////////////////////////////////////////////
/// @file TimeSvc.cpp
/// @author Marcel Ionescu
/// @brief Time service - implementation
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"

#include "GwApp.h"
#include "TimeSvc.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CtimeService
/// @brief Time services
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Check if this worker can handle service type received as parameter
/// @param p_ucServiceType - service type tested
/// @retval true - this service can handle p_ucServiceType
/// @retval false - the service cannot handle p_ucServiceType
/// @remarks The method is called from CInterfaceObjMgr::DispatchUserRequest
/// to decide where to dispatch a user request
////////////////////////////////////////////////////////////////////////////////
bool CTimeService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case TIME_SERVICE:
			return true;
	}
	return false;
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval TBD
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
////////////////////////////////////////////////////////////////////////////////
bool CTimeService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	if( ( p_pHdr->m_ucServiceType != TIME_SERVICE ) || (p_pHdr->m_nDataSize != TIME_REQ_DATA_SIZE))
	{
		LOG("ERROR CTimeService::ProcessUserRequest: service type %u or data size %u incorrect ", p_pHdr->m_ucServiceType, p_pHdr->m_nDataSize );
		return confirm( p_pHdr, p_pHdr->IsYGSAP() ? YGS_FAILURE : TIME_FAIL_OTHER );
	}
			
	if( *(uint8_t *)p_pData != 0 )// first byte: operation: 0: read time, 1: write time
	{
		LOG("ERROR CTimeService::ProcessUserRequest: operation %u not allowed. We only allow read", *(uint8_t *)p_pData );
		return confirm( p_pHdr, p_pHdr->IsYGSAP() ? YGS_NOT_ALLOWED : TIME_NOT_ALLOWED );
	}
			
	struct timeval now;
	MLSM_GetCrtTaiTime( &now );
	SAPStruct::TAI stTAI = { now.tv_sec, now.tv_usec };
	return confirm( p_pHdr, YGS_SUCCESS, &stTAI );
}

bool CTimeService::confirm( CGSAP::TGSAPHdr * p_pHdr, uint8_t p_ucStatus, const SAPStruct::TAI* p_pTAI /*= NULL*/ )
{	uint8_t tmp[ 1 + sizeof(SAPStruct::TAI) ] = { p_ucStatus };	/// the content is irrelevant in case of error
	if(p_pTAI)	memcpy( tmp+1, p_pTAI, sizeof(SAPStruct::TAI) );	// grr, data copy, should be avoided
	g_stApp.m_oGwUAP.SendConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, TIME_SERVICE, tmp, sizeof(tmp), false );
	return YGS_SUCCESS == p_ucStatus;
}
