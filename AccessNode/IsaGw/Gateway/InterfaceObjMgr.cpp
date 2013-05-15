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
/// @file InterfaceObjMgr.cpp
/// @author Marcel Ionescu
/// @brief The interface object manager - implementation
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"
#include "InterfaceObjMgr.h"
#include "GwUAP.h"
////////////////////////////////////////////////////////////////////////////////
/// @class CInterfaceObjMgr
/// @brief Interface object manager. Allocate object id's for services,
/// @brief dispatch field messages (APDU) & timeouts to proper service
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Allocate a new Object ID (OID) and associate it with p_pService
/// @param p_pService Pointer to service
/// @param p_ushPreferredOID Requested OID. If specified, the method does not allocate a new OID, but use requested
/// @return the object ID allocated to p_pService
/// @remarks is not currently used. Keep it for a potential future use.
////////////////////////////////////////////////////////////////////////////////
uint16 CInterfaceObjMgr::AllocateOID( CService * p_pService, uint16 p_ushPreferredOID /*= 0*/ )
{
	switch( m_ushNexAvailOID )	///< List of exceptions: well-known or standard-defined OID's
	{	case SO_OID:
		case PROMISCUOUS_TUNNEL_LOCAL_OID:
		case ARO_OID:
			++m_ushNexAvailOID;
	}

	uint16 ushOID = p_ushPreferredOID ? p_ushPreferredOID : m_ushNexAvailOID;
	/// TODO m_mapServices.find(p_ushPreferredOID) MUST return m_mapServices.end(). No duplicates allowed
	m_mapServices[ ushOID ] = p_pService;
	LOG("OID %d allocated to [%s]%s", ushOID, p_pService->Name(), p_ushPreferredOID ? "(well-known)" :"" );

	/// Do not allocate oid if the user has requested it's own
	/// Potential problems if the user has requested an existing one, or whatever user is requesting is not in the exception list.
	///  The user should always request from the exception list if it request oid explicitly.
	if( p_ushPreferredOID )	
		return p_ushPreferredOID;
	
	return m_ushNexAvailOID++;	///< MUST be postincrement
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Dispatch an ADPU: search the OID and dispatch to respective CService*
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pAPDUIdtf - needed for device TX time with usec resolution
/// @param p_pRsp - the response
/// @param p_pReq - the original request
/// @retval true if the APDU was successfully dispatched
/// @retval false the APDU cannot be dispatched (invalid fields, caller non existent, etc)
/// @remarks CALL whenever an APDU arrives from the field (from ISA100 system) (calls CService*::ProcessAPDU)
/// @todo: define parameters
////////////////////////////////////////////////////////////////////////////////
bool CInterfaceObjMgr::DispatchAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{	//m_stReadRsp/m_stWriteRsp/m_stExecRsp have relevant members at the same offset
	TMap::const_iterator it = m_mapServices.find( p_pRsp->m_stSRVC.m_stReadRsp.m_unDstOID );
	if( it == m_mapServices.end() )
	{
		LOG( "ERROR CInterfaceObjMgr::DispatchAPDU: can't find OID %u", p_pRsp->m_stSRVC.m_stReadRsp.m_unDstOID );
		return false;
	}
	return it->second->ProcessAPDU( p_unAppHandle, p_pAPDUIdtf, p_pRsp, p_pReq );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Dispatch a timeout: search the OID and dispatch to respective CService*
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pOriginalReq - the original request
/// @retval true if the timeout was successfully dispatched
/// @retval false the timeout cannot be dispatched (invalid fields, caller non existent, etc)
/// @remarks CALL whenever a timeout is received for a request generated from this class  (calls CService*::ProcessISATimeout)
/// @todo: define parameters
////////////////////////////////////////////////////////////////////////////////
bool CInterfaceObjMgr::DispatchISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq )
{
	TMap::const_iterator it = m_mapServices.find( p_pOriginalReq->m_stSRVC.m_stReadReq.m_unSrcOID );
	if( it == m_mapServices.end() )
	{
		LOG("ERROR CInterfaceObjMgr::DispatchISATimeout(H %u): can't find Source OID %u", p_unAppHandle, p_pOriginalReq->m_stSRVC.m_stReadReq.m_unSrcOID );
		return false;
	}
	LOG("DispatchISATimeout(H %u)", p_unAppHandle);
	it->second->ProcessISATimeout( p_unAppHandle, p_pOriginalReq );
	return true;	/// Processed ok
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Dispatch a user request: search the service type dispatch to respective CService*
/// @param p_pHdr		request header, as defined by isa100gw-ADD.doc (has member m_nDataSize)
/// @param p_pData		request data (data size is in p_pHdr)
/// @retval true the request was successfully dispatched
/// @retval false the request cannot be dispatched. The caller will send CONFIRM with G_STATUS_UNIMPLEMENTED
/// @remarks (test CService::CanHandleServiceType(pHdr->m_ucServiceType), calls CService*::ProcessUserRequest)
/// @remarks CALL on user (client) requests
////////////////////////////////////////////////////////////////////////////////
bool CInterfaceObjMgr::DispatchUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	for( TMap::const_iterator it = m_mapServices.begin(); it != m_mapServices.end(); ++it )
	{	//LOG("DEBUG CInterfaceObjMgr::DispatchUserRequest(%s) try %s", getGSAPServiceName(p_pHdr->m_ucServiceType), it->second->Name() );
		if( (*it).second->CanHandleServiceType( p_pHdr->m_ucServiceType ))
		{	/// DO NOT pass the retcode from worker
			it->second->ProcessUserRequest( p_pHdr, p_pData );
			return true;
		}
	}
	LOG("WARNING CInterfaceObjMgr::DispatchUserRequest: can't handle service type %d", p_pHdr->m_ucServiceType );
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dispatch a lease deletion (explicit delete or expire) to all m_mapServices::CService* to free resources
/// @param m_nSessionID		request header, as defined by isa100gw-ADD.doc (has member m_nDataSize)
/// @param m_nLeaseID		request data (data size is in p_pHdr)
/// @remarks Identified so far: ClientServer service, BulkTransfer Service
////////////////////////////////////////////////////////////////////////////////
void CInterfaceObjMgr::DispatchLeaseDelete( lease* p_pLease )
{
	for( TMap::const_iterator it = m_mapServices.begin(); it != m_mapServices.end(); ++it )
	{
		it->second->OnLeaseDelete( p_pLease );
	}
}

/// Called on USR2: Dump status to LOG
void CInterfaceObjMgr::Dump( void )
{
	LOG("CInterfaceObjMgr: %d objects, next ID: %u", m_mapServices.size(), m_ushNexAvailOID);
	for( TMap::const_iterator it = m_mapServices.begin(); it != m_mapServices.end(); ++it )
	{
		//LOG("OID %d: %s", it->first, it->second->Name() );
		it->second->Dump( );
	}
}


