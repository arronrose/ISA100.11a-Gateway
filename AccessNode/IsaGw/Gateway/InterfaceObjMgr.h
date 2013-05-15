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
/// @file InterfaceObjMgr.h
/// @author Marcel Ionescu
/// @brief The interface object manager - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef INTERFACE_OBJ_MGR_H
#define INTERFACE_OBJ_MGR_H

#include <map>

#include "Service.h"

/// The Dispersion OID (Subscribe Object). Must be skipped by CInterfaceObjMgr ID allocator
#define SO_OID									 5
#define PROMISCUOUS_TUNNEL_LOCAL_OID			11
#define ARO_OID									12

#define PROMISCUOUS_TUNNEL_LOCAL_TSAP_ID		ISA100_GW_UAP

////////////////////////////////////////////////////////////////////////////////
/// @class CInterfaceObjMgr
/// @brief Interface object manager. Allocate object id's for services, dispatch messages in botth directions
/// @brief Dispatch ISA100 field messages (APDU/timeouts) to worker based on OID (ISA -> GW)
/// @brief Dispatch USER messages (client requests) to worker based on service type (User -> GW)
/// @remarks Dispatch based on OID allow unlimited worker instances, even identhical instances (same service type)
/// @remarks Dispatch based on service type allow only one worker per service type
/// @remarks If we ever want more than one worker instance for a service type, 
/// @remarks dispatch (USER->GW) must be based on additional fields from
/// @remarks TGSAPHdr (CTcpGSAP::m_stReqHeader) or data (CTcpGSAP::m_aReqData)
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CInterfaceObjMgr{

public:
	CInterfaceObjMgr( void  ):m_ushNexAvailOID(1) {};
	~CInterfaceObjMgr( void ){};
	
	/// Allocate a new Object ID (OID) (unledd preferred is specified) and associate it with p_pService
	uint16 AllocateOID( CService * p_pService, uint16 p_ushPreferredOID = 0 );

	/// (ISA -> GW) Dispatch an ADPU: search the OID and dispatch to respective CService*
	/// CALL whenever an APDU arrives from the field (from ISA100 system) (calls CService*::ProcessAPDU)
	bool DispatchAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	
	/// (ISA -> GW) Dispatch a timeout: search the OID and dispatch to respective CService*
	/// CALL whenever a timeout is received from the field (from ISA100 system) (calls CService*::ProcessISATimeout)
	bool DispatchISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq );

	/// (USER -> GW) Dispatch a user request: search the service type dispatch to respective CService*
	/// (test CService::CanHandleServiceType(pHdr->m_ucServiceType), calls CService*::ProcessUserRequest)
	/// CALL on user requests
	bool DispatchUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );

	/// Dispatch a lease deletion (explicit delete or expire) to all m_mapServices::CService* to free resources
	void DispatchLeaseDelete( lease* p_pLease );

	/// Called on USR2: Dump status to LOG
	void Dump( void );

private:
	/// Map Key: the Interface Object Identifier
	/// Map Data: pointer to CService
	typedef std::map<uint16, CService *> TMap;

	uint16	m_ushNexAvailOID;
	TMap	m_mapServices;
};

#endif //INTERFACE_OBJ_MGR_H
