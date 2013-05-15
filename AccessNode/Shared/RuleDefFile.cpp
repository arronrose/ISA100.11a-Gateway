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
* Date:        14.10.2003
* Description: Implementation of Rule Definitions File parser
* Changes:
* Revisions:
****************************************************/

#include "RuleDefFile.h"
#include "DSTHandler.h"
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>

#define LOG_MANDATORY( func, field, idhi, idlo ) \
	LOG( "CRuleDefFile::%s - mandatory field <%s> is missing in rule <%2.2x %2.2x>", \
					 func, field, idhi, idlo )

#define LOG_BAD_VALUE( func, value, field, idhi, idlo ) \
	LOG( "CRuleDefFile::%s - could not recognize value <%s> for field <%s> in rule <%2.2x %2.2x>", \
					 func, value, field, idhi, idlo)

#define LOG_VALUE_TOO_LONG( func, value, field, idhi, idlo ) \
	LOG( "CRuleDefFile::%s - value <%s> too long for field <%s> in rule <%2.2x %2.2x>", \
					 func, value, field, idhi, idlo)

#define LOG_VALUE_TOO_SHORT( func, value, field, idhi, idlo ) \
	LOG( "CRuleDefFile::%s - value <%s> too short for field <%s> in rule <%2.2x %2.2x>", \
					 func, value, field, idhi, idlo)
						 
						 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRuleDefFile::CRuleDefFile()
{
}

CRuleDefFile::~CRuleDefFile()
{
}

///////////////////////////////////////////////////////////////////////////////////
// Name: escapeTypeDelim
// Author: Marius Chile
// Description: Jumps over the spaces and the ':' inside those spaces in a string
// Parameters: p_pszValue      - const char **      - pointer to the targeted string
// Return:  0 - ':' does not occur inside the spaces
//			1 - spaces & ':' successfully jumped over
// Note: This function is called to jump over spaces and ':' from strings of the
//		 following form: <type>[spaces]:[spaces]<value>
///////////////////////////////////////////////////////////////////////////////////
bool CRuleDefFile::escapeTypeDelim(const char ** p_pszValue)
{
	const char * szValue = *p_pszValue + 1;
	
	for ( ; isspace( *szValue ); szValue++ )
		;

	if ( *szValue != ':' )
		return false;

	for ( szValue++ ; isspace( *szValue ); szValue++ )
		;

	*p_pszValue = szValue;
				
	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// Name: validateValue
// Author: Marius Chile
// Description: Validates a value from the rule file and writes it to a buffer
// Parameters: p_szValue      - const char *      - the value to be validated
//			   p_vDestBuf     - void *            - the destination buffer
//			   p_nMaxDestSize - int               - the maximum size of the dest. buffer
//			   p_pnWritten    - unsigned short *  - the nr. of bytes written
//			   p_bNumeric     - bool			  - the value must not be a string
// Return:  0 - an error has occured or value is not valid
//			1 - value is valid, written to buffer
// Note: p_bNumeric = true means that the value to be validated must not be of type string
// <value> is of form:
// [<type>:]<value>
// <type> (optional) is:
// - x for string of hex values (this is the default). Example: x:00 B0 01 F5 03
// - n for decimal values (we don't accept strings of decimal values). Example: n: 1024.
// - string values. Example: "killall -USR2 history"
///////////////////////////////////////////////////////////////////////////////////
bool CRuleDefFile::validateValue(const char * p_szValue, void* p_pvDestBuf,
								 int p_nMaxDestSize, unsigned short * p_pnWritten,
								 bool p_bNumeric)
{
	Value_Type nType;

	// establish type	
	switch( *p_szValue )
	{
	case 'n':
	{
		nType = RULE_VALUE_TYPE_DECIMAL;

		if ( ! escapeTypeDelim( &p_szValue ) )
			return false;

		break;
	}
	case 'x':
	{
		nType = RULE_VALUE_TYPE_HEX;

		if ( ! escapeTypeDelim( &p_szValue ) )
			return false;
		
		break;
	}
	case '"':
	case '\'':
	{
		if (p_bNumeric)
			return false;
		
		nType = RULE_VALUE_TYPE_STRING;
		break;
	}
	default:
	{
		nType = RULE_VALUE_TYPE_HEX;
	}
	}
	
	// validate the value
    switch( nType )
    {
	case RULE_VALUE_TYPE_HEX:
	{
		// hex value expected
		char* pTmp = (char*)p_pvDestBuf;
		int nConverted = 0;
		unsigned int cTmp;
		while (	p_nMaxDestSize-- &&
			   ( nConverted = sscanf(p_szValue, "%2x", &cTmp) ) == 1 )
		{
			*(pTmp++) = (char)cTmp;

			for ( p_szValue += 2; isspace( *p_szValue ); p_szValue++ )
				/*do nothing*/;
		}

		if ( nConverted == 0 )
			return false;

		if ( p_pnWritten )
			*p_pnWritten = pTmp - (char*)p_pvDestBuf;

		return true;
	}
	case RULE_VALUE_TYPE_DECIMAL:
	{
		// decimal value expected
		long lTmp = 0;
		if ( sscanf(p_szValue, "%ld", &lTmp) != 1 )
			return false;

		memcpy(p_pvDestBuf, &lTmp, p_nMaxDestSize);

		if ( p_pnWritten )
			// do not return the nr. of bytes
			*p_pnWritten = 0;

		return true;
	}
	case RULE_VALUE_TYPE_STRING:
	{
		// string value expected (enclosed in quotes)
		char * pEndOfValue = ( *p_szValue == '"' ? strchr((char*)p_szValue+1, '"') :
												   strchr((char*)p_szValue+1, '\''));
		if ( ! pEndOfValue )
			return false;

		*pEndOfValue = '\0';

		EscapeString( (char*)(++p_szValue) );

		snprintf( (char*)p_pvDestBuf, p_nMaxDestSize, "%s", p_szValue );

		if ( p_pnWritten )
			*p_pnWritten = strlen( (char*)p_pvDestBuf );

		return true;
	}
	}
	
	return false;
}

///////////////////////////////////////////////////////////////////////////////////
// Name: getValue
// Author: Marius Chile
// Description: returns the value of a variable from the rule file and writes it to a buffer
// Parameters: p_szField      - const char *      - the variable name
//			   p_bMandatory   - bool			  - check for mandatory variable
// Return:  the value of <p_szField> variable or NULL (if the variable is mandatory and not found)
// Note: p_bMandatory = true means that the value to be retrieved must exist
///////////////////////////////////////////////////////////////////////////////////
const char * CRuleDefFile::getValue(const char * p_szField, bool p_bMandatory)
{
	// move to the first line after the group name
	FindGroup( NULL );

	const char * szValue = findVariable(p_szField);

	// check mandatory field
    if ( p_bMandatory && ( ! szValue || ! *szValue ) )
    {
		LOG_MANDATORY( "getValue", p_szField, m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return NULL;
	}

	return szValue;
}


/***************************************************************
// mandatory fields
***************************************************************/

bool CRuleDefFile::validateId()
{
	const char * szValue = getValue("id", true);
	if ( ! szValue ) return false;

	unsigned short nWritten;
	if ( ! validateValue(szValue, m_pEvents->m_pEventId, sizeof(m_pEvents->m_pEventId), &nWritten) )
	{
		LOG_BAD_VALUE( "validateId", szValue, "id", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	if (nWritten == 1)
	{
		m_pEvents->m_pEventId[1] = m_pEvents->m_pEventId[0];
		m_pEvents->m_pEventId[0] = 0;
	}

	return true;
}

bool CRuleDefFile::validateDestinationType()
{
	const char * szValue = getValue("destination_type", true);
	if ( ! szValue ) return false;

	if ( ! validateValue(szValue, &m_pEvents->m_cDestType, sizeof(m_pEvents->m_cDestType), NULL) )
	{
		LOG_BAD_VALUE( "validateDestinationType", szValue, "destination_type", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	return true;
}

bool CRuleDefFile::validateRuleType()
{
	const char * szValue = getValue("rule_type", true);
	if ( ! szValue ) return false;

	if ( ! validateValue(szValue, &m_pEvents->m_cType, sizeof(m_pEvents->m_cType), NULL) )
	{
		LOG_BAD_VALUE( "validateRuleType", szValue, "rule_type", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	return true;
}

bool CRuleDefFile::validateCommand()
{
	int 			nMaxData = sizeof(m_pEvents->m_pMessageData);
	char 			cCode = 0;					// the cmd code of a command
	char 			pTmpData[nMaxData];        	// the cmd data
	unsigned short 	nLen;						// the len of (cmd code +) cmd data for a command
	char 			* pTmp = m_pEvents->m_pMessageData;
	const char 		* szValue;
	int 			i;
	
	for (i = 0; FindSubgroup("command", ( i == 0 ? false : true ) ); i++)
	{
		nLen = 0;

		// read <cmd_code> (ignored for destination_type = 2)
		szValue = findVariable("code");
		if ( ( ! szValue || ! *szValue || ! validateValue(szValue, &cCode, sizeof(cCode), NULL) ) &&
			 m_pEvents->m_cDestType != 2 )
		{
			LOG_BAD_VALUE( "validateCommand", szValue, "code", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
		}

		// read <cmd_data>
		FindSubgroup(NULL);
        szValue = findVariable("data");
        if ( szValue && *szValue && ! validateValue(szValue, pTmpData, nMaxData, &nLen, false) )
		{
			LOG_BAD_VALUE( "validateCommand", szValue, "data", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
		}
		
		// verify total cmd data len
		if ( i == 0 )
		{
			nMaxData -= nLen;
		}
		else
		{
			if ( i == 1 )
				nMaxData -= sizeof(nLen) + sizeof(cCode);
			
			nMaxData -= nLen + sizeof(nLen) + sizeof(cCode);
		}
		if ( nMaxData < 0 )
		{
			LOG_VALUE_TOO_LONG( "validateCommand", szValue, "data", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
		}

		if ( i == 0 )
		{
			m_pEvents->m_cNodeCommand = cCode;
            memcpy(m_pEvents->m_pMessageData, pTmpData, nLen);
			m_pEvents->m_nDNMsgLen = nLen + 1;
		}
		else
		{
			if ( i == 1 )
			{
				// we have more than 1 command... rearrange data in event struct :
				// move the cmd data to the right and add the cmd code and cmd len
				memmove(pTmp + sizeof(nLen) + sizeof(cCode), pTmp, m_pEvents->m_nDNMsgLen);
				memcpy(pTmp + sizeof(nLen), &m_pEvents->m_cNodeCommand, sizeof(cCode));
	        	unsigned short nNetLen = htons(m_pEvents->m_nDNMsgLen);
				memcpy(pTmp, &nNetLen, sizeof(nLen));
				pTmp += m_pEvents->m_nDNMsgLen + sizeof(nLen);
			}
	
	 		// write total cmd data len = sizeof(cmd code) + sizeof(cmd data)
 			nLen += sizeof(cCode);
        	unsigned short nNetLen = htons(nLen);
 			memcpy(pTmp, &nNetLen, sizeof(nLen));
 			pTmp += sizeof(nLen);

 			// write cmd code
 			memcpy(pTmp, &cCode, sizeof(cCode));
 			pTmp += sizeof(cCode);

	 		// write cmd data
 			nLen -= sizeof(cCode);
 			memcpy(pTmp, pTmpData, nLen);
 			pTmp += nLen;
    	}
	}

	if ( i == 0 )
	{
		LOG( "CRuleDefFile::validateCommand - mandatory group <command> is missing in rule <%2.2x %2.2x>", \
						 m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	if ( i > 1 )
	{
		// linked command
		m_pEvents->m_cNodeCommand = CMD_LINKED;
		m_pEvents->m_nDNMsgLen = pTmp - m_pEvents->m_pMessageData;
	}

	return true;
}

/***************************************************************
// conditional mandatory fields
***************************************************************/

bool CRuleDefFile::validateScheduledTime()
{
	const char * szValue = getValue("scheduled_time");
	if ( szValue && *szValue )
	{
		unsigned short nWritten;
		if ( ! validateValue(szValue, &m_pEvents->m_stSchedTime, sizeof(m_pEvents->m_stSchedTime), &nWritten) )
		{
			LOG_BAD_VALUE( "validateScheduledTime", szValue, "scheduled_time", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
		}

		// all bytes must be present
		if ( nWritten < sizeof(m_pEvents->m_stSchedTime) )
		{
			LOG_VALUE_TOO_SHORT( "validateScheduledTime", szValue, "scheduled_time", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
		}

		// if rule type is 0 and DST is specified, convert scheduled time to local time
		if ( m_pEvents->m_cType == 0 )
		{
	        CDSTHandler dstHandler;
    	    int nUTCOffset;
        	if ( dstHandler.Scan(&nUTCOffset) )
            {
                unsigned char chSec = ( m_pEvents->m_stSchedTime.m_cSec == 0xff ? 0 : m_pEvents->m_stSchedTime.m_cSec );
				
			    // calculate start time in seconds. since 1970
			    struct tm tmScheduledTime = { chSec, m_pEvents->m_stSchedTime.m_cMin, m_pEvents->m_stSchedTime.m_cHour,
                                              m_pEvents->m_stSchedTime.m_cDay, m_pEvents->m_stSchedTime.m_cMon - 1, 100 + m_pEvents->m_stSchedTime.m_cYear,
                        			          0, 0, 0
#ifndef CYG
											  , 0, 0 
#endif											  
				};
			    time_t nScheduledTime;
			    if ( ( nScheduledTime = mktime(&tmScheduledTime) ) == (time_t)(-1) )
				{
					LOG("CRuleDefFile::validateScheduledTime - mktime failed for %s in rule <%2.2x %2.2x>. Scheduled Time left unchanged", szValue, m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
				}
				else
				{
					nScheduledTime -= nUTCOffset * 3600;

					struct tm* ptmScheduledTime = gmtime(&nScheduledTime);
					m_pEvents->m_stSchedTime.m_cSec = ( m_pEvents->m_stSchedTime.m_cSec == 0xff ? 0xff : ptmScheduledTime->tm_sec );
					m_pEvents->m_stSchedTime.m_cMin = ptmScheduledTime->tm_min;
					m_pEvents->m_stSchedTime.m_cHour = ptmScheduledTime->tm_hour;
					m_pEvents->m_stSchedTime.m_cDay = ptmScheduledTime->tm_mday;
					m_pEvents->m_stSchedTime.m_cMon = ptmScheduledTime->tm_mon + 1;
					m_pEvents->m_stSchedTime.m_cYear = ptmScheduledTime->tm_year - 100;
				}
		    }
		    else
		    {
				LOG("CRuleDefFile::validateScheduledTime - Cannot read DST offset. Scheduled Time left unchanged");
			}
        }
		else		
        // warn if used instead of event_definition (i.e. for rule_type 2 or 3)
        if (m_pEvents->m_cType == 2 || m_pEvents->m_cType == 3)
        {
			LOG( "CRuleDefFile::validateScheduledTime - Scheduled Time used instead of Event Definition in rule <%2.2x %2.2x>",
							 m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		}

		m_bHasTime = true;
	}

	return true;
}

bool CRuleDefFile::validateEventDefinition()
{
	const char * szValue = getValue("event_definition");
   	if ( szValue && *szValue )
   	{
   		unsigned short nWritten;
   		if ( ! validateValue(szValue, &m_pEvents->m_stMatch, sizeof(m_pEvents->m_stMatch), &nWritten) )
   		{
			LOG_BAD_VALUE( "validateEventDefinition", szValue, "event_definition", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
   		}

   		// all bytes must be present
   		if ( nWritten < sizeof(m_pEvents->m_stMatch) )
   		{
			LOG_VALUE_TOO_SHORT( "validateEventDefinition", szValue, "event_definition", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
   		}

        // warn if used instead of scheduled_time (i.e. for rule_type 0, 1 or 4)
        if (m_pEvents->m_cType == 0 || m_pEvents->m_cType == 1 || m_pEvents->m_cType == 4)
        {
   			LOG( "CRuleDefFile::validateEventDefinition - Event Definition used instead of Scheduled Time in rule <%2.2x %2.2x>",
   							 m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
   		}
   	}
    else
    {
		if ( ! m_bHasTime )
		{
			LOG( "CRuleDefFile::validateEventDefinition - rule <%2.2x %2.2x> found without scheduled_time or event_definition",
							 m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
			return false;
		}
	}

	return true;
}

bool CRuleDefFile::validateDestinationId()
{
	const char * szValue = getValue("destination_id");

	// field ignored for destination_type = 2
    if (m_pEvents->m_cDestType == 2)
    {
		return true;
	}

	// mandatory field for destination_type different than 2
	if ( ! szValue || ! *szValue )
	{
		LOG_MANDATORY( "validateDestinationId", "destination_id", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	unsigned short nWritten;
	if ( ! validateValue(szValue, &m_pEvents->m_nDestId, sizeof(m_pEvents->m_nDestId), &nWritten) )
	{
		LOG_BAD_VALUE( "validateDestinationId", szValue, "destination_id", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	// all bytes must be present
	if (nWritten < sizeof(m_pEvents->m_nDestId))
	{
		LOG_VALUE_TOO_SHORT( "validateDestinationId", szValue, "destination_id", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	return true;
}

/***************************************************************
// optional fields
***************************************************************/

bool CRuleDefFile::validateActive()
{
	const char * szValue = getValue("active");

    // defaults to 1
   	if ( ! szValue || ! *szValue )
   	{
   		m_pEvents->m_cActiveStatus = 1;
   		return true;
   	}

   	if ( ! validateValue(szValue, &m_pEvents->m_cActiveStatus, sizeof(m_pEvents->m_cActiveStatus), NULL) )
   	{
		LOG_BAD_VALUE( "validateActive", szValue, "active", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
   	}

	return true;
}

bool CRuleDefFile::validateMaxRetries()
{
	const char * szValue = getValue("max_retries");

    // field ignored for destination_type = 1
   	if (m_pEvents->m_cDestType == 1)
   	{
   		m_pEvents->m_nMaxNoRetry = 1;
   		return true;
   	}

   	// defaults to 1
   	if ( ! szValue || ! *szValue )
   	{
   		m_pEvents->m_nMaxNoRetry = 1;
   		return true;
   	}

   	unsigned short nWritten;
   	if ( ! validateValue(szValue, &m_pEvents->m_nMaxNoRetry, sizeof(m_pEvents->m_nMaxNoRetry), &nWritten) )
   	{
		LOG_BAD_VALUE( "validateMaxRetries", szValue, "max_retries", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
   	}

   	if (nWritten == sizeof(m_pEvents->m_nMaxNoRetry))
   	{
   		m_pEvents->m_nMaxNoRetry = ntohs(m_pEvents->m_nMaxNoRetry);
   	}

	return true;
}

bool CRuleDefFile::validateRetryInterval()
{
	// if max_retries is not present it defaults to 60
	const char * szMaxRetries = getValue("max_retries");
	if ( m_pEvents->m_cDestType == 1 || ! szMaxRetries || ! *szMaxRetries )
	{
   		m_pEvents->m_nNoSec = 60;
   		return true;
	}

	const char * szValue = getValue("retry_interval");
   	unsigned short nWritten;
   	if ( ! validateValue(szValue, &m_pEvents->m_nNoSec, sizeof(m_pEvents->m_nNoSec), &nWritten) )
   	{
		LOG_BAD_VALUE( "validateRetryInterval", szValue, "retry_interval", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
   	}

   	if (nWritten == sizeof(m_pEvents->m_nNoSec))
   	{
   		m_pEvents->m_nNoSec = ntohs(m_pEvents->m_nNoSec);
   	}

	return true;
}

bool CRuleDefFile::validateForceStartupSend()
{
	const char * szValue = getValue("force_startup_send");

    // field ignored for destination_type = 1
   	if (m_pEvents->m_cDestType == 1)
   	{
   		m_pEvents->m_nAccessTime = 1;
   		return true;
   	}

   	// defaults to 0 ( set access time to 1 in binary file - Claudiu knows why :) )
   	if ( ! szValue || ! *szValue )
   	{
   		m_pEvents->m_nAccessTime = 1;
   		return true;
   	}

   	if ( ! validateValue(szValue, &m_pEvents->m_nAccessTime, sizeof(m_pEvents->m_nAccessTime), NULL) )
   	{
		LOG_BAD_VALUE( "validateForceStartupSend", szValue, "force_startup_send", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
   	}

   	if (m_pEvents->m_nAccessTime == 0 || m_pEvents->m_nAccessTime == 1)
    {
   		m_pEvents->m_nAccessTime = ! m_pEvents->m_nAccessTime;
   		return true;
   	}
   	else
   	{
		LOG_BAD_VALUE( "validateForceStartupSend", szValue, "force_startup_send", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
   	}

	return true;
}

bool CRuleDefFile::validateStartTime()
{
	const char * szValue = getValue("start_time");

   	// defaults to 0
   	if ( ! szValue || ! *szValue )
   	{
   		m_pEvents->m_nActiveStartTime = 0;
   		return true;
   	}

    // parse value
	unsigned short nWritten;
	CEventsStruct::StTime stStartTime;
	if ( ! validateValue(szValue, &stStartTime, sizeof(stStartTime), &nWritten) )
	{
		LOG_BAD_VALUE( "validateStartTime", szValue, "start_time", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	// all bytes must be present
	if ( nWritten < sizeof(stStartTime) )
	{
		LOG_VALUE_TOO_SHORT( "validateStartTime", szValue, "start_time", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

    // check if value is default to skip mktime trouble
    if ( stStartTime.m_cSec == 0 && stStartTime.m_cMin == 0 && stStartTime.m_cHour == 0 &&
         stStartTime.m_cDay == 0 && stStartTime.m_cMon == 0 && stStartTime.m_cYear == 0 )
 	{
   		m_pEvents->m_nActiveStartTime = 0;
   		return true;
	}
	
    // calculate start time in seconds. since 1970
    struct tm tmStartTime = { stStartTime.m_cSec, stStartTime.m_cMin, stStartTime.m_cHour,
                              stStartTime.m_cDay, stStartTime.m_cMon - 1, 100 + stStartTime.m_cYear,
                              0, 0, 0
#ifndef CYG
							  , 0, 0 
#endif	
	};	//last 2 fields: tm_gmtoff, tm_zone

    if ( ( m_pEvents->m_nActiveStartTime = mktime(&tmStartTime) ) == (time_t)(-1) )
    {
		LOG( "CRuleDefFile::validateStartTime - mktime failed for %s in rule <%2.2x %2.2x>", szValue, m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}
 	
    // substract UTC offset from start time
    CDSTHandler dstHandler;
    int nUTCOffset;
    if ( dstHandler.Scan(&nUTCOffset) )
    {
		m_pEvents->m_nActiveStartTime -= nUTCOffset * 3600;
	}
	else
	{
		LOG("CRuleDefFile::validateStartTime - Cannot read UTC offset. Start time left unchanged");
	}
			
	return true;
}

bool CRuleDefFile::validateEndTime()
{
	const char * szValue = getValue("end_time");

   	// defaults to 0x7fffffff (max time supported by time_t)
   	if ( ! szValue || ! *szValue )
   	{
   		m_pEvents->m_nActiveEndTime = 0x7fffffff;
   		return true;
   	}

    // parse value
	unsigned short nWritten;
	CEventsStruct::StTime stEndTime;
	if ( ! validateValue(szValue, &stEndTime, sizeof(stEndTime), &nWritten) )
	{
		LOG_BAD_VALUE( "validateEndTime", szValue, "end_time", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

	// all bytes must be present
	if ( nWritten < sizeof(stEndTime) )
	{
		LOG_VALUE_TOO_SHORT( "validateEndTime", szValue, "end_time", m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

    // check if value is default to skip mktime trouble
    if ( stEndTime.m_cSec == 0xff && stEndTime.m_cMin == 0xff && stEndTime.m_cHour == 0xff &&
         stEndTime.m_cDay == 0xff && stEndTime.m_cMon == 0xff && stEndTime.m_cYear == 0xff )
 	{
   		m_pEvents->m_nActiveEndTime = 0x7fffffff;
   		return true;
	}
	
    // calculate start time in seconds. since 1970
    struct tm tmEndTime = { stEndTime.m_cSec, stEndTime.m_cMin, stEndTime.m_cHour,
                            stEndTime.m_cDay, stEndTime.m_cMon - 1, 100 + stEndTime.m_cYear,
                            0, 0, 0
#ifndef CYG
							, 0, 0 
#endif											  

	};	//last 2 fields: tm_gmtoff, tm_zone
    if ( ( m_pEvents->m_nActiveEndTime = mktime(&tmEndTime) ) == (time_t)(-1) )
    {
		LOG( "CRuleDefFile::validateEndTime - mktime failed for %s in rule <%2.2x %2.2x>", szValue, m_pEvents->m_pEventId[0], m_pEvents->m_pEventId[1]);
		return false;
	}

    // substract UTC offset from end time
    CDSTHandler dstHandler;
    int nUTCOffset;
    if ( dstHandler.Scan(&nUTCOffset) )
    {
		m_pEvents->m_nActiveEndTime -= nUTCOffset * 3600;
	}
	else
	{
		LOG("CRuleDefFile::validateEndTime - Cannot read UTC offset. End time left unchanged");
	}
				
	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// Name: ReadEvent
// Author: Marius Chile
// Description: Validates the next rule from the rule file and fills in a coresponding object
// Parameters: p_pEvents  - CEventsStruct* - the structure to be filled
//             p_bGetNext - bool		   - get next rule
// Return:  RULE_INVALID - rule is not valid
//			RULE_OK      - rule is valid, structure filled
//			END_OF_FILE  - no more rules available to the end of file
// Note: p_bGetNext = false means find only the first appearance
///////////////////////////////////////////////////////////////////////////////////
Return_Type CRuleDefFile::ReadEvent(CEventsStruct * p_pEvents, bool p_bGetNext)
{
	if ( ! FindGroup("rule", p_bGetNext) )
	{
		return RULE_END_OF_FILE;
	}

	// validate fields and fill the structure
	m_bHasTime = false;
	m_pEvents = p_pEvents;

	// mandatory fields
    if ( ! validateId() ||
    	 ! validateDestinationType() ||
		 ! validateRuleType() ||
		 ! validateCommand() ||
	// conditional mandatory fields
		 ! validateScheduledTime() ||
		 ! validateEventDefinition() ||
		 ! validateDestinationId() ||
	// optional fields
		 ! validateActive() ||
		 ! validateMaxRetries() ||
		 ! validateRetryInterval() ||
		 ! validateForceStartupSend() ||
		 ! validateStartTime() ||
		 ! validateEndTime() )
	{
		LOG( "CRuleDefFile::ReadEvent - rule <%2.2x %2.2x> rejected",
						 p_pEvents->m_pEventId[0], p_pEvents->m_pEventId[1]);
		return RULE_INVALID;
	}
	
	return RULE_OK;
}
