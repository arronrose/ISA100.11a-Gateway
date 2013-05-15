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
                          EventsStruct.h  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/

#ifndef _EVENTSSTRUCT_H	
#define _EVENTSSTRUCT_H
#include <time.h>
#include "Common.h"

#define ANY				0xFF
#define INFINIT			100000000

#define UNK_TYPE		-1
#define NO_MATCH		UNK_TYPE

#define EVTYPE0_FIX_TIME		0
#define EVTYPE1_INTERVAL		1
#define EVTYPE4_FIX_INTERVAL	4

#define EVTYPE2_SEND2CC			2
#define EVTYPE3_DONT_SEND2CC	3



#define ACTIVE          1
#define INACTIVE        0

enum
{
	CMD_CHANGE_RULE_STATUS = 1,
	CMD_CHANGE_RULE_FORCE_SEND = 2
};


#define TRIG0			59
#define TRIG2			59
#define TRIG3			23

#define SECS			60
#define MINS			60




class CEventsStruct
{
public:

class StMatch
{
public:
	event_code	m_nEvCode;
	rf_address	m_nNodeId;
}__attribute__((packed));

class StTime
{
public:
	unsigned char	m_cYear;
	unsigned char	m_cMon;
	unsigned char	m_cDay;
	unsigned char	m_cHour;
	unsigned char	m_cMin;
	unsigned char	m_cSec;
}__attribute__((packed));

public:
    char        m_pEventId[2];
    char        m_cActiveStatus;
    char        m_cAnsPath;

    char        m_cDestType;
    rf_address	m_nDestId;
      
	char 		m_cType;
	union
	{
		StTime		m_stSchedTime;
	    StMatch		m_stMatch;		
	};
    
	time_t		m_nAccessTime;
	time_t		m_nResponseTime;

	time_t		m_nActiveStartTime;
	time_t		m_nActiveEndTime;
	
	short		m_nNoSec;
	short		m_nMaxNoRetry;
	short		m_nNoRetry;
    char		m_cNodeCommand;
    short       m_nDNMsgLen;
	char		m_pMessageData[256];
	
public:
	CEventsStruct();

	//WARNING inline fcts don't work corect with pointers because of the pack attribute of the class
	int			IsAbsDue( time_t p_nTime );	
	int 		IsSignaling( int& p_nNeedWrite );
	int			GetNextSchedTime();
	void		PrepareSend();
	int			CheckMatch( event_code p_nEvCode, rf_address p_nNodeId );
    int         CheckEventId( CEventsStruct& p_rEv );
	int			GetBasePeriod();	

	u_short		GetRuleId();
	void		SetRuleId( u_short p_usRuleId );	
protected:
	int			getSchedTimeInterval();
	time_t		getNextSchedTimeAbs( time_t p_nTime );	
	int			getTrigInterval();
}__attribute__((packed));

#endif
