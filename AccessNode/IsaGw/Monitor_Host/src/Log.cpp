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



#include "Log.h"

#include <../AccessNode/Shared/Common.h>
#include <../AccessNode/Shared/Config.h>

#include <sys/types.h>
#include <sys/stat.h>


enum LogLevel logAPP;
const char *szLogLevel[LL_MAX_LEVEL]={ "NONE", "ERROR","WARN", "INFO","DEBUG"};
void mhlog(enum LogLevel debugLevel, const std::ostream& message)
{
	std::ostringstream ss;
	ss << message.rdbuf() ;
	mhlog(debugLevel, ss.str().c_str() );
}
void mhlog(enum LogLevel debugLevel, const char* message)
{
	LOG_STR( message );
}

struct LogDetails
{
	std::string		strLogFile;
	int 			FileSize;
	int				FileArchiveEnable;
	enum LogLevel 	logLevel;
};
static bool ParseIniFile(const char * fileName, const char *pszSection, LogDetails & logDetails)
{
	CIniParser oIniParser;
	if (!oIniParser.Load(fileName))
		return false;

	char szLogVarString[256] = "";
	if (!oIniParser.GetVarRawString(pszSection, "FileName", szLogVarString, sizeof(szLogVarString)))
		return false;

#ifdef HW_VR900
	logDetails.strLogFile = NIVIS_TMP;
#else
	logDetails.strLogFile = "logs/";
#endif

	logDetails.strLogFile += szLogVarString;
	logDetails.strLogFile += ".log";

	if (!oIniParser.GetVarRawString("MH_LOG", "FileMaxSize", szLogVarString, sizeof(szLogVarString)))
		return false;
	logDetails.FileSize = atoi(szLogVarString);

	//printf("log size = %d", atoi(szLogVarString));
	//fflush(stdout);

	if (!oIniParser.GetVarRawString("MH_LOG", "EnableLogArchive", szLogVarString, sizeof(szLogVarString)))
		return false;
	logDetails.FileArchiveEnable = atoi(szLogVarString);

	if (!oIniParser.GetVarRawString("MH_LOG", "LogLevel_APP", szLogVarString, sizeof(szLogVarString)))
		return false;
	logDetails.logLevel = (enum LogLevel)atoi(szLogVarString);

	return true;
}

static bool InitLogFile(LogDetails & logDetails)
{
	if (!g_stLog.Open(logDetails.strLogFile.c_str()))
	{
		if (mkdir("logs", 0777) == -1)
			return false;
		if (!g_stLog.Open(logDetails.strLogFile.c_str()))
			return false;
	}

	g_stLog.SetMaxSize(logDetails.FileSize);

#ifdef HW_VR900
	std::string logFolder = NIVIS_TMP;
#else
	std::string logFolder = "logs/";
#endif

	if (logDetails.FileArchiveEnable == 1)
		g_stLog.SetStorage(logFolder.c_str());

	return true;
}

extern void sqlitexx_EnableLog(int);
static bool SetLogLevel(enum LogLevel logLevel)
{

	switch (logLevel)
	{
	case LL_DEBUG:
	case LL_INFO:
	case LL_WARN:
	case LL_ERROR:
		logAPP = logLevel;
		break;
	default:
		return false;
	}

	sqlitexx_EnableLog((int)logAPP);
	return true;
}

bool InitLogEnv(const char *pszIniFile)
{
	LogDetails logDetails;
	if (!ParseIniFile(pszIniFile, "MH_LOG" /*section*/, logDetails))
		return false;


	if (!InitLogFile(logDetails))
		return false;

	if (!SetLogLevel(logDetails.logLevel))
		return false;

	/*
	std::ostringstream str;
	str << "asta este " << 3;

	std::string s;
	s = str.str();
	*/

	return true;
}

bool IsLogEnabled(enum LogLevel level)
{
	return level <= logAPP ? true : false;
}
