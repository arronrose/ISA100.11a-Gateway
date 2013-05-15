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
/// @file GwMgmtSvc.h
/// @author Marcel Ionescu
/// @brief Gateway Management services - interface
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"

#include "GwApp.h"
#include "GwMgmtSvc.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CGatewayMgmtService
/// @brief Gateway Management Service
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
bool CGatewayMgmtService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case GATEWAY_CONFIGURATION_READ:
		case GATEWAY_CONFIGURATION_WRITE:
		case DEVICE_CONFIGURATION_READ:
		case DEVICE_CONFIGURATION_WRITE:
			
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
bool CGatewayMgmtService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * /*p_pData*/ )
{
	switch( p_pHdr->m_ucServiceType )
	{ 	case GATEWAY_CONFIGURATION_READ:
		case GATEWAY_CONFIGURATION_WRITE:
		case DEVICE_CONFIGURATION_READ:
		case DEVICE_CONFIGURATION_WRITE:
		
		default:	/// PROGRAMMER ERROR: We should never get to default
			LOG("ERROR CGatewayMgmtService::ProcessUserRequest: unknown/unimplemented service type %u", p_pHdr->m_ucServiceType );
			return false;/// Caller will SendConfirm with G_STATUS_UNIMPLEMENTED
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Process an ADPU 
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pAPDUIdtf - needed for device TX time with usec resolution
/// @param p_pRsp The response from field, unpacked
/// @param p_pReq The original request
/// @retval true - field APDU dispatched ok
/// @retval false - field APDU not dispatched - default handler does not do processing, just log a message
/// @remarks 
////////////////////////////////////////////////////////////////////////////////
bool CGatewayMgmtService::ProcessAPDU( uint16 /*p_unAppHandle*/, APDU_IDTF* /*p_pAPDUIdtf*/, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* /*p_pReq*/ )
{
	//TODO: IMPLEMENT
	LOG("CGatewayMgmtService::ProcessAPDU: invalid ADPU type %X", p_pRsp->m_ucType ); return false;
}




