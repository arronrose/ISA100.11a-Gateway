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
/// @file SessionMgmtSvc.h
/// @author Marcel Ionescu
/// @brief Session Management service - interface
////////////////////////////////////////////////////////////////////////////////

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#define MAX_SESSIONS 16

#include <time.h>
class CSessionMgmtService; 
#include "LeaseMgmtSvc.h"

//SessionStatus
#define SESSION_SUCCESS				0
#define SESSION_SUCCESS_REDUCED		1
#define SESSION_NOT_EXIST			2
#define SESSION_NOT_AVAILABLE		3
#define SESSION_FAIL_OTHER			4

#define EXPIRE_TIME_INFINITE 0
#define EXPIRE_TIME_ERROR 	 1

typedef struct{
	unsigned nSession_ID;
	time_t nExpirationTime;	///< value 0 means indefinite, contrary to GS_Session_Period where -1 means indefinite
	time_t nCreationTime;
	int nNetworkId;
	uint8_t	m_ucVersion;	/// needed to know protocol type, for example for YGSAP
}session_data;

////////////////////////////////////////////////////////////////////////////////
/// @class CSessionMgmtService
/// @brief Session Manager Service 
/// @remarks DO NOT inherit CService:
///	- there is no ISA->GW flow,
///	- there is no possibility to have multiple session managers
///	- and we have no means to send the response back
///		ProcessUserRequest return bool, not suitable to hold GS_Status, we
///		cannot call dirrectly from CGSAP
///		There is no be session ID in case of errors - we cannot link back to
///		caller CGSAP - we cannot send error messages back
////////////////////////////////////////////////////////////////////////////////

class CSessionMgmtService	
{
public:
	CSessionMgmtService();

	/// Session mgmt: create/modify/delete a session. return the operation status
	int RequestSession( uint32& p_nSessionID, long& p_nPeriod, short p_shNetworkId, uint8_t p_ucVersion );

	void Dump( void );
	
	bool IsValidSession( unsigned p_nSessionID );
	bool IsYGSAPSession( unsigned p_nSessionID );

	///p_tExpireTime has meaning only if function returns true
	/// retval false means session not found
	/// @see  EXPIRE_TIME_INFINITE / EXPIRE_TIME_ERROR for p_tExpireTime
	bool GetSessionExpTime( unsigned p_nSessionID, time_t& p_tExpireTime);

	/// Delete the session specified by p_nSessionID
	bool DeleteSession( unsigned p_nSessionID  );

	void CheckExpireSessions();
private:
	
	
	/// Create a session
	int createSession(	uint32& p_nRetSessionId, long& p_nPeriod, short p_shNetworkId, uint8_t p_ucVersion );

	/// Update a session (update period)
	int renewSession(	unsigned p_nSessionIdx, long& p_nPeriod, uint8_t p_ucVersion );

	/// Delete a session specified by it's index, not ID
	int deleteSession(	unsigned p_nSessionIdx );

	/// Find a session, return the index
	int findSession(	unsigned p_nSessionID );
	
	int m_nSessionIndTop;
	session_data SessionArray[ MAX_SESSIONS ];
};

#define GET_NEW_SESSION_ID GET_NEW_ID(m_nSessionIndTop)

#endif
