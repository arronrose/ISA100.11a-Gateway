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
/// @file SessionMgmtSvc.cpp
/// @author Marcel Ionescu
/// @brief Session Management service - implementation
////////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <string.h>
#include <stdio.h>
#include "SystemReportSvc.h"
#include "GwApp.h"

CSessionMgmtService::CSessionMgmtService():m_nSessionIndTop(1)
{
	memset(SessionArray,0,sizeof(SessionArray));
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Session mgmt: create/modify/delete a session
/// @param p_nSessionID	[in/out] GS_Session_ID. If 0, new session is requested, id returned in this parameter
///							If !=0, session delete/modify is requested
/// @param p_nPeriod	[in/out] GS_Session_Period. Period request/response. If 0, session delete is requested.
/// @param p_shNetworkId	GS_Network_ID
/// @return GS_Status operation status
/// @see SessionMgr.h: SessionStatus
/// @remarks
///	- Request a new session: call with p_nSessionID 0
///	- Request a session modify: call with existing p_nSessionID!=0 and a new p_nPeriod > 0
///	TAKE CARE: A limited duration session cannot be changed to an unlimited duration using p_nPeriod -1
///	- Request a session delete: call with existing p_nSessionID!=0 and a new p_nPeriod == 0
////////////////////////////////////////////////////////////////////////////////
int CSessionMgmtService::RequestSession(uint32&  p_nSessionID, long& p_nPeriod, short p_shNetworkId, uint8_t p_ucVersion)
{
	unsigned nReqSessionID = p_nSessionID;
	int nStatus = IsYGSAP(p_ucVersion) ? YGS_FAILURE : SESSION_FAIL_OTHER;
	if (!p_nSessionID)
	{
		nStatus = createSession( p_nSessionID, p_nPeriod, p_shNetworkId, p_ucVersion );
	}
	else
	{
		int nSessionInd=findSession( p_nSessionID );
		if (nSessionInd < 0)
		{	/// PROGRAMMER ERROR: SESSION should have been verified prior to this call
			nStatus = IsYGSAP(p_ucVersion) ? YGS_FAILURE : SESSION_NOT_EXIST;
		}
		else if (!p_nPeriod)
		{
			nStatus = deleteSession( nSessionInd );
		}
		else if (p_nPeriod > 0)
		{
			nStatus = renewSession( nSessionInd, p_nPeriod, p_ucVersion );
		}
	}
	/// Since we do not change the period, do not report request/obtain
	LOG("G_Session_Request (%u->%u) period %d net %d => status %d", nReqSessionID,
		p_nSessionID, p_nPeriod, p_shNetworkId, nStatus);
	return nStatus;
}

void CSessionMgmtService::CheckExpireSessions()
{
	time_t nCurrentTime=time(NULL);
	for( unsigned nSessionInd=0; nSessionInd<MAX_SESSIONS; ++nSessionInd )
	{
		if (SessionArray[nSessionInd].nExpirationTime && (SessionArray[nSessionInd].nExpirationTime < nCurrentTime) )
		{
			LOG("CheckExpireSessions: session %u expired", SessionArray[nSessionInd].nSession_ID );
			deleteSession(nSessionInd);	
		}
	}	
}

/// Session expire 0 is a valid value, time_t may? be unsigned so we use 1 to signal error
/// @see  EXPIRE_TIME_INFINITE / EXPIRE_TIME_ERROR for p_tExpireTime
bool CSessionMgmtService::GetSessionExpTime(unsigned p_nSessionID, time_t& p_tExpireTime)
{
	int nSessionIdx = findSession(p_nSessionID);

	if( p_nSessionID && nSessionIdx >= 0 )
	{
		p_tExpireTime = SessionArray[nSessionIdx].nExpirationTime;
		return true;
	}
	p_tExpireTime = 1;
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Delete expired sessions, THEN test if session id is valid: exist and is not expired.
/// @param p_nSessionID the session ID to verify
/// @remarks TAKE CARE: deleting expired sessions before search is essential to the program functionality
/// @see CGwUAP::IsValidSession()
////////////////////////////////////////////////////////////////////////////////
bool CSessionMgmtService::IsValidSession(unsigned p_nSessionID)
{	/// Get time before checkExpireSessions to avoid second change just after the call which may generate inconsistencies
	time_t nNow = time(NULL);
	/// a good place to check session expiration
	/// TODO: make only one list pass to both find/check all sessions
	//checkExpireSessions();
	int nSessionIdx = findSession(p_nSessionID);
	if( !p_nSessionID || nSessionIdx<0 )
	{
		return false;
	}
	/// This will always return true: call checkExpireSessions() above deletes expired sessions
	return ( !SessionArray[nSessionIdx].nExpirationTime )			/// If not infinite
		|| ( SessionArray[nSessionIdx].nExpirationTime > nNow );	/// Test expiration
} 

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief check if session ID correspond to a YGSAP session
/// @param p_nSessionID the session ID to verify
/// @remarks TAKE CARE: deleting expired sessions before search is essential to the program functionality
/// @remarks make the assumption that all requests will have the same m_ucVersion
/// @todo the sentence above should be enforced
////////////////////////////////////////////////////////////////////////////////
bool CSessionMgmtService::IsYGSAPSession( unsigned p_nSessionID )
{
	int nSessionIdx = findSession(p_nSessionID);
	if( !p_nSessionID || nSessionIdx<0 )
	{
		return false;
	}
	return IsYGSAP( SessionArray[nSessionIdx].m_ucVersion );
}
////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Session mgmt: create a session
/// @param p_nRetSessionId	[out] GS_Session_ID. The new session id is returned here
/// @param p_nPeriod	[in/out] GS_Session_Period. Period request/response.
///		p_nPeriod -1: indefinite session is requested.
///		p_nPeriod 0 is invalid
///		p_nPeriod >0 specified the session duration in seconds
/// @param p_shNetworkId	GS_Network_ID
/// @return GS_Status operation status
/// @see SessionMgr.h: SessionStatus
////////////////////////////////////////////////////////////////////////////////
int CSessionMgmtService::createSession( uint32& p_nRetSessionId, long& p_nPeriod, short p_shNetworkId, uint8_t p_ucVersion )
{
	if( !p_nPeriod )
		return IsYGSAP(p_ucVersion) ? YGS_FAILURE: SESSION_FAIL_OTHER;

	int nSessionInd;
	
	for ( nSessionInd=0; nSessionInd<MAX_SESSIONS; ++nSessionInd)
	{		
		
		if (!(SessionArray[nSessionInd].nSession_ID))
		{
			break;
		}
	}
	if( nSessionInd==MAX_SESSIONS )
		return IsYGSAP(p_ucVersion) ? YGS_LIMIT_EXCEEDED: SESSION_NOT_AVAILABLE;

	p_nRetSessionId = SessionArray[nSessionInd].nSession_ID = GET_NEW_SESSION_ID;
	SessionArray[nSessionInd].nNetworkId		= p_shNetworkId;
	SessionArray[nSessionInd].nCreationTime		= time(NULL);
	SessionArray[nSessionInd].nExpirationTime	= (p_nPeriod<0) ? 0 : SessionArray[nSessionInd].nCreationTime + p_nPeriod;
	SessionArray[nSessionInd].m_ucVersion		= p_ucVersion;
	return SESSION_SUCCESS;/// same as YGS_SUCCESS
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Session mgmt: update a session
/// @param p_nSessionIdx	[in] Session index in internal structure
/// @param p_nPeriod	[in/out] GS_Session_Period. Period request/response, must be >0
/// @param p_shNetworkId	GS_Network_ID
/// @return GS_Status operation status
/// @see SessionMgr.h: SessionStatus
/// @remarks
////////////////////////////////////////////////////////////////////////////////
int CSessionMgmtService::renewSession(unsigned p_nSessionIdx, long& p_nPeriod, uint8_t p_ucVersion )
{
	LOG("CSessionMgmtService::renewSession(idx %u %d)", p_nSessionIdx, p_nPeriod);
	if( p_nPeriod <=0 )
		return IsYGSAP(p_ucVersion) ? YGS_FAILURE : SESSION_FAIL_OTHER;

	time_t nNewExpTime=time(NULL) + p_nPeriod;// new expiration time for the session
	if (SessionArray[p_nSessionIdx].nExpirationTime > 0)
	{
		SessionArray[p_nSessionIdx].nExpirationTime = nNewExpTime;
		g_pLeaseMgr->ProcessSessionUpdate( SessionArray[p_nSessionIdx].nSession_ID, nNewExpTime );
		return SESSION_SUCCESS;	/// same as YGS_SUCCESS
	}	
	return IsYGSAP(p_ucVersion) ? YGS_FAILURE: SESSION_FAIL_OTHER;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Session mgmt: delete a session
/// @param p_nSessionIdx	[in] Session index in internal structure
/// @return GS_Status operation status
/// @see SessionMgr.h: SessionStatus
/// @remarks
////////////////////////////////////////////////////////////////////////////////
int CSessionMgmtService::deleteSession( unsigned p_nSessionIdx )
{
	LOG("CSessionMgmtService::deleteSession(idx %u, ID %u)", p_nSessionIdx, SessionArray[p_nSessionIdx].nSession_ID);
	/// Notify CSystemReportService, CLeaseMgmtService and CMsgTracker to ProcessSessionDelete()
	g_stApp.m_oGwUAP.DispatchSessionDelete( SessionArray[p_nSessionIdx].nSession_ID );
	/// Delete the session
	memset(&SessionArray[p_nSessionIdx], 0, sizeof(SessionArray[p_nSessionIdx]));
	return SESSION_SUCCESS;	/// same as YGS_SUCCESS
}

void CSessionMgmtService::Dump(void)
{
	LOG("Session Management Service session list");
	for(int i=0;i<MAX_SESSIONS;i++)
	{
		if (!SessionArray[i].nSession_ID)
		{
			continue;
		}
		char szExpirationTime[128]={0};
		if( SessionArray[i].nExpirationTime == 0 )
			strcpy( szExpirationTime,"never");
		else
			sprintf( szExpirationTime, "%d(%+d)", (int)SessionArray[i].nExpirationTime, (int)(SessionArray[i].nExpirationTime - time(NULL)));
		
		LOG( " ID %u Created %+d Exp %s NetId %u Proto %sGSAP", SessionArray[i].nSession_ID,
			SessionArray[i].nCreationTime - time(NULL), szExpirationTime, SessionArray[i].nNetworkId, IsYGSAP(SessionArray[i].m_ucVersion) ? "Y" : "");
	}
}

int CSessionMgmtService::findSession(unsigned p_nSessionID)
{
	int nSessionIdx;
	for( nSessionIdx=0; nSessionIdx < MAX_SESSIONS; ++nSessionIdx )
	{
		if (SessionArray[nSessionIdx].nSession_ID == p_nSessionID)
		{
			return nSessionIdx;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief 	Delete the session specified by p_nSessionID
/// @retval true Session deleted
/// @retval false Session ID not found
////////////////////////////////////////////////////////////////////////////////
bool CSessionMgmtService::DeleteSession( unsigned p_nSessionID  )
{
	int nSessionIdx = findSession( p_nSessionID );
	if( p_nSessionID && nSessionIdx >=0 )
	{
		deleteSession( nSessionIdx );
		return true;
	}
	
	return false;
}


