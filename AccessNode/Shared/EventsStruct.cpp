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
                          EventsStruct.cpp  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/

#include <math.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#include "EventsStruct.h"

//not exact values
u_char	g_vTimeUnitsSize[] = { 127, 12, 30, 24, 60, 60 };

CEventsStruct::CEventsStruct()
{
    memset( m_pMessageData, 0, sizeof(m_pMessageData) );
}


u_short	CEventsStruct::GetRuleId()
{ 
	return *(u_short*) m_pEventId;
}


void CEventsStruct::SetRuleId(u_short p_usRuleId)
{
	*(u_short*) m_pEventId = p_usRuleId;
}


//////////////////////////////////////////////////////////////////////////////////
// Description : check if the event has to be send in this moment
// Parameters  :
//              int& p_nNeedWrite		-- tell if the event has to be write back to file
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int CEventsStruct::IsSignaling( int& p_nNeedWrite )
{
	p_nNeedWrite = 0;
	time_t nCurrentTime = time( NULL );    //get current time

    if( m_cActiveStatus == INACTIVE )
    {   return 0;
    }

	if ( m_nActiveStartTime > nCurrentTime || m_nActiveEndTime < nCurrentTime ) 
	{	return 0;
	}

	if( m_nAccessTime == 0 )
	{  //force send
		p_nNeedWrite = 1;
		PrepareSend();	
		return 1;
	}

    if( nCurrentTime < m_nAccessTime )
    {   //if this happen means that time of the system changed to a time in past
        m_nAccessTime = nCurrentTime;
        p_nNeedWrite = 1;
    }
	
	if( m_nResponseTime == 0 )
	{	
		if( nCurrentTime - m_nAccessTime < m_nNoSec )	
			return 0;			//we have to wait longer before resending the event
			
		if( m_nMaxNoRetry > 0 && m_nNoRetry >= m_nMaxNoRetry )	
		{	//ok that's enough we won't send this event again
			m_nResponseTime = nCurrentTime;
			p_nNeedWrite = 1;
			return 0;
		}
		//we are sending again this event because we haven't got a response
		m_nAccessTime = nCurrentTime;
		if( m_nMaxNoRetry > 0 )
			m_nNoRetry++;
		
		p_nNeedWrite = 1;			
		return 1;
	}
	
	int nTrig = getTrigInterval();		
			
	if( nCurrentTime - m_nAccessTime < nTrig )
	{	return 0;
	}

	switch(m_cType) 
	{
	case EVTYPE3_DONT_SEND2CC:
	case EVTYPE2_SEND2CC:
		return 0;

	case EVTYPE1_INTERVAL:
		{
			int nSched = getSchedTimeInterval();
						
			if( nCurrentTime - m_nAccessTime > nSched )
			{
				p_nNeedWrite = 1;
				PrepareSend();	
				return 1;
			}

			if ( nSched <= 0 ) 
			{	LOG_ASSERT("EVTYPE1_INTERVAL nSched == 0");
				return 0;
			}
			
			nCurrentTime -= m_nActiveStartTime;
			nCurrentTime %= nSched; 
			if (nCurrentTime < nTrig)  
			{	p_nNeedWrite = 1;
				PrepareSend();	
				return 1;
			}

			return 0;
		}
	case EVTYPE0_FIX_TIME:
		{	if(! IsAbsDue(nCurrentTime) )
			{	return 0;
			}

			p_nNeedWrite = 1;
			PrepareSend();	
			return 1;			
		}

	case EVTYPE4_FIX_INTERVAL:
		{	int nSched = getSchedTimeInterval();
			int nPeriod = GetBasePeriod();

			if ( !nSched || !nPeriod ) 
			{	LOG("CEventsStruct::IsSignaling: ERROR crt=%d, nPeriod=%d, nSched=%d", nCurrentTime, nPeriod, nSched );
				return 0;
			}

			nCurrentTime %= nPeriod;
			nCurrentTime %= nSched;
			if ( nCurrentTime >= nTrig ) 
			{	return 0;
			}

			p_nNeedWrite = 1;
			PrepareSend();
			return 1;
		}
	}

	return 0;
}

int CEventsStruct::GetNextSchedTime()
{
    if( m_cActiveStatus == INACTIVE )
    {   return INFINIT;
    }

	if( m_nAccessTime == 0 || m_nResponseTime == 0 )
    {   return 0;
    }

	if( m_cType == EVTYPE2_SEND2CC || m_cType == EVTYPE3_DONT_SEND2CC )
	{	return INFINIT;
	}

	time_t nCurrentTime = time( NULL );    //get current time

	if ( m_nActiveStartTime > nCurrentTime || m_nActiveEndTime < nCurrentTime ) 
	{	return 0;
	}

	if (m_nAccessTime > nCurrentTime)	//time shift 
	{	m_nAccessTime = nCurrentTime;
	}
	switch( m_cType )
	{
	case EVTYPE0_FIX_TIME :
		{	time_t whenTime = getNextSchedTimeAbs( nCurrentTime );
			return whenTime < nCurrentTime ? INFINIT : whenTime - nCurrentTime; 	
		}
	case EVTYPE1_INTERVAL :
		{	int nTrig = getTrigInterval();
			int nSched = getSchedTimeInterval();
			
			if ( nSched <= 0 ) 
			{	LOG_ASSERT("EVTYPE1_INTERVAL nSched == 0");
				return 0;
			}

			if (nCurrentTime - m_nAccessTime > nSched) 
			{	return 0;
			}

			int nInterval = (nCurrentTime - m_nActiveStartTime) % nSched;
			if (nCurrentTime - m_nAccessTime > nTrig && nInterval < nTrig) 
			{	return 0;
			}
			
			return nSched - nInterval;
		}
	case EVTYPE4_FIX_INTERVAL:
		{	int nTrig = getTrigInterval();
			int nSched = getSchedTimeInterval();

			if( nCurrentTime - m_nAccessTime < nTrig )
			{	nSched -= (nCurrentTime - m_nAccessTime);
				return nSched - nTrig/5 >= 0 ? nSched - nTrig/5 : 0;
			}
			
			int nPeriod = GetBasePeriod();
			
			if ( !nSched || !nPeriod ) 
			{	LOG("CEventsStruct::GetNextSchedTime() ERROR crtTime=%d, nPeriod=%d, nSched=%d", 
								nCurrentTime, nPeriod, nSched );
				return INFINIT;
			}
			nCurrentTime %= nPeriod;
			if( nCurrentTime % nSched < nTrig )
			{	return 0;
			}

			nSched -= nCurrentTime % nSched;
			
			return  nCurrentTime + nSched <= nPeriod ? nSched : nPeriod - nCurrentTime;
		}
	}	
	return INFINIT;
}

/**  prepare event to send*/
void CEventsStruct::PrepareSend()
{
    m_nResponseTime = 0;
	if( m_nMaxNoRetry > 0 )
		m_nNoRetry = 1;
	m_nAccessTime = time(NULL);	
}





//////////////////////////////////////////////////////////////////////////////////
// Description : check if the event matching the conditions
// Parameters  :
//              int& p_nNeedWrite		-- tell if the event has to be write back to file
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int	CEventsStruct::CheckMatch( event_code p_nEvCode, rf_address p_nNodeId )
{
	unsigned int unNodeAny = 0xffffffff;

    if(m_cActiveStatus != ACTIVE )
    {   return UNK_TYPE;
    }
    
	if( m_cType != EVTYPE2_SEND2CC && m_cType != EVTYPE3_DONT_SEND2CC )
		return UNK_TYPE;
			
	if( ( memcmp( &m_stMatch.m_nEvCode, &p_nEvCode, sizeof(event_code) ) == 0) 
		&& ( (memcmp( &m_stMatch.m_nNodeId, &p_nNodeId, sizeof(rf_address) ) == 0) 
				||	(memcmp( &m_stMatch.m_nNodeId, &unNodeAny, sizeof(rf_address)) == 0)
			)
		)
	{
		m_nAccessTime = 0;
		m_nResponseTime = 0;
		if( m_nMaxNoRetry > 0 )
			m_nNoRetry = 1;
		return m_cType;
	}
		
	return UNK_TYPE;
}

int CEventsStruct::CheckEventId( CEventsStruct& p_rEv )
{
    if( ( memcmp( &m_pEventId, p_rEv.m_nDestId.id + sizeof(p_rEv.m_nDestId.id) - sizeof(m_pEventId),
            sizeof(m_pEventId) ) == 0)
      )
    {    return 1;
    }
    return 0;
}


time_t CEventsStruct::getNextSchedTimeAbs( time_t p_nTime )
{
	struct tm*	tms = gmtime( &p_nTime );

	unsigned char cNone = 0;
	unsigned char pTime[TIME_SIZE];

	unsigned char* pLastFF = &cNone;
	unsigned char* pMatch = (unsigned char*)&m_stSchedTime;
	int i;

	TIME_TO_BYTES( *tms, pTime );

	for( i=0 ; i < TIME_SIZE; i++ )
	{
		if ( pMatch[i] == ANY ) 
		{	pLastFF = pTime + i;
			continue;
		}
        if ( pTime[i] == pMatch[i])
        {   continue;
        }
        
		if ( pTime[i] < pMatch[i]) 
		{	pTime[i] = pMatch[i];
            break;
		}
		else
		{	(*pLastFF) ++;
			break;
		}
	}

	for(; i < TIME_SIZE; i++ )
	{	pTime[i] = pMatch[i] == ANY ? 0 : pMatch[i];
	}

	struct tm	when;
	memcpy( &when, tms, sizeof(struct tm) );
	BYTES_TO_TIME(pTime, when );
		
	return mktime(&when);
}


int CEventsStruct::IsAbsDue(time_t p_nTime)
{
	u_char* pTime = (u_char*)&m_stSchedTime;
	struct tm*	tms = gmtime( &p_nTime );

	unsigned char pCrtTime[TIME_SIZE];
	TIME_TO_BYTES( *tms, pCrtTime );

	int i;
	for( i = 0; i < TIME_SIZE; i++ )
	{	if( pTime[i] != ANY && pCrtTime[i] != pTime[i] )
		{	return 0;
		}
	}

	return 1;
}


int CEventsStruct::getSchedTimeInterval()
{
	u_char* pTime = (u_char*)&m_stSchedTime;
	int i, nSched = 0;

	for( i = YEAR; i < SECOND; i++ )
	{	if (pTime[i] != ANY) 
		{	nSched += pTime[i];
		}
		nSched *= g_vTimeUnitsSize[i+1];
	}

	if( m_stSchedTime.m_cSec != ANY )
	{	nSched += m_stSchedTime.m_cSec;
	}
	
	return nSched;	
}

int CEventsStruct::getTrigInterval()
{
	if( m_stSchedTime.m_cSec != ANY )
	{	return 15; //sec 
	}

	u_char* pTime = (u_char*)&m_stSchedTime;
	int i, nTrig = 1;

	for( i = SECOND; i > YEAR && pTime[i] == ANY; i-- )
		nTrig *= g_vTimeUnitsSize[i];

	return nTrig;	
}



int CEventsStruct::GetBasePeriod()
{
	u_char* pTime = (u_char*)&m_stSchedTime;
	int i = YEAR;

	for( ; i < TIME_SIZE && (pTime[i] == ANY || pTime[i] == 0); i++ )
		;

	int nTime = 1;
	for(; i < TIME_SIZE; i++ )
	{	nTime *= g_vTimeUnitsSize[i];
	}

	return nTime;
}
