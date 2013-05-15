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
                          AppS.cpp  -  description
                             -------------------
    begin                : Tue Apr 16 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/




#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>      // for setpriority()
#include <sys/time.h>          // for setpriority()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>


#include "../Shared/Common.h"
#include "../Shared/log.h"
#include "../Shared/Utils.h"
#include "AppS.h"

#define LONG_TIME		3600
#define MAX_PKT_LEN		32

#define REG_SPACE		512

#define LOG_SCHED_TEST LOG

CAppS	*app = NULL;

static volatile int g_USR1Raised = 0;

CAppS::CAppS()
:CApp( "scheduler" )
{
}

CAppS::~CAppS()
{
}

//////////////////////////////////////////////////////////////////////////////////
// Description : 	compose a command to send to DNC from event
// Parameters  :
//                CEventsStruct* p_pEv      - input  - the event with information about command
//                char*& p_pCommands        - output - buffer containing the command to send
//                int& p_nCommandsLen       - output - length of buffer p_pCommands
// Return      :
//             0 on error or 1 on success
//////////////////////////////////////////////////////////////////////////////////
int CAppS::getCommandFromEvent( const CEventsStruct* p_pEv, char*& p_pCommands, int& p_nCommandsLen  )
{
	if( p_pEv == NULL )
	{
		LOG( "CAppS::getCommandFromEvent : null pointer " );
		return 0;
	}

    if( (unsigned char)p_pEv->m_cNodeCommand == CMD_LINKED )
    {
		if ( p_pEv->m_pMessageData[2] == CMD_WRITE_IN_FLASH )
		{
            unsigned short nTmp = 0;
            memcpy( &nTmp, p_pEv->m_pMessageData, 2);
            nTmp = ntohs(nTmp);
            nTmp --;

            char pFile[256];


            memcpy( pFile, p_pEv->m_pMessageData +3, nTmp );
            pFile[nTmp] = 0;

            int nLen;
			if ( !GetFileData( pFile, p_pCommands, nLen ) )
			{	return 0;
			}

			if ( nLen < REG_SPACE || nLen > REG_SPACE/5 * MAX_PKT_LEN )
			{	LOG( "CAppS::getCommandFromEvent : invalid file size %d, min=%d, max=%d ",
											nLen, REG_SPACE, REG_SPACE/5 * MAX_PKT_LEN );
				return 0;
			}
			int i = 0, j = REG_SPACE, nCrtLen ;


			for( nLen -= REG_SPACE; nLen > 0 ; nLen -= MAX_PKT_LEN, j += MAX_PKT_LEN )
			{	nCrtLen = MAX_PKT_LEN < nLen ? MAX_PKT_LEN : nLen;

				nTmp = htons( nCrtLen + 3 );
				memcpy( p_pCommands + i, &nTmp, sizeof(nTmp) );

				p_pCommands[ i + 2 ] = CMD_WRITE_IN_FLASH;
				nTmp = htons( j - REG_SPACE );
				memcpy( p_pCommands + i + 3, &nTmp, sizeof(nTmp) );

				memcpy( p_pCommands + i + 5, p_pCommands + j, nCrtLen );
				i += nCrtLen +5;
			}
            p_nCommandsLen = i;
			return 1;
		}
        p_nCommandsLen = p_pEv->m_nDNMsgLen;
        p_pCommands = new char[p_nCommandsLen];

        if( p_pCommands == NULL )
	    {
		    LOG_ERR( "CAppS::getCommandFromEvent: can't allocate memory" );
		    return 0;
	    }

        memcpy(p_pCommands, p_pEv->m_pMessageData, p_nCommandsLen );
        return 1;
    }
	//length need for p_pCommands
	int nLen = sizeof(short) + p_pEv->m_nDNMsgLen;

	p_pCommands = new  char[nLen];

	if( p_pCommands == NULL )
	{
		LOG_ERR( "CAppS::getCommandFromEvent: can't allocate memory" );
		return 0;
	}

	//first 2 bytes(short) contain length of command without these bytes
	*((short*)p_pCommands) = htons( p_pEv->m_nDNMsgLen );

	//then a byte for comamnd code
    p_pCommands[sizeof(short)] = p_pEv->m_cNodeCommand;

    memcpy( p_pCommands + sizeof(short) + sizeof(p_pEv->m_cNodeCommand), p_pEv->m_pMessageData,
            p_pEv->m_nDNMsgLen - sizeof(p_pEv->m_cNodeCommand)
           );

	p_nCommandsLen = nLen;
	return 1;
}

int CAppS::Init()
{
    if( !CApp::Init( S_LOG_FILE ) )
        return 0;

	if( !m_stConfig.Init( "Scheduler" ) )
		return 0;

	if (!m_oPipeToMesh.Open( PIPE_2MESH, O_RDWR | O_TRUNC))
	{	return 0;
	}

	if (!m_oPipeToLocalDN.Open( PIPE_2LOCAL, O_RDWR | O_TRUNC))
	{	return 0;
	}

    int 	nErr = m_clEvents.BindToFile( EVENTS_FILE );
	if( nErr < 0 )
	{	LOG( "CAppS::Init :error binding to EVENTS_FILE");
		return 0;
	}

	if( !m_stConfig.GetVar( "SCHEDULER_SLEEP_TIME", &m_nSchedulerSleepTime ) )
	{	LOG(" CAppS::Init() : use SCHEDULER_SLEEP_TIME = %d ",  SCHEDULER_SLEEP_TIME );
		m_nSchedulerSleepTime = SCHEDULER_SLEEP_TIME;
	}
    
	if ( m_nSchedulerSleepTime < SCHEDULER_SLEEP_TIME ) 
	{	LOG(" CAppS::Init() : value for SCHEDULER_SLEEP_TIME = %d too small -> USE %d", 
									m_nSchedulerSleepTime, SCHEDULER_SLEEP_TIME );
		m_nSchedulerSleepTime = SCHEDULER_SLEEP_TIME;
	}

	return 1;
}


/**  */
void CAppS::Run()
{
	long clkPerSec = sysconf(_SC_CLK_TCK);
	clock_t nNextSweepTime =  GetClockTicks();
	
	while(!CApp::IsStop())
	{  	
		TouchPidFile(m_szAppPidFile);

		CheckAndFixWrapClockTicks(GetClockTicks(), &nNextSweepTime);

		if( 	( g_USR1Raised != 0 ) 
			||	( GetClockTicks() > nNextSweepTime ) )
		{
			SweepEvents();
			g_USR1Raised = 0;
			nNextSweepTime = GetClockTicks() + clkPerSec * app->GetSchedSleepTime();
		}
		checkNullRuleFile();
		sleep(1); 
	}
}

/** sweeps events_file (rule_file) and sends the commands who are scheduled to be send at this time*/
void CAppS::SweepEvents()
{
	
	//LOG_SCHED_TEST( "Now there are %d rules", m_clEvents.GetRulesCount() );     

	for( int i = 0; i < m_clEvents.GetRulesCount(); i++ )
	{
		CEventsStruct	clEv;
		if( m_clEvents.IsSignaling( i, clEv ) )
		{
			m_oModulesActivity.SetAct(MODULE_ACT_SH, '1' );
			LOG( "Raised: rule %d, RuleId=%s, DestType=%d, DestId=%s", i, GetHex(&clEv.m_pEventId, sizeof(clEv.m_pEventId) ), 
										clEv.m_cDestType, GetHex(&clEv.m_nDestId, sizeof(clEv.m_nDestId)) );
			
            switch( clEv.m_cDestType )
			{
			    case 0:
				case 3:
				{	char* 	pCommands = NULL;
					int 	nCommandsLen = 0;

					getCommandFromEvent( &clEv, pCommands, nCommandsLen );
					
					if( pCommands )
					{
						g_stLog.WriteHexMsg( "\tCmds: ", (u_char*)pCommands, nCommandsLen );
                        
						CPipe::THeaderToDNC   stDncHeader = {0};
						stDncHeader.m_nMsgId = m_clEvents.GetMsgId( clEv.GetRuleId(), i );
						stDncHeader.m_stNodeId = clEv.m_nDestId;
						stDncHeader.m_cAnsPath =  clEv.m_cAnsPath;
						stDncHeader.m_nAppId = m_stConfig.m_shAppId;
						
						if(m_oPipeToMesh.WriteMsg( &stDncHeader, sizeof(stDncHeader), pCommands , nCommandsLen ) )
						{   LOG( "\tRule %d was sent", i );
						}
						delete [] pCommands;
					}
					break;
				}
				case 1:
					m_clEvents.UpdateRules( clEv );
					m_clEvents.UpdateResponseTForEvent(m_clEvents.GetMsgId(clEv.GetRuleId(), i));
					break;
				case 2:
				{	//launch in current shell, withbenefits  only for scripts.
					setpriority(PRIO_PROCESS, 0, 0);
					//in 30min will the AN watchdog will decide to restart the board if program blocks
					if ( systemf_to( 1800, "{ %s; }", clEv.m_pMessageData) != -1 )//one hour should do. use INT_MAX
					{	m_clEvents.UpdateResponseTForEvent(m_clEvents.GetMsgId( clEv.GetRuleId(), i));
					}
					setpriority(PRIO_PROCESS, 0, -10);
					break;
				}
			}
		}
	}

	m_oModulesActivity.SetAct(MODULE_ACT_SH, '0' );
}



void USR1_Handler(int)
{
	g_USR1Raised = 1;
	signal(SIGUSR1, USR1_Handler);
	//alarm( app->GetSchedSleepTime() );
}

int main()
{
	app = new CAppS();
    
	setpriority(PRIO_PROCESS, 0, -10);
	if( app->Init() )
	{	
		USR1_Handler( 0 );
		app->Run();
		app->Close();
	}

	delete app;
	return 0;
}



void CAppS::checkNullRuleFile()
{
	//rebuild only if at least 1 min the rule_file is empty
	for( int i=0; i < 12; i++ )
	{
		if ( m_clEvents.GetRulesCount() > 0 ) 
		{	return;
		}
		sleep(5);
	}
	::systemf_to(60, "rebuild_rule_file.sh" );

	if ( m_clEvents.GetRulesCount() > 0 ) 
	{	return;
	}

	//if the rule file has 0 rules there is nothing to do for scheduler -> sleep for a while
	sleep(60);
}


