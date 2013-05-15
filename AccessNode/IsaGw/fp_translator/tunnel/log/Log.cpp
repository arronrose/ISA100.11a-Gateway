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


#include "../../Shared/Common.h"
#include "Log.h"

enum LogLevel logAPP = LL_INFO;

bool SetLogLevel(int logLevel)
{

	switch ((enum LogLevel)logLevel)
	{
	case LL_DEBUG:
	case LL_INFO:
	case LL_WARN:
	case LL_ERROR:
		logAPP = (enum LogLevel)logLevel;
		break;
	default:
		return false;
	}

	return true;
}


bool IsLogEnabled(enum LogLevel level)
{
	return level <= logAPP ? true : false;
}

void WriteLog(const char *pszLogLevel, const char *pszLogContent)
{
	LOG("%s - %s", pszLogLevel, pszLogContent);
}
