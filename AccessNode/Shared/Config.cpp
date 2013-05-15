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
                          Config.cpp  -  description
                             -------------------
    begin                : Oct 22 2003
    email                : ion.ticus@nivis.com
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "Config.h"
#include "app.h"

CConfig::CConfig()
{
    memset( m_szModule, 0, sizeof(m_szModule) );
}

CConfig::~CConfig()
{
    // Release() is not needed here because Release() is called in the parent's 
    // destructor. 

    // Release();
}

int CConfig::Init( const char * p_szModule, const char * p_szFileName )
{
    LOG( "CConfig::Init" );
    
    m_nIsMasterAN = NO_RF_MEMBER;
    
    if( ! Load( p_szFileName )
	|| ! CIniParser::FindGroup("GLOBAL")
	|| ( CIniParser::GetVar(NULL, "AN_ID",(unsigned char*)(&m_stAddress), sizeof(m_stAddress)) != sizeof(m_stAddress) )
	|| ( CIniParser::GetVar(NULL, "APP_ID",(unsigned char*)(&m_shAppId), sizeof(m_shAppId)) != sizeof(m_shAppId) )
	|| (m_shAppId == 0)
	|| ((*(int*)&m_stAddress) == 0)
	)
	{                                                                                                            
	    LOG( "PANIC CConfig::Init: section GLOBAL  not found, or AN_ID/APP_ID are wrong");
	    Release();    // calling parent's Release()
	    return 0;
	}
	//LOG("CConfig::Init: AN_ID=%s APP_ID=%s", GetHex(&m_stAddress,sizeof(m_stAddress)), GetHex(&m_shAppId,sizeof(m_shAppId)));
	LOG("CConfig::Init: AN_ID=%s APP_ID=%s", GetHexT(&m_stAddress), GetHexT(&m_shAppId));
	m_ppModulesToWatch[0] = 0;

	strncpy( m_szModule, p_szModule, sizeof(m_szModule)-1 );

	int nLogSize;
	bool bTail;
	int nMoveTimeout;
	char szStorage[PATH_MAX];
	char temp[ 16 ];
	int nFFree;
	
	READ_DEFAULT_VARIABLE_INT ( "MODULES_STOP_TIMEOUT", CApp::m_nStopTimeout, 30 ); //seconds

	READ_DEFAULT_VARIABLE_INT ( "MAX_LOG_SIZE", nLogSize, 512 ); //kBytes

	g_stLog.SetMaxSize( nLogSize * 1024 );
	READ_DEFAULT_VARIABLE_STRING ( "LOG_STORAGE", szStorage, "");



//	g_stLog.SetStorage( szStorage);  will do it only once, because it's heavy
	READ_DEFAULT_VARIABLE_YES_NO ( "TAIL_SAFE", bTail, "NO" );
	g_stLog.SetTail( bTail);
	READ_DEFAULT_VARIABLE_INT ( "MOVE_TIMEOUT", nMoveTimeout, 120 ); //seconds
	g_stLog.SetMoveTimeout( nMoveTimeout);
	READ_DEFAULT_VARIABLE_INT ( "LOG_FLASH_FREE_LIMIT", nFFree, 5120 ); //kBytes
	g_stLog.SetFlashFreeLimit( nFFree );

	char szMaster[64];

	if( !GetVar( "Master_RF", szMaster, sizeof( szMaster ), 0, 1 ) )
	{
		return 0;
	}
	else
	{
		if( !strcasecmp( szMaster, "YES" ) )
		{
			/*LOG( "AN is MASTER RF Node" );*/
			m_nIsMasterAN = RF_MASTER_AN;
		}
		else
		{
			if( !strcasecmp( szMaster, "NO" ) )
			{
				/*LOG( "AN is SLAVE RF Node" );*/
				m_nIsMasterAN = RF_SLAVE_AN;
			}
			else
			{
				/*LOG( "NO RF master / slave specification : \"%s\", asuming no RF Member", szMaster );*/
				m_nIsMasterAN = NO_RF_MEMBER;
			}
		}
	}

 	if ( CIniParser::FindGroup( p_szModule ) == NULL )
		return 0;

	if( GetVar("MAX_LOG_SIZE", &nLogSize) ){
		LOG( "Override value for MAX_LOG_SIZE = %d", nLogSize);
		g_stLog.SetMaxSize( nLogSize * 1024 );
	}
	if( GetVar("TAIL_SAFE", temp, sizeof( temp)) ){
		bTail=( !strcasecmp( temp, "Y") || !strcasecmp( temp, "YES") || !strcasecmp( temp, "1") || !strcasecmp( temp, "TRUE"));
		LOG( "Override value for TAIL_SAFE = %s", bTail?"TRUE":"FALSE");
		g_stLog.SetTail( bTail );
	}
	if( GetVar("MOVE_TIMEOUT", &nMoveTimeout) ){
		LOG( "Override value for MOVE_TIMEOUT = %d", nMoveTimeout);
		g_stLog.SetMoveTimeout( nMoveTimeout);
	}
	if( GetVar("LOG_STORAGE", szStorage, sizeof( szStorage)) ){
		LOG( "Override value for LOG_STORAGE = %s", szStorage);
	}

	g_stLog.SetStorage( szStorage);  //we didn't do it before


	ReadAndSetLogLevel();
	return 1;
}

const char * CConfig::FindGroup(const char * p_szGroup, bool p_bGetNext, bool p_bLogErrors )
{
	char szGroup[64];
	sprintf( szGroup, "%.31s_%.31s", m_szModule, p_szGroup );

	return
		CIniParser::FindGroup( szGroup, p_bGetNext, p_bLogErrors );
}

int CConfig::GetVarAsBaudRate( const char * p_szVarName, int * p_pnValue, int p_nPos )
{
     char szBaudRate[16];
     if( !GetVar( p_szVarName, szBaudRate, sizeof(szBaudRate), p_nPos ) )
          return 0;

     int  nBaudRate = GetBaudRate( szBaudRate );
     if( nBaudRate == -1 )
          return 0;

     *p_pnValue = nBaudRate;
     return 1;
}

int CConfig::GetBaudRate( const char * p_szBaudRate )
{
	if(  *p_szBaudRate == 'B' )
	{
		switch( atoi( p_szBaudRate+1 ) )
		{
		case 50: return B50;
		case 75: return B75;
		case 110: return B110;
		case 134: return B134;
		case 150: return B150;
		case 200: return B200;
		case 300: return B300;
		case 600: return B600;
		case 1200: return B1200;
		case 1800: return B1800;
		case 2400: return B2400;
		case 4800: return B4800;
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		case 230400: return B230400;
		}
	}

	LOG( "CConfig::GetBaudRate - unknown baud rate [%s]", p_szBaudRate );
	return -1;
}

int CConfig::GetInterfaceNo(const char *p_szInterface)
{
    int nInterface;

    if( ! strcmp(p_szInterface,"000") )      nInterface = INTREFACE_LOCAL_ARM;
    else if( ! strcmp(p_szInterface,"1xx") ) nInterface = INTERFACE_SPI;
	else if( ! strcmp(p_szInterface,"lg") ) nInterface = INTERFACE_MESH;
	else if( ! strcmp(p_szInterface,"mesh") ) nInterface = INTERFACE_MESH;
	else if( ! strcmp(p_szInterface,"local") ) nInterface = INTERFACE_LOCALDN;
    else
    {
        LOG( "Config::GetInterfaceNo() - invalid interface :%s", p_szInterface );
        return INTERFACE_UNK;
    }
	return nInterface;
}


void CConfig::ReadAndSetLogLevel()
{
	if ( CIniParser::FindGroup( "GLOBAL" ) == NULL )
		return;

	char szlogLvl[32];
	READ_DEFAULT_VARIABLE_STRING ( "LOG_LEVEL", szlogLvl, "ERR");

	NLOG_LVL nLogLevel = NLogLevelGetType(szlogLvl);

	LOG(" CConfig::ReadAndSetLogLevel: GLOBAL/LOG_LEVEL = %s -> %s", szlogLvl, NLogLevelGetName(nLogLevel));

	if (nLogLevel != NLOG_LVL_UNK)
	{
		g_nNLogLevel = nLogLevel;
	}
	else LOG(" CConfig::ReadAndSetLogLevel: GLOBAL/LOG_LEVEL unk -> use default: %s", NLogLevelGetName(g_nNLogLevel));


	if ( CIniParser::FindGroup( m_szModule ) == NULL )
		return;

	READ_DEFAULT_VARIABLE_STRING ( "LOG_LEVEL", szlogLvl, "ERR");

	nLogLevel = NLogLevelGetType(szlogLvl);

	LOG(" CConfig::ReadAndSetLogLevel: %s/LOG_LEVEL = %s -> %s", m_szModule, szlogLvl, NLogLevelGetName(nLogLevel));

	if (nLogLevel != NLOG_LVL_UNK)
	{
		g_nNLogLevel = nLogLevel;
	}
	else LOG(" CConfig::ReadAndSetLogLevel: %s/LOG_LEVEL unk -> use default: %s", m_szModule, NLogLevelGetName(g_nNLogLevel));
}
