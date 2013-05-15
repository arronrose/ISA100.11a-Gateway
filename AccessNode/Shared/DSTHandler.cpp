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
                          DSTHandler.cpp  -  description
                             -------------------
    begin                : Tue Aug 24 2004
    email                : marius.chile@nivis.com
 ***************************************************************************/

#include <netinet/in.h>
#include "DSTHandler.h"
#include "IniParser.h"
#include "Common.h"
#include <stdlib.h>


#define BAD_UTC_OFFSET 99

CDSTHandler::CDSTHandler(){
}
CDSTHandler::~CDSTHandler(){
}

///////////////////////////////////////////////////////////////////////////////////
// NAME: Scan
// AUTHOR: Marius Chile
// DESCRIPTION: Checks the current DST offset based on the DST.cfg file
// PARAMETERS: p_pchOffset - the offset will be returned here
// RETURN: true - offset successfully returned
//		   false - an error has occured, offset invalid
///////////////////////////////////////////////////////////////////////////////////
bool CDSTHandler::Scan(int* p_pnOffset)
{
    CIniParser dstFile;
	int nUTCOffset = BAD_UTC_OFFSET;

    // load DST file
    if ( dstFile.Load( DST_CFG_FILE ) )
    {
		do
		{
			time_t nUTCTime;
			
			// get UTC offset
			if ( ! dstFile.GetVar("GLOBAL", "UTCOffset", &nUTCOffset) )
				break;

			// check offset validity
			if ( nUTCOffset < -12 || nUTCOffset > 13 )
			{
				LOG("CDSTHandler::scan - UTCOffset has wrong value %d", nUTCOffset);
				nUTCOffset = BAD_UTC_OFFSET;
				break;
			}

			// look for the first DSTCalendar group appearance
			if ( ! dstFile.FindGroup( "DSTCalendar" ) )
			{				
				break;
			}

			// get current time
            nUTCTime = time(NULL);
			
			do
			{
                int nSpringTime;
                int nFallTime;

                if ( ! dstFile.GetVar( NULL, "Spring", &nSpringTime, 0, false ) ||
                     ! dstFile.GetVar( NULL, "Fall", &nFallTime, 0, false ))
   				{
		   			LOG("CDSTHandler::scan - DSTCalendar group found without Spring or Fall parameters");
					continue;
		   		}
		     						
				if ( nUTCTime >= nSpringTime && nUTCTime <= nFallTime)
				{
					nUTCOffset++; // add 1 to offset (in DST time)						
					break;
				}
			    
				
				// go to next DSTCalendar group
			} while ( dstFile.FindGroup("DSTCalendar", true, false) ); 
		} while (0);
		
		dstFile.Release();
    }
	
	*p_pnOffset = nUTCOffset;
	return (nUTCOffset == BAD_UTC_OFFSET ? false : true);
}
