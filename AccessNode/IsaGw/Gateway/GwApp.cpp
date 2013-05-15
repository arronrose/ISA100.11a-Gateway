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

/// @file GwApp.cpp
/// @brief Definitions for the entire gateway application
#include "../../Shared/log_callback.h"
#include "../../Shared/Utils.h"
#include "../../Shared/SignalsMgr.h"

#include "GwApp.h"
#include "../ISA100/callbacks.h"
#include "TcpGSAP.h"


CGwApp g_stApp; ///< the gateway application object

static void sRequestTimeout( uint16 p_ushAppHandle, uint8 p_ucSrcSAP, uint8 p_ucDstSAP, uint8 p_ucSFC );
static void sContractNotification( uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pNetAddr );
static void sOnJoin( bool p_bJoined );

extern "C" void c_updateSMAddr64( const uint8 * /*p_pSMAddr64*/ );
		
CGwApp::CGwApp()
:CApp( "isa_gw" )
{}

CGwApp::~CGwApp()
{}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief On USR2: re-load some config variables and dump the full application status
////////////////////////////////////////////////////////////////////////////////
void CGwApp::USR2_Handler( void )
{
	HUP_Handler();
	Dump();
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief On HUP: re-load some config variables
////////////////////////////////////////////////////////////////////////////////
void CGwApp::HUP_Handler( void )
{
	m_stCfg.Reload();
	LogCallback_SetLevel( m_stCfg.getStackLogLevel() );
}


/// @brief Mandatory initializations
///
/// This method initializes various stuff inside the ISA100 stack, then opens the isa_gw.log, reads the configuration from config.ini,
/// opens the two sockets, initializes the interface code between the Gateway Application and the ISA100 stack, creates the first entry
/// in m_RoutingTable pointing to the SM based on the configuration, initializes the timers and the ccm encryption engine.
/// @retval 0 Failure
/// @retval 1 Success
int CGwApp::Init()
{
	/// Start with 10M log, a big enough value so the log is not truncated on start
	/// Then on GENERAL section, the log should be set high
	if( !CApp::Init( NIVIS_TMP"isa_gw.log", 1024*1024*10) )
	{
		LOG( "CGwApp::Init failed.");
		return 0 ;
	}
	
	if (!m_stCfg.Init())
	{
		return 0;
	}

	LogCallback_SetFunc( &CallbackLogWithLevel );	// The callback should be initialized before any LOG
	LogCallback_SetLevel( m_stCfg.getStackLogLevel() );
	
	if( !LibIsaInit( m_stCfg.getGW_IPv6(), m_stCfg.getSM_IPv6(), m_stCfg.getGW_UDPPort(),
			m_stCfg.m_pu8GWEUI64, m_stCfg.m_u8SecurityManager, m_stCfg.m_u8AppJoinKey,
			m_stCfg.getPingTimeout(), DEVICE_TYPE_GW, (uint16)m_stCfg.m_nSubnetID, m_stCfg.m_nCrtUTCAdj ) )
	{	//FATAL, cannot continue
		return 0;
	}

	TLDE_Duplicate_Init(m_stCfg.m_nTldeHListSize, m_stCfg.m_nTldeHListTimeWindow);

	LibIsaProvisionDMAP( VERSION, "GATEWAY", m_stCfg.m_szGWTag );
	LibIsaRegisterCallbacks( &sRequestTimeout, &sOnJoin, &sContractNotification );
			
	CSignalsMgr::Install( SIGUSR2 );
	CSignalsMgr::Install( SIGHUP );

	/// Here we should wait a little then check for g_stApp.IsStop().
	/// It is NOT mandatory: main loop will check g_stApp.IsStop() anyway,
	/// and we do not know how long to wait
			
	LOG("Initializing DMAP and GwUAP");
	DMAP_Init();

	if( !m_oGSAPServer.StartServer( g_stApp.m_stCfg.getGSAP_TCPPort(), 20 ) )
	{
		LOG("ERROR CGwApp::Init: Cannot listen on GSAP port %d. Another instance is running?", g_stApp.m_stCfg.getGSAP_TCPPort() );
		return 0;
	}

	if( g_stApp.m_stCfg.getYGSAP_TCPPort() )	// if 0, do not expose YGSAP interface
	{
		if( !m_oYGSAPServer.StartServer( g_stApp.m_stCfg.getYGSAP_TCPPort(), 20 ) )
		{
			LOG("ERROR CGwApp::Init: Cannot listen on port %d for GSAP. Another instance is running?", g_stApp.m_stCfg.getYGSAP_TCPPort() );
			return 0;
		}
	}

	if( g_stApp.m_stCfg.getYGSAP_TCPPort_SSL() )	// if 0, do not expose YGSAP ssl interface
	{
		if( !m_oYGSAPServer_SSL.StartServer( g_stApp.m_stCfg.getYGSAP_TCPPort_SSL(), 20 ) )
		{
			LOG("ERROR CGwApp::Init: Cannot listen on port %d for ssl GSAP. Another instance is running?", g_stApp.m_stCfg.getYGSAP_TCPPort_SSL() );
			return 0;
		}
	}
	
	if( g_stApp.m_stCfg.getGSAP_TCPPort_SSL() )	// if 0, do not expose GSAP ssl interface
	{
		if( !m_oGSAPServer_SSL.StartServer( g_stApp.m_stCfg.getGSAP_TCPPort_SSL(), 20 ) )
		{
			LOG("ERROR CGwApp::Init: Cannot listen on port %d for GSAP. Another instance is running?", g_stApp.m_stCfg.getGSAP_TCPPort_SSL() );
			return 0;
		}
	}

	m_oGwUAP.Init();
	
	timer100ms.SetTimer(100);
	timer1s.SetTimer(1000);
	LOG("CGwApp::Init ok");
	return 1;	//all ok
}

CTcpGSAP * CGwApp::lookForNewTcpSession(CServerSocket& p_rServer, bool p_bSsl/* = false*/)
{
	if( !p_rServer.CheckRecv( 1000 ) )
	{
		return NULL;
	}

	CTcpSocket * pClientConn = 0;
	if (p_bSsl)
	{
		CTcpSecureSocket* secureSocket = new CTcpSecureSocket();

		bool rc = secureSocket->InitSSL( CTcpSecureSocket::SERVER_SIDE,
			g_stApp.m_stCfg.m_szSslServerKey,
			g_stApp.m_stCfg.m_szSslServerCertif,
			g_stApp.m_stCfg.m_szSslCaCertif );

		if(!rc) 
		{	
			delete secureSocket;
			return NULL;
		}

		pClientConn = secureSocket;
	}
	else
	{
		pClientConn = new CTcpSocket();
	}
	
	if( p_rServer.Accept( pClientConn ))
	{	
		CTcpGSAP * pGSAP = new CTcpGSAP( pClientConn );
		LOG("New GSAP connection from %s", pGSAP->Identify() );
		///add the client connection (TCP GSAP) to the GSAP list
		m_oGwUAP.AddGSAP( pGSAP );

		return pGSAP; /// ONLY place returnin non-NULL
	}
	else	// signal or something else interrupted accept
	{
		LOG("CGwApp::lookForNewTcpSession: accept() INTERRUPTED");
		delete pClientConn;
	}
	return NULL;
}

#define LOG2FLASH_IDLE_TIMEOUT 3  // number of idle (no-connection) seconds before flushing the buffer
#define LOG2FLASH_MIN_DISTANCE 20 // min distance (seconds) between log2flash (exception: flush when buffer is full)
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief log2flash with protection to aggressive connects
/// @param p_tLastLog time of last log2flash
/// @param p_szBuf text buffer with connection identifiers in last seconds
/// @param p_unBufSize total size of p_szBuf
/// @param p_szOneEntry text buffer with current connection identifier
/// @param p_unOneEntrySize total size of p_szOneEntry
/// @return time_t the current time
/// @remarks it logs2flash when:
/// @remarks 	buffer is full or
/// @remarks 	enough time has passed since last log2flash
/// @remarks in second case it also log all connection occured since last log2flasf (stored in buffer)
/// @remarks if not enough time has passed (aggressive logging) it does not log2flash
/// @remarks 	but it store the connection into buffer for logging latter
/// @todo TODO make a class CLog2flashBuffered
////////////////////////////////////////////////////////////////////////////////
time_t log2flash_protected( time_t& p_tLastLog, char * p_szBuf, unsigned p_unBufSize, const char * p_szOneEntry, unsigned p_unOneEntrySize )
{
	strncat( p_szBuf, p_szOneEntry, p_unBufSize - strlen(p_szBuf) - 1);
	time_t tNow = time( NULL );

	if( tNow - p_tLastLog < LOG2FLASH_MIN_DISTANCE )
	{
		if( p_unBufSize -  strlen(p_szBuf) - 1 < p_unOneEntrySize) ///no space left for one more
		{
			log2flash( "ISA_GW: Connection:%s", p_szBuf );
			p_szBuf[0] = 0;
			p_tLastLog = tNow;
		}
	}
	else
	{
		log2flash( "ISA_GW: Connection:%s", p_szBuf );
		p_szBuf[0] = 0;
		p_tLastLog = tNow;
	}
	return tNow;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief log2flash (flush) the buffer on idle (no connection)
/// @param p_tLastLog time of last log2flash
/// @param p_tLastConnect time of last activity (connect)
/// @param p_szBuf buffer storing recent conneciton identifiers
/// @todo TODO make a class CLog2flashBuffered
////////////////////////////////////////////////////////////////////////////////
void log2flash_maintenance( time_t& p_tLastLog, time_t& p_tLastConnect, char * p_szBuf )
{
	if( p_szBuf[0] )
	{	time_t tNow = time(NULL);	/// do not get time each pass..
		if( tNow - p_tLastConnect >= LOG2FLASH_IDLE_TIMEOUT ) /// idle (no connection) in last 3 seconds
		{	log2flash( "ISA_GW: Connection:%s", p_szBuf );
			p_szBuf[0] = 0;
			/// comment: allow fresh connects to get logged immediately if the buffer is empty (more aggressive)
			/// uncomment: ensure time interval between log2flash except when buffer fills up
			p_tLastLog = tNow;
		}
	}
}
#define LOG2FLASH_BUF_SIZE 256
#define LOG2FLASH_ONE_ENTRY_SIZE 40
void CGwApp::lookForNewTcpSessions( void )
{
	static time_t tLastLog		= 0;
	static time_t tLastConnect	= 0;/// TODO use CMicroSec
	static char szBuf[ LOG2FLASH_BUF_SIZE ] = {0};
	char szTmp[ LOG2FLASH_ONE_ENTRY_SIZE ];

	log2flash_maintenance( tLastLog, tLastConnect, szBuf );
	
	CTcpGSAP * pGSAP = NULL;

	if ( (pGSAP = lookForNewTcpSession(m_oGSAPServer)) )
	{	snprintf( szTmp, sizeof(szTmp), " %s(%s)", "GSAP", pGSAP->Identify() );
		tLastConnect = log2flash_protected( tLastLog, szBuf, sizeof(szBuf), szTmp, sizeof(szTmp) );
	}

	///check for tcp sessions in the enabled servers
	if ( g_stApp.m_stCfg.getYGSAP_TCPPort() && ((pGSAP = lookForNewTcpSession(m_oYGSAPServer))) )
	{	snprintf( szTmp, sizeof(szTmp), " %s(%s)", "YGSAP", pGSAP->Identify() );
		tLastConnect = log2flash_protected( tLastLog, szBuf, sizeof(szBuf), szTmp, sizeof(szTmp) );
	}

	if ( g_stApp.m_stCfg.getGSAP_TCPPort_SSL() && ((pGSAP = lookForNewTcpSession(m_oGSAPServer_SSL, true))) )
	{	snprintf( szTmp, sizeof(szTmp), " %s(%s)", "GSAP SSL", pGSAP->Identify()  );
		tLastConnect = log2flash_protected( tLastLog, szBuf, sizeof(szBuf), szTmp, sizeof(szTmp) );
	}

	if ( g_stApp.m_stCfg.getYGSAP_TCPPort_SSL() && ((pGSAP = lookForNewTcpSession(m_oYGSAPServer_SSL, true)) ))
	{	snprintf( szTmp, sizeof(szTmp), " %s(%s)", "YGSAP SSL", pGSAP->Identify() );
		tLastConnect = log2flash_protected( tLastLog, szBuf, sizeof(szBuf), szTmp, sizeof(szTmp) );
	}
}

/// @brief Main application loop
///
/// Unless we have been sent a kill signal, repeat the following:
///	- try to read an ISA packet and send it up the stack
///	- run the Application code to deal with any application ISA packet received
///	- run the DMAP code to deal with any management ISA packet received
///	- try to read a Host Application packet and either send a response or create an ISA packet for it
///	- run the ISA Application Queue code to clear processed packets and send pending packets down the stack
///	- run periodic housekeeping tasks (TouchPidFile, expire timed out packets, check SM pings
int CGwApp::Run()
{
	while (!IsStop())
	{
		DMAP_Task();
		lookForNewTcpSessions();
		m_oGwUAP.GwUAP_Task();
		ASLDE_ASLTask(); // actually pushes one ISA packet down the wire
		
/*		if( timer100ms.IsSignaling() ) // 100ms Tasks (do we really need this?)
		{
			DMAP_CheckTenthCounter();
			timer100ms.SetTimer(100);
		}
*/		
		if( timer1s.IsSignaling() ) // 1sec Tasks (timeout checks, mostly)
		{
			ASLDE_PerformOneSecondOperations();
			DMAP_DMO_CheckNewDevInfoTTL();
			TouchPidFile(m_szAppPidFile);
			DMAP_CheckSecondCounter();

//			DMO_PerformOneSecondTasks(); DMAP_CheckSecondCounter() takes care of this
//			DMAP_DMO_CheckSMLink(); DMAP_CheckSecondCounter() takes care of this

			timer1s.SetTimer(1000);
		}
		
		if( CSignalsMgr::IsRaised( SIGUSR2 ) )
		{
			USR2_Handler();
			CSignalsMgr::Reset(SIGUSR2);
		}
		if( CSignalsMgr::IsRaised( SIGHUP ) )
		{
			HUP_Handler();
			CSignalsMgr::Reset(SIGHUP);
		}
	}
	return 1;
}

/// @brief Cleanup method
void CGwApp::Close()
{
	//worker services shutdown: if we delete contracts, then the shutdown must be done in the main loop, involving communications
	LibIsaShutdown();
	
	LOG("CGwApp::Close CLOSING socket");
	m_oGSAPServer.Close();
	m_oYGSAPServer.Close();
	CApp::Close();
}

void CGwApp::Dump( void )	///< Dump to log the gateway status
{
	LOG("******* ISA STACK status BEGIN *******");
	SLME_PrintKeys();
	DMO_PrintContracts();
	NLME_PrintContracts();
	NLME_PrintRoutes();
	ASLDE_Dump();
	LOG("******* ISA STACK status END   *******");
	m_oGwUAP.Dump();
}

/// This is a housekeeping function called at join time by the DMAP, when the EUI-64 address of the SM
/// has just been received and must be filled in the m_RoutingTable
/// @param p_pSMAddr64 the EUI-64 address of the SM
extern "C" void c_updateSMAddr64( const uint8 * /*p_pSMAddr64*/ )
{
//	ROUTING_ENTRY * pSMRoute = (ROUTING_ENTRY*) g_stApp.m_RoutingTable.FindFirstEntry();
//	memcpy( pSMRoute->m_aAddr64, p_pSMAddr64, sizeof(pSMRoute->m_aAddr64) );
//	LOG_HEX( "Updated the SM Addr64:", pSMRoute->m_aAddr64, sizeof(pSMRoute->m_aAddr64));
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Callback to notify ISA request timeout
/// @param p_ushAppHandle the request application handle 
/// @param p_ucSrcSAP source SAP (should be always me, ISA100_GW_UAP)
/// @param p_ucDstSAP destination SAP
/// @param p_ucSFC service failure code: always some failure
////////////////////////////////////////////////////////////////////////////////
static void sRequestTimeout( uint16 p_ushAppHandle, uint8 p_ucSrcSAP, uint8 p_ucDstSAP, uint8 /*p_ucSFC*/ )
{	/// p_ucSFC is always failure in this version, no need to pass it through
	if(p_ucSrcSAP == ISA100_GW_UAP)
		g_stApp.m_oGwUAP.DispatchISATimeout( p_ushAppHandle );
	else if (p_ucSrcSAP != ISA100_DMAP_PORT - ISA100_START_PORTS) // the DMAP does not need timeout notifications
		LOG("ERROR DispatchISATimeout(H %u DstSAP %u): cannot handle SrcSAP %u. IGNORED", p_ushAppHandle, p_ucDstSAP, p_ucSrcSAP);
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Callback to notify contract created / deleted
/// @param p_nContractId the contract id
/// @param p_pContract pointer to contract attributes
/// @remarks
///     p_pContract != NULL notifies contract created
///     p_pContract == NULL notifies contract deleted
////////////////////////////////////////////////////////////////////////////////
static void sContractNotification( uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract )
{
	g_stApp.m_oGwUAP.ContractNotification( p_nContractId, p_pContract );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Callback to notify join/unjoin
/// @param p_bJoined true when joined, false when unjoined
////////////////////////////////////////////////////////////////////////////////
static void sOnJoin( bool p_bJoined )
{
	g_stApp.m_oGwUAP.OnJoin( p_bJoined );
}
