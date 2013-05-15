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
/// Date:         January 2008
/// Description:  This file implements the device manager application process
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include "provision.h"
#include "tlme.h"
#include "slme.h"
#include "nlme.h"
#include "dmap.h"
#include "dmap_dmo.h"
//#include "dmap_co.h"
#include "dmap_dpo.h"
#include "dmap_armo.h"
//#include "dmap_udo.h"
#include "uap_mo.h"
#include "aslsrvc.h" 
#include "uap.h"
#include "asm.h"
#include "system.h"
#include "porting.h"

typedef void (*PF_SWAP)(void);
extern const DLL_MIB_ADV_JOIN_INFO c_stDefJoinInfo;

/*==============================[ DMAP ]=====================================*/
uint16 g_unAppHandle = 0;

uint8 g_ucSMLinkTimeoutConf;
uint8 g_ucSMLinkTimeout;
uint8 g_ucLinkProbed;

const uint8 c_aTLMEMethParamLen[TLME_METHOD_NO-1] = {1, 18, 16, 18, 18}; // input arguments lenghts for tlmo methods  
    
uint16 g_unSysMngContractID; 

SHORT_ADDR  g_unDAUXRouterAddr;

uint8               g_ucJoinStatus;
JOIN_RETRY_CONTROL  g_stJoinRetryCntrl;

uint32 __attribute__((section(".noinit"))) g_aulNewNodeChallenge[4];
static uint8 g_aucSecMngChallenge[16];

#if defined(ROUTING_SUPPORT) || defined(BACKBONE_SUPPORT)
  NEW_DEVICE_JOIN_INFO g_stDevInfo[MAX_SIMULTANEOUS_JOIN_NO];
  uint8 g_ucDevInfoEntryNo;
#endif // #if defined(ROUTING_SUPPORT || defined(BACKBONE_SUPPORT)
/*==============================[ DMO ]======================================*/
static void DMAP_processReadRequest( READ_REQ_SRVC * p_pReadReq,
                                     APDU_IDTF *     p_pIdtf);    

static void DMAP_processWriteRequest( WRITE_REQ_SRVC * p_pWriteReq,
                                      APDU_IDTF *      p_pIdtf);                                 

static void DMAP_processExecuteRequest( EXEC_REQ_SRVC * p_pExecReq,
                                        APDU_IDTF *     p_pIdtf);

static void DMAP_processReadResponse(READ_RSP_SRVC * p_pReadRsp, APDU_IDTF * p_pAPDUIdtf);

static void DMAP_processWriteResponse(WRITE_RSP_SRVC * p_pWriteRsp);

static void DMAP_processExecuteResponse( EXEC_RSP_SRVC * p_pExecRsp,
                                         APDU_IDTF *     p_pAPDUIdtf );
static void DMAP_joinStateMachine(void);
  
static uint16 DMO_generateSMJoinReqBuffer(uint8* p_pBuf);
static uint16 DMO_generateSecJoinReqBuffer(uint8* p_pucBuf);
static void DMO_checkSecJoinResponse(EXEC_RSP_SRVC* p_pExecRsp);
static void DMO_checkSMJoinResponse(EXEC_RSP_SRVC* p_pExecRsp);

static void DMAP_DMO_applyJoinStatus( uint8 p_ucSFC );

static void DMAP_processDMOResponse(uint16 p_unSrcOID, EXEC_REQ_SRVC* p_pExecReq, EXEC_RSP_SRVC* p_pExecRsp);

  #define DMAP_DMO_forwardJoinReq(...)  SFC_INVALID_SERVICE
  #define DMAP_DMO_forwardJoinResp(...)  

/*==============================[ DLMO ]=====================================*/
static void DMAP_startJoin(void);

static uint8 NLMO_prepareJoinSMIBS(void);
static uint8 NLMO_updateJoinSMIBS(void);
  
/*==============================[ TLMO ]=====================================*/
static uint8 TLMO_execute( uint8    p_ucMethID,
                           uint16   p_unReqSize,
                           uint8*   p_pReqBuf,
                           uint16 * p_pRspSize,
                           uint8*   p_pRspBuf);

/*==============================[ NLMO ]=====================================*/
static uint8 NLMO_execute( uint8    p_ucMethID,
                           uint16   p_unReqSize,
                           uint8 *  p_pReqBuf,
                           uint16 * p_pRspSize,
                           uint8 *  p_pRspBuf);

/*===========================[ DMAP implementation ]=========================*/

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Initializes the device manager application process
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_Init()
{  
//  UDO_Init();
  
  g_unSysMngContractID = INVALID_CONTRACTID;

  g_ucJoinStatus = DEVICE_DISCOVERY;

  DPO_Init();

#if defined(ROUTING_SUPPORT) || defined(BACKBONE_SUPPORT)  
  g_ucDevInfoEntryNo = 0;
#endif 
  
//  SLME_Init();   deleted??
  DMO_Init();
//  CO_Init();
  ARMO_Init();
	g_ucSMLinkTimeout = g_ucSMLinkTimeoutConf;
LOG_ISA100(LOGLVL_ERR,"DMAP join status: NOT JOINED");
}

//
int g_nRequestForLinuxBbr = 0;
int g_nATT_TR_Changed = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief		sniff messages for BBR and respond to request on ATT and routing table  
/// @param  none
/// @return  -- 1 - handled on BBR; 0 - fwd request to TR
/// @remarks -- called only on linux BBR
///      Access level: user level
///      Context:
////////////////////////////////////////////////////////////////////////////////////////////////////
int DMAP_BBR_Sniffer()
{
	// check if there is an incoming APDU that has to be processed by DMAP
	APDU_IDTF stAPDUIdtf;
	const uint8 * pAPDUStart;
	int nLastMsgRet = 0;
	int nMsg = 0;

	// Get all APDUs

	while ( (pAPDUStart = ASLDE_GetMyAPDUfromBuff( UAP_DMAP_ID, &stAPDUIdtf )) )
	{	
		GENERIC_ASL_SRVC  stGenSrvc;
		const uint8 *     pNext = pAPDUStart;
		nMsg++;
		nLastMsgRet = 1;
		while ( (pNext = ASLSRVC_GetGenericObject( pNext, stAPDUIdtf.m_unDataLen - (pNext - pAPDUStart), &stGenSrvc, stAPDUIdtf.m_aucAddr )) )
		{
			g_nRequestForLinuxBbr = 0;
			switch ( stGenSrvc.m_ucType )
			{
			case SRVC_READ_REQ  : DMAP_processReadRequest(  &stGenSrvc.m_stSRVC.m_stReadReq, &stAPDUIdtf ); break;
			case SRVC_WRITE_REQ : DMAP_processWriteRequest( &stGenSrvc.m_stSRVC.m_stWriteReq, &stAPDUIdtf ); break;
			case SRVC_EXEC_REQ  : DMAP_processExecuteRequest( &stGenSrvc.m_stSRVC.m_stExecReq, &stAPDUIdtf ); break;
			case SRVC_READ_RSP  : DMAP_processReadResponse( &stGenSrvc.m_stSRVC.m_stReadRsp, &stAPDUIdtf ) ; break;          
			case SRVC_WRITE_RSP : DMAP_processWriteResponse( &stGenSrvc.m_stSRVC.m_stWriteRsp ); break;          
			case SRVC_EXEC_RSP  : DMAP_processExecuteResponse( &stGenSrvc.m_stSRVC.m_stExecRsp, &stAPDUIdtf ); break;
			case SRVC_ALERT_ACK : ARMO_ProcessAlertAck(stGenSrvc.m_stSRVC.m_stAlertAck.m_ucAlertID); break;
			default: LOG_ISA100(LOGLVL_ERR, "Invalid service type %d", stGenSrvc.m_ucType); break;
			}     
			nLastMsgRet &= g_nRequestForLinuxBbr;
		}
		//ASLDE_DeleteRxAPDU( (uint8*)pAPDUStart ); // todo: check if always applicable       
	}

	return nMsg == 1 && nLastMsgRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Main DMAP task 
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_Task()
{
  if ( g_ucJoinStatus != DEVICE_JOINED )
  {
      DMAP_joinStateMachine();      
  }
  
  // check if there is an incoming APDU that has to be processed by DMAP
  APDU_IDTF stAPDUIdtf;
  const uint8 * pAPDUStart = ASLDE_GetMyAPDU( UAP_DMAP_ID, 0 /*GETAPDU_MSEC*/, &stAPDUIdtf );
  
  if ( pAPDUStart )
  {
      GENERIC_ASL_SRVC  stGenSrvc;
      const uint8 *     pNext = pAPDUStart;
  
	  LOG_ISA100(LOGLVL_INF, "DMAP_Task: msg srcSAP=%d dstSAP=%d len=%d %s", stAPDUIdtf.m_ucSrcTSAPID, stAPDUIdtf.m_ucDstTSAPID, stAPDUIdtf.m_unDataLen
		, (stAPDUIdtf.m_ucPriorityAndFlags & TLDE_ECN_MASK) == TLDE_ECN_MASK ? "(CONGESTION)":"" );
      if( (stAPDUIdtf.m_ucPriorityAndFlags & TLDE_ECN_MASK) == TLDE_ECN_MASK)
      {
          DMO_NotifyCongestion( INCOMING_DATA_ECN, &stAPDUIdtf); 
      }

      while ( (pNext = ASLSRVC_GetGenericObject( pNext,
                                                stAPDUIdtf.m_unDataLen - (pNext - pAPDUStart),
                                                &stGenSrvc,
                                                stAPDUIdtf.m_aucAddr))
              )
      {
	LOG_ISA100(LOGLVL_INF, "DMAP_Task: object srcSAP=%d dstSAP=%d type=%d SOID=%d DOID=%d", stAPDUIdtf.m_ucSrcTSAPID, stAPDUIdtf.m_ucDstTSAPID, stGenSrvc.m_ucType, stGenSrvc.m_stSRVC.m_stReadReq.m_unSrcOID, stGenSrvc.m_stSRVC.m_stReadReq.m_unDstOID );
          switch ( stGenSrvc.m_ucType )
          {
          case SRVC_READ_REQ  : DMAP_processReadRequest(  &stGenSrvc.m_stSRVC.m_stReadReq, &stAPDUIdtf ); break;
          case SRVC_WRITE_REQ : DMAP_processWriteRequest( &stGenSrvc.m_stSRVC.m_stWriteReq, &stAPDUIdtf ); break;
          case SRVC_EXEC_REQ  : DMAP_processExecuteRequest( &stGenSrvc.m_stSRVC.m_stExecReq, &stAPDUIdtf ); break;
          case SRVC_READ_RSP  : DMAP_processReadResponse( &stGenSrvc.m_stSRVC.m_stReadRsp, &stAPDUIdtf ) ; break;          
          case SRVC_WRITE_RSP : DMAP_processWriteResponse( &stGenSrvc.m_stSRVC.m_stWriteRsp ); break;          
          case SRVC_EXEC_RSP  : DMAP_processExecuteResponse( &stGenSrvc.m_stSRVC.m_stExecRsp, &stAPDUIdtf ); break;
          case SRVC_ALERT_ACK : ARMO_ProcessAlertAck(stGenSrvc.m_stSRVC.m_stAlertAck.m_ucAlertID);          break;
          }        
      }
      //ASLDE_DeleteRxAPDU( (uint8*)pAPDUStart ); // todo: check if always applicable       
  }
  
//  CO_ConcentratorTask();
  
  if(DEVICE_JOINED == g_ucJoinStatus)
  {
    ARMO_Task();  
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generic function for writing an attribute
/// @param  p_ucTSAPID - SAP ID of the process
/// @param  p_unObjID - object identifier
/// @param  p_pIdtf - attribute identifier structure
/// @param  p_pBuf - buffer containing attribute data
/// @param  p_unLen - buffer size
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 SetGenericAttribute( uint8       p_ucTSAPID,
                           uint16      p_unObjID,
                                ATTR_IDTF * p_pIdtf,
                                const uint8 *     p_pBuf,
                                uint16      p_unLen)
{
    if( UAP_DMAP_ID == p_ucTSAPID )
    {    
	switch( p_unObjID )
	{
        case DMAP_NLMO_OBJ_ID:  return NLMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );
//        case DMAP_UDO_OBJ_ID:   return UDO_Write( p_pIdtf->m_unAttrID & 0x003F, p_unLen, p_pBuf );
//        case DMAP_DLMO_OBJ_ID:  return DLMO_Write( p_pIdtf, &p_unLen, p_pBuf );
        case DMAP_DMO_OBJ_ID:   return DMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );            
        case DMAP_ARMO_OBJ_ID:  return ARMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );            
        case DMAP_DSMO_OBJ_ID:  return DSMO_Write( p_pIdtf->m_unAttrID, p_unLen, p_pBuf );
        case DMAP_DPO_OBJ_ID:   return DPO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf ); 
        case DMAP_ASLMO_OBJ_ID: return ASLMO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);               
/*  case DMAP_HRCO_OBJ_ID:
                g_pstCrtCO = AddConcentratorObject(p_unObjID, UAP_DMAP_ID);
                if( g_pstCrtCO )
                {
                    //the appropriate CO element already exist or was just added - update/set the object attributes  
                    //g_pstCrtCO will be used inside CO_Write!!!!!
                    return CO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf );
                }
      break;
*/
        case DMAP_TLMO_OBJ_ID:               
            if (p_unLen < 0xFF)
            {
                return TLME_SetRow( p_pIdtf->m_unAttrID,
                                     0,                   // no TAI cutover
                                     p_pBuf,
                                     (uint8)p_unLen); 
            }
            break;
	}
    }
    else
    {
        //UAP_PROCESS_ID
        switch( p_unObjID )
        {
/*        case UAP_DATA_OBJ_ID: 
            if( p_pIdtf->m_unAttrID < DIGITAL_DATA_ATTR_ID_OFFSET )
                return UAPDATA_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);
            else
                return UAPDATA_Write(p_pIdtf->m_unAttrID - DIGITAL_DATA_ATTR_ID_OFFSET + UAP_DATA_DIGITAL1, p_unLen, p_pBuf);
*/            
        case UAP_MO_OBJ_ID:   return UAPMO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);  
/*        case UAP_PO_OBJ_ID:   return UAP_PO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf); 
            case UAP_CO_OBJ_ID:
                g_pstCrtCO = AddConcentratorObject(p_unObjID, UAP_APP1_ID);
                if( g_pstCrtCO )
                {
                    //the appropriate CO element already exist or was just added - update/set the object attributes  
                    //g_pstCrtCO will be used inside CO_Write!!!!!
                    return CO_Write(p_pIdtf->m_unAttrID, p_unLen, p_pBuf);
                }
                break;
*/
//            case UAP_DISP_OBJ_ID: break;
        }
    }

    return SFC_INVALID_OBJ_ID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generic function for reading an attribute
/// @param  p_ucTSAPID - SAP ID of the process
/// @param  p_unObjID - object identifier
/// @param  p_pIdtf - attribute identifier structure
/// @param  p_pBuf - output buffer containing attribute data
/// @param  p_punLen - buffer size
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 GetGenericAttribute( uint8       p_ucTSAPID,
                           uint16      p_unObjID,
                                ATTR_IDTF * p_pIdtf,
                                uint8 *     p_pBuf,
                                uint16 *    p_punLen)
{
    if( UAP_DMAP_ID == p_ucTSAPID )
    {
        switch( p_unObjID )
        {
        case DMAP_NLMO_OBJ_ID:  return NLMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );  
        //case DMAP_UDO_OBJ_ID:   return UDO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf ); 
        //case DMAP_DLMO_OBJ_ID:  return DLMO_Read( p_pIdtf, p_punLen, p_pBuf );                      
        case DMAP_DMO_OBJ_ID:   return DMO_Read( p_pIdtf, p_punLen, p_pBuf );
        case DMAP_DSMO_OBJ_ID:  return DSMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
        case DMAP_ARMO_OBJ_ID:  return ARMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
        case DMAP_DPO_OBJ_ID:   return DPO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );                
        case DMAP_ASLMO_OBJ_ID: return ASLMO_Read( p_pIdtf->m_unAttrID, p_punLen, p_pBuf );  
        
/*      
        case DMAP_HRCO_OBJ_ID:
            g_pstCrtCO = FindConcentratorByObjId(p_unObjID, UAP_DMAP_ID);
            if( g_pstCrtCO )
            {
                return CO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
            }
            break;        
*/      
        case DMAP_TLMO_OBJ_ID  : 
              *p_punLen = 0;
              return  TLME_GetRow( p_pIdtf->m_unAttrID, 
                                   NULL,        // no indexed attributes in TLMO
                                   0,           // no indexed attributes in TLMO
                                   p_pBuf,
                                   (uint8*)p_punLen );
                
	}
    }  
    else
    {
        //UAP_PROCESS_ID
        switch( p_unObjID )
        {
        case UAP_MO_OBJ_ID:    return UAPMO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf);  
/*        case UAP_DATA_OBJ_ID:  
            if( p_pIdtf->m_unAttrID < DIGITAL_DATA_ATTR_ID_OFFSET )
                return UAPDATA_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf);
            else
                return UAPDATA_Read(p_pIdtf->m_unAttrID - DIGITAL_DATA_ATTR_ID_OFFSET + UAP_DATA_DIGITAL1, p_punLen, p_pBuf);
                        
        case UAP_PO_OBJ_ID:    return UAP_PO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf); 
        case UAP_CO_OBJ_ID:     
            g_pstCrtCO = FindConcentratorByObjId(p_unObjID, UAP_APP1_ID);
            if( g_pstCrtCO )
            {
                return CO_Read(p_pIdtf->m_unAttrID, p_punLen, p_pBuf );
            }
            break;
        
//            case UAP_DISP_OBJ_ID: break;  */
        }
    }
              
    *p_punLen = 0;
    return SFC_INVALID_OBJ_ID;
}

void DMAP_ExecuteGenericMethod(  uint8           p_ucTSAPID,                                 
                                 EXEC_REQ_SRVC * p_pExecReq,
                                 APDU_IDTF *     p_pIdtf,
                                 EXEC_RSP_SRVC * p_pExecRsp )
{
    p_pExecRsp->m_unLen = 0;
    
    if( p_ucTSAPID != UAP_DMAP_ID )            
        return; 
    
    switch( p_pExecReq->m_unDstOID )
    {
/*        case DMAP_UDO_OBJ_ID:
            UDO_ProcessExecuteRequest( p_pExecReq, p_pExecRsp );
            break;
        
        case DMAP_DLMO_OBJ_ID:
            p_pExecRsp->m_ucSFC = DLMO_Execute( p_pExecReq->m_ucMethID,
                                                p_pExecReq->m_unLen,
                                                p_pExecReq->p_pReqData,
                                                &p_pExecRsp->m_unLen,
                                                p_pExecRsp->p_pRspData);      
            break;*/
        
        case DMAP_TLMO_OBJ_ID:
            p_pExecRsp->m_unLen = 0;
            p_pExecRsp->m_ucSFC = TLMO_execute( p_pExecReq->m_ucMethID,
                                                p_pExecReq->m_unLen,
                                                p_pExecReq->p_pReqData,
                                                &p_pExecRsp->m_unLen,
                                                p_pExecRsp-> p_pRspData);
            break; 
        
        case DMAP_NLMO_OBJ_ID:
            p_pExecRsp->m_unLen = 0;
            p_pExecRsp->m_ucSFC = NLMO_execute( p_pExecReq->m_ucMethID,
                                                p_pExecReq->m_unLen,
                                                p_pExecReq->p_pReqData,
                                                &p_pExecRsp->m_unLen,
                                                p_pExecRsp->p_pRspData);
            break; 
        
        case DMAP_DMO_OBJ_ID:
            p_pExecRsp->m_unLen = 0;      
            switch( p_pExecReq->m_ucMethID )
            {
/*                case DMO_PROXY_SM_JOIN_REQ: 
                case DMO_PROXY_SM_CONTR_REQ:
                case DMO_PROXY_SEC_SYM_REQ:
                  
                    if (!p_pIdtf) 
                        return; // just a protection if this function is called incorrectly
                                   
                    p_pExecRsp->m_ucSFC = DMAP_DMO_forwardJoinReq( p_pExecReq, p_pIdtf );
                    if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
                    {
                        ASLSRVC_AddGenericObject( p_pExecRsp,
                                                  SRVC_EXEC_RSP,
                                                  0,                 // priority 
                                                  UAP_DMAP_ID,       // SrcTSAPID
                                                  UAP_DMAP_ID,       // DstTSAPID
                                                  0,
                                                  blabla,  // dest EUI64 addr
                                                  0,                   // no contract
                                                  0  );
                    }
                    return; // don't send a response yet */
                    
                default:
                    p_pExecRsp->m_ucSFC = DMO_Execute( p_pExecReq->m_ucMethID,
                                                       p_pExecReq->m_unLen,
                                                       p_pExecReq->p_pReqData,
                                                       &p_pExecRsp->m_unLen,
                                                       p_pExecRsp->p_pRspData);
            }
            break;
            
        case DMAP_DSMO_OBJ_ID:
            DSMO_Execute( p_pExecReq, p_pExecRsp );
            break;
            
        case DMAP_DPO_OBJ_ID:     
            p_pExecRsp->m_ucSFC = DPO_Execute(  p_pExecReq->m_ucMethID,
                                                p_pExecReq->m_unLen, 
                                                p_pExecReq->p_pReqData,
                                                &p_pExecRsp->m_unLen,
                                                p_pExecRsp->p_pRspData);
            break;            
            
        case DMAP_ARMO_OBJ_ID:    
            p_pExecRsp->m_ucSFC = ARMO_Execute( p_pExecReq->m_ucMethID,
                                                p_pExecReq->m_unLen, 
                                                p_pExecReq->p_pReqData,
                                                &p_pExecRsp->m_unLen,
                                                p_pExecRsp->p_pRspData);
            break;
            
        case DMAP_ASLMO_OBJ_ID:
            p_pExecRsp->m_ucSFC = ASLMO_Execute( p_pExecReq->m_ucMethID,
                                                 p_pExecReq->m_unLen, 
                                                 p_pExecReq->p_pReqData,
                                                 &p_pExecRsp->m_unLen,
                                                 p_pExecRsp->p_pRspData);
            break;
            
        default: 
            p_pExecRsp->m_ucSFC = SFC_INVALID_OBJ_ID;
            p_pExecRsp->m_unLen = 0;
            break;  
    }
    
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Implements the join state machine
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_joinStateMachine(void)
{
    switch ( g_ucJoinStatus )
    {      
    case DEVICE_DISCOVERY:    
    #ifdef BACKBONE_SUPPORT
        if( g_ucProvisioned && (MLSM_GetCrtTaiSec() > 1576800000) ) // make sure the time was synchronized (aprox. at least 1stJanuary2008)
            {    
	        NLMO_prepareJoinSMIBS();
        	DMAP_startJoin();
            }
    #endif
        break;
        
    case DEVICE_SECURITY_JOIN_REQ_SENT:
    case DEVICE_SM_JOIN_REQ_SENT:
    case DEVICE_SM_CONTR_REQ_SENT: 
        if (!g_stJoinRetryCntrl.m_unJoinTimeout) 
        {
		LOG_ISA100(LOGLVL_DBG, "DMAP_joinStateMachine: state SECURITY_JOIN_REQ_SENT, SM_JOIN_REQ_SENT or SM_CONTR_REQ_SENT (%d)", g_ucJoinStatus);
                  DMAP_DLMO_ResetStack();
        }
        break; 
        
    case DEVICE_SEND_SM_JOIN_REQ:
    {
        //retry until the sufficient space for request
        //send the SM Join Request
        EXEC_REQ_SRVC stExecReq;
        uint8 aucReqParams[MAX_PARAM_SIZE];
        
        stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
        stExecReq.m_unDstOID = DMAP_DMO_OBJ_ID;
        stExecReq.m_ucMethID = DMO_PROXY_SM_JOIN_REQ;  
        stExecReq.p_pReqData = aucReqParams;
        stExecReq.m_unLen    = DMO_generateSMJoinReqBuffer( stExecReq.p_pReqData );    
        
        if( SFC_SUCCESS == ASLSRVC_AddGenericObject(  &stExecReq,
                                                    SRVC_EXEC_REQ,
                                                    3,                 // maximum priority
                                                    UAP_DMAP_ID,   // SrcTSAPID
                                                    UAP_DMAP_ID,   // DstTSAPID
                                                    GET_NEW_APP_HANDLE(),
                                                    NULL,              // dest EUI64 address
                                                    JOIN_CONTRACT_ID,  // ContractID
                                                    0, //p_unBinSize
							0
                                                     ) )
        {
            //for each Request from join process need to update the timeout 
            g_stJoinRetryCntrl.m_unJoinTimeout = 1 << (c_stDefJoinInfo.m_mfDllJoinBackTimeout & 0x0F);
            g_ucJoinStatus = DEVICE_SM_JOIN_REQ_SENT;
        }
        break;
    }    
    
    case DEVICE_SEND_SM_CONTR_REQ:
    {
        //retry until the sufficient space for request
        //send the SM Join Contract Request
        EXEC_REQ_SRVC stExecReq;
            
        stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
        stExecReq.m_unDstOID = DMAP_DMO_OBJ_ID;
        stExecReq.m_ucMethID = DMO_PROXY_SM_CONTR_REQ;  
        stExecReq.p_pReqData = c_oEUI64BE;  //just the EUI64 of the device needed
        stExecReq.m_unLen    = 8;    
    
        if( SFC_SUCCESS == ASLSRVC_AddGenericObject(  &stExecReq,
                                                    SRVC_EXEC_REQ,
                                                    3,                 // maximum priority
                                                    UAP_DMAP_ID,   // SrcTSAPID
                                                    UAP_DMAP_ID,   // DstTSAPID
                                                    GET_NEW_APP_HANDLE(),
                                                    NULL,              // dest EUI64 address
                                                    JOIN_CONTRACT_ID,   // ContractID
                                                    0, //p_unBinSize
							0
                                                    ) )
        {
            //for each Request from join process need to update the timeout 
            g_stJoinRetryCntrl.m_unJoinTimeout = 1 << (c_stDefJoinInfo.m_mfDllJoinBackTimeout & 0x0F);
            g_ucJoinStatus = DEVICE_SM_CONTR_REQ_SENT;
        }
        break;
    }    
        

    case DEVICE_SEND_SEC_CONFIRM_REQ:
    {
        //retry until the sufficient space for request
        //send the SM Join Security Confirm Request
        uint8 aucTemp[48];
        
        //prepare the request buffer
        memcpy(aucTemp, g_aulNewNodeChallenge, 16);
        memcpy(aucTemp + 16, g_aucSecMngChallenge, 16);
        memcpy(aucTemp + 32, c_oEUI64BE, 8);
        memcpy(aucTemp + 40, c_oSecManagerEUI64BE, 8);
            
        if( SFC_SUCCESS == Keyed_Hash_MAC(g_aJoinAppKey, aucTemp, sizeof(aucTemp)) )
        {
            EXEC_REQ_SRVC stExecReq;
            stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
            stExecReq.m_unDstOID = SM_PSMO_OBJ_ID;
            stExecReq.m_ucMethID = PSMO_SECURITY_JOIN_CONF;  
            stExecReq.p_pReqData = aucTemp;
            stExecReq.m_unLen    = 16;    
        
            if( SFC_SUCCESS == ASLSRVC_AddGenericObject(  &stExecReq,
                                                        SRVC_EXEC_REQ,
                                                        3,                 // maximum priority
                                                        UAP_DMAP_ID,       // SrcTSAPID
                                                        UAP_SMAP_ID,       // DstTSAPID
                                                        GET_NEW_APP_HANDLE(),
                                                        NULL,              // dest EUI64 address
                                                        g_unSysMngContractID,        // SM Contract ID
                                                        0, //p_unBinSize
							0
                                                        ) )
            {
                //for each Request from join process need to update the timeout 
                g_stJoinRetryCntrl.m_unJoinTimeout = 1 << (c_stDefJoinInfo.m_mfDllJoinBackTimeout & 0x0F);
                g_ucJoinStatus = DEVICE_SEC_CONFIRM_REQ_SENT;
            }
        }
        break;
    }    
    
    case DEVICE_SEC_CONFIRM_REQ_SENT:
        if (!g_stJoinRetryCntrl.m_unJoinTimeout)
        {
		LOG_ISA100(LOGLVL_DBG, "DMAP_joinStateMachine: state DEVICE_SEC_CONFIRM_REQ_SENT");
            DMAP_DLMO_ResetStack();
        }
        break;
    } 
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes a read request in the application queue and passes it to the target object 
/// @param  p_pReadReq - read request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processReadRequest( READ_REQ_SRVC * p_pReadReq,
                              APDU_IDTF *     p_pIdtf)

{
  READ_RSP_SRVC stReadRsp;
  uint8         aucRspBuff[MAX_GENERIC_VAL_SIZE]; // todo: check the size of the buffer

  stReadRsp.m_unDstOID = p_pReadReq->m_unSrcOID;
  stReadRsp.m_unSrcOID = p_pReadReq->m_unDstOID;
  stReadRsp.m_ucReqID  = p_pReadReq->m_ucReqID;
  
  stReadRsp.m_pRspData = aucRspBuff;
  stReadRsp.m_unLen = sizeof(aucRspBuff); // inform following functions about maximum available buffer size;
  
  // check the ECN of the request
  if ((p_pIdtf->m_ucPriorityAndFlags & TLDE_ECN_MASK) == TLDE_ECN_MASK)
  {
      stReadRsp.m_ucFECCE = 1;
  }
  
  stReadRsp.m_ucSFC = GetGenericAttribute( UAP_DMAP_ID,
                                           p_pReadReq->m_unDstOID,    
                                                &p_pReadReq->m_stAttrIdtf, 
                                                stReadRsp.m_pRspData, 
                                                &stReadRsp.m_unLen );
  
  // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
  ASLSRVC_AddGenericObject(  &stReadRsp,
                             SRVC_READ_RSP,
                             0,                         // priority
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID
                             GET_NEW_APP_HANDLE(),
                             NULL,                      // EUI64 addr
                             g_unSysMngContractID,
                             0, //p_unBinSize
				0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes a write request in the application queue and passes it to the target object 
/// @param  p_pWriteReq - write request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processWriteRequest( WRITE_REQ_SRVC * p_pWriteReq,
                               APDU_IDTF *      p_pIdtf)
{
  WRITE_RSP_SRVC stWriteRsp;  
  
  stWriteRsp.m_unDstOID = p_pWriteReq->m_unSrcOID;
  stWriteRsp.m_unSrcOID = p_pWriteReq->m_unDstOID;
  stWriteRsp.m_ucReqID  = p_pWriteReq->m_ucReqID;
  
  // check the ECN of the request
  if ((p_pIdtf->m_ucPriorityAndFlags & TLDE_ECN_MASK) == TLDE_ECN_MASK)
  {
      stWriteRsp.m_ucFECCE = 1;
  }
  


  stWriteRsp.m_ucSFC = SetGenericAttribute( UAP_DMAP_ID,
                                            p_pWriteReq->m_unDstOID,
                                                 &p_pWriteReq->m_stAttrIdtf,
                                                 p_pWriteReq->p_pReqData,
                                                 p_pWriteReq->m_unLen );                 
                                             
  // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
  ASLSRVC_AddGenericObject(  &stWriteRsp,
                             SRVC_WRITE_RSP,
                             0,
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID; faster than extracting it from contract
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID; faster than extracting it from contract                             
                             GET_NEW_APP_HANDLE(),
                             NULL,                      // EUI64 addr
                             g_unSysMngContractID,
                             0,                       // p_unBinSize
				0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes an execute request in the application queue and passes it to the target object
/// @param  p_pExecReq - execute request structure
/// @param  p_pIdtf - apdu structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processExecuteRequest( EXEC_REQ_SRVC * p_pExecReq,
                                 APDU_IDTF *     p_pIdtf )
{
    EXEC_RSP_SRVC stExecRsp;
    uint8         aucRsp[MAX_RSP_SIZE]; // check this size
    
    stExecRsp.p_pRspData = aucRsp;
    stExecRsp.m_unDstOID = p_pExecReq->m_unSrcOID;
    stExecRsp.m_unSrcOID = p_pExecReq->m_unDstOID;
    stExecRsp.m_ucReqID  = p_pExecReq->m_ucReqID;
    
    // check the ECN of the request
    if ((p_pIdtf->m_ucPriorityAndFlags & TLDE_ECN_MASK) == TLDE_ECN_MASK)
    {
        stExecRsp.m_ucFECCE = 1;
    }
    
    DMAP_ExecuteGenericMethod( UAP_DMAP_ID, p_pExecReq, p_pIdtf, &stExecRsp );                                 

    // todo: check if Add succsesfull, otherwise mark the processed apdu as NOT_PROCESSED
    ASLSRVC_AddGenericObject(  &stExecRsp,
                             SRVC_EXEC_RSP,
                             0,                         // priority
                             p_pIdtf->m_ucDstTSAPID,    // SrcTSAPID
                             p_pIdtf->m_ucSrcTSAPID,    // DstTSAPID                             
                             GET_NEW_APP_HANDLE(),
                             NULL,                      // EUI64 addr
                             g_unSysMngContractID,
                             0,                       // p_unBinSize
				0 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Processes a read response service
/// @param  p_pReadRsp - pointer to the read response structure
/// @param  p_pAPDUIdtf - pointer to the appropriate request APDU
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processReadResponse(READ_RSP_SRVC * p_pReadRsp, APDU_IDTF * p_pAPDUIdtf)
{
    uint16            unTxAPDULen;
    GENERIC_ASL_SRVC  stGenSrvc;
    
    const uint8 * pTxAPDU = ASLDE_SearchOriginalRequest( p_pReadRsp->m_ucReqID,
                                                         p_pReadRsp->m_unSrcOID,
                                                         p_pReadRsp->m_unDstOID,
                                                         p_pAPDUIdtf,
                                                         &unTxAPDULen, NULL);
    if( NULL == pTxAPDU )
        return;
    
    if( NULL == ASLSRVC_GetGenericObject( pTxAPDU, unTxAPDULen, &stGenSrvc , NULL) )
        return;
    
    if( SRVC_READ_REQ != stGenSrvc.m_ucType )
        return;
    
    if( stGenSrvc.m_stSRVC.m_stReadReq.m_unDstOID != p_pReadRsp->m_unSrcOID )
        return; 
    
    if (p_pReadRsp->m_ucFECCE)
    {
        DMO_NotifyCongestion( OUTGOING_DATA_ECN, p_pAPDUIdtf ); 
    }
    
    if( SFC_SUCCESS == p_pReadRsp->m_ucSFC && SM_STSO_OBJ_ID == p_pReadRsp->m_unSrcOID )
    {
    
        switch( stGenSrvc.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID )
        {
#ifdef BACKBONE_SUPPORT
            case STSO_CTR_UTC_OFFSET:
                if( sizeof(g_stDPO.m_nCurrentUTCAdjustment) == p_pReadRsp->m_unLen )   
                {
                    g_stDPO.m_nCurrentUTCAdjustment = ((uint16)p_pReadRsp->m_pRspData[0] << 8) | p_pReadRsp->m_pRspData[1];
                }    
                break;
            case STSO_NEXT_UTC_TIME:
                if( sizeof(g_stDMO.m_ulNextDriftTAI) == p_pReadRsp->m_unLen )
                {
                    g_stDMO.m_ulNextDriftTAI = ((uint32)*p_pReadRsp->m_pRspData << 24) |
                                               ((uint32)*(p_pReadRsp->m_pRspData+1) << 16) | 
                                               ((uint32)*(p_pReadRsp->m_pRspData+2) << 8) | 
                                               *(p_pReadRsp->m_pRspData+3);
                }
                break;
            case STSO_NEXT_UTC_OFFSET:
                if( sizeof(g_stDMO.m_unNextUTCDrift) == p_pReadRsp->m_unLen )
                {
                    g_stDMO.m_unNextUTCDrift = ((uint16)p_pReadRsp->m_pRspData[0] << 8) | p_pReadRsp->m_pRspData[1];
                }
                break;
#endif  //#ifdef BACKBONE_SUPPORT
            default:;
        }
    }
    else
    {
        //update for other desired attributes
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes a write response service
/// @param  p_pWriteRsp - pointer to the write response structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processWriteResponse(WRITE_RSP_SRVC * p_pWriteRsp)
{
  // TBD; no write request is issued yet by DMAP  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, 
/// @brief  Processes an device management object response
/// @param  p_unSrcOID - source object identifier
/// @param  p_ucMethID - method identifier
/// @param  p_pExecRsp - pointer to the execute response structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static void DMAP_processDMOResponse(uint16 p_unSrcOID, EXEC_REQ_SRVC* p_pExecReq, EXEC_RSP_SRVC* p_pExecRsp)
{
	LOG_ISA100(LOGLVL_DBG, "DMAP_processDMOResponse:soid:%d, methid:%d", p_unSrcOID, p_pExecReq->m_ucMethID);
  switch(p_unSrcOID)
  {
    case DMAP_DMO_OBJ_ID:
      {
          if(DMO_PROXY_SEC_SYM_REQ == p_pExecReq->m_ucMethID && DEVICE_SECURITY_JOIN_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device Security Join Response
            DMO_checkSecJoinResponse(p_pExecRsp);
            break;
          }  
          if(DMO_PROXY_SM_JOIN_REQ == p_pExecReq->m_ucMethID && DEVICE_SM_JOIN_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device SM Join Response
            DMO_checkSMJoinResponse(p_pExecRsp);
            break;
          }
          if(DMO_PROXY_SM_CONTR_REQ == p_pExecReq->m_ucMethID && DEVICE_SM_CONTR_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device SM Contract Response
            DMO_ProcessFirstContract(p_pExecRsp);   
            if( DEVICE_SEND_SEC_CONFIRM_REQ == g_ucJoinStatus )
            {
                //the contract response was successfully processed
		uint8 status = NLMO_updateJoinSMIBS();
	        if( SFC_SUCCESS != status )
	        {
			LOG_ISA100(LOGLVL_DBG, "DMAP_processDMOResponse: NLMO_updateJoinSMIBS() returned SFC %d", status);
	                DMAP_DLMO_ResetStack();
	        }
	        else
	        {
	              //contract and session key available - update the TL security level
	              uint8 ucTmp;
	              if( SLME_FindTxKey(g_stDMO.m_aucSysMng128BitAddr,ISA100_START_PORTS,ISA100_SMAP_PORT, &ucTmp ) )// have key to SM's UAP
	              {
	                  g_stDSMO.m_ucTLSecurityLevel = SECURITY_ENC_MIC_32;
	              }
	        }
	    }
            break;
          }
      }
      break;
    
    case SM_PSMO_OBJ_ID:
      {
/*          if(PSMO_SECURITY_JOIN_REQ == p_pExecReq->m_ucMethID)
          {
            //proxy router Security Join Response
            DMAP_DMO_forwardJoinResp(p_pExecRsp);
            break;
          }*/
          if( PSMO_SECURITY_JOIN_CONF == p_pExecReq->m_ucMethID && DEVICE_SEC_CONFIRM_REQ_SENT == g_ucJoinStatus )
          {
            //backbone or device Security Join Confirm Response
            DMAP_DMO_applyJoinStatus(p_pExecRsp->m_ucSFC);
            break;
          }
      }
      break;
/*      
    case SM_DMSO_OBJ_ID:
      {
          if((DMSO_JOIN_REQUEST == p_pExecReq->m_ucMethID) ||
             (DMSO_CONTRACT_REQUEST == p_pExecReq->m_ucMethID))
          {
            //proxy router SM Join Response or SM Contract Request
            DMAP_DMO_forwardJoinResp(p_pExecRsp);            
            break;
          }
      }
      break;
*/      
    case SM_SCO_OBJ_ID:
      {
        if(SCO_REQ_CONTRACT == p_pExecReq->m_ucMethID)
        {
          //process contract response
          DMO_ProcessContractResponse(p_pExecReq, p_pExecRsp);
          break;
        }
        /*
        if(SCO_TERMINATE_CONTRACT == p_pExecReq->m_ucMethID)
        {
          //TODO
        }
        */
      }
      break;
      
    default:
      break;
      
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Processes an execute response in the application queue and passes it to the target object 
/// @param  p_pExecRsp - pointer to the execute response structure
/// @param  p_pAPDUIdtf - pointer to the appropriate request APDU
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_processExecuteResponse( EXEC_RSP_SRVC * p_pExecRsp,
                                  APDU_IDTF *     p_pAPDUIdtf )
{  
    uint16            unTxAPDULen;
    GENERIC_ASL_SRVC  stGenSrvc;
    
	LOG_ISA100(LOGLVL_INF, "DMAP_processExecuteResponse");
    const uint8 * pTxAPDU = ASLDE_SearchOriginalRequest( p_pExecRsp->m_ucReqID,                                                 
                                                         p_pExecRsp->m_unSrcOID,
                                                         p_pExecRsp->m_unDstOID,
                                                         p_pAPDUIdtf,                                                         
                                                         &unTxAPDULen, NULL);  
    if( NULL == pTxAPDU )
        return;
    
    if( NULL == ASLSRVC_GetGenericObject( pTxAPDU, unTxAPDULen, &stGenSrvc, NULL ) )
        return;
    
    if( SRVC_EXEC_REQ != stGenSrvc.m_ucType )
        return;
    
    if( stGenSrvc.m_stSRVC.m_stExecReq.m_unDstOID != p_pExecRsp->m_unSrcOID )
        return; 
    
    if (p_pExecRsp->m_ucFECCE)
    {
        DMO_NotifyCongestion( OUTGOING_DATA_ECN, p_pAPDUIdtf ); 
    }
    
    switch( p_pExecRsp->m_unDstOID )
    {
        case DMAP_DLMO_OBJ_ID  :
        case DMAP_TLMO_OBJ_ID  :
        case DMAP_NLMO_OBJ_ID  :
        case DMAP_ASLMO_OBJ_ID : break;
        
        case DMAP_DSMO_OBJ_ID  :
            if( PSMO_SEC_NEW_SESSION == stGenSrvc.m_stSRVC.m_stExecReq.m_ucMethID )
            {
                //response for the Update_Session_Key_Request not needed to be parsed 
            }
            break;
        
        case DMAP_DMO_OBJ_ID   : 
          {
            DMAP_processDMOResponse(p_pExecRsp->m_unSrcOID, 
                                    &stGenSrvc.m_stSRVC.m_stExecReq,
                                    p_pExecRsp);
          }
          break;        
        default:
          break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Updates join status on a succesfull join-confirm-response
/// @param  p_ucSFC - service feedback code
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_applyJoinStatus( uint8 p_ucSFC )
{
      if ( p_ucSFC != SFC_SUCCESS )
      {      
	LOG_ISA100(LOGLVL_DBG, "DMAP_DMO_applyJoinStatus called with SFC:%d", p_ucSFC);
          DMAP_DLMO_ResetStack();   
          return;  
      }  
          
      g_ucJoinStatus = DEVICE_JOINED;
LOG_ISA100(LOGLVL_ERR, "DMAP join status: JOINED");
	UAP_OnJoin();
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei, Mircea Vlasin
/// @brief  Checks the link with the System Manager, based on a SM's response
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
///      Context: if no SM response during some seconds reset the stack
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DMO_CheckSMLink(void)
{
    if( DEVICE_JOINED == g_ucJoinStatus )
    {
        if( g_ucSMLinkTimeout )
        {
	    NLME_CONTRACT_ATTRIBUTES* pContract = NLME_FindContract(g_unSysMngContractID);
	    uint8 nRetryTimeout = pContract ? pContract->m_nRTO : g_unDmapRetryTout;
            if( g_ucDmapMaxRetry * nRetryTimeout >= g_ucSMLinkTimeout && !g_ucLinkProbed )
            {
                //generate a ReadRequest to the SM for keepalive reason
                READ_REQ_SRVC stReadReq;
                stReadReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
                stReadReq.m_unDstOID = SM_STSO_OBJ_ID;

                stReadReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_NO_INDEX;
                stReadReq.m_stAttrIdtf.m_unAttrID = STSO_CTR_UTC_OFFSET;
                
                if( SFC_SUCCESS == ASLSRVC_AddGenericObject( &stReadReq,
                                                             SRVC_READ_REQ,
                                                             3,                 // priority
                                                             UAP_DMAP_ID,       // SrcTSAPID
                                                             UAP_SMAP_ID,       // DstTSAPID
                                                             GET_NEW_APP_HANDLE(),
                                                             NULL,                  // dest EUI64 address
                                                             g_unSysMngContractID,   // SM Contract ID
                                                             0, //p_unBinSize
                                                             0 )  )
                {
		    g_ucLinkProbed = 1;
                }
            }
            g_ucSMLinkTimeout--;
        }
        else // timeout expired
        {
		LOG_ISA100(LOGLVL_ERR, "DMAP_DMO_CheckSMLink: SM is not responding!");
            DMAP_DLMO_ResetStack();
            g_ucSMLinkTimeout = g_ucSMLinkTimeoutConf;
        }    
    }  
}

#define CONTRACT_BUF_LEN    53
#define ATT_BUF_LEN         34
#define ROUTE_BUF_LEN       50
#if  CONTRACT_BUF_LEN  != 53 
	#error Someone changed the contract buffer length!
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Prepares indexed attributes needed for the join process
/// @param  none
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_prepareJoinSMIBS(void)
{

#ifndef BACKBONE_SUPPORT
    DLMO_SMIB_ENTRY stEntry;
    // search neighbor table EUI64 address of router
    if( SFC_SUCCESS == DLME_GetSMIBRequest( DL_NEIGH, g_unDAUXRouterAddr, &stEntry) )
    {
        if (memcmp(stEntry.m_stSmib.m_stNeighbor.m_aEUI64,
                   c_aucInvalidEUI64Addr, 
                   sizeof(c_aucInvalidEUI64Addr)))
        {
            
            // add a contract to the advertising router
            uint8 aucTmpBuf[CONTRACT_BUF_LEN];
            aucTmpBuf[0] = (uint8)(JOIN_CONTRACT_ID) >> 8;
            aucTmpBuf[1] = (uint8)(JOIN_CONTRACT_ID);      
            memcpy(aucTmpBuf + 2, c_aLocalLinkIpv6Prefix, 8);
            memcpy(aucTmpBuf + 10, c_oEUI64BE, 8);
            //double the index
            memcpy(aucTmpBuf + 18, aucTmpBuf, 18);
            
            memcpy(aucTmpBuf + 36, c_aLocalLinkIpv6Prefix, 8);  
            DLME_CopyReversedEUI64Addr(aucTmpBuf + 44, stEntry.m_stSmib.m_stNeighbor.m_aEUI64);
            
            aucTmpBuf[52] = 0x03; // b1b0 = priority; b3= include contract flag;
            
            NLME_SetRow( NLME_ATRBT_ID_CONTRACT_TABLE, 
                        0, 
                        aucTmpBuf,
                        CONTRACT_BUF_LEN );   
            
            
            // add the address translation for the advertising router
            memcpy(aucTmpBuf, c_aLocalLinkIpv6Prefix, 8);
            DLME_CopyReversedEUI64Addr(aucTmpBuf + 8, stEntry.m_stSmib.m_stNeighbor.m_aEUI64);
            memcpy(aucTmpBuf + 16, aucTmpBuf, 16);
            aucTmpBuf[32] = g_unDAUXRouterAddr >> 8;
            aucTmpBuf[33] = g_unDAUXRouterAddr;
            
            NLME_SetRow( NLME_ATRBT_ID_ATT_TABLE, 
                        0, 
                        aucTmpBuf,
                        ATT_BUF_LEN );
            
            return SFC_SUCCESS;
        }
    }
    return SFC_FAILURE;
 #else
    //add a contract to the SM
    uint8 aucTmpBuf[CONTRACT_BUF_LEN];
    aucTmpBuf[0] = (uint8)(JOIN_CONTRACT_ID) >> 8;
    aucTmpBuf[1] = (uint8)(JOIN_CONTRACT_ID);      
    memcpy(aucTmpBuf + 2, g_stDMO.m_auc128BitAddr, 16);
LOG_HEX_ISA100(LOGLVL_DBG,"Setting SM address for JOIN:", g_stDMO.m_aucSysMng128BitAddr, 16);
    //double the index
    memcpy(aucTmpBuf + 18, aucTmpBuf, 18);
    
    memcpy(aucTmpBuf + 36, g_stDMO.m_aucSysMng128BitAddr, 16);  
    aucTmpBuf[52] = 0x03; // b1b0 = priority; b3= include contract flag;
    
    NLME_SetRow( NLME_ATRBT_ID_CONTRACT_TABLE, 
                0, 
                aucTmpBuf,
                CONTRACT_BUF_LEN );   
    
    //add a route to the SM
    memcpy(aucTmpBuf, g_stDMO.m_aucSysMng128BitAddr, 16);
    //double the index
    memcpy(aucTmpBuf + 16, g_stDMO.m_aucSysMng128BitAddr, 16);
    memcpy(aucTmpBuf + 32, g_stDMO.m_aucSysMng128BitAddr, 16);
    aucTmpBuf[48] = 1;    // m_ucNWK_HopLimit = 1
    aucTmpBuf[49] = 1;    // m_bOutgoingInterface = 1
    
    NLME_SetRow( NLME_ATRBT_ID_ROUTE_TABLE, 
                0, 
                aucTmpBuf,
                ROUTE_BUF_LEN); 
    
    return SFC_SUCCESS;
 #endif   
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Updates indexed attributes for the join process
/// @param  none
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_updateJoinSMIBS(void)
{
uint8 aucTmpBuf[CONTRACT_BUF_LEN];    //the maximum size for the used buffer

#ifndef BACKBONE_SUPPORT
    //delete the ATT for advertiser router
    DLMO_SMIB_ENTRY stEntry;
    memcpy(aucTmpBuf, c_aLocalLinkIpv6Prefix, 8);
    
    // search neighbor table EUI64 address of router
    if( SFC_SUCCESS == DLME_GetSMIBRequest(DL_NEIGH, g_unDAUXRouterAddr, &stEntry) )
    {
        DLME_CopyReversedEUI64Addr(aucTmpBuf + 8, stEntry.m_stSmib.m_stNeighbor.m_aEUI64);
        NLME_DeleteRow(NLME_ATRBT_ID_ATT_TABLE, 0, aucTmpBuf, 16 );
    }
#endif

    //for the BBR no needed to change the existing route to SM  
    //add the real ATT table with SM
    memcpy(aucTmpBuf, g_stDMO.m_aucSysMng128BitAddr, 16);
    memcpy(aucTmpBuf + 16, aucTmpBuf, 16);
    aucTmpBuf[32] = g_stDMO.m_unSysMngShortAddr >> 8;
    aucTmpBuf[33] = (uint8)g_stDMO.m_unSysMngShortAddr;
            
    return NLME_SetRow( NLME_ATRBT_ID_ATT_TABLE, 
                        0, 
                        aucTmpBuf,
                        ATT_BUF_LEN );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC
/// @brief  Reinitializes all device layers and sets the device on discovery state
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_DLMO_ResetStack(void)
{
   LOG_ISA100(LOGLVL_INF, "DMAP_DLMO_ResetStack");
  ASLDE_Init();
  TLME_Init();
  NLME_Init();
  SLME_Init();  
  
  DMAP_Init();
  UAP_OnJoinReset();
  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Checks the timeouts based on second accuracy 
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_CheckSecondCounter(void)
{
#ifndef BACKBONE_SUPPORT
  UpdateBannedRouterStatus();
#endif
  
  if (g_stJoinRetryCntrl.m_unJoinTimeout)  g_stJoinRetryCntrl.m_unJoinTimeout--;
  DMAP_DMO_CheckSMLink();
  DMO_PerformOneSecondTasks();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Generates a Security join request buffer
/// @param  p_pucBuf - the request buffer
/// @return data size
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 DMO_generateSecJoinReqBuffer(uint8* p_pucBuf)
{
    //TO DO - need to generate the unique device challenge
    
    g_aulNewNodeChallenge[0] =  g_unRandValue;
    g_aulNewNodeChallenge[1] =  rand();

    g_aulNewNodeChallenge[2] =  MLSM_GetCrtTaiSec();
    
    memcpy(p_pucBuf, c_oEUI64BE, 8);
  
    memcpy(p_pucBuf + 8, g_aulNewNodeChallenge, 16);
    
    //add the Key Info field
    #warning "need to be specified the content coding of the Key_Info"
    p_pucBuf[24] = 0x00;
    
    //add the algorihm identifier field
    p_pucBuf[25] = 0x01;     //only the AES_CCM symmetric key algorithm must used 
                             //b7..b4 - public key algorithm and options
                             //b3..b0 - symmetric key algorithm and mode
    
    AES_Crypt_User(g_aJoinAppKey, (const uint8*)g_aulNewNodeChallenge, p_pucBuf, 26, p_pucBuf + 26, 0);
    
    return (26 + 4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Validates join security response
/// @param  p_pExecRsp - pointer to the response service structure 
/// @return None
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
#define SEC_JOIN_RESP_SIZE      71
void DMO_checkSecJoinResponse(EXEC_RSP_SRVC* p_pExecRsp)
{
	LOG_ISA100(LOGLVL_DBG,"DMO_checkSecJoinResponse: Soid:%d Doid:%d RId:%d FECCE:%02X SFC:%d Len:%d",
		p_pExecRsp->m_unSrcOID, p_pExecRsp->m_unDstOID, p_pExecRsp->m_ucReqID, p_pExecRsp->m_ucFECCE, p_pExecRsp->m_ucSFC, p_pExecRsp->m_unLen);
    if( SFC_VALUE_LIMITED == p_pExecRsp->m_ucSFC )
    {
        //the advertisement router not support another device to join
        g_stJoinRetryCntrl.m_unJoinTimeout = 0;     //force rejoin process
        return;
    }
    
    if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
        return;     //wait timeout then rejoin
    
    if( p_pExecRsp->m_unLen < SEC_JOIN_RESP_SIZE )
        return;
    
    uint8 aucHashEntry[MAX_HMAC_INPUT_SIZE];
    
    memcpy(aucHashEntry, p_pExecRsp->p_pRspData, 16);       //Security Manager Challenge
    memcpy(aucHashEntry + 16, g_aulNewNodeChallenge, 16);   //New Device Challenge
    memcpy(aucHashEntry + 32, c_oEUI64BE, 8);
    memcpy(aucHashEntry + 40, c_oSecManagerEUI64BE, 8);
    
#ifdef DLL_KEY_ID_NOT_AUTHENTICATED
    memcpy(aucHashEntry + 48, p_pExecRsp->p_pRspData + 32, SEC_JOIN_RESP_SIZE - 65); //32(Offset) + 1(DLL Key Id) + 32(Keys Content)
    memcpy(aucHashEntry + 48 + SEC_JOIN_RESP_SIZE - 65, p_pExecRsp->p_pRspData + SEC_JOIN_RESP_SIZE - 32, 32);  //copy the Keys Content
#else
    memcpy(aucHashEntry + 48, p_pExecRsp->p_pRspData + 32, SEC_JOIN_RESP_SIZE - 32);
#endif
    
LOG_HEX_ISA100(LOGLVL_DBG,"aucHashEntry before Keyed_Hash_MAC:", aucHashEntry, MAX_HMAC_INPUT_SIZE);
    if( SFC_SUCCESS == Keyed_Hash_MAC(g_aJoinAppKey, aucHashEntry, MAX_HMAC_INPUT_SIZE) )
    {
        //validate Hash_B field
        if( !memcmp(p_pExecRsp->p_pRspData + 16, aucHashEntry, 16) )
        {
            //retain the System Manager Challenge
            memcpy(g_aucSecMngChallenge, p_pExecRsp->p_pRspData, 16);
            
            //compute the Master Key
    #warning " for Hash_B field validation the Device Challenge can be placed before SM Challenge? "
            memcpy(aucHashEntry, g_aulNewNodeChallenge, 16);   //New Device Challenge
            memcpy(aucHashEntry + 16, p_pExecRsp->p_pRspData, 16);  //Security Manager Challenge
            //the Device and SecMngr EUI64s not overwrited inside "aucHashEntry"
        
            Keyed_Hash_MAC(g_aJoinAppKey, aucHashEntry, 48);    //the first 16 bytes from "aucHashEntry" represent the Master Key
        
            //decrypt the DLL Key using the Master Key
#warning " not specified the nonce generation "
            //MIC is not transmitted inside response - Encryption with MIC_SIZE = 4
            uint8 ucEncKeyOffset = SEC_JOIN_RESP_SIZE - 32;
            
            //for pure decryption no result validation needed 
            AES_Decrypt_User_NoMIC( aucHashEntry,              //Master_Key - first 16 bytes
                              (const uint8*)g_aulNewNodeChallenge,     //first 13 bytes 
                              NULL, 0,                   // No authentication
                              p_pExecRsp->p_pRspData + ucEncKeyOffset,
                              32);

            //add the DLL
            uint8* pucKeyPolicy = p_pExecRsp->p_pRspData + ucEncKeyOffset - 7;  //2*3 bytes(Master Key, DLL and Session key policies) + 1 byte(DLL KeyID) 
            uint32 ulLifeTime;
            
            //add the Master Key
            ulLifeTime = (((uint16)(pucKeyPolicy[0])) << 8) | pucKeyPolicy[1];
            ulLifeTime *= 1800;
            
            SLME_SetKey(    NULL, // p_pucPeerIPv6Address, 
                            ISA100_START_PORTS,  // p_ucUdpSPort,
			    ISA100_START_PORTS,  // p_unUdpDPort
                            0,  // p_ucKeyID  - hardcoded
                            aucHashEntry,  // p_pucKey, 
                            c_oSecManagerEUI64BE,  // p_pucIssuerEUI64, 
                            0, // p_ulValidNotBefore
                            0, // ulLifeTime, // p_ulSoftLifetime,
                            0, // ulLifeTime*2, // p_ulHardLifetime,
                            SLM_KEY_USAGE_MASTER, // p_ucUsage, 
                            0 // p_ucPolicy -> need correct policy
                            );

            //add the DLL Key
            ulLifeTime = (((uint16)(pucKeyPolicy[2])) << 8) | pucKeyPolicy[3];
            ulLifeTime *= 1800;
/*          SLME_SetKey(    NULL, // p_pucPeerIPv6Address, 
                            0,  // p_ucUdpPorts,
                            pucKeyPolicy[6],  // p_ucKeyID,
                            pucKeyPolicy + 7,  // p_pucKey, 
                            NULL,  // p_pucIssuerEUI64, 
                            0, // p_ulValidNotBefore
                            ulLifeTime, // p_ulSoftLifetime,
                            ulLifeTime*2, // p_ulHardLifetime,
                            SLM_KEY_USAGE_DLL, // p_ucUsage, 
                            0 // p_ucPolicy -> need correct policy
                            );
*/            
            //add the SM Session key
            const uint8 * pKey =  pucKeyPolicy + 7 + 16;
            memset( aucHashEntry, 0, 16 ); 
            ulLifeTime = ((uint16)(pucKeyPolicy[4])) << 8 | pucKeyPolicy[5];
            if( ulLifeTime || memcmp( pKey, aucHashEntry, 16 ) ) // TL encrypted if ulLifeTime || key <> 0
            {
                ulLifeTime *= 1800;
            
                SLME_SetKey(    g_stDMO.m_aucSysMng128BitAddr, // p_pucPeerIPv6Address, 
                            ISA100_START_PORTS,  // p_ucUdpSPort
                            ISA100_START_PORTS + UAP_SMAP_ID, //dest port - SM's SM_UAP 
                            0,  // p_ucKeyID,
                            pKey,  // p_pucKey, 
                            NULL,  // p_pucIssuerEUI64, 
                            0, // p_ulValidNotBefore
                            ulLifeTime, // p_ulSoftLifetime,
                            ulLifeTime*2, // p_ulHardLifetime,
                            SLM_KEY_USAGE_SESSION, // p_ucUsage, 
                            0 // p_ucPolicy -> need correct policy
                            );
            }
            
            g_ucJoinStatus = DEVICE_SEND_SM_JOIN_REQ;
        }
    }
    
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Generates a System Manager join request buffer
/// @param  p_pBuf - the request buffer
/// @return data size
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint16 DMO_generateSMJoinReqBuffer(uint8* p_pBuf)
{
  uint8* pucTemp = p_pBuf;
  
  memcpy(p_pBuf, c_oEUI64BE, 8);
  p_pBuf += 8;

  *(p_pBuf++) = g_unDllSubnetId >> 8;
  *(p_pBuf++) = g_unDllSubnetId;
  
  //Device Role Capability
  *(p_pBuf++) = g_stDPO.m_unDeviceRole >> 8;
  *(p_pBuf++) = g_stDPO.m_unDeviceRole;
  
  *(p_pBuf++) = sizeof(g_stDMO.m_aucTagName);
  memcpy(p_pBuf, g_stDMO.m_aucTagName, sizeof(g_stDMO.m_aucTagName));  
  p_pBuf += sizeof(g_stDMO.m_aucTagName);
  
  *(p_pBuf++) = c_ucCommSWMajorVer;
  *(p_pBuf++) = c_ucCommSWMinorVer;
  
  *(p_pBuf++) = c_ucSWRevInfoSize;
  memcpy(p_pBuf, c_aucSWRevInfo, c_ucSWRevInfoSize);
  p_pBuf += c_ucSWRevInfoSize;

// when Device Capability inside the SM Join Request
  uint16 unSize;
  DLMO_ReadDeviceCapability(&unSize, p_pBuf); 
  p_pBuf += unSize;

  return p_pBuf - pucTemp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mircea Vlasin
/// @brief  Validates join SM response
/// @param  p_pExecRsp - pointer to the response service structure 
/// @return None
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_checkSMJoinResponse(EXEC_RSP_SRVC* p_pExecRsp)
{
    //TO DO - wait specs to clarify if authentication with MIC32 needed    
    
    if( SFC_VALUE_LIMITED == p_pExecRsp->m_ucSFC )
    {
        //the advertisement router not support another device to join
        g_stJoinRetryCntrl.m_unJoinTimeout = 0;     //force rejoin process
        return;
    }
    
    if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
    {
	LOG_ISA100(LOGLVL_ERR, "ERROR: DMO_checkSMJoinResponse: p_pExecRsp->m_ucSFC: %d", p_pExecRsp->m_ucSFC);
        return; //wait timeout then rejoin
    }
        
    if( p_pExecRsp->m_unLen < 46 )
    {
	LOG_ISA100(LOGLVL_ERR, "ERROR: DMO_checkSMJoinResponse: p_pExecRsp->m_unLen(%d) < 46", p_pExecRsp->m_unLen);
        return;
    }
    memcpy(g_stDMO.m_auc128BitAddr, p_pExecRsp->p_pRspData, 16);
    p_pExecRsp->p_pRspData += 16;
    
    g_stDMO.m_unShortAddr = (uint16)p_pExecRsp->p_pRspData[0] << 8 | p_pExecRsp->p_pRspData[1];
    g_stDMO.m_unAssignedDevRole = (uint16)p_pExecRsp->p_pRspData[2] << 8 | p_pExecRsp->p_pRspData[3];
    p_pExecRsp->p_pRspData += 4;
    
    memcpy(g_stDMO.m_aucSysMng128BitAddr, p_pExecRsp->p_pRspData, 16);
    p_pExecRsp->p_pRspData += 16;
    
    g_stDMO.m_unSysMngShortAddr = (uint16)p_pExecRsp->p_pRspData[0] << 8 | p_pExecRsp->p_pRspData[1];
    p_pExecRsp->p_pRspData += 2;
    
    memcpy(g_stDMO.m_unSysMngEUI64, p_pExecRsp->p_pRspData, 8);

	LOG_ISA100(LOGLVL_INF, "DMO_checkSMJoinResponse: %d", g_stDMO.m_unSysMngShortAddr);
    //update the SM Session Key's issuer
    SLME_UpdateJoinSessionsKeys( g_stDMO.m_unSysMngEUI64, g_stDMO.m_aucSysMng128BitAddr );
    
    g_ucJoinStatus = DEVICE_SEND_SM_CONTR_REQ;
}


/*===========================[ DMO implementation ]==========================*/
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman, Eduard Erdei
/// @brief  Initiates a join request for a non routing device
/// @param  none
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMAP_startJoin(void)
{
//  DLL_MIB_ADV_JOIN_INFO stJoinInfo;  
  
//  if( SFC_SUCCESS != DLME_GetMIBRequest(DL_ADV_JOIN_INFO, &stJoinInfo) )
//      return;
  
  EXEC_REQ_SRVC stExecReq;
  uint8         aucReqParams[MAX_PARAM_SIZE]; // todo: check this size
    
  stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
  stExecReq.m_unDstOID = DMAP_DMO_OBJ_ID;
  stExecReq.m_ucMethID = DMO_PROXY_SEC_SYM_REQ;  
  stExecReq.p_pReqData = aucReqParams;
  stExecReq.m_unLen    = DMO_generateSecJoinReqBuffer( stExecReq.p_pReqData );    
   
  ASLSRVC_AddGenericObject(  &stExecReq,
                             SRVC_EXEC_REQ,
                             0,                 // priority
                             UAP_DMAP_ID,   // SrcTSAPID
                             UAP_DMAP_ID,   // DstTSAPID
                             GET_NEW_APP_HANDLE(),
                             NULL,              // dest EUI64 address
                             JOIN_CONTRACT_ID,   // ContractID
                             0, // p_unBinSize
				0
                               ); 

  g_stJoinRetryCntrl.m_unJoinTimeout = 1 << (c_stDefJoinInfo.m_mfDllJoinBackTimeout & 0x0F);                        
  g_ucJoinStatus = DEVICE_SECURITY_JOIN_REQ_SENT;  
}

//////////////////////////////////////TLMO Object///////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Executes a transport layer management object method
/// @param  p_unMethID  - method identifier
/// @param  p_unReqSize - request buffer size
/// @param  p_pReqBuf   - request buffer containing method parameters
/// @param  p_pRspSize  - pointer to uint16 where to pu response size
/// @param  p_pRspBuf   - pointer to response buffer (output data)
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 TLMO_execute(uint8    p_ucMethID,
                   uint16   p_unReqSize,
                   uint8 *  p_pReqBuf,
                   uint16 * p_pRspSize,
                   uint8 *  p_pRspBuf)
{
  uint8 ucSFC = SFC_SUCCESS; 
  *p_pRspSize = 0;
  
  uint8 * pStart = p_pRspBuf;
  
  if (!p_ucMethID || p_ucMethID >= TLME_METHOD_NO)
      return SFC_INVALID_METHOD;
  
  if (p_unReqSize != c_aTLMEMethParamLen[p_ucMethID-1])
      return SFC_INVALID_SIZE;     
  
  *(p_pRspBuf++) = 0; // SUCCESS.... TL editor is not aware that execute service already has a SFC
  
  switch(p_ucMethID)
  {
  case TLME_RESET_METHID: 
      TLME_Reset(*p_pReqBuf); 
      break;
    
  case TLME_HALT_METHID: 
    {
        uint16 unPortNr = (((uint16)*(p_pReqBuf+16)) << 8) | *(p_pReqBuf+17);
        TLME_Halt( p_pReqBuf, unPortNr ); 
        break;
    }
      
  case TLME_PORT_RANGE_INFO_METHID:
    {
        TLME_PORT_RANGE_INFO stPortRangeInfo;  
        TLME_GetPortRangeInfo( p_pReqBuf, &stPortRangeInfo );
        
        p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, stPortRangeInfo.m_unNbActivePorts);
        p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, stPortRangeInfo.m_unFirstActivePort);
        p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, stPortRangeInfo.m_unLastActivePort);        
        break;          
    }
    
  case TLME_GET_PORT_INFO_METHID:
  case TLME_GET_NEXT_PORT_INFO_METHID:  
    {
        TLME_PORT_INFO stPortInfo;  
        uint16 unPorNo = (((uint16)*(p_pReqBuf+16)) << 8) | *(p_pReqBuf+17);
        
        uint8 ucStatus;

        if (p_ucMethID == TLME_GET_PORT_INFO_METHID)
        {
            ucStatus = TLME_GetPortInfo( p_pRspBuf, unPorNo, &stPortInfo);
        }
        else //if (p_ucMethID == TLME_GET_NEXT_PORT_INFO_METHID)
        {
            ucStatus = TLME_GetNextPortInfo( p_pRspBuf, unPorNo, &stPortInfo);
        }
        
        if (SFC_SUCCESS == ucStatus)
        {
            p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, unPorNo );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_unUID );
            
            *(p_pRspBuf++) = 0; // not compressed  
              
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_IN_OK] );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_IN] );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_OUT_OK] );
            p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, stPortInfo.m_aTPDUCount[TLME_PORT_TPDU_OUT] );
        }
        else
        {
            *pStart = 1; // FAIL
        }
        break;     
    }  
  }
  
  *p_pRspSize = p_pRspBuf - pStart;
  
  return ucSFC;
  
}



//////////////////////////////////////NLMO Object///////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Executes network layer management object methods
/// @param  p_unMethID - method identifier
/// @param  p_unReqSize - request buffer size
/// @param  p_pReqBuf - request buffer containing method parameters
/// @param  p_pRspSize - response buffer size
/// @param  p_pRspBuf - response buffer
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 NLMO_execute(uint8    p_ucMethID,
                   uint16   p_unReqSize,
                   uint8 *  p_pReqBuf,
                   uint16 * p_pRspSize,
                   uint8 *  p_pRspBuf)
{
  uint8 ucSFC;
  #warning " Change: ASL works with uint16 length. DMAP works with uint8 length"
  uint8 ucRspLen = *p_pRspSize;
  
  if ( p_unReqSize < 2 )
      return SFC_INVALID_SIZE;
  
  uint16 unAttrID = ((uint16)p_pReqBuf[0] << 8) | p_pReqBuf[1];
  p_pReqBuf += 2;
  p_unReqSize -= 2;
  
  uint32 ulSchedTAI = 0;
  if( (NLMO_GET_ROW_RT != p_ucMethID) 
      && (NLMO_GET_ROW_CONTRACT_TBL != p_ucMethID) 
      && (NLMO_GET_ROW_ATT != p_ucMethID) )
  {
      if ( p_unReqSize < 4 )
          return SFC_INVALID_SIZE;
      
      ulSchedTAI = ((uint32)p_pReqBuf[0] << 24) |
                   ((uint32)p_pReqBuf[1] << 16) |
                   ((uint32)p_pReqBuf[2] << 8) |
                   p_pReqBuf[3];
      p_pReqBuf += 4;
      p_unReqSize -= 4;
  } 
  
  *p_pRspSize = 0;

//Claudiu Hobeanu: code written to be less intrusive in original device stack 
  if ( (p_ucMethID == NLMO_GET_ROW_RT || p_ucMethID == NLMO_GET_ROW_ATT || p_ucMethID == NLMO_SET_ROW_RT || p_ucMethID == NLMO_SET_ROW_ATT || p_ucMethID == NLMO_DEL_ROW_RT || p_ucMethID == NLMO_DEL_ROW_ATT )
     && (unAttrID == NLME_ATRBT_ID_ROUTE_TABLE || unAttrID == NLME_ATRBT_ID_ATT_TABLE)
     ) 
  {  
	 LOG_ISA100(LOGLVL_INF, "NLMO_execute: RequestForLinuxBbr p_ucMethID=%d unAttrID=%d", p_ucMethID, unAttrID);
	 g_nRequestForLinuxBbr = 1;
     g_nATT_TR_Changed = 1;
  } 

  switch(p_ucMethID)
  {        
  case NLMO_GET_ROW_RT:
  case NLMO_GET_ROW_CONTRACT_TBL:
  case NLMO_GET_ROW_ATT:    
      ucSFC = NLME_GetRow( unAttrID,
                           p_pReqBuf,
                           p_unReqSize,    
                           p_pRspBuf,  
                           &ucRspLen);                 
      *p_pRspSize = ucRspLen;
      break;
      
  case NLMO_SET_ROW_RT:
  case NLMO_SET_ROW_CONTRACT_TBL:
  case NLMO_SET_ROW_ATT:    
      ucSFC = NLME_SetRow( unAttrID,
                           ulSchedTAI,
                           p_pReqBuf,
                           p_unReqSize);     
      break;     
  
  case NLMO_DEL_ROW_RT:  
  case NLMO_DEL_ROW_CONTRACT_TBL:  
  case NLMO_DEL_ROW_ATT:      
      ucSFC = NLME_DeleteRow( unAttrID,
                              ulSchedTAI,
                              p_pReqBuf,
                              p_unReqSize);       
      break;

  default: 
      ucSFC = SFC_INVALID_METHOD;
      break;
  }
  
  return ucSFC;
}

