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
#include <ostream>

#include <../AccessNode/Shared/Common.h>

//LOG_LEVEL_APP      = 1  # app log level: 4 dbg, 3 inf, 2 warn, 1 err

enum LogLevel
{
	LL_ERROR = 1,
	LL_WARN,
	LL_INFO,
	LL_DEBUG,
	LL_MAX_LEVEL
};


bool InitLogEnv(const char *pszIniFile);
bool IsLogEnabled(enum LogLevel);

void mhlog(enum LogLevel debugLevel, const std::ostream& message ) ;
void mhlog(enum LogLevel debugLevel, const char* message) ;


#define LOG_DEF(name) inline void __NOOP(){}
#define LOG_DEF_BY_CLASS(class_) LOG_DEF(class_)

#define LOG_TRACE_ENABLED() IsLogEnabled(LL_TRACE)
#define LOG_TRACE(message) __noop

#define LOG_DEBUG_ENABLED() IsLogEnabled(LL_DEBUG)
#define LOG_DEBUG(message) \
	do	\
	{	\
		if(IsLogEnabled(LL_DEBUG))		\
		{	mhlog(LL_DEBUG, std::stringstream().flush() <<message);	\
		}	\
	}while(0)

#define LOG_INFO_ENABLED() IsLogEnabled(LL_INFO)
#define LOG_INFO(message) \
	do	\
	{	\
		if(IsLogEnabled(LL_INFO))		\
		{	mhlog(LL_INFO, std::stringstream().flush() <<message);	\
		}	\
	}while(0)

#define LOG_WARN_ENABLED() IsLogEnabled(LL_WARN)
#define LOG_WARN(message) \
	do	\
	{	if(IsLogEnabled(LL_WARN))		\
	{	mhlog(LL_WARN, std::stringstream().flush() <<message);	\
	}	\
	}while(0)

#define LOG_ERROR_ENABLED() IsLogEnabled(LL_ERROR)
#define LOG_ERROR(message) \
	do	\
	{	\
		if(IsLogEnabled(LL_ERROR))		\
		{	mhlog(LL_ERROR, std::stringstream().flush() <<message);	\
		}	\
	}while(0)


#endif

