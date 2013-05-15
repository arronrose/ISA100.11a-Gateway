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
/// Author:       Nivis LLC, Mihaela Goloman
/// Date:         November 2008
/// Description:  This file implements the alert report management object in DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "dmap_armo.h"
//#include "dlmo_utils.h"
#include "dmap.h"
#include "aslsrvc.h"
//#include "mlsm.h"
#include "tlde.h"
#include "uap.h"
#include "porting.h"
#include "callbacks.h"

#include <arpa/inet.h>


#define ARMO_DEFAULT_CONF_TIMEOUT  10

extern uint8 * ASLSRVC_insertExtensibleValue(  uint8 * p_pData,
                                        uint16  p_unValue);
extern const uint8 * ASLSRVC_extractExtensibleValue( const uint8 * p_pData,
                                              uint16 * p_pValue);

ARMO_STRUCT g_aARMO[ALERT_CAT_NO];

uint16 g_unContractReqTimestamp;

uint8  g_ucAlarmTimeout;

uint8  g_aucARMOQueue[ARMO_QUEUE_SIZE];  

uint8* g_pARMOQueueEnd = g_aucARMOQueue;

static void ARMO_readAlertCommEndpoint(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
static void ARMO_writeAlertCommEndpoint(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

static void ARMO_configureEndpoint(ALERT_COMM_ENDPOINT * p_pEndpoint, uint8 p_ucCategory );

static uint8 ARMO_recoverNextAlarm(uint8 p_ucCategory, uint8 p_ucLastID);
static uint8 ARMO_generateRecoveryEvent(uint8 p_ucCategory, uint8 p_ucAlertType);

const DMAP_FCT_STRUCT c_aARMOFct[ARMO_ATTR_NO] = {
	{ 0,   0,                                                      DMAP_EmptyReadFunc ,         NULL },
	{ ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_stAlertMaster),     ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_ucConfTimeout),     DMAP_ReadUint8,             DMAP_WriteUint8 } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_ucAlertsDisable),   DMAP_ReadUint8,             DMAP_WriteUint8 } ,
	
	{ ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_stAlertMaster),    ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_ucConfTimeout),    DMAP_ReadUint8,             DMAP_WriteUint8 } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_ucAlertsDisable),  DMAP_ReadUint8,             DMAP_WriteUint8 } ,

	{ ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_stAlertMaster),     ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_ucConfTimeout),     DMAP_ReadUint8,             DMAP_WriteUint8 } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_ucAlertsDisable),   DMAP_ReadUint8,             DMAP_WriteUint8 } ,

	{ ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_stAlertMaster),      ARMO_readAlertCommEndpoint, ARMO_writeAlertCommEndpoint } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_ucConfTimeout),      DMAP_ReadUint8,             DMAP_WriteUint8 } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_ucAlertsDisable),    DMAP_ReadUint8,             DMAP_WriteUint8 } ,

	{ ATTR_CONST(g_aARMO[ALERT_CAT_COMM_DIAG].m_stRecoveryDescr),  DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_SECURITY].m_stRecoveryDescr),   DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_DEV_DIAG].m_stRecoveryDescr),   DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor } ,
	{ ATTR_CONST(g_aARMO[ALERT_CAT_PROCESS].m_stRecoveryDescr),    DMAP_ReadAlertDescriptor,  DMAP_WriteAlertDescriptor } ,
};

void ARMO_readAlertCommEndpoint(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
{
  ALERT_COMM_ENDPOINT* pstAlertCommEndpoint = (ALERT_COMM_ENDPOINT*)p_pValue;
  
  memcpy(p_pBuf, pstAlertCommEndpoint->m_aNetworkAddr, 16);
  p_pBuf += 16;
  
  *(p_pBuf++) = pstAlertCommEndpoint->m_unTLPort >> 8;
  *(p_pBuf++) = pstAlertCommEndpoint->m_unTLPort;
    
  *(p_pBuf++) = pstAlertCommEndpoint->m_unObjID >> 8;
  *(p_pBuf++) = pstAlertCommEndpoint->m_unObjID;  

  *p_ucSize = 20; 
}

void ARMO_writeAlertCommEndpoint(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
{
  ALERT_COMM_ENDPOINT* pstAlertCommEndpoint = (ALERT_COMM_ENDPOINT*)p_pValue;
  
  memcpy(pstAlertCommEndpoint->m_aNetworkAddr, p_pBuf, 16);
  
  pstAlertCommEndpoint->m_unTLPort = ((uint16)p_pBuf[16] << 8) | p_pBuf[17];
  pstAlertCommEndpoint->m_unObjID = ((uint16)p_pBuf[18] << 8) | p_pBuf[19];
  
  pstAlertCommEndpoint->m_ucContractStatus = ARMO_NO_CONTR;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Initializes ARMO's attributes
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_Init()
{
  //defaults
  for(uint8 ucCnt=0; ucCnt < ALERT_CAT_NO; ucCnt++)
  {
      if( !g_aARMO[ucCnt].m_ucConfTimeout )
      {
          g_aARMO[ucCnt].m_ucConfTimeout = ARMO_DEFAULT_CONF_TIMEOUT;
	memcpy(g_aARMO[ucCnt].m_stAlertMaster.m_aNetworkAddr, g_stDMO.m_aucSysMng128BitAddr, 16);
	g_aARMO[ucCnt].m_stAlertMaster.m_unTLPort = ISA100_SMAP_PORT;
	g_aARMO[ucCnt].m_stAlertMaster.m_unObjID = SM_ARO_OBJ_ID;
	g_aARMO[ucCnt].m_stAlertMaster.m_ucContractStatus = ARMO_NO_CONTR;
      }
  }
}

inline uint8 ARMO_getNextAlertId( uint8 p_ucCategory)
{
    // alert id's generated separately for each category
    // ack comes only with id, so the following line 
    // is a trick to avoid duplicate id's from different categories
    return (p_ucCategory << 6) | ((g_aARMO[p_ucCategory].m_ucAlertsNo++) & 0x3F);
}



void ARMO_FwAlertToTR(const ALERT* p_pAlert, const uint8* p_pBuf)
{
	ALERT_BBR2TR stTrAlert;

	stTrAlert.m_unDetObjTLPort	= htons(p_pAlert->m_unDetObjTLPort);
	stTrAlert.m_unDetObjID		= htons(p_pAlert->m_unDetObjID);
	stTrAlert.m_ulSeconds		= htonl(p_pAlert->m_stDetectionTime.m_ulSeconds);
	stTrAlert.m_unFract			= htons(p_pAlert->m_stDetectionTime.m_unFract);
	
	stTrAlert.m_ucCategory		= p_pAlert->m_ucCategory;
	stTrAlert.m_ucClass			= p_pAlert->m_ucClass;
	stTrAlert.m_ucDirection		= p_pAlert->m_ucDirection;
	stTrAlert.m_ucID			= p_pAlert->m_ucID;
	stTrAlert.m_ucType			= p_pAlert->m_ucType;
	stTrAlert.m_ucPriority		= p_pAlert->m_ucPriority;

	LOG_ISA100(LOGLVL_INF,"ARMO_FwAlertToTR: port=%d obj=%d cat=%d class=%d dir=%d type=%d",
					p_pAlert->m_unDetObjTLPort, p_pAlert->m_unDetObjID, p_pAlert->m_ucCategory,p_pAlert->m_ucClass, p_pAlert->m_ucDirection, p_pAlert->m_ucType);
	g_pCallbackSendAlertTR( (uint8_t*)&stTrAlert, sizeof(stTrAlert), p_pBuf, p_pAlert->m_unSize );	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Adds an alert to the ARMO alert queue
/// @param  p_pAlert - pointer to an alert type structure
/// @param  p_pBuf   - pointer to a buffer that contains alert data (Alert data len is inside p_pAlert->m_unSize)
/// @return service feddback code
/// @remarks
///      Access level: user level
uint8 ARMO_AddAlertToQueue(const ALERT* p_pAlert, const uint8* p_pBuf)
{
	if (g_pCallbackSendAlertTR)
	{
		ARMO_FwAlertToTR(p_pAlert,p_pBuf);
		ARMO_ProcessAlertAck(p_pAlert->m_ucID);
		return SFC_SUCCESS;
	}

  uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + p_pAlert->m_unSize );
  
  if (unLen > sizeof(g_aucARMOQueue))
	{
		LOG_ISA100(LOGLVL_ERR, "ARMO_AddAlertToQueue: alert too big (size %d); will not fit even if queue (size %d) is empty", unLen, sizeof g_aucARMOQueue);
		return SFC_INVALID_SIZE;
	}
  
  if( !g_aARMO[p_pAlert->m_ucCategory].m_ucAlertsDisable && (unLen < sizeof(g_aucARMOQueue)) ) // valid alert
  {  
	TIME ulSeconds;
	uint16 unFract;
	MLSM_GetCrtTaiTime( &ulSeconds, &unFract );
      while( (g_pARMOQueueEnd + unLen) > (g_aucARMOQueue + sizeof(g_aucARMOQueue) ) ) // not enough space
      {
          // remove first alarm (oldest one); do not check that g_pARMOQueueEnd > g_aucARMOQueue
          ALERT* pAlert = (ALERT*)g_aucARMOQueue;
          uint16 unAlertLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
	LOG_ISA100(LOGLVL_INF, "WARNING: ARMO_AddAlertToQueue: not enough space in queue, removing oldest alarm (age %ds)", ulSeconds-pAlert->m_stDetectionTime.m_ulSeconds);
          g_pARMOQueueEnd -= unAlertLen;
          memmove(g_aucARMOQueue, g_aucARMOQueue+unAlertLen, g_pARMOQueueEnd-g_aucARMOQueue); 
      }
    
      ALERT* pNext = (ALERT*)g_pARMOQueueEnd;
      
      // copy alert header
      memcpy(pNext, p_pAlert, sizeof(ALERT));
      
      pNext->m_ucID = ARMO_getNextAlertId( p_pAlert->m_ucCategory );
      pNext->m_ulNextSendTAI = 0;  
      
      // fill the detection time 
	pNext->m_stDetectionTime.m_ulSeconds = ulSeconds;
	pNext->m_stDetectionTime.m_unFract = unFract;
      
      // copy alert data  
      memcpy(g_pARMOQueueEnd + sizeof(ALERT), p_pBuf, p_pAlert->m_unSize);
      
      g_pARMOQueueEnd += unLen;
      
	LOG_ISA100(LOGLVL_DBG, "ARMO_AddAlertToQueue: successful");
      return SFC_SUCCESS;
  }
  LOG_ISA100(LOGLVL_INF, "WARNING: ARMO_AddAlertToQueue: alerts are disabled for this category (%d)", p_pAlert->m_ucCategory);
  return SFC_OBJECT_STATE_CONFLICT; // alerts are disabled for this category
}


void ARMO_ProcessAlertAck(uint8 p_ucID)
{
  uint8* pBuf = g_aucARMOQueue;
  
  g_ucAlarmTimeout = 0; // clean alert timeout
  
  while (  pBuf < g_pARMOQueueEnd )
  {
    ALERT* pAlert = (ALERT*)pBuf;
    uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
    if(pAlert->m_ucID == p_ucID) // alert found
    {
      g_pARMOQueueEnd -= unLen;
      memmove(pBuf, pBuf+unLen, g_pARMOQueueEnd-pBuf);
      break;
    }
    
    pBuf += unLen;
  }
  if( (p_ucID >> 6) == ALERT_CAT_PROCESS )
  {
      UAP_NotifyAlertAcknowledge( p_ucID );
  }      
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Handles the establishment of the four alarm contracts for each alarm category
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_checkAlarmContracts(void)
{  
  for( uint8 ucIdx = 0; ucIdx < ALERT_CAT_NO; ucIdx++ )
  {
      switch (g_aARMO[ucIdx].m_stAlertMaster.m_ucContractStatus)
      {
          case ARMO_CONTRACT_ACTIVE: break;
          
          case ARMO_AWAIT_CONTRACT_ESTABLISHMENT: 
          case ARMO_WAITING_CONTRACT_TERMINATION:   
              // check if the time for contract establishment or contract termination expired;
              if ( ((uint16)MLSM_GetCrtTaiSec() -  g_unContractReqTimestamp) > CONTRACT_WAIT_TIMEOUT )
              {
                  g_aARMO[ucIdx].m_stAlertMaster.m_ucContractStatus = ARMO_NO_CONTR;
              }                
              break;
          
          case ARMO_NO_CONTR:  
//              if (g_aARMO[ucIdx].m_stAlertMaster.m_unContractID)
//              {
//                  // there was an active contract running for this endpoint; endpoint has been reconfigured,
//                  // so termination of the previous contract is needed.    
//                  if ( SFC_SUCCESS == DMO_RequestContractTermination( g_aARMO[ucIdx].m_stAlertMaster.m_unContractID,
//                                                                      CONTRACT_TERMINATION) )
//                  {                  
//                      g_aARMO[ucIdx].m_stAlertMaster.m_ucContractStatus = ARMO_WAITING_CONTRACT_TERMINATION;
//                      g_unContractTimeout = MLSM_GetCrtTaiSec() + (uint16)CONTRACT_WAIT_TIMEOUT;  
//                  }
//              }
//              else 
//              {
                   ARMO_configureEndpoint(&g_aARMO[ucIdx].m_stAlertMaster, ucIdx);               
//              }
              
              break;
      }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Issues a new contract request to DMO object of DMAP
/// @param  p_pEndpoint - pointer to an endpoint descriptor
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_configureEndpoint(ALERT_COMM_ENDPOINT * p_pEndpoint, uint8 p_ucCategory )
{
  if( p_pEndpoint->m_unTLPort ) // end point was configured
  {      
      // first check if there is already a useful contract in DMAP
      DMO_CONTRACT_ATTRIBUTE * pContract = DMO_GetContract( p_pEndpoint->m_aNetworkAddr, 
                                                            p_pEndpoint->m_unTLPort, 
                                                            ISA100_DMAP_PORT, 
                                                            SRVC_APERIODIC_COMM );  
      if (pContract)
      {
          g_aARMO[p_ucCategory].m_stAlertMaster.m_unContractID     = pContract->m_unContractID;
          g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus = ARMO_CONTRACT_ACTIVE;  
      }
      
      // contract not available; request a new contract. 
      DMO_CONTRACT_BANDWIDTH stBandwidth;  
      stBandwidth.m_stAperiodic.m_nComittedBurst  = -15;  // 1 APDU over a 10 second period
      stBandwidth.m_stAperiodic.m_nExcessBurst    = 1;    // 1 APDU per second
      stBandwidth.m_stAperiodic.m_ucMaxSendWindow = 5;    
    
      uint8 ucStatus = DMO_RequestNewContract(  p_pEndpoint->m_aNetworkAddr,
                                                p_pEndpoint->m_unTLPort,
                                                ISA100_DMAP_PORT,           // p_unSrcSAP,
                                                0xFFFFFFFF,                 // contract life
                                                SRVC_APERIODIC_COMM,        // p_ucSrcvType,
                                                DMO_PRIORITY_BEST_EFFORT,   // p_ucPriority,
                                                MAX_APDU_SIZE,              // p_unMaxAPDUSize,
                                                1,                          //  p_ucReliability,
                                                &stBandwidth );  
      if (SFC_SUCCESS == ucStatus)
      {
          g_unContractReqTimestamp = (uint16)MLSM_GetCrtTaiSec();  
          g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus = ARMO_AWAIT_CONTRACT_ESTABLISHMENT;  
      }  
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Called by DMAP each time a new contract entry is written into DMO contract table.
/// @param  p_pContract - pointer to the new contract
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_NotifyAddContract(DMO_CONTRACT_ATTRIBUTE * p_pContract)
{
  ARMO_STRUCT * pArmo = g_aARMO;
  for( ; pArmo < g_aARMO+ALERT_CAT_NO; pArmo++ )
  {
      // check if this alarm category is waiting for a contract
      if( (pArmo->m_stAlertMaster.m_ucContractStatus == ARMO_AWAIT_CONTRACT_ESTABLISHMENT)
          && (pArmo->m_stAlertMaster.m_unTLPort  == p_pContract->m_unDstTLPort)
          && (!memcmp(pArmo->m_stAlertMaster.m_aNetworkAddr,
                      p_pContract->m_aDstAddr128,
                      sizeof(p_pContract->m_aDstAddr128)))
            )
      {
          pArmo->m_stAlertMaster.m_unContractID = p_pContract->m_unContractID;
          pArmo->m_stAlertMaster.m_ucContractStatus = ARMO_CONTRACT_ACTIVE;
      }                      
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks if a deleted contract in DMAP is in the scope of the ARMO object
/// @param  p_unContractID  - contract ID
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_NotifyContractDeletion(uint16 p_unContractID)
{
  // tbd
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Implements the alarm recovery state machine: generates the recovery start and end events
///         and recovers all required alarms.
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_checkRecoveryState(void)
{
// Generated by ARMO for the process alert master indicating that the alarm recovery command has been received; 
//   all outstanding process alarms are reported after this event is raised 
// Since the standard says "raised" and not "confirmed" we implement in this way.
// The order of events may be different at destination 
#warning "to decide if the state machine should wait for RECOVERY_START event acknowledge before reporting the alerts from queue "
  
  for (uint8 ucIdx = 0; ucIdx < ALERT_CAT_NO; ucIdx++)
  {
      if (g_aARMO[ucIdx].m_stRecovery.m_ucState != RECOVERY_DISABLED)
      {
          
          switch (g_aARMO[ucIdx].m_stRecovery.m_ucState)
          {
            case RECOVERY_ENABLED:  
                if (SFC_SUCCESS == ARMO_generateRecoveryEvent(ucIdx, ALARM_RECOVERY_START))
                {
                    g_aARMO[ucIdx].m_stRecovery.m_ucState = RECOVERY_START_SENT;                    
                } 
                break;            
            
            case RECOVERY_START_SENT:
                if (SFC_SUCCESS != ARMO_recoverNextAlarm(ucIdx, g_aARMO[ucIdx].m_stRecovery.m_ucLastID))
                {
                    g_aARMO[ucIdx].m_stRecovery.m_ucState = RECOVERY_DONE;        
                }
                break;
                
            case RECOVERY_DONE:
                if (SFC_SUCCESS == ARMO_generateRecoveryEvent(ucIdx, ALARM_RECOVERY_END))
                {
                    g_aARMO[ucIdx].m_stRecovery.m_ucState = RECOVERY_DISABLED; // recovery finished; disble recovery mode                    
                } 
                break;  
          }
          
          return TRUE; // do not check following categories untill current category is finished;
      }
  }
  
  return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_recoverNextAlarm(uint8 p_ucCategory, uint8 p_ucLastID)
{ 
  uint8 * pBuf = g_aucARMOQueue;
  
  ALERT_REP_SRVC stAlertRep;  
  stAlertRep.m_unSrcOID = DMAP_ARMO_OBJ_ID;
  
  while(pBuf < g_pARMOQueueEnd)
  {
      ALERT * pAlert = (ALERT*)pBuf;
      uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
      
      if( (pAlert->m_ucCategory == p_ucCategory) 
          && ( (pAlert->m_ucID > p_ucLastID) || (((p_ucLastID & 0x3F) > 61) && ((pAlert->m_ucID & 0x3F) < 3)) )  // try to catch alertId overflows also  
          && (g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus == ARMO_CONTRACT_ACTIVE) )
      {
          stAlertRep.m_unDstOID = g_aARMO[p_ucCategory].m_stAlertMaster.m_unObjID;          
                    
          memcpy(&stAlertRep.m_stAlertInfo, pAlert, sizeof(ALERT));
          stAlertRep.m_pAlertValue = pBuf + sizeof(ALERT);
          
          if (SFC_SUCCESS == ASLSRVC_AddGenericObject( &stAlertRep,
                                                       SRVC_ALERT_REP,
                                                       g_aARMO[p_ucCategory].m_stRecoveryDescr.m_ucPriority, // priority
                                                       UAP_DMAP_ID,                                           // SrcTSAPID 
                                                       g_aARMO[p_ucCategory].m_stAlertMaster.m_unTLPort & 0x0F,// DstTSAPID
                                                       0,
                                                       NULL,
                                                       g_aARMO[p_ucCategory].m_stAlertMaster.m_unContractID,
                                                       0, //p_unBinSize
							0 ))
          {
              g_aARMO[p_ucCategory].m_stRecovery.m_ucLastID = pAlert->m_ucID;
          }   
          return SFC_SUCCESS;
      }
      pBuf += unLen;
  }
  
  return SFC_FAILURE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generates an ARMO type event. This event is not added to the queue because it has to be 
///         sent first. All other alarms are reported after this event is sent!
/// @return SFC_SUCCESS if succesflyy sent to ASL queu
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 ARMO_generateRecoveryEvent(uint8 p_ucCategory, uint8 p_ucAlertType)
{  
  if (g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus != ARMO_CONTRACT_ACTIVE)
  {
	LOG_ISA100(LOGLVL_INF,"WARNING: ARMO_generateRecoveryEvent: contract status:%d for category:%d", g_aARMO[p_ucCategory].m_stAlertMaster.m_ucContractStatus, p_ucCategory);
      return SFC_NO_CONTRACT;  
  }
  
  if( g_aARMO[p_ucCategory].m_stRecoveryDescr.m_bAlertReportDisabled )
  {
	LOG_ISA100(LOGLVL_INF,"WARNING: ARMO_generateRecoveryEvent: alert report disabled for category %d", p_ucCategory);
      return SFC_SUCCESS;
  }
  
  ALERT_REP_SRVC stAlertRep;  
  stAlertRep.m_unSrcOID = DMAP_ARMO_OBJ_ID;
  stAlertRep.m_unDstOID = g_aARMO[p_ucCategory].m_stAlertMaster.m_unObjID;
  
  MLSM_GetCrtTaiTime( &stAlertRep.m_stAlertInfo.m_stDetectionTime.m_ulSeconds, 
                      &stAlertRep.m_stAlertInfo.m_stDetectionTime.m_unFract );
  
  stAlertRep.m_stAlertInfo.m_unDetObjTLPort = 0xF0B0;  // standard ISA100 DMAP port 
  stAlertRep.m_stAlertInfo.m_unDetObjID     = DMAP_ARMO_OBJ_ID;
  stAlertRep.m_stAlertInfo.m_ucCategory     = p_ucCategory;
  stAlertRep.m_stAlertInfo.m_ucType         = p_ucAlertType;
  stAlertRep.m_stAlertInfo.m_ucClass        = ALERT_CLASS_EVENT; 
  stAlertRep.m_stAlertInfo.m_ucPriority     = g_aARMO[p_ucCategory].m_stRecoveryDescr.m_ucPriority;
  stAlertRep.m_stAlertInfo.m_ucDirection    = ALARM_DIR_RET_OR_NO_ALARM;  
  
  stAlertRep.m_stAlertInfo.m_ucID = ARMO_getNextAlertId(p_ucCategory);
  
  stAlertRep.m_stAlertInfo.m_unSize = 0;  
  
  return ASLSRVC_AddGenericObject( &stAlertRep,
                                   SRVC_ALERT_REP,
                                   g_aARMO[p_ucCategory].m_stRecoveryDescr.m_ucPriority , // priority
                                   UAP_DMAP_ID,       // SrcTSAPID 
                                   g_aARMO[p_ucCategory].m_stAlertMaster.m_unTLPort & 0x0F,       // 
                                   0,
                                   NULL,
                                   g_aARMO[p_ucCategory].m_stAlertMaster.m_unContractID,
                                   0, //p_unBinSize
					0 );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Implements alert reporting management object's state machine
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_Task()
{    
  uint8* pBuf = g_aucARMOQueue;
  
  ARMO_checkAlarmContracts();
  
  // if recovery mode is active, alarm reporting will be performed inside ARMO_checkRecoveryState()
  if (TRUE == ARMO_checkRecoveryState())
  {
      if( pBuf < g_pARMOQueueEnd )
      {
          if( !g_ucAlarmTimeout ) // if not alert timeout on progress
          {
              g_ucAlarmTimeout = 1; // start ACK timeout
          }
      }
      return; 
  }
  
  ALERT_REP_SRVC stAlertRep;    
  stAlertRep.m_unSrcOID = DMAP_ARMO_OBJ_ID;
    
  uint32 ulCrtSec = MLSM_GetCrtTaiSec();
  while( pBuf < g_pARMOQueueEnd )
  {
    ALERT* pAlert = (ALERT*)pBuf;
    uint16 unLen = DMAP_GetAlignedLength( sizeof(ALERT) + pAlert->m_unSize );
    
    stAlertRep.m_unDstOID = g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_unObjID;
    
    memcpy(&stAlertRep.m_stAlertInfo, pAlert, sizeof(ALERT)); 
    stAlertRep.m_pAlertValue = pBuf + sizeof(ALERT);
    
    pBuf += unLen;  // prepare for the next while loop
    
    // check if the associated contract is active
    if (g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_ucContractStatus == ARMO_CONTRACT_ACTIVE)
    {
        if(  pAlert->m_ulNextSendTAI < ulCrtSec ) 
        {
            if(SFC_SUCCESS == ASLSRVC_AddGenericObject( &stAlertRep,
                                                        SRVC_ALERT_REP,
                                                        pAlert->m_ucPriority, // pAlert->m_ucPriority,  // priority
                                                        UAP_DMAP_ID,          // SrcTSAPID !?
                                                        g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_unTLPort & 0x0F,       // DstTSAPID !?
                                                        0,
                                                        NULL,
                                                        g_aARMO[pAlert->m_ucCategory].m_stAlertMaster.m_unContractID,
                                                       0, //p_unBinSize
							0 ))
              
            {
                pAlert->m_ulNextSendTAI = ulCrtSec + g_aARMO[pAlert->m_ucCategory].m_ucConfTimeout;
            }            
        }
    }
    if( !g_ucAlarmTimeout ) // if not alert timeout on progress
    {
        g_ucAlarmTimeout = 1; // start ACK timeout
    }
  }
}

uint8 ARMO_Execute(uint8   p_ucMethID, 
                   uint16  p_unReqSize, 
                   uint8*  p_pReqBuf,
                   uint16* p_pRspSize,
                   uint8*  p_pRspBuf)
{
  // ARMO has only one method: alarm recovery method  
  *p_pRspSize = 0; 
  
  if(ARMO_ALARM_RECOVERY != p_ucMethID)
      return SFC_INVALID_METHOD;
  
  if(1 != p_unReqSize)
      return SFC_INVALID_SIZE;

  uint8  ucAlertCategory = *p_pReqBuf;       
  if (ucAlertCategory >= ALERT_CAT_NO)
      return SFC_INCONSISTENT_CONTENT;
  
  // check if recovery is already active for this category
  if (g_aARMO[ucAlertCategory].m_stRecovery.m_ucState != RECOVERY_DISABLED)
      return SFC_OBJECT_STATE_CONFLICT;
  
  // enable the recovery state for thi category (generation of the alarm_recovery_start 
  // will be handled by th ARMO_Task;
  g_aARMO[ucAlertCategory].m_stRecovery.m_ucState         = RECOVERY_ENABLED;
  
  return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks if the generated alerts are acknowledged in a reasonable time. Resets the ISA100
///         stack if alerts are not acknowledged
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void ARMO_CheckAlertAckTimeout( void )
{  
  if( g_ucAlarmTimeout )
  {
      if( (++g_ucAlarmTimeout) > ARMO_ALERT_ACK_TIMEOUT )
      {
          // no Alert ACK received within ARMO_ALERT_ACK_TIMEOUT seconds; reset the stack
          DMAP_DLMO_ResetStack();
      }
  }
}
