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
                          Events.cpp  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stdlib.h>

#include "Common.h"
#include "Events.h"




CEvents::CEvents()
{
	m_nFd = -1;
}

CEvents::~CEvents()
{
	Release();
}

/**
	release all the resources owned: buffer with file name, fd, buffer with events
	return : none
*/
void CEvents::Release()
{
	if( m_nFd > -1 )
	{	close(m_nFd);
		m_nFd = -1;
	}
}


//////////////////////////////////////////////////////////////////////////////////
// Description : 	open file p_pFileName
//					and compute no. events contains in file
// Parameters  :
//              const char * p_pFileName    - input - the file name
// Return      :
//             0 on error or 1 on success
// Obs		   :
//				must be called before any other operation
//				doesn't load events in memory
//////////////////////////////////////////////////////////////////////////////////
bool CEvents::BindToFile( const char* p_pFileName )
{
	if( p_pFileName == NULL )
	{	LOG( "CEvents::BindToFile : error p_pFileName == NULL " );
		return false;
	}

	if( m_nFd >= 0 )
	{	Release();
	}
	
	m_nFd = open( p_pFileName, O_RDWR | O_CREAT, 666 );
	if( m_nFd < 0 )
	{	LOG_ERR( "CEvents::BindToFile : can't open file %s ", p_pFileName );
		return false;
	}

	m_nNoEvents = GetRulesCount();
	return true;
}



int CEvents::GetRulesCount() 
{ 
	int nNoEvs = lseek( m_nFd, 0, SEEK_END ) / sizeof(CEventsStruct);

	if (nNoEvs != m_nNoEvents ) 
	{	m_nNoEvents = nNoEvs;
		LOG("Rules count changed to %d", m_nNoEvents );
	}

	return m_nNoEvents; 
}


//////////////////////////////////////////////////////////////////////////////////
// Description : 		write at a given offset p_nLen bytes from buffer p_nBuff
// Parameters  :
//                      const int	p_nOff 		- input 	offset in file
//						const void* p_pBuff     - input     data to be written
//						const int	p_nLen      - input 	the length of data to be written
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int CEvents::writeAtOffset( const int p_nOff, const void* p_pBuff, const int p_nLen  )
{
	LOG_UNUSED_COD;
	if( p_pBuff == NULL || p_nLen <= 0 )
	{
  		LOG( "CEvents::WriteAtOffset : buffer or p_nLen 0 " );
		return 0;
	}

	if( lseek( m_nFd, p_nOff, SEEK_SET) < 0 )
	{	LOG( "CEvents::WriteAtOffset : error call lseek" );
		return 0;
	}

	FLOCK( m_nFd, LOCK_EX );
	//FCNTL( m_nFd, F_WRLCK, p_nOff, p_nLen);

	int c = write( m_nFd, p_pBuff, p_nLen );
	if( c <= 0 )
 		LOG_ERR( "CEvents::WriteAtOffset : error on writing " );

	FLOCK( m_nFd, LOCK_UN );
	//FCNTL( m_nFd, F_UNLCK, p_nOff, p_nLen);

	return (c > 0);
}


//////////////////////////////////////////////////////////////////////////////////
// Description :		write event i in file
//						if events are not loaded returns error
// Parameters  :
//                      const int 		i 	- input 	index of event
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int CEvents::WriteEvent( int i, CEventsStruct& p_clEv )
{
	if ( i > GetRulesCount() ) //could be added only continous rules
	{	LOG("CEvents::WriteEvent(): Error : RulePos = %d > RulesNo = %d", i, m_nNoEvents );
		return 0;
	}
	if( lseek( m_nFd, i*sizeof(CEventsStruct), SEEK_SET ) < 0 )
	{	LOG_ERR( "CEvents::WriteEvent : error call lseek at %d", i*sizeof(CEventsStruct));
		return 0;
	}

	int c = write( m_nFd, &p_clEv, sizeof(CEventsStruct) );
	if( c <= 0 )
	{	LOG_ERR( "CEvents::WriteEvent : error on writing rule no. %d", i );
	}

	return (c > 0);
}

//////////////////////////////////////////////////////////////////////////////////
// Description : 		read event i from file
//						if events are not loaded returns error
// Parameters  :
//                      const int 		i 	- input 	index of event
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int CEvents::ReadEvent( int i, CEventsStruct& p_clEv )
{
	if( lseek( m_nFd, i*sizeof(CEventsStruct), SEEK_SET ) < 0 )
	{	LOG_ERR( "CEvents::ReadEvent : error call lseek at %d", i*sizeof(CEventsStruct) );
		return 0;
	}

	int c = read( m_nFd, &p_clEv, sizeof(CEventsStruct) );
	if( c <= 0 )
	{	LOG_ERR( "CEvents::ReadEvent : error on reading rule no. %d", i );
	}

	return (c>0);
}


//////////////////////////////////////////////////////////////////////////////////
// Description : 		update ResponseTime for a event at a given offset by writing it in file and
//							if the events are loaded in memory write in memory too
// Parameters  :
//                      const int 		p_nOffset 	- input 	offset in file
//						const time_t 	p_nTime   	- input 	the time value to be written
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int CEvents::UpdateResponseTForEvent( int p_nMsgId, time_t p_nTime )
{
	int res = 0;

	LOG( "CEvents::UpdateResponseTForEvent : MsgId=%08X, RuleNo=%04X, RuleId=%04X ", 
				p_nMsgId, GetRulePos(p_nMsgId), GetRuleId(p_nMsgId) );

	Lock();
	GetRulesCount();
	do 
	{	if( GetRulePos(p_nMsgId) >= m_nNoEvents )
		{	LOG( "CEvents::UpdateResponseTForEvent : not a valid rule no(%04X)", GetRulePos(p_nMsgId) );
			break;
		}	

		CEventsStruct clEv;
		if (!ReadEvent(GetRulePos(p_nMsgId), clEv ) ) 
		{	break;
		}
		if ( GetRuleId(p_nMsgId) != clEv.GetRuleId() ) 
		{	LOG( "CEvents::UpdateResponseTForEvent : not a valid rule id (%04X)!=(%04X)", GetRuleId(p_nMsgId), clEv.GetRuleId() );
			break;
		}
		clEv.m_nResponseTime = p_nTime == -1 ? time(NULL) : p_nTime;
		
		//test sleep place
		//LOG("Sleep 30"); sleep(30);
		WriteEvent( GetRulePos(p_nMsgId), clEv );
		res = 1;
	} while(false);
	
	Unlock();
	return res;
}


int CEvents::IsSignaling( int i, CEventsStruct& p_clEv )
{
	int nSig = 0; 

	Lock();
	do 
	{	if (GetRulesCount() <= i )
		{	break;
		}	

		if( !ReadEvent( i, p_clEv ) )
		{	break;
		}

		int nNeedWrite = 0;
		nSig = p_clEv.IsSignaling(nNeedWrite);

		if( nNeedWrite )
		{	//a error on write should stop the send of this event
			nSig =  WriteEvent( i , p_clEv ) && nSig;
		}
	} while(false);
	Unlock();

	return nSig;
}



int CEvents::updateRule(CEventsStruct &p_clEv, CEventsStruct &p_clCrtRule)
{
	if( p_clCrtRule.CheckEventId(p_clEv ) )
    {
        LOG( "CEvents::updateRule() : rule %2.2x %2.2x, cmd = %2.2x ",
                        p_clCrtRule.m_pEventId[0], p_clCrtRule.m_pEventId[1], p_clEv.m_cNodeCommand );
		switch( p_clEv.m_cNodeCommand )
		{
		case CMD_CHANGE_RULE_STATUS:
			p_clCrtRule.m_cActiveStatus = p_clEv.m_pMessageData[0] ;
			break;
		case  CMD_CHANGE_RULE_FORCE_SEND:
			p_clCrtRule.m_nAccessTime = 0;
			break;
		default:
			LOG( "CEvents::updateRule() : invalid cmd = %x ", p_clEv.m_cNodeCommand);
		}
		return 1;
	}
	return 0;
}

int CEvents::MatchEvents( event_code p_nEvCode, rf_address p_nNodeId )
{
	Lock();
	GetRulesCount();

	int nType = UNK_TYPE;
	CEventsStruct	clEv;
	for( int i = 0; i < m_nNoEvents; i++ )
	{
		if( !ReadEvent( i, clEv ) )
		{   break;
		}

		int t = clEv.CheckMatch( p_nEvCode, p_nNodeId );

		if( t != UNK_TYPE )
		{   nType = t;
			WriteEvent( i , clEv );
        }
	}

	Unlock();

	if( nType != UNK_TYPE )
		system_to(60, "killall -USR1 scheduler");//should be quick, 60 sec should be enough...

	LOG( "CEvents::MatchEvents: evCode[%02X %02X], DN[%s] -> %d",p_nEvCode.id[0], p_nEvCode.id[1], Id2string( p_nNodeId ), nType);
	return nType;
}


//this fct only read the rule_file and for faster execution use no sync
int CEvents::GetTimeUntilNextSched()
{
	int nTime = INFINIT;
	CEventsStruct	clEv;

	GetRulesCount();
	for( int i = 0; i < m_nNoEvents && nTime != 0; i++ )
	{
		if( ReadEvent( i, clEv) )
		{
			int nEvDue = clEv.GetNextSchedTime();
			if ( nEvDue < nTime )
			{
				nTime = nEvDue;
			}
		}
		else
		{
			//happen only if rule_file is modified in this time,
			//say that is busy ( have something to do after 0 s!)
			nTime = 0;
		}
	}

	return nTime;
}

int CEvents::UpdateRule( unsigned short p_usRuleId , unsigned char* p_pSchedTime )
{
	CEventsStruct	clEv;
	int				ret = 0;

	Lock();
	GetRulesCount();

	for( int i = 0; i < m_nNoEvents; i++ )
	{
		if( !ReadEvent( i, clEv ) )
		{   break;
		}

		if ( memcmp( &clEv.m_pEventId, &p_usRuleId, 2) == 0 )
		{	if ( clEv.m_cType != EVTYPE1_INTERVAL && clEv.m_cType != EVTYPE4_FIX_INTERVAL)
			{	break;
			}
			p_pSchedTime[SECOND] = ANY; // safe condition for 15/06/2004 delivery
			memcpy( &clEv.m_stSchedTime, p_pSchedTime, 6 );
			WriteEvent( i, clEv );
			LOG( "CEvents::UpdateRule() :Rule changed");
			ret = 1;
			break;
		}
	}

	Unlock();
	return ret;
}



int CEvents::UpdateRules( CEventsStruct& p_clEv )
{
 	CEventsStruct	clEv;
	int				ret = 0;

	Lock();
	GetRulesCount();
	for( int i = 0; i < m_nNoEvents; i++ )
	{
		if( !ReadEvent( i, clEv ) )
		{   break;
		}

        if( updateRule( p_clEv, clEv  ) )
        {   WriteEvent( i , clEv );
            ret = 1;
			break;
		}
	}
	Unlock();

	if( !ret )
	{	g_stLog.WriteHexMsg( "CEvents::UpdateRules() : no rule with id :",
							(unsigned char*)&p_clEv.m_nDestId, sizeof(p_clEv.m_nDestId)  );
	}
	return ret;
}

int CEvents::UpdateRules( const rf_address &p_rDest, int p_nActivate, int p_nChanged, const char* p_pVer )
{
	CEventsStruct	clEv;
	int				nFound = 0;
	u_short			usMaxRuleId = 0;
	
	LOG( "CEvents::UpdateRules() : DestId=%s, Activate=%d,  TypeChanged = %d", 
			GetHex( (u_char*)&p_rDest, sizeof(p_rDest)), p_nActivate, p_nChanged );

	Lock();
	GetRulesCount();
	
	for( int i = 0; i < m_nNoEvents; i++ )
	{
		if( !ReadEvent( i, clEv ) )
		{   break;
		}

		if (usMaxRuleId < clEv.GetRuleId() ) 
		{	usMaxRuleId = clEv.GetRuleId();
		}

		if ( memcmp( &clEv.m_nDestId, &p_rDest, sizeof(clEv.m_nDestId) ) == 0 
				&& ( clEv.m_cType == EVTYPE1_INTERVAL || clEv.m_cType == EVTYPE4_FIX_INTERVAL
				|| clEv.m_cType == EVTYPE0_FIX_TIME )
			)
		{	if( clEv.m_cActiveStatus != p_nActivate)
			{	LOG( "CEvents::UpdateRules() :RuleId=%04x, Activate=%d", clEv.GetRuleId(), p_nActivate );
				clEv.m_cActiveStatus = p_nActivate;
				WriteEvent( i, clEv );
			}
			
			if (!p_nActivate ) 
			{	continue;
			}
			nFound = 1;
			if (p_nChanged) 
			{	LOG( "CEvents::UpdateRules() : rule modified");
				addSamplingRule(clEv,p_pVer);
				WriteEvent( i, clEv );
			}
			break;
		}
	}


	if ( p_nActivate && !nFound ) 
	{	LOG( "CEvents::UpdateRules() : sampling rule not found -> add");
		memset( &clEv, 0, sizeof(clEv));
		memcpy( &clEv.m_nDestId, &p_rDest, sizeof(rf_address) );
		clEv.SetRuleId( usMaxRuleId + 1  );
		
		addSamplingRule( clEv, p_pVer );
		WriteEvent( m_nNoEvents, clEv );
		GetRulesCount(); 
	}
	Unlock();
	
	return 1;
}

int CEvents::addSamplingRule( CEventsStruct& p_rEv, const char* p_pVer )
{
	p_rEv.m_cActiveStatus	= ACTIVE;
	p_rEv.m_cAnsPath		= 1;
	p_rEv.m_cDestType		= 0;
		
	p_rEv.m_cType			= EVTYPE4_FIX_INTERVAL;
	memset( &p_rEv.m_stSchedTime, 0xff, sizeof(p_rEv.m_stSchedTime));
	p_rEv.m_stSchedTime.m_cMin = 15;

	p_rEv.m_nMaxNoRetry	=	1;
	p_rEv.m_nNoSec		=	60;

	p_rEv.m_cNodeCommand		= CMD_SONDE_REMOTE_CMD;
	if (strncmp(p_pVer,"12", 2)<0) 
		strcpy( p_rEv.m_pMessageData, "{measure}0M!");		
	else
		strcpy( p_rEv.m_pMessageData, "{measure}0C!");		
	p_rEv.m_nDNMsgLen		= strlen(p_rEv.m_pMessageData) + 1;
	
	p_rEv.m_nAccessTime = 1;
	p_rEv.m_nResponseTime = time(NULL);
	p_rEv.m_nActiveStartTime = 0;
	p_rEv.m_nActiveEndTime = 0x7fffffff;

	return 1;
}

//to use for test only
int CEvents::ForceSend(int p_nRulePos)
{
	int res = 0;

	LOG( "CEvents::ForceSend : RuleNo=%d", p_nRulePos );

	Lock();
	GetRulesCount();
	do 
	{	if(p_nRulePos >= m_nNoEvents )
		{	LOG( "CEvents::ForceSend : not a valid rule no(%04X)", p_nRulePos);
			break;
		}	

		CEventsStruct clEv;
		if (!ReadEvent(p_nRulePos, clEv ) ) 
		{	break;
		}

		clEv.m_nResponseTime = 0; 
		clEv.m_nAccessTime = 0;
		
		//test sleep place
		//LOG("Sleep 30"); sleep(30);
		WriteEvent( p_nRulePos, clEv );
		res = 1;
	} while(false);
	
	Unlock();
	return res;	
}
