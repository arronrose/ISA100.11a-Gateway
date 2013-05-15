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
                          EvFileMan.cpp  -  description
                             -------------------
    begin                : Tue May 14 2002
    email                : claudiu.hobeanu@nivis.com
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#include "../Shared/Common.h"
#include "../Shared/Events.h"



#define MAX_LINE    2048
#define FIELD_DELIM  ','
#define COMMENTC     ';'

void DisplayRules();


int main( int argc, char** argv )
{
    if( argc == 1 || argv[1][0] == 'h' )
    {
        printf(" Usage ./%s h[elp]						-> this help\n", argv[0]);
		printf("    or ./%s d[isplay]						-> display file %s\n", argv[0], EVENTS_FILE );
		printf("    or ./%s m[atch] xxxx xxxxxxxx	-> match\n", argv[0]);
		printf("    or ./%s u[pdate] xxxxxxxx		-> UpdateResponseTime\n", argv[0]);
		printf("    or ./%s n[ext]						-> NextSchedTime\n", argv[0]);
		printf("    or ./%s p[lug] xxxxxxxx 1/0 1/0		-> Sonde [Un]Plug Dest Active Changed\n", argv[0]);

        return 0;
    }

    printf("Rule file = %s\n", EVENTS_FILE);

	g_stLog.OpenStdout();
    CEvents		evs;
    if( !evs.BindToFile(EVENTS_FILE) )
	{	return 0;
	}

	switch(argv[1][0]) 
	{
	case 'd' :
		DisplayRules();
		break;
	case 'n' :
		{	int nForever = 0;
			if( argc > 2 )
			{	nForever = atoi(argv[2]);
			}
			do
			{	int t = evs.GetTimeUntilNextSched();
				LOG("Time until next sched %ds", t);
				if (nForever) 
				{	sleep(3);
				}
			}while(nForever);

		}
		break;
	case 'u' :
		{	if ( argc <= 2 ) 
			{	LOG("Too few parameters");
				break;
			}
			u_int nMsgId = 0; 
			sscanf( argv[2], "%x", &nMsgId );
    		evs.UpdateResponseTForEvent( nMsgId );
		}
		break;
	case 'f' :
		{	if ( argc <= 2 ) 
			{	LOG("Too few parameters");
				break;
			}
			u_int nRule = 0; 
			sscanf( argv[2], "%d", &nRule );
    		evs.ForceSend( nRule );
		}
		break;
	case 'm' :
		{  	if ( argc <= 3 ) 
			{	LOG("Too few parameters");
				break;
			}
			int evCode; 
    		int nodeId;
			
			sscanf( argv[2], "%x", &evCode );
			sscanf( argv[3], "%x", &nodeId );

			evCode = htons((u_short)evCode);
			nodeId = htonl(nodeId);

			event_code  stEvCode;
			rf_address  stNodeId;

			memcpy( &stEvCode, &evCode, sizeof(short) );
			memcpy( &stNodeId, &nodeId, sizeof(int) );

	   		evs.MatchEvents( stEvCode, stNodeId );
    	}
		break;
	case 'p' :
		{  	if ( argc <= 5 ) 
			{	LOG("Too few parameters");
				break;
			}
    		int nodeId;
			int nActive =0, nChanged =0;
			
			sscanf( argv[2], "%x", &nodeId );
			sscanf( argv[3], "%d", &nActive );
			sscanf( argv[4], "%d", &nChanged );

			nodeId = htonl(nodeId);

			rf_address  stNodeId;
			memcpy( &stNodeId, &nodeId, sizeof(int));

	   		evs.UpdateRules( stNodeId, nActive, nChanged, argv[5] );
    	}
		break;
	default:
		LOG("Cmd not implemented");
	}

	printf("\n");
}


void DisplayRules()
{
    FILE* pFBin = fopen( EVENTS_FILE, "r" );
    if( pFBin == NULL )
    {   LOG("Error at opening file %s", EVENTS_FILE );
  	    return;
    }

    CEventsStruct	ev;
    int i = 0;


    while(!feof(pFBin))
    {
    	int c = fread(  &ev, sizeof(CEventsStruct), 1, pFBin );
		if( c < 0 )
		{  	LOG("Error reading from file %s", EVENTS_FILE );
			break;
		}
        if( c  == 0  )
        {   break;
        }
		printf("\nRule %d with RuleId %04x (%08x) -> %s\n", i, ev.GetRuleId(), CEvents::GetMsgId( ev.GetRuleId(), i), 
					ev.m_cActiveStatus == ACTIVE ? "Active" : "Inactive");

		printf("\tRuleType = %02x, ",ev.m_cType);
		if( ev.m_cType != EVTYPE2_SEND2CC && ev.m_cType != EVTYPE3_DONT_SEND2CC )
		{	printf("SchedTime = %s\n", GetHex( &ev.m_stSchedTime, sizeof(ev.m_stSchedTime) ) );	
		}
		else
		{	printf("Match: EvCode = %s, NodeId = %s;\n", 
				GetHex( ev.m_stMatch.m_nEvCode.id, sizeof(ev.m_stMatch.m_nEvCode.id)),
				GetHex( ev.m_stMatch.m_nNodeId.id, sizeof(ev.m_stMatch.m_nNodeId.id))
					);	
		}
		printf( "\tDestType = %d, DestId = %s\n", (int)ev.m_cDestType, GetHex(&ev.m_nDestId.id, sizeof(ev.m_nDestId.id)));

		printf("\tCmdCode = %02x, CmdLen = %d\n", (u_char)ev.m_cNodeCommand, ev.m_nDNMsgLen );
		if (ev.m_cDestType == 0x02 || ev.m_cNodeCommand == CMD_SONDE_REMOTE_CMD 
				|| ev.m_cNodeCommand == CMD_YSI_BATCH ) 
			printf("\tMessage Data = %.*s\n", ev.m_nDNMsgLen-1, ev.m_pMessageData );	
		else
			printf("\tMessage Data = %s\n", GetHex( ev.m_pMessageData , ev.m_nDNMsgLen - 1, ' ' )	);

		time_t nTmp ;

		nTmp = ev.m_nAccessTime;
		printf("\tAccessTime   = %10d, %s", (int)ev.m_nAccessTime, ctime(&nTmp) );
		nTmp = ev.m_nResponseTime;
		printf("\tResponseTime = %10d, %s", (int)ev.m_nResponseTime, ctime(&nTmp) );

		nTmp = ev.m_nActiveStartTime;
		printf("\tStartValidTime = %10d, %s", (int)ev.m_nActiveStartTime, ctime(&nTmp) );

		nTmp = ev.m_nActiveEndTime;
		printf("\tEndValidTime   = %10d, %s", (int)ev.m_nActiveEndTime, ctime(&nTmp) );
				
	    //printf( "\n\tRetry interval on fail = %d", ev.m_nNoSec );		
	    //printf( "\n\tMax fail retry no = %d", ev.m_nMaxNoRetry );		
	    //printf( "\n\tCrt. fail retry no = %d\n", ev.m_nNoRetry );		
		i++;
     }

    fclose(pFBin);
}
