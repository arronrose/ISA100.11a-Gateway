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
/// @file              ServerService
/// @author 	Claudiu Hobeanu
/// @brief 		Declaration of CServerService class
////////////////////////////////////////////////////////////////////////////////

#ifndef ServerService_h__
#define ServerService_h__

#include "Service.h"

////////////////////////////////////////////////////////////////////////////////
/// @class 		CServerService
/// @author 	Claudiu Hobeanu
/// @brief 		handle GServer 
/// @remarks	
////////////////////////////////////////////////////////////////////////////////
class CServerService :	public CService
{
public:
	CServerService(const char * p_szName ) :CService(p_szName){};
	virtual ~CServerService(void);

public:
	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;

	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );

	/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
	virtual bool ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq);
	
	bool ProcessTunnelAPDU( APDU_IDTF* pAPDUIdtf, GENERIC_ASL_SRVC* p_pReq);

private:

};

#endif // ServerService_h__
