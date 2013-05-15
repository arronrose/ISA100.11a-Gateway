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

/****************************************************
* Name:        RuleDefFile.cpp
* Author:      Marius Chile
* Date:        21.10.2003
* Description: The program that reads the rule file into a binary form (file)
* Changes:
* Revisions:
****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/file.h>
#include <unistd.h>


#include "../Shared/Common.h"
#include "../Shared/RuleDefFile.h"
#include "../Shared/Locks.h"

#define MAX_EVENT_NO    4096

int main( int argc, char** argv )
{
    g_stLog.Open( EVT_LOG_FILE );
	
    if( argc < 2 || ( ( argc == 2 || argc == 3 ) && ! strcmp(argv[1], "-o") ) )
    {
	printf( "Usage: %s [-o OutputFile] File1 [File 2 File 3 ...]", argv[0]);
	printf( "Note: Output file = /tmp/rule_file by default" );
        exit(1);
    }

    char * pBinName;
    int nRuleFileIndex;		// the position in argv of the first rule file to be parsed
    if ( ! strcmp(argv[1], "-o") )
    {
		pBinName = argv[2];
		nRuleFileIndex = 3;
	}
	else
	{
		pBinName = (char*)EVENTS_FILE;
		nRuleFileIndex = 1;
	}

    int nFBin = -1;	
	for( ;; ) 
	{
		nFBin = open( pBinName, O_WRONLY| O_CREAT | O_TRUNC,  0666 );

		if( nFBin <= 0 )
		{   LOG_ERR( "Error opening file %s", pBinName );
 			exit(1);
		}
		FLOCK( nFBin, LOCK_EX );

		if ( lseek( nFBin, 0, SEEK_END ) == 0 ) 
		{	break;
		}
		LOG("The file %s was not TRUNC -> retrying", pBinName );
		FLOCK( nFBin, LOCK_UN );
		close(nFBin);
	} 

    int nNoEvents = 0;
    int nErr = 0;
    int i;
    event_code pEventId[MAX_EVENT_NO];
	CRuleDefFile ruleFile;
   	CEventsStruct	ev;
   	Return_Type nRet;
	for ( ; nRuleFileIndex < argc; nRuleFileIndex++ )
    {	
	    if( !ruleFile.Load(argv[nRuleFileIndex]) )
	    {
			LOG_ERR( "Error opening file %s. Movin' on...", argv[nRuleFileIndex] );
        	continue;
    	}
    
    	memset(&ev, 0, sizeof(ev));
    	while( ( nRet = ruleFile.ReadEvent(&ev, true) ) != RULE_END_OF_FILE )
    	{
    	    nErr = 1;

        	if( nRet == RULE_INVALID )
        	{
				nErr = 0;
		    	memset(&ev, 0, sizeof(ev));
            	continue;
        	}

        	for( i = 0; i < nNoEvents; i++ )
        	{   if( memcmp( pEventId + i, ev.m_pEventId, sizeof(event_code) ) == 0 )
            	{
            		break;
            	}
        	}
        	if( i < nNoEvents )
        	{
        		LOG( "Warning : duplicated event id  %2.2X %2.2X. Event ignored", ev.m_pEventId[0], ev.m_pEventId[1] );
				nErr = 0;
		    	memset(&ev, 0, sizeof(ev));
        		continue;
        	}

        	memcpy( pEventId + nNoEvents, ev.m_pEventId, sizeof(event_code) );

        	ev.m_nResponseTime = time(NULL);
        	ev.m_cAnsPath = 1;

			int c = write( nFBin, &ev, sizeof(CEventsStruct) );
			if( c <= 0 )
			{
        		LOG_ERR( "Error writing in file %s", pBinName );
		    	break;
			}

	    	memset(&ev, 0, sizeof(ev));
			nNoEvents++;
			if ( nNoEvents == MAX_EVENT_NO )
			{
		    	LOG( "Max events reached (%d). Bailing out.", nNoEvents );
				ruleFile.Release();
				FLOCK( nFBin, LOCK_UN );
    			close(nFBin);
       			exit(1);
			}
			nErr = 0;
    	}

    	if( !nErr )
			LOG( "Reading %s file ended successfully", argv[nRuleFileIndex] );
		else
		{
			LOG( "An error occured during reading file %s", argv[nRuleFileIndex] );
			nErr = 0;
		}
    	LOG( "Read a total of %d events", nNoEvents );

		ruleFile.Release();
	}

	FLOCK( nFBin, LOCK_UN );		
    close(nFBin);
}
