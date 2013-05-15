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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         January 2008
/// Description:  This file implements the application layer queue
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

#include "../../Shared/MicroSec.h"
#include "porting.h"
#include "../../Shared/DurationWatcher.h"

#include "string.h"
#include "aslde.h"
#include "aslsrvc.h"
#include "tlde.h"
#include "dmap.h"
#include "dmap_armo.h"
#include "uap.h"


#ifdef UT_ACTIVED
#if( (UT_ACTIVED == UT_ASL_ONLY) )
#include "../UnitTests/ASL/unit_test_ASL.h"
#endif
#endif

#define SM_ADDR_HI_BYTE (((uint8*)&g_stDMO.m_unSysMngShortAddr)[1])
#define SM_ADDR_LO_BYTE (((uint8*)&g_stDMO.m_unSysMngShortAddr)[0])


/// expect 3 unsigned parameters: m_unContractID, m_nInUseWndSize, m_nCrtWndSize
#define SLIDING_WND_FMT "Sliding window (C %3u) usage %u/%u"

/*  RX/TX APDU buffers  */
CApduListRx g_oAslMsgListRx;
unsigned g_nAslMsgListRxCount;
CApduListTx g_oAslMsgListTx; // TX

/* APDU confirmation array  */
static APDU_CONFIRM_IDTF g_astAPDUConf[MAX_APDU_CONF_ENTRY_NO];
static uint8  g_ucAPDUConfNo;

uint8 g_ucHandle = 0;
extern uint8 g_bNativePublish;

/* Tx  APDU buffer; used by TL also! */
uint8  g_oAPDU[MAX_APDU_SIZE + MIC_SIZE];

/* ASLMO object definition and ASLMO alert counters.  */
ASLMO_STRUCT  g_stASLMO;
const uint16  c_unMaxASLMOCounterNo = MAX_ASLMO_COUNTER_N0;  // number of devices for which counts can be simultaneously maintained

/* ASLMO counters */
MALFORMED_APDU_COUNTER g_astASLMOCounters[MAX_ASLMO_COUNTER_N0];
uint16                 g_unCounterNo;    

const DMAP_FCT_STRUCT c_aASLMOFct[ASLMO_ATTR_NO] = {
	{0,   0,                                              DMAP_EmptyReadFunc,       NULL},
	{ATTR_CONST(g_stASLMO.m_bMalformedAPDUAdvise),        DMAP_ReadUint8,           DMAP_WriteUint8},
	{ATTR_CONST(g_stASLMO.m_ulCountingPeriod),            DMAP_ReadUint32,          DMAP_WriteUint32},
	{ATTR_CONST(g_stASLMO.m_unMalformedAPDUThreshold),    DMAP_ReadUint16,          DMAP_WriteUint16},
	{ATTR_CONST(g_stASLMO.m_stAlertDescriptor),           DMAP_ReadAlertDescriptor, DMAP_WriteAlertDescriptor},
	{ATTR_CONST(c_unMaxASLMOCounterNo),                   DMAP_ReadUint16,          DMAP_WriteUint16}
};


/***************************** local functions ********************************/

static uint8 ASLDE_searchHighestPriority(APDU_IDTF_TX * p_pIdtf, NLME_CONTRACT_ATTRIBUTES ** p_ppContract);

static uint8 * ASLDE_concatenateAPDUs(  uint16      p_unContractID,
									  uint8 *     p_pOutBuf,
									  uint8 *     p_pOutBufEnd,
									  uint8       p_ucHandle,
									  NLME_CONTRACT_ATTRIBUTES * p_pContract );

static void ASLDE_flushAPDUBuffer(void);

static void ASLDE_checkTLConfirms(void);

static void ASLDE_cleanAPDUBuffer(void);

static void ASLDE_retryMessage(CAslTxMsg::Ptr& p_pTxMsg);

static uint8 ASLMO_reset(uint8 p_ucResetType);

static void ASLDE_computeTXtimeout(NLME_CONTRACT_ATTRIBUTES * p_pContract, uint32 p_nRTT);
static int ASLDE_SlidingWindowAllowsTX(NLME_CONTRACT_ATTRIBUTES * p_pNLContract);
static int ASLDE_SlidingWindowIncreaseUsage(NLME_CONTRACT_ATTRIBUTES * p_pNLContract);
static void ASLDE_ReceiveResponseForSlidingWindow(NLME_CONTRACT_ATTRIBUTES * p_pContract);
static void ASLDE_ReceiveTimeoutForSlidingWindow(NLME_CONTRACT_ATTRIBUTES * p_pContract);
static int ASLDE_BandwidthLimitAllowsTX(NLME_CONTRACT_ATTRIBUTES * p_pNLContract);
static void ASLDE_RecordPacketNextTXTime(NLME_CONTRACT_ATTRIBUTES * p_pNLContract);

CProcessTimes	g_oProcessStartTime;
static unsigned g_unTaskTimeSpentReal = 0;
static unsigned g_unTaskTimeSpentProc = 0;
static unsigned g_nTaskCounter = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Initializes the common Rx/Tx APDU buffer and the confirmation buffer
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_Init(void)
{
	g_ucAPDUConfNo = 0;
	g_oAslMsgListTx.clear();
	g_oAslMsgListRx.clear();
	g_nAslMsgListRxCount = 0;
  
  // init aslmo object and counters
  memset(&g_stASLMO, 0, sizeof(g_stASLMO));
  memset(&g_astASLMOCounters, 0, sizeof(g_astASLMOCounters));
  //g_ulResetCounter = g_stASLMO.m_ulCountingPeriod; // default counting period is zero
  
  // all ASLMO attributes default values are zero, except the alert descriptor priority
  g_stASLMO.m_stAlertDescriptor.m_ucPriority = ALERT_PR_MEDIUM_M;
}


void ASLDE_LogLoad ()
{
	double dMin1, dMin5, dMin15, dTotal;
	if (!GetProcessLoad (dMin1, dMin5, dMin15, dTotal, GET_PROCESS_LOAD_AT_ONE_MIN))
	{	return;
	}
	
	char szTmp[2048];

	sprintf(szTmp, "ProcessLoad: 1/5/15 min: %.2f/%.2f/%.2f total: %.2f", dMin1, dMin5, dMin15, dTotal );
	LOG_ISA100 (LOGLVL_ERR, "%s ASLDE_ASLTask Avg real %dms used %dms ", szTmp,
		g_nTaskCounter ? (g_unTaskTimeSpentReal / g_nTaskCounter) : 0,
		g_nTaskCounter ? (g_unTaskTimeSpentProc / g_nTaskCounter) : 0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Performs application layer message management (app queue state machine)
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_ASLTask(void)
{	
	//CMicroSec uSec;
	//int nElapsed[2];

	++g_nTaskCounter;

	//CProcessTimes oProcTimeStart;

	ASLDE_checkTLConfirms();
	//nElapsed[0] = uSec.GetElapsedMSec();

	ASLDE_flushAPDUBuffer();
	//nElapsed[1] = uSec.GetElapsedMSec();

	//g_unTaskTimeSpentReal += nElapsed[1];
	
	//CProcessTimes oProcTimeEnd;

	//g_unTaskTimeSpentProc += oProcTimeEnd.ProcGetTotalDiff(oProcTimeStart);

	


	//if(		oProcTimeEnd.ProcGetTotalDiff(oProcTimeStart) > DurationWatcherProc 
	//	||	oProcTimeEnd.RealGetDiff(oProcTimeStart) > DurationWatcherReal )
	//{	char szBuf[ 256 ] = {0,};
	//	sprintf( szBuf, "ms real %d+%-3d=%3d Avg %d|%d %3d(proc)=%3d(usr)+%3d(sys)",
	//				nElapsed[0], nElapsed[1]-nElapsed[0], nElapsed[1], 
	//				g_nTaskCounter ? (g_unTaskTimeSpentReal/g_nTaskCounter) : 0, g_nTaskCounter ? (g_unTaskTimeSpentProc / g_nTaskCounter) : 0  
	//				, oProcTimeEnd.ProcGetTotalDiff(oProcTimeStart) 
	//				, oProcTimeEnd.ProcGetUserDiff(oProcTimeStart) 
	//				, oProcTimeEnd.ProcGetSysDiff(oProcTimeStart) );

	//	LOG_ISA100(	(nElapsed[1] > 250) ? LOGLVL_ERR : LOGLVL_INF,
	//			"%s ASLDE_ASLTask %d times [%35s] RX %d TX %2d Contracts %d Routes %d",
	//			(nElapsed[1] > 250) ?  "WARN" : "    ", g_nTaskCounter, szBuf, g_nAslMsgListRxCount,
	//			g_oAslMsgListTx.size(), g_stNlme.m_oContractsMap.size(), g_stNlme.m_oNlmeRoutesMap.size() );
	//}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei, Marcel Ionescu
/// @brief  searches for an RX message that belongs to the requester's SAP ID
/// @param  p_ucTSAPID - SAP ID of the caller
/// @param p_nTimeoutUSec - the max timeout, specified in microseconds (sec*10^-6)
///	The method will wait up to p_nTimeoutUSec for an UDP message only if there are no APDU's to process
/// If there are APDU's in the buffer, the method is guaranteed to return immediately (not necessarily returning an APDU, the APDU's )
/// @param  p_pAddr    - output: destination of the highest priority APDU
/// @return pointer to the received TSDU, NULL if no APDU is available for TSAPID
/// @remarks
///      Access level: user level
/// @remarks [Marcel]: If there are APDU's to process, the method does not wait
///    for UDP data, but check the UDP socket and get an eventual message - which may produce a new APDU
/// If there are no APDU's in the buffer, the UDP select() is done with timeout - we have no other job anyway
/// Take care: the APDU might be for some other TSAPID, this is why we always check the UDP socket
/// A second reason would be - get UDP message as soon as possible from the socket
/// The user is free to specify p_unTimeoutUSec and sleep()/LibIsaCheckRecv() in some other place
///
/// Reccomended usage: use a reasonable small timeout in ALL calls to ASLDE_GetMyAPDU.
///	The app's will not wait for each other: the method returns immediately if there are APDU's in the buffer
///
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLDE_GetMyAPDU( uint8 p_ucTSAPID, int p_unTimeoutUSec, APDU_IDTF* p_pIdtf )
{
	// If we have RX APDU's in buffer, check UDP without timeout, to ensure immediate return
	// No APDU's in buffer - we might as well wait for UDP data, there is nothing else to do
	if (g_nDeviceType != DEVICE_TYPE_BBR) //just in case protection on BBR
	{
		if (p_unTimeoutUSec > 0 && !g_oAslMsgListRx.empty())
		{
			p_unTimeoutUSec = 0;			
		}

		RecvUDP( p_unTimeoutUSec ); /// We have already cut p_unTimeoutUSec to 0 if it was necessary
	}

	return ASLDE_GetMyAPDUfromBuff(p_ucTSAPID,p_pIdtf);
}



static uint8_t s_pGetMyAPDU_Buffer[64*1024];
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Searches for an RX message that belongs to the requester's SAP ID
/// @param  p_ucTSAPID - SAP ID of the caller
/// @param  p_pIdtf    - output: highest priority APDU information
/// @return pointer to the received TSDU
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLDE_GetMyAPDUfromBuff( uint8 p_ucTSAPID, APDU_IDTF* p_pIdtf )
{
	CApduListRx::iterator it;
	static unsigned nLastLogged;
	uint32 now = MLSM_GetCrtTaiSec();
	for (it = g_oAslMsgListRx.begin(); it != g_oAslMsgListRx.end(); ++it)
	{
		CAslRxMsg::Ptr pRxMsgHdr = *it;

		// search for an RX TSDU
		if ( pRxMsgHdr->m_stApduIdtf.m_ucDstTSAPID == p_ucTSAPID
			&& pRxMsgHdr->m_ucStatus == APDU_NOT_PROCESSED )
		{
			memcpy (p_pIdtf, &pRxMsgHdr->m_stApduIdtf, sizeof(APDU_IDTF) );

			g_bNativePublish = (pRxMsgHdr->m_stApduIdtf.m_ucSrcTSAPID == UAP_DMAP_ID) ? 0 : 1;

			//just for safety live unused sizeof(APDU_IDTF)
			memcpy(s_pGetMyAPDU_Buffer + sizeof(APDU_IDTF), &pRxMsgHdr->m_oVecData[0], p_pIdtf->m_unDataLen);
			--g_nAslMsgListRxCount;
			if (nLastLogged + 10 < now && now-pRxMsgHdr->m_nRecvTime > 2)
			{// if the current RX message has been received more than 2 seconds before, log this situation once every 10 seconds
				LOG_ISA100(LOGLVL_INF, "ASL_Q_RX:%d packets waiting, current delay:%d s", g_nAslMsgListRxCount, now-pRxMsgHdr->m_nRecvTime );
				nLastLogged = now;
			}
			g_oAslMsgListRx.erase(it);

			return s_pGetMyAPDU_Buffer + sizeof(APDU_IDTF);
		}
	}

	return NULL; // nothing Rx TSDU found
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Searches for a TX message for a specified contract
/// @param  p_unContractID  - the contract ID
/// @return 
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_DiscardAPDU( uint16 p_unContractID )
{
#warning This is only needed for sending publish packets. Or maybe not even then. Should we fix it?
/*  uint8 * pData = g_aucAPDUBuff;

  // check timeout of APDUs in the ASL queue
  while (pData < g_pAslMsgQEnd)
  {
      if( ((ASL_QUEUE_HDR*)pData)->m_ucDirection == TX_APDU
          && 2 == ((ASL_QUEUE_HDR*)pData)->m_ucPeerAddrLen
          && ((ASL_QUEUE_HDR*)pData)->m_stInfo.m_stTx.m_stDest.m_unContractId == p_unContractID )
      {
          ((ASL_QUEUE_HDR*)pData)->m_ucStatus = APDU_READY_TO_CLEAR;          
          
          if( ((ASL_QUEUE_HDR*)pData)->m_ucStatus == APDU_SENT_WAIT_JUST_TL_CNF )
          {
              // if the APDU is already on the DLL queue, try to discard that message also
              DLDE_DiscardMsg( ((ASL_QUEUE_HDR*)pData)->m_stInfo.m_stTx.m_ucConcatHandle );
          }
          return; 
      }
      
      pData += ((ASL_QUEUE_HDR*)pData)->m_unLen;
  }*/
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Searches for a TX message that belongs to the requester's SAP ID
/// @param  p_ucReqID       - the request ID of the APDU that has to be found
/// @param  p_unDstObjID    - destination objID, to which the request was sent
/// @param  p_unSrcObjID    - destination objID, from which the request was sent
/// @param  p_pAPDUIdtf     - pointer to the identifier of response APDU
/// @param  p_pLen          - output parameter: pointer to length of APDU
/// @return pointer to the APDU or NULL if not found
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLDE_SearchOriginalRequest( uint8    p_ucReqID,
									uint16       p_unDstObjID,
									uint16       p_unSrcObjID,
									APDU_IDTF *  p_pAPDUIdtf,
									uint16 * p_pLen,
									uint16 *     p_punAppHandle )
{
//	LOG_ISA100(LOGLVL_DBG, "ASLDE_SearchOriginalRequest: searching reqid:%d, doid:%d, soid:%d, SrcTSAP:%d, DstTSAP:%d in %d elements",
//		p_ucReqID, p_unDstObjID, p_unSrcObjID, p_pAPDUIdtf->m_ucDstTSAPID, p_pAPDUIdtf->m_ucSrcTSAPID, g_oAslMsgListTx.size());
	CApduListTx::iterator it;
	for (it = g_oAslMsgListTx.begin(); it != g_oAslMsgListTx.end(); it++)
	{
		CAslTxMsg::Ptr pTxMsg = *it;
		
		if ( !( IS_ASL_SVRC_REQUEST(pTxMsg->m_oTxApduData[0])
			&& pTxMsg->m_stQueueHdr.m_unDstOID == p_unDstObjID
			&& pTxMsg->m_stQueueHdr.m_unSrcOID == p_unSrcObjID
			&& pTxMsg->m_stQueueHdr.m_ucReqID == p_ucReqID
			&& pTxMsg->m_stQueueHdr.m_ucSrcSAP == p_pAPDUIdtf->m_ucDstTSAPID
			&& pTxMsg->m_stQueueHdr.m_ucDstSAP == p_pAPDUIdtf->m_ucSrcTSAPID )
			)
		{
//			LOG_ISA100(LOGLVL_DBG,"tried and failed: reqid:%d, doid:%d, soid:%d, SrcTSAP:%d, DstTSAP:%d",
//				p_ucReqID, p_unDstObjID, p_unSrcObjID, p_pAPDUIdtf->m_ucSrcTSAPID, p_pAPDUIdtf->m_ucDstTSAPID);
			continue;
		}

		if (pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_CLEAR)
		{
			//just to be safe
			LOG_ISA100(LOGLVL_ERR, "ASLDE_SearchOriginalRequest: ReqID=%d DstObjID=%d SrcObjID=%d - msg already cleared", p_ucReqID, p_unDstObjID, p_unSrcObjID);
			continue;
		}

		CNlmeContractPtr pContract = pTxMsg->m_pWeakContract.lock();

		if (pContract && !memcmp( pContract->m_aDestAddress, p_pAPDUIdtf->m_aucAddr, sizeof p_pAPDUIdtf->m_aucAddr ))
		{
			*p_pLen = pTxMsg->m_stQueueHdr.m_unAPDULen;
			if(p_punAppHandle) *p_punAppHandle = pTxMsg->m_stQueueHdr.m_unAppHandle;
			// since we now have the contract, it's a good time to compute the TX timeout
			ASLDE_computeTXtimeout( pContract.get(), MLSM_GetCrtTaiSec()-pTxMsg->m_stQueueHdr.m_unSendTime );
			// this moment also marks the successful completion of a successful transaction for the sliding window. Tell them.
			if(pTxMsg->m_stQueueHdr.m_bInSlidingWindow){
				ASLDE_ReceiveResponseForSlidingWindow(pContract.get());
			}
			LOG_ISA100(LOGLVL_DBG,"Got a response: delete the TX APDU handle %u", pTxMsg->m_stQueueHdr.m_unAppHandle);
			pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
			return &pTxMsg->m_oTxApduData[0];
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASLDE_CheckAPDUTimeoutCountersOnRX
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief  Manages the timeout counters of the RX queue APDUs
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
///      Context: Must be called every second
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_CheckAPDUTimeoutCountersOnRX(void)
{
	int nCrtTime = MLSM_GetCrtTaiSec();
	 
	for (CApduListRx::iterator itRxMsg = g_oAslMsgListRx.begin(); itRxMsg != g_oAslMsgListRx.end(); )
	{
		CAslRxMsg::Ptr pRxMsg = *itRxMsg;

		if (pRxMsg->m_nRecvTime + APP_QUE_TTL_SEC > nCrtTime)
		{
			break;/// The list is ordered oldest first, it is safe to interrupt parsing
		}
			
		if (pRxMsg->m_ucStatus != APDU_READY_TO_CLEAR)
		{
			LOG_ISA100(LOGLVL_ERR,"WARNING, APDU %s:%u -> %u (status %d) old Rx:%+d. DELETED, not handled by APP",
				GetHex(pRxMsg->m_stApduIdtf.m_aucAddr, sizeof pRxMsg->m_stApduIdtf.m_aucAddr),
				pRxMsg->m_stApduIdtf.m_ucSrcTSAPID, pRxMsg->m_stApduIdtf.m_ucDstTSAPID, pRxMsg->m_ucStatus, pRxMsg->m_nRecvTime - nCrtTime);
		}
		itRxMsg = g_oAslMsgListRx.erase( itRxMsg );
		--g_nAslMsgListRxCount;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// ASLDE_CheckAPDUTimeoutCountersOnTX
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief  Manages the timeout counters of the TX queue APDUs 
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
///      Context: Must be called every second
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_CheckAPDUTimeoutCountersOnTX(void)
{
	CApduListTx::iterator it;
	for (it = g_oAslMsgListTx.begin(); it != g_oAslMsgListTx.end(); it++)
	{
		CAslTxMsg::Ptr pTxMsg = *it;

		if( APDU_READY_TO_CLEAR != pTxMsg->m_stQueueHdr.m_ucStatus && !--pTxMsg->m_stQueueHdr.m_unLifetime)
		{
			LOG_ISA100(LOGLVL_ERR, "WARNING: found really old TX APDU (C %d, H %d) in state %d. Deleting and sending timeout to user",
						pTxMsg->m_stQueueHdr.m_nContractId, pTxMsg->m_stQueueHdr.m_unAppHandle, pTxMsg->m_stQueueHdr.m_ucStatus);
			if(pTxMsg->m_pWeakContract.lock() && pTxMsg->m_stQueueHdr.m_bInSlidingWindow){
				ASLDE_ReceiveResponseForSlidingWindow(pTxMsg->m_pWeakContract.lock().get());
			}
			pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
			// notify other UAPs about this failure;
			UAP_DataTimeout( pTxMsg->m_stQueueHdr.m_unAppHandle,pTxMsg->m_stQueueHdr.m_ucSrcSAP, pTxMsg->m_stQueueHdr.m_ucDstSAP, SFC_TIMEOUT);
		}
		if(		pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_CLEAR 
			||	pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND 
			||  pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND_NO_FC )
		{
			continue;
		}

		if (  pTxMsg->m_stQueueHdr.m_unTimeout )
		{
			 pTxMsg->m_stQueueHdr.m_unTimeout--;
			continue;
		}

		if(  pTxMsg->m_stQueueHdr.m_ucStatus != APDU_CONFIRMED_WAIT_RSP){
			LOG_ISA100(LOGLVL_ERR, "WARNING: found an expired TX APDU (H %d) in state %d. Retrying it...",  pTxMsg->m_stQueueHdr.m_unAppHandle,  pTxMsg->m_stQueueHdr.m_ucStatus);
		}
		// call APDU retry function
		ASLDE_retryMessage(pTxMsg);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Manages the timeout counters of the queue APDUs, and ASLMO malformed APDU time intervals
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
///      Context: Must be called every second
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_PerformOneSecondOperations(void)
{
	//WATCH_DURATION_INIT_DEF(oDurationWatcher);
	// check timeout of APDUs in the ASL queue
	ASLDE_CheckAPDUTimeoutCountersOnRX();
	ASLDE_CheckAPDUTimeoutCountersOnTX();
	//clean ready to clear messages
	ASLDE_cleanAPDUBuffer();
	SLME_KeyUpdateTask();
	ASLDE_LogLoad();
  
  // check ASLMO malformed APDU counters time interval    
  for (uint16 unIdx = 0; unIdx < g_unCounterNo; unIdx++)
  {                            
      // check ASLMO reset counter
      if (g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter)
      {
          if( g_astASLMOCounters[unIdx].m_ulResetCounter )
          {
              g_astASLMOCounters[unIdx].m_ulResetCounter--;     
          }
          else 
          {
              // the time interval from the first detected malformed APDU expired
              // without the threshold being met; ASL counters shall be reset to zero;
              g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter = 0;
              g_astASLMOCounters[unIdx].m_ulThresholdExceedTime  = 0;
              g_astASLMOCounters[unIdx].m_ulResetCounter = g_stASLMO.m_ulCountingPeriod;
              
          }
          if (g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter >= g_stASLMO.m_unMalformedAPDUThreshold)
          {              
               g_astASLMOCounters[unIdx].m_ulThresholdExceedTime++; 
               g_astASLMOCounters[unIdx].m_ulResetCounter = g_stASLMO.m_ulCountingPeriod; // do not let reset timer to expire
          }    
      }  
  } 
  //WATCH_DURATION_DEF(oDurationWatcher);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Performs retry strategy for unsuccesfull Tx APDUs
/// @param  p_pMsgHdr - pointer to message header
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_retryMessage(CAslTxMsg::Ptr& p_pTxMsg)
{
	CNlmeContractPtr pNLContract = p_pTxMsg->m_pWeakContract.lock();

	LOG_ISA100(LOGLVL_INF,"ASLDE_retryMessage WARNING: timeout detected for contract %d (%s), handle %u (%d retries left)",
		p_pTxMsg->m_stQueueHdr.m_nContractId, pNLContract ? "found" : "NOT FOUND!!", p_pTxMsg->m_stQueueHdr.m_unAppHandle, p_pTxMsg->m_stQueueHdr.m_ucRetryNo);

	if(pNLContract)
	{
		ASLDE_ReceiveTimeoutForSlidingWindow(pNLContract.get());
	}

	if (p_pTxMsg->m_stQueueHdr.m_ucRetryNo)
	{
		p_pTxMsg->m_stQueueHdr.m_ucRetryNo--;
		p_pTxMsg->m_stQueueHdr.m_unDefTimeout *= 2;
		p_pTxMsg->m_stQueueHdr.m_unDefTimeout = p_pTxMsg->m_stQueueHdr.m_unDefTimeout>60 ? 60 : p_pTxMsg->m_stQueueHdr.m_unDefTimeout;
		p_pTxMsg->m_stQueueHdr.m_unTimeout = p_pTxMsg->m_stQueueHdr.m_unDefTimeout;
		p_pTxMsg->m_stQueueHdr.m_ucStatus  = APDU_READY_TO_SEND_NO_FC;
		LOG_ISA100(LOGLVL_INF,"ASLDE_retryMessage: resending packet for contract %d with timeout %ds",
			p_pTxMsg->m_stQueueHdr.m_nContractId, p_pTxMsg->m_stQueueHdr.m_unTimeout);
	}
	else
	{   // all retries have failed; delete the APDU;
		// notify the sliding window mechanism to free one slot
		if(pNLContract && p_pTxMsg->m_stQueueHdr.m_bInSlidingWindow)
		{
			ASLDE_ReceiveResponseForSlidingWindow(pNLContract.get());
		}
		p_pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
		LOG_ISA100(LOGLVL_INF,"ASLDE_retryMessage WARNING: one APDU (handle %d) deleted because all retries have failed", p_pTxMsg->m_stQueueHdr.m_unAppHandle );
		// notify other UAPs about this failure;
		UAP_DataTimeout( p_pTxMsg->m_stQueueHdr.m_unAppHandle,p_pTxMsg->m_stQueueHdr.m_ucSrcSAP, p_pTxMsg->m_stQueueHdr.m_ucDstSAP, SFC_TIMEOUT);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Searches for the highest priority APDU
/// @param  p_pIdtf  - attribute identifier structure
/// @param  p_pNLContract - [out] the contract to send the highest priority APDU on
/// @return service feedback code
/// @remarks
///      Access level: user level
///      Context: Called by ASLDE_Concatenate();
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ASLDE_searchHighestPriority(APDU_IDTF_TX * p_pIdtf, NLME_CONTRACT_ATTRIBUTES ** p_ppContract)
{
	int nTotalPackets = 0;
	int nTXReadyPackets = 0;
	int nDelayedBySlidingWnd = 0;
	int nDelayedByBandLimit = 0;
	static uint32 nLastLogged;
	static int	nLastTotalLogged = 0;	

	p_pIdtf->m_ucPriorityAndFlags = 0;
	p_pIdtf->bAPDUFound = false;

	CApduListTx::iterator itNext;
	for (itNext = g_oAslMsgListTx.begin(); itNext != g_oAslMsgListTx.end(); )
	{
		CApduListTx::iterator it = itNext++;
		CAslTxMsg::Ptr pTxMsg = *it;
				
		// search for ready to send TX messages		

		++nTotalPackets;
		if (	(pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND || pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND_NO_FC) 
		&&	(!p_pIdtf->bAPDUFound || (pTxMsg->m_stQueueHdr.m_ucPriorityAndFlags & 0x0F) > (p_pIdtf->m_ucPriorityAndFlags & 0x0F)))
		{
				CNlmeContractPtr pTmpContract = pTxMsg->m_pWeakContract.lock();

				if( pTmpContract )
				{
					if( pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND && !ASLDE_SlidingWindowAllowsTX( pTmpContract.get() ))
					{
						if(!pTxMsg->m_stQueueHdr.m_bDelayedBySlidingWnd){
							LOG_ISA100( LOGLVL_INF, SLIDING_WND_FMT " delays APDU (H %u)", pTmpContract->m_unContractID,
								pTmpContract->m_nInUseWndSize, pTmpContract->m_nCrtWndSize, pTxMsg->m_stQueueHdr.m_unAppHandle );
							pTxMsg->m_stQueueHdr.m_bDelayedBySlidingWnd = 1;
						}
						++nDelayedBySlidingWnd;
						continue;
					}
					if(pTxMsg->m_stQueueHdr.m_ucObeyContractBandwidth && !ASLDE_BandwidthLimitAllowsTX( pTmpContract.get() ))
					{
						if (!pTxMsg->m_stQueueHdr.m_bDelayedByBandLimit){
							LOG_ISA100( LOGLVL_INF,"Bandwidth limit(Contract %3u) delays APDU (H %u)", pTmpContract->m_unContractID, pTxMsg->m_stQueueHdr.m_unAppHandle);
							pTxMsg->m_stQueueHdr.m_bDelayedByBandLimit = 1;
						}
						++nDelayedByBandLimit;
						continue;
					}
				}
				else
				{
					LOG_ISA100(LOGLVL_INF, "WARNING: delete the TX APDU handle %u because it needs inexistent contract %u", 
										pTxMsg->m_stQueueHdr.m_unAppHandle, pTxMsg->m_stQueueHdr.m_nContractId);

					pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
										// notify other UAPs about this failure;
					UAP_DataTimeout( pTxMsg->m_stQueueHdr.m_unAppHandle, pTxMsg->m_stQueueHdr.m_ucSrcSAP,pTxMsg->m_stQueueHdr.m_ucDstSAP, SFC_NO_CONTRACT);
					g_oAslMsgListTx.erase(it);
					continue;
				}

			*p_ppContract = pTmpContract.get();//will always be non-NULL
			ASLDE_loadIDTF( p_pIdtf, &pTxMsg->m_stQueueHdr );
			++nTXReadyPackets;
		}
		
	}

	if ( p_pIdtf->bAPDUFound )
	{
		return SFC_SUCCESS;
	}
	else
	{
		uint32 now = MLSM_GetCrtTaiSec();
		if (nLastLogged != now &&  nTotalPackets != nLastTotalLogged)
		{
			LOG_ISA100(LOGLVL_INF, "ASL_Q_TX:%d Ready:%d FlowCtrl-wait:%d Bandwidth-wait:%d",
				nTotalPackets, nTXReadyPackets, nDelayedBySlidingWnd, nDelayedByBandLimit );
			nLastLogged = now;
			nLastTotalLogged = nTotalPackets;
		}
		return SFC_ELEMENT_NOT_FOUND;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Attaches the confirmation status of the TL of an TSDU, to the corresponding APDUs in the
///         common Rx/Tx APDU buffer
/// @param  none
/// @return none
/// @remarks HANDLEs in ASL layer are in the 0x00 - 0xFF range
///      Access level: interrupt level
///////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_checkTLConfirms( void )
{
	APDU_CONFIRM_IDTF * pConf = g_astAPDUConf;

	while (pConf < (g_astAPDUConf + g_ucAPDUConfNo))
	{
		CApduListTx::iterator itNext;
		for (itNext = g_oAslMsgListTx.begin(); itNext != g_oAslMsgListTx.end(); )
		{
			CApduListTx::iterator it = itNext++;
			CAslTxMsg::Ptr pTxMsg = *it;
			

			if( (pTxMsg->m_stQueueHdr.m_ucStatus == APDU_SENT_WAIT_JUST_TL_CNF || pTxMsg->m_stQueueHdr.m_ucStatus == APDU_SENT_WAIT_CNF_AND_RESPONSE)
			&& (pTxMsg->m_stQueueHdr.m_ucConcatHandle == pConf->m_ucHandle) )
			{
				if ( pConf->m_ucConfirmStatus == SFC_SUCCESS )
				{
					if ( pTxMsg->m_stQueueHdr.m_ucStatus == APDU_SENT_WAIT_JUST_TL_CNF )
					{   LOG_ISA100(LOGLVL_DBG,"successfully sent; no response expected; delete the TX APDU handle %u", pTxMsg->m_stQueueHdr.m_unAppHandle);
						pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
						g_oAslMsgListTx.erase(it);
					}
					else if ( pTxMsg->m_stQueueHdr.m_ucStatus == APDU_SENT_WAIT_CNF_AND_RESPONSE )
					{   // successfully sent; wait for the response
						pTxMsg->m_stQueueHdr.m_ucStatus = APDU_CONFIRMED_WAIT_RSP;
					}
				}
				else
				{
					if( SFC_NO_CONTRACT == pConf->m_ucConfirmStatus )
					{
						//check if publishing contract
//								
//						CO_STRUCT* pstCO = FindConcentratorByContract(pHdr->m_stInfo.m_stTx.m_nContractID);
//						if( pstCO )
//						{
//							pstCO->m_stContract.m_ucStatus = CONTRACT_REQUEST_TERMINATION;
//						}
						//if no contract the message will be discarded - no retry needed 
						LOG_ISA100(LOGLVL_ERR,"ASLDE_checkTLConfirms: SFC_NO_CONTRACT delete the TX APDU handle %u", pTxMsg->m_stQueueHdr.m_unAppHandle);
						pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
						g_oAslMsgListTx.erase(it);

					}
					else
					{
						LOG_ISA100(LOGLVL_INF,"TX confirm status %d for handle %u, let it retry in 2 seconds", pConf->m_ucConfirmStatus, pTxMsg->m_stQueueHdr.m_unAppHandle);
						pTxMsg->m_stQueueHdr.m_unTimeout = 2;
					}
				}
			}
		}
		pConf++;
	}
	g_ucAPDUConfNo = 0;
}


class PredicateReadyToCLearTx
{

public:
	bool operator()	(const CAslTxMsg::Ptr& p_pAslMsg )
	{
		
		
		return p_pAslMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_CLEAR;
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Deletes the TX APDUs marked as APDU_READY_TO_CLEAR
/// @param  none
/// @return none
/// @remarks HANDLEs in ASL layer are in the 0x00 - 0xFF range
///      Access level: interrupt level
///////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_cleanAPDUBuffer(void)
{
	int nTxSize = g_oAslMsgListTx.size();
	g_oAslMsgListTx.remove_if(PredicateReadyToCLearTx());
	nTxSize -= g_oAslMsgListTx.size();

	if (nTxSize)
	{
		LOG_ISA100(LOGLVL_DBG, "ASL Delete TX:%d", nTxSize);
	}
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Handles a application layer queue element, constructs the APDU and commits it to the
///         Transport Layer
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
///////////////////////////////////////////////////////////////////////////////////////////////////
void ASLDE_flushAPDUBuffer(void)
{
	uint8 * pEnd;
	APDU_IDTF_TX stAPDUIdtf;
	NLME_CONTRACT_ATTRIBUTES * pNLContract = NULL;
	//WATCH_DURATION_INIT_DEF(oDurationWatcher);

	if ( SFC_SUCCESS != ASLDE_searchHighestPriority(&stAPDUIdtf, &pNLContract) )
		return; // nothing found

	//WATCH_DURATION_DEF(oDurationWatcher);

	if (pNLContract->m_unAssignedMaxNSDUSize <= 8+4+4) // //TL header(8), security header(4), mic(4)
	{
		LOG_ISA100(LOGLVL_ERR,"PROGRAMMER ERROR: contractID=%d m_unAssignedMaxNSDUSize=%d <= %d", pNLContract->m_unContractID, pNLContract->m_unAssignedMaxNSDUSize, 8+4+4);
	}	
	// concatenate all APDUs for on same contract
	pEnd =  ASLDE_concatenateAPDUs( pNLContract->m_unContractID,
		g_oAPDU,
		g_oAPDU + pNLContract->m_unAssignedMaxNSDUSize-8-4-4, //TL header(8), security header(4), mic(4)
		g_ucHandle,
		pNLContract);

	//WATCH_DURATION_DEF(oDurationWatcher);
	TLDE_DATA_Request( NULL,                      // destination EUI64 ADDR
		pNLContract,              // contract
		stAPDUIdtf.m_ucPriorityAndFlags,// priority
		pEnd-g_oAPDU,              // data length
		g_oAPDU,                   // app data
		g_ucHandle++,                // handle
		stAPDUIdtf.m_ucSrcTSAPID,  // src port
		stAPDUIdtf.m_ucDstTSAPID,   // dst port
		stAPDUIdtf.m_ucAllowEncryption
		);
	//WATCH_DURATION_DEF(oDurationWatcher);
	ASLDE_RecordPacketNextTXTime(pNLContract);
	++pNLContract->m_unPktCount;
	//WATCH_DURATION_DEF(oDurationWatcher);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Copies (concatenates) more APDU's into a single TSDU
/// @param  p_unContractID - contract identifier
/// @param  p_pOutBuf - output buffer
/// @param  p_pOutBufEnd - pointer to the output buffer's end
/// @param  p_ucHandle - concatenation handler
/// @param  p_pContract - the NLME contract to use for concatenation
/// @return output buffer
/// @remarks
///      Access level: user level
///////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLDE_concatenateAPDUs( uint16      p_unContractID,
							   uint8 *     p_pOutBuf,
							   uint8 *     p_pOutBufEnd,
							   uint8       p_ucHandle,
							   NLME_CONTRACT_ATTRIBUTES * p_pContract  )
{
	char szLog[1024] = "Concatenating application handles:";
	int nLogLen = strlen(szLog);
	nLogLen += snprintf(szLog+nLogLen, sizeof(szLog)-nLogLen, "(C %u) ", p_unContractID);
	uint8 * pOrigStart = p_pOutBuf;

	CApduListTx::iterator it;
	for (it = g_oAslMsgListTx.begin(); it != g_oAslMsgListTx.end(); it++)
	{
		CAslTxMsg::Ptr pTxMsg = *it;

		if ( pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND || pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND_NO_FC )
		{
			if ( p_unContractID == pTxMsg->m_stQueueHdr.m_nContractId )
			{
				if ( pTxMsg->m_stQueueHdr.m_unAPDULen <= (p_pOutBufEnd - p_pOutBuf))
				{
					if( pTxMsg->m_stQueueHdr.m_ucStatus == APDU_READY_TO_SEND )
					{
						if( !ASLDE_SlidingWindowIncreaseUsage( p_pContract ))
						{
							continue;
						}
						pTxMsg->m_stQueueHdr.m_bInSlidingWindow = true;
					}
					uint8 ucAPDUByte = pTxMsg->m_oTxApduData[0]; // first byte of the APDU header
					uint8 ucServType = ucAPDUByte & 0x1F;
					pTxMsg->m_stQueueHdr.m_ucConcatHandle = p_ucHandle;
					if(!pTxMsg->m_stQueueHdr.m_unSendTime)
					{
						pTxMsg->m_stQueueHdr.m_unSendTime = MLSM_GetCrtTaiSec();
					}
					nLogLen += snprintf(szLog+nLogLen, sizeof(szLog)-nLogLen, "%u ", pTxMsg->m_stQueueHdr.m_unAppHandle);
					if(nLogLen > (int)sizeof(szLog)-1)
					{
						nLogLen = sizeof(szLog)-1;
						szLog[nLogLen] = 0;
					}

					/* **** Object Addressing byte (first byte of the APDU header) ********************/
					/*             b7                  |       b6 b5    |    b4 b3 b2 b1 b0           */
					/* srvc primitive: 0=req, 1 = resp |  obj addr mode |    ASL srvc type            */

					if( pTxMsg->m_stQueueHdr.m_bNoRspExpected
						|| (ucAPDUByte & 0x80)                       // response service primitive type
						|| (ucServType == SRVC_TYPE_ALERT_REP)    // alert report service type
						|| (ucServType == SRVC_TYPE_ALERT_ACK)    // alert acknowledge service type
						|| (ucServType == SRVC_TYPE_PUBLISH) )    // publish service type
					{
						pTxMsg->m_stQueueHdr.m_ucStatus = APDU_SENT_WAIT_JUST_TL_CNF;  // just wait the below layer confirm and delete the APDU from the queue
					}
					else
					{
						pTxMsg->m_stQueueHdr.m_ucStatus = APDU_SENT_WAIT_CNF_AND_RESPONSE;
					}

					memcpy(p_pOutBuf, &pTxMsg->m_oTxApduData[0], pTxMsg->m_stQueueHdr.m_unAPDULen);

					p_pOutBuf += pTxMsg->m_stQueueHdr.m_unAPDULen;
				}
				else
				{
					if(pOrigStart == p_pOutBuf)
					{
						LOG_ISA100(LOGLVL_ERR, "ERROR: can't concatenate APDU handle %u of size %ub, contractID=%d, to buffer of size %ub! Deleting it!",
							 pTxMsg->m_stQueueHdr.m_unAppHandle,  pTxMsg->m_stQueueHdr.m_unAPDULen, p_unContractID, p_pOutBufEnd - p_pOutBuf);
						pTxMsg->m_stQueueHdr.m_ucStatus = APDU_READY_TO_CLEAR;
					}
					// there is no space in the output buffer for this APDU; postpone it for next time and try another APDU now
				}
			}
		}		
	}
	LOG_ISA100(LOGLVL_INF, szLog);
	return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Used by Transport Layer to indicate to the Application Layer that a new APDU has been
///         received. The APDU is placed in the Application Layer queue.
/// @param  p_pSrcAddr      - pointer to source address (always 16 bytes; if first octet is 0xFF,
///                           the last 8 bytes represent MAC address
/// @param  p_ucSrcTsap - source transport SAP
/// @param  p_ucPriorityAndFlags - priority and flags
/// @param  p_unTSDULen - message length
/// @param  p_pAppData      - pointer to the received APDU
/// @param  p_ucDstTsap - destination transport SAP
/// @param  p_ucTransportTime  - time spent by this APDU in the ISA100 network
/// @param  p_ucSecurityCtrl   - value of the security control byte in the security header
/// @return none
/// @remarks
///      Access level: user level
///////////////////////////////////////////////////////////////////////////////////////////////////
/// @todo   TBD (Ticus): How check req id is incremental even if restarts?
void TLDE_DATA_Indication ( const uint8 * p_pSrcAddr,       // 128 bit or MAC address
						   uint8         p_ucSrcTsap,
						   uint8         p_ucPriorityAndFlags,
						   uint16        p_unTSDULen,
						   const uint8 * p_pAppData,
						   uint8         p_ucDstTsap,
						   uint8         p_ucTransportTime,
						   struct timeval* p_ptvTxTime,
						   uint8         p_ucSecurityCtrl)
{
	if (p_unTSDULen < 3)// minimal data
	{
		LOG_ISA100(LOGLVL_ERR, "TLDE_DATA_Indication: ERROR: len < 3");
		return;
	}	

	//add test with MAX allowed in queue
	CAslRxMsg::Ptr pNewMsg (new CAslRxMsg);


	pNewMsg->m_ucStatus			= APDU_NOT_PROCESSED;
	pNewMsg->m_nRecvTime		= MLSM_GetCrtTaiSec();

	pNewMsg->m_stApduIdtf.m_ucPriorityAndFlags	= p_ucPriorityAndFlags;
	pNewMsg->m_stApduIdtf.m_ucSrcTSAPID		= p_ucSrcTsap;
	pNewMsg->m_stApduIdtf.m_ucDstTSAPID		= p_ucDstTsap;	
	
	pNewMsg->m_stApduIdtf.m_ucTransportTime = p_ucTransportTime;
	memcpy( &pNewMsg->m_stApduIdtf.m_tvTxTime, p_ptvTxTime, sizeof(struct timeval) );
	pNewMsg->m_stApduIdtf.m_ucSecurityCtrl	= p_ucSecurityCtrl;	

	pNewMsg->m_stApduIdtf.m_unDataLen		= p_unTSDULen;
	memcpy (pNewMsg->m_stApduIdtf.m_aucAddr, p_pSrcAddr, sizeof pNewMsg->m_stApduIdtf.m_aucAddr);

	// copy the APDU
	pNewMsg->m_oVecData.assign (p_pAppData, p_pAppData + p_unTSDULen);

	g_oAslMsgListRx.push_back(pNewMsg);
	++g_nAslMsgListRxCount;

	if(p_ucSecurityCtrl != SECURITY_CTRL_ENC_MIC_32 && p_ucSecurityCtrl != SECURITY_ENC_MIC_32 && p_ucSecurityCtrl != SECURITY_NONE)
		LOG_ISA100(LOGLVL_ERR, "TLDE_DATA_Indication:INCORRECT p_ucSecurityCtrl: %02X", p_ucSecurityCtrl);
	
	// reset the SM link timeout counter for any incomming message from System Manager
	if (!memcmp(p_pSrcAddr, g_stDMO.m_aucSysMng128BitAddr, 16))
	{
		NLME_CONTRACT_ATTRIBUTES* pContract = NLME_FindContract(g_unSysMngContractID);
		uint8 nTimeout = 3*g_ucDmapMaxRetry * (pContract ? pContract->m_nRTO : g_unDmapRetryTout); //no contract? use the setting
		g_ucSMLinkTimeout = g_ucSMLinkTimeoutConf > nTimeout ? g_ucSMLinkTimeoutConf : nTimeout;
		g_ucLinkProbed = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Called by the transport layer to signal the confirmation status of a previously issued
///         APDU
/// @param  p_hHandle  - application layer handler
/// @param  p_ucStatus - confirmation status
/// @return none
/// @remarks HANDLEs in ASL layer are in the 0x00 - 0xFF range
///      Access level: interrupt level
///////////////////////////////////////////////////////////////////////////////////////////////////
void TLDE_DATA_Confirm( HANDLE  p_hHandle,
					   uint8   p_ucStatus)
{
	if ( g_ucAPDUConfNo < MAX_APDU_CONF_ENTRY_NO )
	{
		g_astAPDUConf[g_ucAPDUConfNo].m_ucHandle = p_hHandle;
		g_astAPDUConf[g_ucAPDUConfNo++].m_ucConfirmStatus = p_ucStatus;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Implements the ASLMO object reset method
/// @param  p_ucResetType - reset type
/// @return service feedback code
/// @remarks
///      Access level: user level\n
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ASLMO_reset(uint8 p_ucResetType)
{
  uint8 ucSFC = SFC_FAILURE;
  
  switch (p_ucResetType)
  {
  case ASLMO_DYNAMIC_DATA_RESET: 
      memset(g_astASLMOCounters, 0, sizeof(g_astASLMOCounters));
      g_unCounterNo   = 0;
      ucSFC = SFC_SUCCESS;
      break;      
      
  case ASLMO_RESET_TO_FACTORY_DEF: break;           // tbd
      
  case ASLMO_RESET_TO_PROVISIONED_SETTINGS: break;  // tbd
      
  case ASLMO_WARM_RESET: break;                     // tbd
      
  default: ucSFC = SFC_INVALID_ARGUMENT; break;     
  }
  
  return ucSFC;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  
/// @param 
/// @return 
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ASLMO_AddMalformedAPDU(const uint8 * p_pSrcAddr128)
{
  if (!p_pSrcAddr128 || !g_stASLMO.m_bMalformedAPDUAdvise || !g_stASLMO.m_unMalformedAPDUThreshold || !g_stASLMO.m_ulCountingPeriod)
      return; // nothing to do; 
  
  uint16 unIdx;  
  // first check if there is already a counter assigned to this address
  for (unIdx = 0; unIdx < g_unCounterNo; unIdx++)
  {
      if (!memcmp(p_pSrcAddr128, g_astASLMOCounters[unIdx].m_auc128BitAddr, 16))
      {
          g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter++;          
          break;
      }
  }
  
  // new address
  if ((unIdx == g_unCounterNo) && (unIdx < MAX_ASLMO_COUNTER_N0))  
  {
     g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter = 1; // new entry;
     g_astASLMOCounters[unIdx].m_ulResetCounter = g_stASLMO.m_ulCountingPeriod;
     memcpy(g_astASLMOCounters[unIdx].m_auc128BitAddr, p_pSrcAddr128, 16);
     g_unCounterNo++;
  } 
  
  // check if alert threshold has been reached
  if (g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter >= g_stASLMO.m_unMalformedAPDUThreshold
      && !g_stASLMO.m_stAlertDescriptor.m_bAlertReportDisabled) // alert enabled
  {
      // alert condition reached; generate the alert
        ALERT stAlert;
        uint8 aucAlertBuf[22];
        
        stAlert.m_ucPriority = g_stASLMO.m_stAlertDescriptor.m_ucPriority & 0x7F;
        stAlert.m_unDetObjTLPort = 0xF0B0; // ASLMO is DMAP port
        stAlert.m_unDetObjID = DMAP_ASLMO_OBJ_ID; 
        stAlert.m_ucClass = ALERT_CLASS_EVENT; 
        stAlert.m_ucDirection = ALARM_DIR_RET_OR_NO_ALARM; 
        stAlert.m_ucCategory = ALERT_CAT_COMM_DIAG; 
        stAlert.m_ucType = ASLMO_MALFORMED_APDU;     
        
        // build alert data
        stAlert.m_unSize = sizeof(aucAlertBuf); 
        
        memcpy( aucAlertBuf, 
                g_astASLMOCounters[unIdx].m_auc128BitAddr,
                16 );
        
        DMAP_InsertUint16( aucAlertBuf+16, g_astASLMOCounters[unIdx].m_unMalformedAPDUCounter );    
        
        DMAP_InsertUint32( aucAlertBuf+18, g_astASLMOCounters[unIdx].m_ulThresholdExceedTime );           
        

        ARMO_AddAlertToQueue( &stAlert, aucAlertBuf );
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief This function executes the ASLMO methods 
/// @param  p_unMethID - method identifier
/// @param  p_unReqSize - input parameters size
/// @param  p_pReqBuf - input parameters buffer
/// @param  p_pRspSize - output parameters size
/// @param  p_pRspBuf - output parameters buffer
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ASLMO_Execute(uint8   p_ucMethID, 
                    uint16  p_unReqSize, 
                    uint8*  p_pReqBuf,
                    uint16* p_pRspSize,
                    uint8*  p_pRspBuf)
{
  *p_pRspSize = 0;
  
  if (p_ucMethID != ASLMO_RESET_METHOD_ID)
      return SFC_INVALID_METHOD;    
  
  uint8 ucResetType = *p_pReqBuf;
  
  if ( (p_unReqSize != 1) || (!ucResetType) || (ucResetType > ASLMO_DYNAMIC_DATA_RESET) )
      return SFC_INVALID_ARGUMENT;
  
  return ASLMO_reset(ucResetType);
}


void ASLDE_loadIDTF(APDU_IDTF_TX * p_pIdtf, const ASL_QUEUE_HDR * p_pHdr )
{
    p_pIdtf->bAPDUFound = true;
    p_pIdtf->m_ucPriorityAndFlags  = p_pHdr->m_ucPriorityAndFlags;
    p_pIdtf->m_ucSrcTSAPID = p_pHdr->m_ucSrcSAP;
    p_pIdtf->m_ucDstTSAPID = p_pHdr->m_ucDstSAP;
    p_pIdtf->m_ucAllowEncryption = p_pHdr->m_ucAllowEncryption;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Computes the retransmission timeout for each link
/// @param  p_pContract - the contract structure where the necessary variables are.
/// @param  p_nRTT - the current round trip time, measured in seconds
/// @return none
///////////////////////////////////////////////////////////////////////////////////////////////////
static void ASLDE_computeTXtimeout(NLME_CONTRACT_ATTRIBUTES * p_pContract, uint32 p_nRTT)
{
	if (!p_nRTT)
	{
		p_nRTT = 1; // Faster than light communication is not acceptable.
	}

	p_nRTT <<= 3; // our implementation uses units of 1/8 s

	if (!p_pContract->m_nSRTT)
	{
		p_pContract->m_nSRTT = p_nRTT;
		p_pContract->m_nRTTV = p_nRTT>>1;
	}else{
		uint32 nDelta = p_nRTT > p_pContract->m_nSRTT ? p_nRTT-p_pContract->m_nSRTT : p_pContract->m_nSRTT-p_nRTT;
		p_pContract->m_nRTTV = (3*p_pContract->m_nRTTV >> 2) + (nDelta >> 2);
		p_pContract->m_nSRTT = (7*p_pContract->m_nSRTT >> 3) + (p_nRTT >> 3);
	}
	p_pContract->m_nRTO = p_pContract->m_nSRTT + (p_pContract->m_nRTTV<<2 ? p_pContract->m_nRTTV<<2 : 1);
	int bRoundUp = (p_pContract->m_nRTO & 0x7) > 0; // we want to round up if the computed RTO is bigger than an integer number of seconds
	p_pContract->m_nRTO >>= 3; // let's go back to seconds
	p_pContract->m_nRTO += bRoundUp;
	if ( p_pContract->m_nRTO < 1 )  { p_pContract->m_nRTO = 1; } //inferior cap
	if ( p_pContract->m_nRTO > 60 ) { p_pContract->m_nRTO = 60;} //superior cap
	LOG_ISA100(LOGLVL_INF, "Computed %ds TX retry timer (last RTT: %ds) for contract:%d", p_pContract->m_nRTO, p_nRTT>>3, p_pContract->m_unContractID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Checks if a contract's sliding window allows a future TX
/// @param  p_pNLContract - the contract structure from the NL
/// @return 0 - sending forbidden: sliding window already at max
///         1 - sending permitted: sliding window can accept new packets, or non aperiodic contract
/// @remarks The two contract structures must exist and refer to the same contract ID.
///////////////////////////////////////////////////////////////////////////////////////////////////
static int ASLDE_SlidingWindowAllowsTX(NLME_CONTRACT_ATTRIBUTES * p_pNLContract)
{
	if(!p_pNLContract->m_ucMaxSendWindow)
	{
		return 1; //non aperiodic packet, there is no sliding window
	}
	if(p_pNLContract->m_nCrtWndSize > p_pNLContract->m_ucMaxSendWindow)
	{
		p_pNLContract->m_nCrtWndSize = p_pNLContract->m_ucMaxSendWindow; // enforce contract bandwidth limit
	}
	if(p_pNLContract->m_nCrtWndSize > p_pNLContract->m_nInUseWndSize)
	{
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Tries to increase the sliding window usage and reports the success or failure of the operation
/// @param  p_pNLContract - the contract structure from the NL
/// @return 0 - sending forbidden: sliding window already at max
///         1 - sending permitted: sliding window increased by one, or non aperiodic contract
///////////////////////////////////////////////////////////////////////////////////////////////////
static int ASLDE_SlidingWindowIncreaseUsage(NLME_CONTRACT_ATTRIBUTES * p_pContract)
{
	if(!p_pContract->m_ucMaxSendWindow)
	{
		return 1; //non aperiodic packet, there is no sliding window
	}
	if(ASLDE_SlidingWindowAllowsTX( p_pContract ))
	{
		p_pContract->m_nInUseWndSize++;
		LOG_ISA100( LOGLVL_DBG, SLIDING_WND_FMT, p_pContract->m_unContractID, p_pContract->m_nInUseWndSize, p_pContract->m_nCrtWndSize);
		return 1;
	}
	LOG_ISA100( LOGLVL_DBG, SLIDING_WND_FMT " (max %2u) full, cannot add", p_pContract->m_unContractID,
		p_pContract->m_nInUseWndSize, p_pContract->m_nCrtWndSize, p_pContract->m_ucMaxSendWindow);
	return 0;	/// The caller should log this condition
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Performs the sliding window computations in case of successful responses
/// @param  p_pContract - the contract structure from the NL
/// @return none
///////////////////////////////////////////////////////////////////////////////////////////////////
static void ASLDE_ReceiveResponseForSlidingWindow(NLME_CONTRACT_ATTRIBUTES * p_pContract)
{
	if(!p_pContract->m_ucMaxSendWindow)
	{
		return; //there is no sliding window
	}
	if(p_pContract->m_nInUseWndSize)
	{
		p_pContract->m_nInUseWndSize--;
	}else{
		char szLog[7][1024] = {	"",
					"WARNING:READY_TO_SEND application handles:",
					"WARNING:READY_TO_SEND_NO_FC application handles:",
					"WARNING:SENT_WAIT_JUST_TL_CNF application handles:",
					"WARNING:SENT_WAIT_CNF_AND_RESPONSE application handles:",
					"WARNING:CONFIRMED_WAIT_RSP application handles:",
					"WARNING:READY_TO_CLEAR application handles:"};
		int nLogLen[7] = { 0, strlen(szLog[1]), strlen(szLog[2]),  strlen(szLog[3]), strlen(szLog[4]), strlen(szLog[5]), strlen(szLog[6])};
		LOG_ISA100( LOGLVL_ERR, "WARNING: Sliding window inconsistent! Received a response even though it was empty! Counting actual APDU's waiting for responses:");
		CApduListTx::iterator it;
		for (it = g_oAslMsgListTx.begin(); it != g_oAslMsgListTx.end(); it++)
		{
			CAslTxMsg::Ptr pTxMsg = *it;
			if( pTxMsg->m_stQueueHdr.m_nContractId == p_pContract->m_unContractID )
			{
				nLogLen[pTxMsg->m_stQueueHdr.m_ucStatus] += snprintf(szLog[pTxMsg->m_stQueueHdr.m_ucStatus]+nLogLen[pTxMsg->m_stQueueHdr.m_ucStatus], sizeof(szLog[pTxMsg->m_stQueueHdr.m_ucStatus])-nLogLen[pTxMsg->m_stQueueHdr.m_ucStatus], "%u ", pTxMsg->m_stQueueHdr.m_unAppHandle);
				if(nLogLen[pTxMsg->m_stQueueHdr.m_ucStatus] > (int)sizeof(szLog[pTxMsg->m_stQueueHdr.m_ucStatus])-1)
				{
					nLogLen[pTxMsg->m_stQueueHdr.m_ucStatus] = sizeof(szLog[pTxMsg->m_stQueueHdr.m_ucStatus])-1;
					szLog[pTxMsg->m_stQueueHdr.m_ucStatus][nLogLen[pTxMsg->m_stQueueHdr.m_ucStatus]] = 0;
				}
			}
		}
		LOG_ISA100( LOGLVL_ERR, szLog[1]);
		LOG_ISA100( LOGLVL_ERR, szLog[2]);
		LOG_ISA100( LOGLVL_ERR, szLog[3]);
		LOG_ISA100( LOGLVL_ERR, szLog[4]);
		LOG_ISA100( LOGLVL_ERR, szLog[5]);
		LOG_ISA100( LOGLVL_ERR, szLog[6]);
	}
	if( p_pContract->m_nCrtWndSize ) // we are not in a timeout recovery process
	{
		p_pContract->m_nSuccessTransmissions++;
	}
	if( !p_pContract->m_nInUseWndSize && !p_pContract->m_nCrtWndSize )// we have just cleared the pipe after a timeout
	{
		p_pContract->m_nCrtWndSize = 1; //restart the sliding window
		LOG_ISA100( LOGLVL_INF, SLIDING_WND_FMT " (max %2u) restarted", p_pContract->m_unContractID,
			p_pContract->m_nInUseWndSize, p_pContract->m_nCrtWndSize, p_pContract->m_ucMaxSendWindow);
	}
	if( p_pContract->m_nSuccessTransmissions > p_pContract->m_nCrtWndSize ) //congestion avoidance: more than m_nCrtWndSize transactions completed
	{
		p_pContract->m_nCrtWndSize++; // therefore it's safe to increase the window
		p_pContract->m_nSuccessTransmissions = 0;
		LOG_ISA100( LOGLVL_INF, SLIDING_WND_FMT  " (max %2u) size incremented", p_pContract->m_unContractID,
			p_pContract->m_nInUseWndSize, p_pContract->m_nCrtWndSize, p_pContract->m_ucMaxSendWindow);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Performs the sliding window reset in case of timeouts
/// @param  p_pNLContract - the contract structure from the NL
/// @return none
///////////////////////////////////////////////////////////////////////////////////////////////////
static void ASLDE_ReceiveTimeoutForSlidingWindow(NLME_CONTRACT_ATTRIBUTES * p_pContract)
{
	if(!p_pContract->m_ucMaxSendWindow)
	{
		return; //non aperiodic packet, there is no sliding window
	}
	p_pContract->m_nCrtWndSize = 0;
	p_pContract->m_nSuccessTransmissions = 0;
	LOG_ISA100( LOGLVL_INF, SLIDING_WND_FMT " (max %2u) reset on timeout", p_pContract->m_unContractID,
		p_pContract->m_nInUseWndSize, p_pContract->m_nCrtWndSize, p_pContract->m_ucMaxSendWindow);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Checks if a contract's bandwidth allows a future TX
/// @param  p_pNLContract - the contract structure from the NL
/// @return 0 - sending forbidden: last packet has been sent too recently
///         1 - sending permitted: enough time has passed since sending the last packet
///////////////////////////////////////////////////////////////////////////////////////////////////
static int ASLDE_BandwidthLimitAllowsTX(NLME_CONTRACT_ATTRIBUTES * p_pNLContract)
{
	if(tvNow.tv_sec > p_pNLContract->m_stSendNoEarlierThan.tv_sec)
	{
		return 1;
	}
	if(tvNow.tv_sec == p_pNLContract->m_stSendNoEarlierThan.tv_sec && tvNow.tv_usec > p_pNLContract->m_stSendNoEarlierThan.tv_usec)
	{
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihai Buha
/// @brief  Records the time of the earliest future send opportunity in the contract
/// @param  p_pNLContract - the contract structure from the NL
/// @return none
///////////////////////////////////////////////////////////////////////////////////////////////////
static void ASLDE_RecordPacketNextTXTime(NLME_CONTRACT_ATTRIBUTES * p_pNLContract)
{
	unsigned long nPacketIntervalMS = p_pNLContract->m_nComittedBurst > 0 ? 1000/p_pNLContract->m_nComittedBurst : -1000*p_pNLContract->m_nComittedBurst;
	p_pNLContract->m_stSendNoEarlierThan = tvNow;
	p_pNLContract->m_stSendNoEarlierThan.tv_usec += 1000*nPacketIntervalMS;
	p_pNLContract->m_stSendNoEarlierThan.tv_sec += p_pNLContract->m_stSendNoEarlierThan.tv_usec / 1000000;
	p_pNLContract->m_stSendNoEarlierThan.tv_usec %= 1000000;
}

void ASLDE_Dump( void )
{
	LOG_ISA100(LOGLVL_ERR, "  ASLDE Tx que %d Rx que: %d ", g_oAslMsgListTx.size(), g_nAslMsgListRxCount);
	LOG_ISA100(LOGLVL_ERR, "----------------------------" );
}
