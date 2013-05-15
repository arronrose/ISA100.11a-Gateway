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

#include "LowPANTunnel.h"
#include "Shared/Utils.h"
#include "BackBoneApp.h"
#include "Shared/SignalsMgr.h"
#include "WaitingQueues.h"
#include "BbrGlobal.h"

#include "../ISA100/porting.h"
#include "Shared/log_callback.h"
#include "../ISA100/callbacks.h"
#include "../ISA100/nlme.h"
#include "../ISA100/aslde.h"
#include "../ISA100/dmap_armo.h"

CBackBoneApp::CBackBoneApp()
:CApp( "BBR" )
{
	;
}

CBackBoneApp::~CBackBoneApp()
{
	;
}

int CBackBoneApp::Init()
{
	CSignalsMgr::Install(SIGUSR1);
	CSignalsMgr::Install(SIGUSR2);
	CSignalsMgr::Install(SIGHUP);

	if( !CApp::Init( NIVIS_TMP"backbone.log") )
	{	ERR( "CBackBoneApp.Init: CApp.Init failed.");
		return 0 ;
	}

	if( !m_cfg.Init() )
	{	ERR( "CBackBoneApp.Init: Config.Init failed");
		return false ;
	}	

	LogCallback_SetFunc( &CallbackLogWithLevel );
	LogCallback_SetLevel( (LOGLVL)m_cfg.m_nLogLevelStack );
	
	Callback_SetFunc( &CBackBoneApp::SendToDL );
	Callback_SetFunc( &CBackBoneApp::SendAlertToTR );

	if( !LibIsaInit(  g_pApp->m_cfg.getBB_IPv6(), g_pApp->m_cfg.getSM_IPv6(), g_pApp->m_cfg.getBB_Port(), g_pApp->m_cfg.m_pu8BbrEUI64, g_pApp->m_cfg.m_u8SecurityManager, g_pApp->m_cfg.m_u8AppJoinKey, 1, DEVICE_TYPE_BBR, 0, g_pApp->m_cfg.m_nCrtUTCAdj) )
	{	//FATAL, cannot continue
		return 0;
	}

	TLDE_Duplicate_Init(g_pApp->m_cfg.m_nTldeHListSize, g_pApp->m_cfg.m_nTldeHListTimeWindow);	

	LibIsaProvisionDMAP( VERSION, "BACKBONE", g_pApp->m_cfg.m_szBBRTag );
	
	if (!m_obTunnel.Start())
	{	
		return 0;
	}
	LOG("CBackBoneApp::Init: done");

	return true ;
}

int CBackBoneApp::SendToDL (IPv6Packet* p_pIPv6, int p_nLen)
{
	uint16_t u16SrcAddr;
	uint16_t u16DstAddr;

	bool bHasAtt = Get6LowPanAddrs( p_pIPv6, p_nLen, u16SrcAddr, u16DstAddr );

	if (!bHasAtt)
	{
		//return 0; //quick fix

		//add to waiting queue
		ProtocolPacket* pPktForTR = new ProtocolPacket;

		if (!pPktForTR->AllocateCopy((uint8_t*)p_pIPv6, p_nLen))
		{
			delete pPktForTR;
			return 0;
		}
		
		CMsgWaitATT::Ptr pPtr (new CMsgWaitATT);

		pPtr->Load(pPktForTR,CMsgWaitATT::DIR_TR_TO);
		
		g_pApp->m_obTunnel.GetWaitListToTr()->push_back(pPtr);

		return 1;
	}

	ProtocolPacket* pPktForTR = g_pApp->m_obTunnel.IpV6ToLowPAN(p_pIPv6, p_nLen, u16SrcAddr, u16DstAddr);

	if (!pPktForTR)
	{
		return 0;
	}
	
	//send packet to TR
	if (g_pApp->m_obTunnel.AddToTty(pPktForTR))
	{	delete pPktForTR;
	}
	
	return 1;
}	


int CBackBoneApp::SendAlertToTR( const uint8_t* p_pAlertHeader, int p_nHeaderLen, const uint8_t* p_pData, int p_nDataLen)
{
	return g_pApp->m_obTunnel.SendAlertToTR(p_pAlertHeader, p_nHeaderLen, p_pData, p_nDataLen );
}


void CBackBoneApp::Run()
{
	timer1s.SetTimer(1000);
	

	while( !CApp::IsStop() )
	{	
		RecvUDP(1000);// recv SM or GW requests and fwd ISA packet down the wire -> will call SendToDL

		ASLDE_ASLTask(); //send responses back to SM

		m_obTunnel.Run(); //read packets from the TR
		
		//TODO: add code to clean UAP 1-15 incoming queues only to be safe

		if (timer1s.IsSignaling()) // 1sec Tasks (timeout checks, mostly)
		{
			TouchPidFile(m_szAppPidFile);

			timer1s.SetTimer(1000);
			ASLDE_PerformOneSecondOperations();
		}
		

		if (CSignalsMgr::IsRaised(SIGHUP)) 
		{
			m_cfg.Reload();
			LogCallback_SetLevel( (LOGLVL)m_cfg.m_nLogLevelStack );
			CSignalsMgr::Reset(SIGUSR2);
		}
		
		if (CSignalsMgr::IsRaised(SIGUSR2)) 
		{
			m_cfg.Reload();
			LogCallback_SetLevel( (LOGLVL)m_cfg.m_nLogLevelStack );

			//m_obTunnel.LogRoutes();
			LOG("******* ISA STACK status BEGIN *******");
			SLME_PrintKeys();
			DMO_PrintContracts();
			NLME_PrintContracts();
			NLME_PrintRoutes();
			NLME_PrintATT();
			LOG("******* ISA STACK status END   *******");
			CSignalsMgr::Reset(SIGUSR2);
		}

		if (CSignalsMgr::IsRaised(SIGUSR1)) 
		{
			//generateAlert();
			LOG("Signal SIGUSR1 set");
			m_obTunnel.ReadTmpCmd();
			CSignalsMgr::Reset(SIGUSR1);
		}
	}

}

void CBackBoneApp::generateAlert()
{
	uint16_t u16Port = 0xf0b0;
	ALERT stAlert;

	stAlert.m_ucPriority = TLME_ILLEGAL_USE_OF_PORT; //g_stTlme.m_aAlertDesc[p_unAlertType] & 0x7F;
	stAlert.m_unDetObjTLPort = 0xF0B0; // TLME is DMAP port
	stAlert.m_unDetObjID = DMAP_TLMO_OBJ_ID; 
	stAlert.m_ucClass = ALERT_CLASS_EVENT; 
	stAlert.m_ucDirection = ALARM_DIR_IN_ALARM; 
	stAlert.m_ucCategory = ALERT_CAT_COMM_DIAG; 
	stAlert.m_ucType = TLME_ILLEGAL_USE_OF_PORT; 
	stAlert.m_unSize = sizeof(u16Port); 

	ARMO_AddAlertToQueue( &stAlert, (uint8_t*)&u16Port );
}
