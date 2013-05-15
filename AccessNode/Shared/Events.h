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

/***************************************************************************
                          Events.h  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/



#ifndef _EVENTS_H
#define _EVENTS_H
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "EventsStruct.h"
#include "Locks.h"

//#define LOCK_TYPE_FCNTL

#ifdef LOCK_TYPE_FCNTL
	#define LOCK_EV( p_nFd, p_nLock, i )\
			{\
		    CEventsStruct ev ={0}\
			FCNTL( m_nFd, F_RDLCK, i*sizeof(CEventsStruct), sizeof(CEventsStruct) );\
			};
#else
	#define LOCK_EV( p_nFd, p_nLock ) flock(p_nFd,p_nLock);
#endif	


#define WITH_LOCK 		1
#define WITHOUT_LOCK    0

extern u_char	g_vTimeUnitsSize[];

class CEvents
{
public:
	int ForceSend( int p_nRulePos );
	CEvents();
	~CEvents();
	
	bool BindToFile( const char* p_pFileName = EVENTS_FILE );
	void Release();
	
	int GetRulesCount();
	
	int IsSignaling( int i, CEventsStruct& p_clEv );
	int GetTimeUntilNextSched();
	int MatchEvents( event_code p_nEvCode, rf_address p_nNodeId );	
	
	int WriteEvent( int i, CEventsStruct& p_clEv );
	int ReadEvent( int i, CEventsStruct& p_clEv );

	int UpdateResponseTForEvent( int p_nMsgId, time_t p_nTime = -1 );

	int UpdateRules( CEventsStruct& p_clEv );
	int UpdateRule( unsigned short p_usRuleId , unsigned char* p_pSchedTime );
	int UpdateRules( const rf_address& p_rDest, int p_nActivate, int p_nChanged, const char* p_pVer );
	
	void Lock( int p_nLockType = LOCK_EX ) { if (m_nFd > 0) FLOCK(m_nFd, p_nLockType); }
	void Unlock() { FLOCK( m_nFd, LOCK_UN ); }

	static int		GetMsgId( u_short p_nRuleId, int p_nRulePos ) { return p_nRulePos <<16 | p_nRuleId; }
	static u_short	GetRuleId( int p_nMsgId ) { return p_nMsgId & 0x0000ffff; }
	static int		GetRulePos( int p_nMsgId ) { return p_nMsgId >> 16; }

protected:
	/***/
	int addSamplingRule( CEventsStruct& p_rEv, const char* p_pVer);
   	int updateRule( CEventsStruct& p_clEv,  CEventsStruct& p_clCrtRule );
	int writeAtOffset( const int p_nOff, const void* p_pBuff, const int p_nLen );

private:
	int 			m_nFd;
	int 			m_nNoEvents;
};

#endif
