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
                          app.h  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    email                : marcel.ionescu@nivis.com
 ***************************************************************************/
#ifndef APP_H
#define APP_H

#include "Common.h"
#include "ModulesActivity.h"

#include "../Shared/ObjList.h"

//lint -library

/**main application's class
    *all apps will derivate from this their main body
    *@author Marcel Ionescu
    */
class CApp
{
public:
	CModulesActivity	m_oModulesActivity;

	/* public functions declaration. */
	CApp( const char * p_szAppName );
	~CApp();

	/** make any necessary initialisation  */
	int Init( const char *p_lpszLogFile, int p_nMaxLogSize = 524288 );
	void Close();

	static int IsStop() { return m_nStop; }
	static void SetStop( int p_nStop = 1 ) { m_nStop = p_nStop; }
	static const char* GetVersion();
	static const char * AppName( void ) { return m_szModule; };
	
	char		m_szAppPidFile[256];
	static int	m_nStopTimeout;
	
protected: // Private methods
	static char	m_szModule[64];
	/** return version (as a string) as defined in global.conf included by global Makefile*/
	inline const char * version( void );

	static void	HandlerSIGTERM(int);

	static volatile int		m_nStop;
	static int			m_nSyncFd;
	int				m_nStarted;
};

/// Make it visible to callers which are not using CApp
void	HandlerFATAL(int p_signal);// SIGABRT/ SIGSEGV
#endif
