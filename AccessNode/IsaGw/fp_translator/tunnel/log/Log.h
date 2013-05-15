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

#ifndef NLIB_LOG_H_ //we replace the "log.h" from nlib
#define NLIB_LOG_H_

#include <string>
#include <sstream>

//LOG_LEVEL_APP      = 1  # app log level: 4 dbg, 3 inf, 2 warn, 1 err

enum LogLevel
{
	LL_DEBUG = 4,
	LL_INFO = 3,
	LL_WARN = 2,
	LL_ERROR = 1
};

//bool SetLogLevel(int logLevel);
bool IsLogEnabled(enum LogLevel);
void WriteLog(const char *pszLogLevel, const char *pszLogContent);


#define LOG_DEBUG(message) \
		do { \
			if(IsLogEnabled(LL_DEBUG)) { \
				std::ostringstream str; \
				str << message; \
				WriteLog("DEBUG", str.str().c_str()); \
			} \
		} while(0);


#define LOG_INFO(message) \
		do { \
			if(IsLogEnabled(LL_INFO)) { \
				std::ostringstream str; \
				str << message; \
				WriteLog("INFO", str.str().c_str()); \
			} \
		} while(0);

#define LOG_WARN(message) \
		do { \
			if(IsLogEnabled(LL_WARN)) { \
				std::ostringstream str; \
				str << message; \
				WriteLog("WARN", str.str().c_str()); \
			} \
		} while(0);

#define LOG_ERROR(message) \
		do { \
			if(IsLogEnabled(LL_ERROR)) { \
				std::ostringstream str; \
				str << message; \
				WriteLog("ERROR", str.str().c_str()); \
			} \
		} while(0);


#define LOG_INFO_ENABLED() IsLogEnabled(LL_INFO)
#define LOG_DEBUG_ENABLED() IsLogEnabled(LL_DEBUG)

#endif

