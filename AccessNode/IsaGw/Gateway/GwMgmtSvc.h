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
#ifndef GW_MGMT_SERVICE_H
#define GW_MGMT_SERVICE_H

#include "../ISA100/porting.h"
#include "Service.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CGatewayMgmtService
/// @brief Gateway Management Service
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CGatewayMgmtService: public CService{
public:
	CGatewayMgmtService( const char * p_szName ) :CService(p_szName){};
	
	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;
	
	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	
	/// (ISA -> GW) Process an ADPU 
	virtual bool ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
};

#endif //GW_MGMT_SERVICE_H
