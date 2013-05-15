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
* Name:        RuleDefFile.h
* Author:      Marius Chile
* Date:        14.10.2003
* Description: Definition of Rule Definitions File parser
* Changes:
* Revisions:
****************************************************/

#ifndef RULEDEFFILE_H
#define RULEDEFFILE_H

#include "IniParser.h"
#include "EventsStruct.h"

// types of errors that might occur
enum Err_Type
{
	RULE_NO_ERROR,
	RULE_MANDATORY_FIELD_MISSING,
	RULE_UNRECOGNIZED_VALUE_FORMAT,
	RULE_VALUE_TOO_LONG,
	RULE_VALUE_TOO_SHORT
};

// the return values of ReadEvent
enum Return_Type
{
	RULE_OK,
	RULE_INVALID,
	RULE_END_OF_FILE
};

// the possible value types
enum Value_Type
{
	RULE_VALUE_TYPE_DECIMAL,
	RULE_VALUE_TYPE_HEX,
	RULE_VALUE_TYPE_STRING
};

///////////////////////////////////////////////////////////////////////////////////
// Name:        CRuleDefFile
// Author:      Marius Chile
// Description: Validates the rules from a rule file and fills in a CEventsStruct
///////////////////////////////////////////////////////////////////////////////////
class CRuleDefFile : public CIniParser
{
private:
	bool 			m_bHasTime;
	CEventsStruct*	m_pEvents;

private:
	bool escapeTypeDelim(const char ** p_pszValue);

	const char * getValue(const char * p_szField, bool p_bMandatory = false);
	
	// mandatory fields
	bool validateId();
	bool validateDestinationType();
	bool validateRuleType();
	bool validateCommand();
	// conditional mandatory fields
	bool validateScheduledTime();
	bool validateEventDefinition();
	bool validateDestinationId();
	// optional fields
	bool validateActive();
	bool validateMaxRetries();
	bool validateRetryInterval();
	bool validateForceStartupSend();
	bool validateStartTime();
	bool validateEndTime();

	bool validateValue(const char * p_szValue, void* p_pvDestBuf, int p_nMaxDestSize,
					   unsigned short * p_pnWritten, bool p_bNumeric = true);
	
public: 
	CRuleDefFile();
	~CRuleDefFile();

	Return_Type ReadEvent(CEventsStruct * p_pEvents, bool p_bGetNext = false);
};

#endif
