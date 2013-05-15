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

///o/////////////////////////////////////////////////////////////////////////////////////////////////
/// @file
/// @verbatim
/// Author:       Nivis LLC, Eduard Erdei
/// Date:         January 2008
/// Description:  This file implements the application sub-layer data entity 
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "string.h"
#include "nlme.h"
#include "tlde.h"
#include "aslsrvc.h"
#include "dmap_dmo.h"
#include "uap_mo.h"
#include "dmap.h"
#include "asm.h"
#include "porting.h"

//TODO: this should be somehow merged with the definitions in BulkTransferSrvc
#define UDO_GROUP_FW_ACTIVATION_TAI 133

/* ASL services request ID          */
//uint8   g_ucReqID;
uint8	g_bNativePublish;

/***************************** local functions ********************************/

static uint8 * ASLSRVC_insertAPDUHeader(uint8    p_ucSrvPrimitive,
                                        uint8    p_ucSrvType,
                                        uint16   p_unSrcOID,
                                        uint16   p_unDstOID,
                                        uint8 *  p_pOutBuf);

static const uint8 * ASLSRVC_extractAPDUHeader( const uint8 * p_pData, 
                                                uint16 *      p_unSrcOID,
                                                uint16 *      p_unDstOID);

static uint8 * ASLSRVC_insertAID(ATTR_IDTF *  p_pAttrIdtf, 
                                 uint8 *      p_pOutBuf);

static const uint8 * ASLSRVC_extractAID(  const uint8 * p_pData , 
                                          ATTR_IDTF *   p_pAttrIdtf);

static uint8 * ASLSRVC_insertExtensibleValue( uint8* p_pData, 
                                              uint16 p_unValue);

static const uint8 * ASLSRVC_extractExtensibleValue(  const uint8 * p_pData,
                                                      uint16 * p_pValue);

static const uint8 * ASLSRVC_setReadReqObj( READ_REQ_SRVC * p_pReq,
                                            uint16          p_unOutBufLen,
                                            uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getReadReqObj( const uint8 *   p_pData, 
                                            uint16          p_unDataLen,
                                            READ_REQ_SRVC * p_pObj);

static const uint8 * ASLSRVC_setReadRspObj( READ_RSP_SRVC * p_pObj,
                                            uint16          p_unOutBufLen,
                                            uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getReadRspObj( const uint8 *   p_pData, 
                                            uint16          p_unDataLen,
                                            READ_RSP_SRVC * p_pObj);

static const uint8 * ASLSRVC_setWriteReqObj(  WRITE_REQ_SRVC * p_pObj,
                                              uint16           p_unOutBufLen,
                                              uint8 *          p_pOutBuf);

static const uint8 * ASLSRVC_getWriteReqObj(  const uint8 *    p_pData, 
                                              uint16           p_unDataLen,
                                              WRITE_REQ_SRVC * p_pObj);

static const uint8 * ASLSRVC_setWriteRspObj(  WRITE_RSP_SRVC * p_pObj,
                                              uint16           p_unOutBufLen,
                                              uint8 *          p_pOutBuf);

static const uint8 * ASLSRVC_getWriteRspObj(  const uint8 *    p_pData, 
                                              uint16           p_unDataLen,
                                              WRITE_RSP_SRVC * p_pObj);

static const uint8 * ASLSRVC_setExecuteReqObj(  EXEC_REQ_SRVC * p_pObj,
                                                uint16          p_unOutBufLen,
                                                uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getExecuteReqObj(  const uint8 *   p_pData, 
                                                uint16          p_unDataLen,
                                                EXEC_REQ_SRVC * p_pObj);

static const uint8 * ASLSRVC_setExecuteRspObj(  EXEC_RSP_SRVC * p_pObj,
                                                uint16          p_unOutBufLen,
                                                uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getExecuteRspObj(  const uint8 *   p_pData, 
                                                uint16          p_unDataLen,
                                                EXEC_RSP_SRVC * p_pObj );

static const uint8 * ASLSRVC_setTunnelReqObj(  TUNNEL_REQ_SRVC * p_pObj,
											  uint16          p_unOutBufLen,
											  uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getTunnelReqObj(  const uint8 *   p_pData, 
											  uint16          p_unDataLen,
											  TUNNEL_REQ_SRVC * p_pObj);

static const uint8 * ASLSRVC_setTunnelRspObj(  TUNNEL_RSP_SRVC * p_pObj,
											  uint16          p_unOutBufLen,
											  uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getTunnelRspObj(  const uint8 *   p_pData, 
											  uint16          p_unDataLen,
											  TUNNEL_RSP_SRVC * p_pObj );

static const uint8 * ASLSRVC_setAlertRepObj(    ALERT_REP_SRVC * p_pObj,
                                                uint16          p_unOutBufLen,
                                                uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getAlertRepObj(    const uint8 *   p_pData, 
                                                uint16          p_unDataLen,
                                                ALERT_REP_SRVC * p_pObj );

static const uint8 * ASLSRVC_setAlertAckObj(    ALERT_ACK_SRVC * p_pObj,
                                                uint16          p_unOutBufLen,
                                                uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getAlertAckObj(    const uint8 *   p_pData, 
                                                uint16          p_unDataLen,
                                                ALERT_ACK_SRVC * p_pObj );

static const uint8 * ASLSRVC_setPublishObj(     PUBLISH_SRVC * p_pObj,
                                                uint16          p_unOutBufLen,
                                                uint8 *         p_pOutBuf);

static const uint8 * ASLSRVC_getPublishObj(     const uint8 *   p_pData, 
                                                uint16          p_unDataLen,
                                                PUBLISH_SRVC * p_pObj );


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generic object parser
/// @param  p_pInBuffer   - a pointer to the buffer that has to be parsed
/// @param  p_nInBufLen   - length of the buffer that has to be parsed
/// @param  p_pGenericObj - pointer to generic object where to put the result
/// @param  p_pSrcAddr128 - ipv6 address of the APDU originator
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_GetGenericObject( const uint8 *       p_pInBuffer, 
                                        uint16              p_unInBufLen, 
                                        GENERIC_ASL_SRVC *  p_pGenericObj, 
                                        const uint8 *       p_pSrcAddr128 )
{
  const uint8 * pNext = NULL; 
  bool bReqID = false;
  
  if (!p_unInBufLen) // at least one char
      return pNext;  
  
  p_pGenericObj->m_ucType = (*p_pInBuffer) & 0x9F;  

  switch (p_pGenericObj->m_ucType)
  {
  case SRVC_READ_RSP : pNext = ASLSRVC_getReadRspObj(   p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stReadRsp ); bReqID = true; break;
  case SRVC_WRITE_RSP: pNext = ASLSRVC_getWriteRspObj(  p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stWriteRsp ); bReqID = true; break;
  case SRVC_EXEC_RSP : pNext = ASLSRVC_getExecuteRspObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stExecRsp ); bReqID = true; break;
  case SRVC_TUNNEL_RSP : pNext = ASLSRVC_getTunnelRspObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stTunnelRsp ); break;

  case SRVC_READ_REQ : pNext = ASLSRVC_getReadReqObj(   p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stReadReq ); bReqID = true; break;
  case SRVC_WRITE_REQ: pNext = ASLSRVC_getWriteReqObj(  p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stWriteReq ); bReqID = true; break;
  case SRVC_EXEC_REQ : pNext = ASLSRVC_getExecuteReqObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stExecReq ); bReqID = true; break;
  case SRVC_TUNNEL_REQ : pNext = ASLSRVC_getTunnelReqObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stTunnelReq ); break;

  case SRVC_PUBLISH   : pNext = ASLSRVC_getPublishObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stPublish); break;
  case SRVC_ALERT_REP : pNext = ASLSRVC_getAlertRepObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stAlertRep); break;
  case SRVC_ALERT_ACK : pNext = ASLSRVC_getAlertAckObj(p_pInBuffer, p_unInBufLen, &p_pGenericObj->m_stSRVC.m_stAlertAck); break;
  }      

  if (!pNext) // a malformed APDU has been detected
  {      
      ASLMO_AddMalformedAPDU(p_pSrcAddr128); 
  }
  LOG_ISA100(LOGLVL_INF, "ASLSRVC_Get Srvc x%02X OID %d->%d ReqID %3d Addr[%s]",
			p_pGenericObj->m_ucType,
			p_pGenericObj->m_stSRVC.m_stReadReq.m_unSrcOID,
			p_pGenericObj->m_stSRVC.m_stReadReq.m_unDstOID,
			bReqID ? p_pGenericObj->m_stSRVC.m_stReadReq.m_ucReqID : -1,
			p_pSrcAddr128 ? GetHexC(p_pSrcAddr128, 16) : ""
	);

  return pNext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generic object binarizer; binarizes a generic object into a specified buffer
/// @param  p_pObj          - pointer to the generic object
/// @param  p_ucServiceType - service type (object type)
/// @param  p_ucPriority    - message priority
/// @param  p_ucSrcTSAPID   - source SAP identifier
/// @param  p_ucDstSAPID    - destination SAP identifier
/// @param  p_ucMaxRetries  - maximum number of retransmissions
/// @param  p_ucRetryTimeout- time to discard
/// @param  p_unAppHandle   - application handler
/// @param  p_pDstAddr      - destination EUI64 address
/// @param  p_unContractID  - contract identifier
/// @param  p_ucObeyContractBandwidth
/// @param  p_ucAllowEncryption - send the object as plain text or as set by the security settings
/// @param  p_unAPDULifetime - max number of seconds to keep this packet inside the stack
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8 ASLSRVC_AddGenericObjectEncryptable( void *        p_pObj,
													uint8         p_ucServiceType,
													uint8         p_ucPriority,
													uint8         p_ucSrcTSAPID,
													uint8         p_ucDstTSAPID,
													uint16        p_unAppHandler,
													const uint8 * p_pDstAddr,
													uint16        p_unContractID,
													uint16        p_unBinSize,
													uint8         p_ucObeyContractBandwidth,
													uint8         p_ucAllowEncryption,
													uint8			p_ucNoRspExpected,
													uint16			p_unAPDULifetime )
                                
{
	CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.find (p_unContractID);

	if (it == g_stNlme.m_oContractsMap.end())
	{
		LOG_ISA100(LOGLVL_ERR,"ASLSRVC_AddGenericObjectEncryptable: contract %d missing!", p_unContractID);
		return SFC_NO_CONTRACT;
	}

	CNlmeContractPtr pContract =  it->second;
	if( !p_unBinSize )
	{
		switch (p_ucServiceType){
		case SRVC_READ_REQ:
		case SRVC_WRITE_REQ:
		case SRVC_EXEC_REQ:
			((READ_REQ_SRVC*)p_pObj)->m_ucReqID = pContract->m_ucReqID;
		default:;
		}
	}
	return ASLSRVC_AddGenericObjectEncryptableCore( p_pObj, p_ucServiceType, p_ucPriority, p_ucSrcTSAPID, p_ucDstTSAPID, p_unAppHandler,
							p_pDstAddr, p_unContractID, p_unBinSize, p_ucObeyContractBandwidth, p_ucAllowEncryption,
							p_ucNoRspExpected, p_unAPDULifetime, pContract);
}

uint8 ASLSRVC_AddGenericObjectEncryptableCore( void *        p_pObj,
													uint8         p_ucServiceType,
													uint8         p_ucPriority,
													uint8         p_ucSrcTSAPID,
													uint8         p_ucDstTSAPID,
													uint16        p_unAppHandler,
													const uint8 * p_pDstAddr,
													uint16        p_unContractID,
													uint16        p_unBinSize,
													uint8         p_ucObeyContractBandwidth,
													uint8         p_ucAllowEncryption,
													uint8			p_ucNoRspExpected,
													uint16			p_unAPDULifetime,
													CNlmeContractPtr p_pContract )
{
  // first check if the contract is periodic and if there is already an APDU on this contract in the queue
      DMO_CONTRACT_ATTRIBUTE * pDMOContract = DMO_FindContract( p_unContractID );
      
      if( pDMOContract && pDMOContract->m_ucServiceType == SRVC_PERIODIC_COMM )
      {
          // the previous request has to be replaced by the current request
          ASLDE_DiscardAPDU( p_unContractID );
      }
  
	if ( g_nDeviceType == DEVICE_TYPE_BBR && !g_nRequestForLinuxBbr)
	{
		return SFC_SUCCESS;
	}
	
	if (g_nDeviceType == DEVICE_TYPE_BBR)
	{	LOG_ISA100(LOGLVL_INF, "ASLSRVC_AddGenericObjectEncryptable: ADDING RSP !( g_nDeviceType == DEVICE_TYPE_BBR && !g_nRequestForLinuxBbr)");
	}	

	// signal the contract info or addrEUI64 info
	if (p_pDstAddr)
	{   // no contract; EUI64 destination        
		LOG_ISA100(LOGLVL_ERR,"ASLSRVC_AddGenericObjectEncryptable PROGRAMMER ERROR: EUI64 based APDU's are NOT supported!");
		return SFC_FAILURE;
	} 
	if(!p_pContract)
	{
		CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.find (p_unContractID);	

		if (it == g_stNlme.m_oContractsMap.end())
		{
			LOG_ISA100(LOGLVL_ERR,"ASLSRVC_AddGenericObjectEncryptable: contract %d missing!", p_unContractID);
			return SFC_NO_CONTRACT;
		}

		p_pContract =  it->second;
	}
	
	CAslTxMsg::Ptr pAslMsgPtr(new CAslTxMsg); //pNext->m_unLen

	pAslMsgPtr->m_pWeakContract = p_pContract;



  /* bit0-3 = priority; bit4 = DE; bit5-6 = ECN */
  pAslMsgPtr->m_stQueueHdr.m_ucPriorityAndFlags = (p_ucPriority & 0x0F) | 0x20; // ECN = 01; DE = 0;
  pAslMsgPtr->m_stQueueHdr.m_ucStatus    = APDU_READY_TO_SEND_NO_FC;
  pAslMsgPtr->m_stQueueHdr.m_ucSrcSAP    = p_ucSrcTSAPID;
  pAslMsgPtr->m_stQueueHdr.m_ucDstSAP    = p_ucDstTSAPID;
  pAslMsgPtr->m_stQueueHdr.m_unTimeout   = g_unDmapRetryTout; //this is a good default, and we'll recompute the timeout anyway based on the contract RTO, a few lines below
  pAslMsgPtr->m_stQueueHdr.m_unLifetime  = p_unAPDULifetime;
  
  pAslMsgPtr->m_stQueueHdr.m_unSendTime	= 0;
  pAslMsgPtr->m_stQueueHdr.m_unDefTimeout = pAslMsgPtr->m_stQueueHdr.m_unTimeout;
  pAslMsgPtr->m_stQueueHdr.m_ucRetryNo    = (UAP_DMAP_ID == p_ucSrcTSAPID ? g_ucDmapMaxRetry : g_ucMaxUAPRetries);
  pAslMsgPtr->m_stQueueHdr.m_unAppHandle  = p_unAppHandler; // GET_NEW_APP_HANDLE();
  pAslMsgPtr->m_stQueueHdr.m_ucAllowEncryption = p_ucAllowEncryption;
  pAslMsgPtr->m_stQueueHdr.m_ucObeyContractBandwidth = p_ucObeyContractBandwidth;
  pAslMsgPtr->m_stQueueHdr.m_bInSlidingWindow = false;
  pAslMsgPtr->m_stQueueHdr.m_bNoRspExpected = p_ucNoRspExpected;
  pAslMsgPtr->m_stQueueHdr.m_bDelayedBySlidingWnd = 0;
  pAslMsgPtr->m_stQueueHdr.m_bDelayedByBandLimit = 0;

	memcpy( pAslMsgPtr->m_stQueueHdr.m_aucDest, p_pContract->m_aDestAddress, sizeof(IPV6_ADDR) );
	pAslMsgPtr->m_stQueueHdr.m_nContractId = p_unContractID;

	pAslMsgPtr->m_stQueueHdr.m_unTimeout = pAslMsgPtr->m_stQueueHdr.m_unDefTimeout = p_pContract->m_nRTO;//contract is valid 
      

  pAslMsgPtr->m_stQueueHdr.m_ucReqID      = p_pContract->m_ucReqID;
  pAslMsgPtr->m_stQueueHdr.m_unSrcOID	= ((READ_REQ_SRVC*)p_pObj)->m_unSrcOID;
  pAslMsgPtr->m_stQueueHdr.m_unDstOID	= ((READ_REQ_SRVC*)p_pObj)->m_unDstOID;

  //add text on number of message in list
  uint8  u8ApduTmpBuff[COMMON_APDU_BUF_SIZE];
  
  uint16 unOutBufLen = sizeof(u8ApduTmpBuff) ;
  uint8 * pStart = u8ApduTmpBuff;  
  uint8 * pBuf   = pStart;
   
  
  if( !p_unBinSize ) 
  {
	switch (p_ucServiceType)
	{
	case SRVC_READ_RSP:   pBuf = (uint8*)ASLSRVC_setReadRspObj( (READ_RSP_SRVC*)p_pObj, unOutBufLen, pBuf ); break;
	case SRVC_WRITE_RSP:  pBuf = (uint8*)ASLSRVC_setWriteRspObj( (WRITE_RSP_SRVC*)p_pObj, unOutBufLen, pBuf); break;
	case SRVC_EXEC_RSP:   pBuf = (uint8*)ASLSRVC_setExecuteRspObj( (EXEC_RSP_SRVC*)p_pObj, unOutBufLen, pBuf); break;
	case SRVC_TUNNEL_RSP:   pBuf = (uint8*)ASLSRVC_setTunnelRspObj( (TUNNEL_RSP_SRVC*)p_pObj, unOutBufLen, pBuf); break;
	case SRVC_READ_REQ:   pBuf = (uint8*)ASLSRVC_setReadReqObj( (READ_REQ_SRVC*)p_pObj, unOutBufLen, pBuf);
							pAslMsgPtr->m_stQueueHdr.m_ucReqID = ((READ_REQ_SRVC*)p_pObj)->m_ucReqID;
							if(pAslMsgPtr->m_stQueueHdr.m_ucReqID == p_pContract->m_ucReqID)
							{	++p_pContract->m_ucReqID;
							}
							pAslMsgPtr->m_stQueueHdr.m_ucStatus = APDU_READY_TO_SEND;break;
	case SRVC_WRITE_REQ:  pBuf = (uint8*)ASLSRVC_setWriteReqObj( (WRITE_REQ_SRVC*)p_pObj, unOutBufLen, pBuf);
							pAslMsgPtr->m_stQueueHdr.m_ucReqID = ((WRITE_REQ_SRVC*)p_pObj)->m_ucReqID;
							if(pAslMsgPtr->m_stQueueHdr.m_ucReqID == p_pContract->m_ucReqID)
							{	++p_pContract->m_ucReqID;
							}
							pAslMsgPtr->m_stQueueHdr.m_ucStatus = APDU_READY_TO_SEND;break;
	case SRVC_EXEC_REQ:   pBuf = (uint8*)ASLSRVC_setExecuteReqObj( (EXEC_REQ_SRVC*)p_pObj, unOutBufLen, pBuf);
							pAslMsgPtr->m_stQueueHdr.m_ucReqID = ((EXEC_REQ_SRVC*)p_pObj)->m_ucReqID;
							if(pAslMsgPtr->m_stQueueHdr.m_ucReqID == p_pContract->m_ucReqID)
							{	++p_pContract->m_ucReqID;
							}
							pAslMsgPtr->m_stQueueHdr.m_ucStatus = APDU_READY_TO_SEND;break;
	case SRVC_TUNNEL_REQ:   pBuf = (uint8*)ASLSRVC_setTunnelReqObj( (TUNNEL_REQ_SRVC*)p_pObj, unOutBufLen, pBuf);
							pAslMsgPtr->m_stQueueHdr.m_ucStatus = APDU_READY_TO_SEND;break;
	case SRVC_PUBLISH:    pBuf = (uint8*)ASLSRVC_setPublishObj( (PUBLISH_SRVC*)p_pObj, unOutBufLen, pBuf); break;  
	case SRVC_ALERT_REP:  pBuf = (uint8*)ASLSRVC_setAlertRepObj( (ALERT_REP_SRVC*)p_pObj, unOutBufLen, pBuf);break;
	case SRVC_ALERT_ACK:  pBuf = (uint8*)ASLSRVC_setAlertAckObj( (ALERT_ACK_SRVC*)p_pObj, unOutBufLen, pBuf); break;    
	}
	if (pBuf == NULL){
		LOG_ISA100( LOGLVL_ERR, "ERROR: ASLSRVC_AddGenericObject(H %u):INSUFFICIENT_DEVICE_RESOURCES", p_unAppHandler);
		return SFC_INSUFFICIENT_DEVICE_RESOURCES;
	}
	//ugly hack to cope with strange GE request: the groupFWActivationTAI method on the UDO object in SMAP should get a special timeout
	if( p_ucServiceType==SRVC_EXEC_REQ && p_ucDstTSAPID==UAP_SMAP_ID && ((EXEC_REQ_SRVC*)p_pObj)->m_unDstOID==SM_UDO_OBJ_ID && ((EXEC_REQ_SRVC*)p_pObj)->m_ucMethID==UDO_GROUP_FW_ACTIVATION_TAI)
	{
		pAslMsgPtr->m_stQueueHdr.m_unTimeout = pAslMsgPtr->m_stQueueHdr.m_unDefTimeout = g_unGEspecialTimeout;
	}
  }
  else
  {
     if( unOutBufLen < p_unBinSize )
     {
          return SFC_INSUFFICIENT_DEVICE_RESOURCES;
     }
     
     memcpy( pBuf, p_pObj, p_unBinSize );
     pBuf += p_unBinSize;
  }


  pAslMsgPtr->m_stQueueHdr.m_unAPDULen = pBuf - pStart; 
//  pAslMsgPtr->m_stQueueHdr.m_unLen     = pBuf - u8ApduTmpBuff;

	if(p_ucNoRspExpected && APDU_READY_TO_SEND == pAslMsgPtr->m_stQueueHdr.m_ucStatus)
	{
		pAslMsgPtr->m_stQueueHdr.m_ucStatus = APDU_READY_TO_SEND_NO_FC;
	}

	pAslMsgPtr->m_oTxApduData.assign (pStart, pStart + pAslMsgPtr->m_stQueueHdr.m_unAPDULen);
	
	g_oAslMsgListTx.push_back(pAslMsgPtr);

  LOG_ISA100(LOGLVL_INF, "ASLSRVC_Add Srvc x%02X Pri %d tsap %d->%d OID %d->%d MxRtry %d RtryTo %d AppH %d (ReqID %d) Addr[%s] C %d ObeyB %d Enc %d NoRsp %d",
					p_ucServiceType,
					p_ucPriority,
					p_ucSrcTSAPID,
					p_ucDstTSAPID,
					pAslMsgPtr->m_stQueueHdr.m_unSrcOID,
					pAslMsgPtr->m_stQueueHdr.m_unDstOID,
					pAslMsgPtr->m_stQueueHdr.m_ucRetryNo,
					pAslMsgPtr->m_stQueueHdr.m_unTimeout,
					p_unAppHandler, pAslMsgPtr->m_stQueueHdr.m_ucReqID,
					p_pDstAddr ? GetHexC(p_pDstAddr, 8) : "",
					p_unContractID,
					p_ucObeyContractBandwidth,
					p_ucAllowEncryption,
					p_ucNoRspExpected
				);

  return SFC_SUCCESS;
}

uint8 ASLSRVC_AddGenericObject( void *        p_pObj, 
                                uint8         p_ucServiceType,
                                uint8         p_ucPriority,
                                uint8         p_ucSrcTSAPID,
                                uint8         p_ucDstTSAPID,
//                                uint8         p_ucMaxRetries,
//                                uint16        p_unRetryTimeout,
                                uint16        p_unAppHandle,
                                const uint8 * p_pDstAddr, 
                                uint16        p_unContractID,
                                uint16        p_unBinSize,
                                uint8         p_ucObeyContractBandwidth,
								uint8			p_ucNoRspExpected,
								uint16			p_unAPDULifetime)
{
  return ASLSRVC_AddGenericObjectEncryptable( p_pObj, p_ucServiceType, p_ucPriority, p_ucSrcTSAPID, p_ucDstTSAPID,
                                              p_unAppHandle, p_pDstAddr, p_unContractID, p_unBinSize, p_ucObeyContractBandwidth, 1, p_ucNoRspExpected, p_unAPDULifetime );
}

uint8 ASLSRVC_AddGenericObjectPlain( void *        p_pObj, 
                                uint8         p_ucServiceType,
                                uint8         p_ucPriority,
                                uint8         p_ucSrcTSAPID,
                                uint8         p_ucDstTSAPID,
//                                uint8         p_ucMaxRetries,
//                                uint16        p_unRetryTimeout,
                                uint16        p_unAppHandle,
                                const uint8 * p_pDstAddr, 
                                uint16        p_unContractID,
                                uint16        p_unBinSize,
                                uint8         p_ucObeyContractBandwidth,
								uint8			p_ucNoRspExpected,
								uint16			p_unAPDULifetime)
{
  return ASLSRVC_AddGenericObjectEncryptable( p_pObj, p_ucServiceType, p_ucPriority, p_ucSrcTSAPID, p_ucDstTSAPID,
                                              p_unAppHandle, p_pDstAddr, p_unContractID, p_unBinSize, p_ucObeyContractBandwidth, 0, p_ucNoRspExpected, p_unAPDULifetime );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a read request structure into a specified buffer
/// @param  p_pObj        - pointer to the read request structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setReadReqObj(  READ_REQ_SRVC * p_pObj,
                                      uint16          p_unOutBufLen,
                                      uint8 *         p_pOutBuf)
{
  if( p_unOutBufLen < (5+1+6) )
      return NULL;
  
  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader(SRVC_REQUEST,
                                       SRVC_TYPE_READ,
                                       p_pObj->m_unSrcOID,
                                       p_pObj->m_unDstOID,
                                       p_pOutBuf);  
  // insert request ID
  *(p_pOutBuf++) = p_pObj->m_ucReqID;
  // insert attribute ID
  return ASLSRVC_insertAID(&p_pObj->m_stAttrIdtf, p_pOutBuf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts a readRequest structure object
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the read request structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getReadReqObj(  const uint8 *   p_pData, 
                                      uint16          p_unDataLen,
                                      READ_REQ_SRVC * p_pObj)
{
  const uint8 * pDataStart = p_pData;  
  
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(  p_pData, 
                                        &p_pObj->m_unSrcOID,
                                        &p_pObj->m_unDstOID);
  // extract request ID
  p_pObj->m_ucReqID = *(p_pData++); 
  
  // extract attribute identifier
  p_pData = ASLSRVC_extractAID(p_pData, &p_pObj->m_stAttrIdtf);
  
  if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
  {
      return p_pData;      
  }
  
  return NULL; // there were not enough bytes to read
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a read response into a specified buffer
/// @param  p_pObj        - pointer to the read response structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setReadRspObj(  READ_RSP_SRVC * p_pObj,
                                      uint16          p_unOutBufLen,
                                      uint8 *         p_pOutBuf)
{  
  if ( SFC_SUCCESS != p_pObj->m_ucSFC ) p_pObj->m_unLen = 0;
    
  if( p_unOutBufLen < (5+1+1+1+2+p_pObj->m_unLen) )
    return NULL;
  
  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_RESPONSE,
                                        SRVC_TYPE_READ,
                                        p_pObj->m_unSrcOID,
                                        p_pObj->m_unDstOID,
                                        p_pOutBuf);  
  // insert request ID
  *(p_pOutBuf++) = p_pObj->m_ucReqID;
  // insert FECCE
  *(p_pOutBuf++) = p_pObj->m_ucFECCE & 0x01;
  // insert SFC
  *(p_pOutBuf++) = p_pObj->m_ucSFC;
  
  if ( SFC_SUCCESS == p_pObj->m_ucSFC )
  {
      // copy response data
      p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unLen);
      memcpy(p_pOutBuf, p_pObj->m_pRspData, p_pObj->m_unLen);
      p_pOutBuf += p_pObj->m_unLen;
  }  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts a readResponse APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the read response structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getReadRspObj(  const uint8 *   p_pData, 
                                      uint16          p_unDataLen,
                                      READ_RSP_SRVC * p_pObj)
{
  const uint8 * pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(  p_pData, 
                                        &p_pObj->m_unSrcOID,
                                        &p_pObj->m_unDstOID);
  // extract request ID
  p_pObj->m_ucReqID = *(p_pData++);
  
  // extract FECCE
  p_pObj->m_ucFECCE = *(p_pData++);
  
  // extract SFC
  p_pObj->m_ucSFC = *(p_pData++);
  
  if ( SFC_SUCCESS == p_pObj->m_ucSFC )
  {
      p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_unLen);
      p_pObj->m_pRspData = (uint8*)p_pData;
      p_pData += p_pObj->m_unLen;
  }

  if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
  {
      return p_pData;      
  }
  
  return NULL; // there were not enough bytes to read  
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a write request structure into a specified buffer
/// @param  p_pObj        - pointer to the write request structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setWriteReqObj( WRITE_REQ_SRVC * p_pObj,
                                      uint16           p_unOutBufLen,
                                      uint8 *          p_pOutBuf)
{
  
  if( p_unOutBufLen < ( 5+1+6+p_pObj->m_unLen ) )
      return NULL;
  
  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_REQUEST,
                                        SRVC_TYPE_WRITE,
                                        p_pObj->m_unSrcOID,
                                        p_pObj->m_unDstOID,
                                        p_pOutBuf);  
  // insert request ID
  *(p_pOutBuf++) = p_pObj->m_ucReqID;
  // insert attribute ID
  p_pOutBuf = ASLSRVC_insertAID(&p_pObj->m_stAttrIdtf, p_pOutBuf);
  
  if ( p_pOutBuf ) // ASLDE_insertAID() may return NULL
  {
      p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unLen);
      memcpy(p_pOutBuf, p_pObj->p_pReqData, p_pObj->m_unLen);
      p_pOutBuf += p_pObj->m_unLen;
  }
  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extract writeRequest APDU object
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the write request structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getWriteReqObj( const uint8 *    p_pData, 
                                      uint16           p_unDataLen,
                                      WRITE_REQ_SRVC * p_pObj)
{
  const uint8 * pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
                                      &p_pObj->m_unSrcOID,
                                      &p_pObj->m_unDstOID);  
  // extract request ID
  p_pObj->m_ucReqID = *(p_pData++);  
  
  // extract attribute identifier
  p_pData = ASLSRVC_extractAID(p_pData, &p_pObj->m_stAttrIdtf);
  
  // extract length of generic value 
  p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_unLen);
  
  // set the pointer to write data
  p_pObj->p_pReqData = (uint8*)p_pData;
  p_pData += p_pObj->m_unLen;
  
  if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
  {
      return p_pData;      
  }
  
  return NULL; // there were not enough bytes to read 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a write response into a specified buffer
/// @param  p_pObj        - pointer to the read response structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setWriteRspObj( WRITE_RSP_SRVC * p_pObj,
                                      uint16           p_unOutBufLen,
                                      uint8 *          p_pOutBuf)
{
  if( p_unOutBufLen < ( 5+1+1+1 ) )
      return NULL;
  
  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_RESPONSE,
                                        SRVC_TYPE_WRITE,
                                        p_pObj->m_unSrcOID,
                                        p_pObj->m_unDstOID,
                                        p_pOutBuf); 
  
  // insert request ID
  *(p_pOutBuf++) = p_pObj->m_ucReqID;
  // insert FECCE
  *(p_pOutBuf++) = p_pObj->m_ucFECCE & 0x01;
  // insert SFC
  *(p_pOutBuf++) = p_pObj->m_ucSFC;
  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts a writeResponse APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the write response structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getWriteRspObj( const uint8 *    p_pData, 
                                      uint16           p_unDataLen,
                                      WRITE_RSP_SRVC * p_pObj)
{
  const uint8 * pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(  p_pData, 
                                        &p_pObj->m_unSrcOID,
                                        &p_pObj->m_unDstOID);
  // extract request ID
  p_pObj->m_ucReqID = *(p_pData++);
  
  // extract FECCE
  p_pObj->m_ucFECCE = *(p_pData++);
  
  // extract SFC
  p_pObj->m_ucSFC = *(p_pData++);
  
  if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
  {
      return p_pData;      
  }
  
  return NULL; // there were not enough bytes to read  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a method invocation request structure into a specified buffer
/// @param  p_pObj        - pointer to the method invocation request structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setExecuteReqObj( EXEC_REQ_SRVC * p_pObj,
                                        uint16          p_unOutBufLen,
                                        uint8 *         p_pOutBuf)
{
  if( p_unOutBufLen < (5+1+3+p_pObj->m_unLen) )
      return NULL;
  
  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_REQUEST,
                                        SRVC_TYPE_EXECUTE,
                                        p_pObj->m_unSrcOID,
                                        p_pObj->m_unDstOID,
                                        p_pOutBuf);  
  // insert request ID
  *(p_pOutBuf++) = p_pObj->m_ucReqID;
  // insert method ID
  *(p_pOutBuf++) = p_pObj->m_ucMethID;
  // insert method parameter size
  p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unLen);
  // copy the method parameters; p_pObj->m_unLen should always fit into 15 bits
  memcpy(p_pOutBuf, p_pObj->p_pReqData, p_pObj->m_unLen);
  p_pOutBuf += p_pObj->m_unLen;
  
  return p_pOutBuf;
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts writeRequest APDU object
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the method invocation request structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getExecuteReqObj( const uint8 *   p_pData, 
                                        uint16          p_unDataLen,
                                        EXEC_REQ_SRVC * p_pObj)
{
  const uint8 * pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
                                      &p_pObj->m_unSrcOID,
                                      &p_pObj->m_unDstOID);
  // extract request ID
  p_pObj->m_ucReqID = *(p_pData++);
  
  // extract method ID
  p_pObj->m_ucMethID = *(p_pData++);
  
  // extract length of request parameters
  p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_unLen);
  
  // set the pointer to the method parameters
  p_pObj->p_pReqData = (uint8*)p_pData;
  p_pData += p_pObj->m_unLen;
  
  if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
  {
      return p_pData;      
  }
  
  return NULL; // there were not enough bytes to read 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a method invocation response into a specified buffer
/// @param  p_pObj        - pointer to the method invocation structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setExecuteRspObj( EXEC_RSP_SRVC * p_pObj,
                                        uint16          p_unOutBufLen,
                                        uint8 *         p_pOutBuf)
{
  if( p_unOutBufLen < (5+1+1+1+2+p_pObj->m_unLen) )
      return NULL;
  
  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_RESPONSE,
                                        SRVC_TYPE_EXECUTE,
                                        p_pObj->m_unSrcOID,
                                        p_pObj->m_unDstOID,
                                        p_pOutBuf);  
  // insert request ID
  *(p_pOutBuf++) = p_pObj->m_ucReqID;
  // insert FECCE
  *(p_pOutBuf++) = p_pObj->m_ucFECCE & 0x01;
  // insert SFC
  *(p_pOutBuf++) = p_pObj->m_ucSFC;
  
  // todo: open issue no 48. specification missmatch; size of repsone param must be optional, but how?
  // insert size of response parameters
  p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unLen);
  if ( p_pObj->m_unLen )
  {      
      memcpy(p_pOutBuf, p_pObj->p_pRspData, p_pObj->m_unLen);
      p_pOutBuf += p_pObj->m_unLen;
  }  
  return p_pOutBuf;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts an executeResponse APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the method invocatioan response structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getExecuteRspObj( const uint8 *   p_pData, 
                                        uint16          p_unDataLen,
                                        EXEC_RSP_SRVC * p_pObj )
{
  const uint8 * pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
                                      &p_pObj->m_unSrcOID,
                                      &p_pObj->m_unDstOID);
  // extract request ID
  p_pObj->m_ucReqID = *(p_pData++);
  
  // extract FECCE
  p_pObj->m_ucFECCE = *(p_pData++);
  
  // extract SFC
  p_pObj->m_ucSFC = *(p_pData++);
  
  // todo: open issue no 48. specification missmatch; size of repsone param must be optional, but how?
  // extract size of response parameters
  p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_unLen);
  
  if ( p_pObj->m_unLen )
  {   // set the pointer to the respone params
      p_pObj->p_pRspData = (uint8*)p_pData;
      p_pData += p_pObj->m_unLen;
  }

  if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
  {
      return p_pData;      
  }
  
  return NULL; // there were not enough bytes to read 
}


///////////////////////////////////////////////////==================================================================

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief  binarizes a method invocation request structure into a specified buffer
/// @param  p_pObj        - pointer to the method invocation request structure
/// @param  p_unOutBufLen - available lenght of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setTunnelReqObj( TUNNEL_REQ_SRVC * p_pObj,
									   uint16          p_unOutBufLen,
									   uint8 *         p_pOutBuf)
{
	if( p_unOutBufLen < (5+1+3+p_pObj->m_unLen) )
		return NULL;

	// insert APDU header
	p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_REQUEST,
		SRVC_TYPE_EXECUTE,
		p_pObj->m_unSrcOID,
		p_pObj->m_unDstOID,
		p_pOutBuf);  

	// insert method parameter size
	p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unLen);
	// copy the method parameters; p_pObj->m_unLen should always fit into 15 bits
	memcpy(p_pOutBuf, p_pObj->m_pReqData, p_pObj->m_unLen);
	p_pOutBuf += p_pObj->m_unLen;

	return p_pOutBuf;
} 

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief  extract tunnel Request APDU object
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the method invocation request structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getTunnelReqObj( const uint8 *   p_pData, 
									   uint16          p_unDataLen,
									   TUNNEL_REQ_SRVC * p_pObj)
{
	const uint8 * pDataStart = p_pData; 

	// extract source and destination OIDs
	p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
		&p_pObj->m_unSrcOID,
		&p_pObj->m_unDstOID);
	
	// extract length of request parameters
	p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_unLen);

	// set the pointer to the method parameters
	p_pObj->m_pReqData = (uint8*)p_pData;
	p_pData += p_pObj->m_unLen;

	if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
	{
		return p_pData;      
	}

	return NULL; // there were not enough bytes to read 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief  binarizes a method invocation response into a specified buffer
/// @param  p_pObj        - pointer to the method invocation structure
/// @param  p_unOutBufLen - available lenght of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_setTunnelRspObj( TUNNEL_RSP_SRVC * p_pObj,
									   uint16          p_unOutBufLen,
									   uint8 *         p_pOutBuf)
{
	if( p_unOutBufLen < (5+1+1+1+2+p_pObj->m_unLen) )
		return NULL;

	// insert APDU header
	p_pOutBuf = ASLSRVC_insertAPDUHeader( SRVC_RESPONSE,
		SRVC_TYPE_EXECUTE,
		p_pObj->m_unSrcOID,
		p_pObj->m_unDstOID,
		p_pOutBuf);  
	
	// insert FECCE
	*(p_pOutBuf++) = p_pObj->m_ucFECCE & 0x01;
	
	
	// insert size of response parameters
	p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unLen);
	if ( p_pObj->m_unLen )
	{      
		memcpy(p_pOutBuf, p_pObj->m_pRspData, p_pObj->m_unLen);
		p_pOutBuf += p_pObj->m_unLen;
	}  
	return p_pOutBuf;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Claudiu Hobeanu
/// @brief  extracts a tunnel Response APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the method invocatioan response structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_getTunnelRspObj( const uint8 *   p_pData, 
									   uint16          p_unDataLen,
									   TUNNEL_RSP_SRVC * p_pObj )
{
	const uint8 * pDataStart = p_pData; 

	// extract source and destination OIDs
	p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
		&p_pObj->m_unSrcOID,
		&p_pObj->m_unDstOID);

	// extract FECCE
	p_pObj->m_ucFECCE = *(p_pData++);


	// extract size of response parameters
	p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_unLen);

	if ( p_pObj->m_unLen )
	{   // set the pointer to the response params
		p_pObj->m_pRspData = (uint8*)p_pData;
		p_pData += p_pObj->m_unLen;
	}

	if ( (uint16)(p_pData - pDataStart) <= p_unDataLen )
	{
		return p_pData;      
	}

	return NULL; // there were not enough bytes to read 
}

///////////////////////////////////////////////////==================================================================

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Binarizes an alert report into a specified buffer
/// @param  p_pObj        - pointer to the alert report structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8* ASLSRVC_setAlertRepObj(ALERT_REP_SRVC* p_pObj,
                                    uint16          p_unOutBufLen,
                                    uint8*          p_pOutBuf)
{
  if(p_unOutBufLen < (5 + 1 + 2 + 2 + 6 + 1 + 1 + 2 + p_pObj->m_stAlertInfo.m_unSize))
      return NULL;

  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader(SRVC_REQUEST,
                                       SRVC_TYPE_ALERT_REP,
                                       p_pObj->m_unSrcOID,
                                       p_pObj->m_unDstOID,
                                       p_pOutBuf);  
  
  
  //alert identifier
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_ucID; 
  
  // alert reporting service specific data
  
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_unDetObjTLPort >> 8;
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_unDetObjTLPort;
  
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_unDetObjID >> 8;
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_unDetObjID;
  
  //alert info:
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_stDetectionTime.m_ulSeconds >> 24;
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_stDetectionTime.m_ulSeconds >> 16;
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_stDetectionTime.m_ulSeconds >> 8;
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_stDetectionTime.m_ulSeconds;
  
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_stDetectionTime.m_unFract >> 8;
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_stDetectionTime.m_unFract;  
  
  *p_pOutBuf = p_pObj->m_stAlertInfo.m_ucClass << ROT_ALERT_CLASS; 
  *p_pOutBuf |= (p_pObj->m_stAlertInfo.m_ucDirection << ROT_ALARM_DIRECTION) & MASK_ALARM_DIRECTION;
  *p_pOutBuf |= (p_pObj->m_stAlertInfo.m_ucCategory << ROT_ALERT_CATEGORY) & MASK_ALERT_CATEGORY;
  *(p_pOutBuf++) |= (p_pObj->m_stAlertInfo.m_ucPriority & MASK_ALERT_PRIORITY);
  
  *(p_pOutBuf++) = p_pObj->m_stAlertInfo.m_ucType; 
  
  p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_stAlertInfo.m_unSize);
  
  if(p_pObj->m_stAlertInfo.m_unSize)
    memcpy(p_pOutBuf, p_pObj->m_pAlertValue, p_pObj->m_stAlertInfo.m_unSize);
  p_pOutBuf += p_pObj->m_stAlertInfo.m_unSize;

  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Extracts an alert report APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the alert report structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8* ASLSRVC_getAlertRepObj(const uint8*     p_pData, 
                                    uint16           p_unDataLen,
                                    ALERT_REP_SRVC*  p_pObj)
{
  const uint8* pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
                                      &p_pObj->m_unSrcOID,
                                      &p_pObj->m_unDstOID);
  //alert identifier
  p_pObj->m_stAlertInfo.m_ucID = *(p_pData++); 
  
  // alert reporting service specific data
  
  p_pObj->m_stAlertInfo.m_unDetObjTLPort = (uint16)*(p_pData++) << 8;
  p_pObj->m_stAlertInfo.m_unDetObjTLPort |= *(p_pData++);
  
  p_pObj->m_stAlertInfo.m_unDetObjID = (uint16)*(p_pData++) << 8;
  p_pObj->m_stAlertInfo.m_unDetObjID |= *(p_pData++);
  

  //alert info
  p_pObj->m_stAlertInfo.m_stDetectionTime.m_ulSeconds = ((uint32)*p_pData << 24) |
                                                        ((uint32)*(p_pData+1) << 16) | 
                                                        ((uint32)*(p_pData+2) << 8) | 
                                                        *(p_pData+3);
  p_pData += 4;
  p_pObj->m_stAlertInfo.m_stDetectionTime.m_unFract = ((uint16)*p_pData << 8) |
                                                      *(p_pData+1);
  p_pData += 2;
    
  p_pObj->m_stAlertInfo.m_ucClass		= (*p_pData & MASK_ALERT_CLASS) >> ROT_ALERT_CLASS;
  p_pObj->m_stAlertInfo.m_ucDirection	= (*p_pData & MASK_ALARM_DIRECTION) >> ROT_ALARM_DIRECTION;
  p_pObj->m_stAlertInfo.m_ucCategory	= (*p_pData & MASK_ALERT_CATEGORY) >> ROT_ALERT_CATEGORY;
  p_pObj->m_stAlertInfo.m_ucPriority	= (*p_pData & MASK_ALERT_PRIORITY) >> ROT_ALERT_PRIORITY;
  p_pData++;
  
  p_pObj->m_stAlertInfo.m_ucType = *(p_pData++); 
  
  p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pObj->m_stAlertInfo.m_unSize);
  p_pObj->m_pAlertValue = (uint8*)p_pData;
  p_pData += p_pObj->m_stAlertInfo.m_unSize;

  if((uint16)(p_pData - pDataStart) > p_unDataLen)
    return NULL;   // there were not enough bytes to read    
   
  return p_pData; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Binarizes an alert ack into a specified buffer
/// @param  p_pObj        - pointer to the alert report structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8* ASLSRVC_setAlertAckObj(ALERT_ACK_SRVC* p_pObj,
                                    uint16          p_unOutBufLen,
                                    uint8*          p_pOutBuf)
{
  if(p_unOutBufLen < (5 + 1))
      return NULL;

  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader(SRVC_REQUEST,
                                       SRVC_TYPE_ALERT_ACK,
                                       p_pObj->m_unSrcOID,
                                       p_pObj->m_unDstOID,
                                       p_pOutBuf);  
  
  
  //alert identifier
  *(p_pOutBuf++) = p_pObj->m_ucAlertID; 
  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Extracts an alert ack APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the alert ack structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8* ASLSRVC_getAlertAckObj(const uint8*     p_pData, 
                                    uint16           p_unDataLen,
                                    ALERT_ACK_SRVC*  p_pObj )
{
  const uint8* pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
                                      &p_pObj->m_unSrcOID,
                                      &p_pObj->m_unDstOID);
  //alert identifier
  p_pObj->m_ucAlertID = *(p_pData++); 

  if((uint16)(p_pData - pDataStart) > p_unDataLen)
    return NULL;   // there were not enough bytes to read    
  
  return p_pData; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Binarizes a non-native publish data into a specified buffer
/// @param  p_pObj        - pointer to the publish structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8* ASLSRVC_setPublishObj(PUBLISH_SRVC*   p_pObj,
                                   uint16          p_unOutBufLen,
                                   uint8*          p_pOutBuf)
{
  if(p_unOutBufLen < (5 + 1 +  2 + p_pObj->m_unSize))
      return NULL;

  // insert APDU header
  p_pOutBuf = ASLSRVC_insertAPDUHeader(SRVC_RESPONSE,
                                       SRVC_TYPE_PUBLISH,
                                       p_pObj->m_unPubOID,
                                       p_pObj->m_unSubOID,
                                       p_pOutBuf);  
  
  if(p_pObj->m_ucNativePublish){
	// insert freshness sequence number
	*(p_pOutBuf++) = p_pObj->m_ucContentVersion;
  }

  // insert freshness sequence number
  *(p_pOutBuf++) = p_pObj->m_ucFreqSeqNo;
  // insert length of data
  p_pOutBuf= ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pObj->m_unSize);
  // copy publish data
  memcpy(p_pOutBuf, p_pObj->m_pData, p_pObj->m_unSize);
  p_pOutBuf += p_pObj->m_unSize;
  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Mihaela Goloman
/// @brief  Extracts a native/non native publish APDU object from a buffer
/// @param  p_pData     - a pointer in the APDU buffer
/// @param  p_unDataLen - length of APDU
/// @param  p_pObj      - a pointer to the publish structure
/// @return NULL if fails, pointer to next object if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GetGenericObject
///      Note: don't check inside if length is too short, that is checked by caller  
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8* ASLSRVC_getPublishObj(const uint8*   p_pData, 
                                   uint16         p_unDataLen,
                                   PUBLISH_SRVC*  p_pObj)
{
  const uint8* pDataStart = p_pData; 
   
  // extract source and destination OIDs
  p_pData = ASLSRVC_extractAPDUHeader(p_pData, 
                                      &p_pObj->m_unPubOID,
                                      &p_pObj->m_unSubOID);
  p_pObj->m_ucNativePublish = g_bNativePublish;

  p_pObj->m_unSize = p_unDataLen - (uint16)(p_pData - pDataStart);
  p_pObj->m_pData = (uint8*)p_pData;
  if(g_bNativePublish)
  {
	p_pObj->m_ucContentVersion = *(p_pData++);
  }
  p_pObj->m_ucFreqSeqNo = *(p_pData++);
  
  p_pData = pDataStart + p_unDataLen;
  
  return p_pData; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes an APDU header
/// @param  p_pObj        - pointer to the read request structure
/// @param  p_unOutBufLen - available length of the output buffer
/// @param  p_unOutBuffer - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLSRVC_insertAPDUHeader( uint8    p_ucSrvPrimitive,
                                  uint8    p_ucSrvType,
                                  uint16   p_unSrcOID,
                                  uint16   p_unDstOID,
                                  uint8 *  p_pOutBuf)
{
  *p_pOutBuf = (p_ucSrvPrimitive << 7) | p_ucSrvType;
  
  // determine object identifier addressing mode; inferred mode not implemented
  if (p_unSrcOID < 0x000F && p_unDstOID < 0x000F)
  {
      *(p_pOutBuf++) |= OID_COMPACT << 5;
      *(p_pOutBuf++) = (p_unSrcOID << 4) | p_unDstOID;
  }
  else if (p_unSrcOID < 0x00FF && p_unDstOID < 0x00FF)
  {
      *(p_pOutBuf++) |= OID_MID_SIZE << 5;
      *(p_pOutBuf++) = p_unSrcOID;
      *(p_pOutBuf++) = p_unDstOID;
  }
  else 
  {
      *(p_pOutBuf++) |= OID_FULL_SIZE << 5; 
      *(p_pOutBuf++) = p_unSrcOID >> 8;
      *(p_pOutBuf++) = p_unSrcOID;
      *(p_pOutBuf++) = p_unDstOID >> 8;
      *(p_pOutBuf++) = p_unDstOID;      
  }
  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Parses an APDU header
/// @param  p_pObj        - pointer to the read request structure
/// @param  p_pSrcOID - pointer to location where to put the source object ID
/// @param  p_pDstOID - pointer to location where to put the destination object ID
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Obs: caller must ensure that *p_pSrcOID and *p_pDstOID always contains the previous src and 
///           dst object IDs, except for the first APDU in a concatenated stream.
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_extractAPDUHeader(  const uint8 * p_pData, 
                                          uint16 * p_pSrcOID,
                                          uint16 * p_pDstOID)
{
  uint8 ucOIDMode = ( *(p_pData++) & OBJ_ADDRESSING_TYPE_MASK ) >> 5;
  
  switch ( ucOIDMode )
  {
      case OID_COMPACT:
          *p_pSrcOID = (uint16)( (*p_pData) >> 4 );
          *p_pDstOID = (uint16)( (*(p_pData++)) & 0x000F );
          break;
          
      case OID_MID_SIZE:
          *p_pSrcOID = (uint16)(*(p_pData++));
          *p_pDstOID = (uint16)(*(p_pData++));
          break;
          
      case OID_FULL_SIZE:
          *p_pSrcOID = (uint16)((p_pData[0] << 8) | p_pData[1]);
          *p_pDstOID = (uint16)((p_pData[2] << 8) | p_pData[3]);
          p_pData += 4;
          break;
          
      case OID_INFERRED:
          break;
          
      default:
          break;
  }
  
  return p_pData;  
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes an attribute identifier structure
/// @param  p_pAttrIdtf - pointer to attribute identifier structure
/// @param  p_unOutBuf  - pointer to the output buffer
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLSRVC_insertAID(  ATTR_IDTF * p_pAttrIdtf, 
                            uint8 *     p_pOutBuf)
{
  uint8 ucTwelveBitID;  
  
  // attribute ID coding
  if ( p_pAttrIdtf->m_unAttrID <= 0x003F )
  {
      ucTwelveBitID = 0;
  }
  else if ( p_pAttrIdtf->m_unAttrID <= 0x0FFF )
  {
      ucTwelveBitID = 1;
  }
  else return NULL; // atribute ID does not fit into 12 bits
  
  if ( (p_pAttrIdtf->m_ucAttrFormat > ATTR_NO_INDEX) && (p_pAttrIdtf->m_unIndex1 > 0x7FFF) )
      return NULL; // index1 does not fit into 15 bits
  
  if ( (p_pAttrIdtf->m_ucAttrFormat == ATTR_TWO_INDEX) && (p_pAttrIdtf->m_unIndex2 > 0x7FFF) )
      return NULL; // index2 does not fit into 15 bits
  
  switch (p_pAttrIdtf->m_ucAttrFormat)
  {
      case ATTR_NO_INDEX:     
          if (ucTwelveBitID)
          {
              *(p_pOutBuf++) = (p_pAttrIdtf->m_unAttrID >> 8) | 0xC0;
              *(p_pOutBuf++) = p_pAttrIdtf->m_unAttrID;
          }
          else 
          {
              *(p_pOutBuf++) = p_pAttrIdtf->m_unAttrID;
          }
          break;
                              
      case ATTR_ONE_INDEX:            
          if (ucTwelveBitID)
          {
              *(p_pOutBuf++) = (p_pAttrIdtf->m_unAttrID >> 8) | 0xD0 ; 
              *(p_pOutBuf++) = p_pAttrIdtf->m_unAttrID;                       
          }
          else
          {
              *(p_pOutBuf++) = (p_pAttrIdtf->m_unAttrID) | 0x40;               
          }
          p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pAttrIdtf->m_unIndex1);
          break;
        
      case ATTR_TWO_INDEX:  
          if (ucTwelveBitID)
          {
              *(p_pOutBuf++) = (p_pAttrIdtf->m_unAttrID >> 8) | 0xE0 ; 
              *(p_pOutBuf++) = p_pAttrIdtf->m_unAttrID;                       
          }
          else
          {
              *(p_pOutBuf++) = (p_pAttrIdtf->m_unAttrID) | 0x80;               
          }
          p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pAttrIdtf->m_unIndex1);
          p_pOutBuf = ASLSRVC_insertExtensibleValue(p_pOutBuf, p_pAttrIdtf->m_unIndex2);
          break;    
  }  
  
  return p_pOutBuf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts an attribute identifier structure
/// @param  p_pData - data buffer
/// @param  p_pAttrIdtf - pointer to the attribute identifier structure
/// @return NULL if fails, pointer to the current position in buffer if success
/// @remarks
///      Access level: user level
///      Context: Called from ASLDE_GenericSetObject
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_extractAID( const uint8 * p_pData , 
                                  ATTR_IDTF *   p_pAttrIdtf)
{
  uint8 ucAttrForm = (*p_pData) >> 6;
  
    switch (ucAttrForm)
    {
        case ATTR_TYPE_SIX_BIT_NO_INDEX:
            //p_pAttrIdtf->m_ucAttrFormat = ATTR_NO_INDEX; // this is not needed by caller
            p_pAttrIdtf->m_unAttrID = (uint16)(( *(p_pData++) ) & 0x3F);
            break;
          
        case ATTR_TYPE_SIX_BIT_ONE_DIM:
            //p_pAttrIdtf->m_ucAttrFormat = ATTR_ONE_INDEX; // this is not needed by caller
            p_pAttrIdtf->m_unAttrID = (uint16)(( *(p_pData++) ) & 0x3F);
            p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pAttrIdtf->m_unIndex1);
            break;
          
        case ATTR_TYPE_SIX_BIT_TWO_DIM:
            //p_pAttrIdtf->m_ucAttrFormat = ATTR_TWO_INDEX; // this is not needed by caller
            p_pAttrIdtf->m_unAttrID = (uint16)(( *(p_pData++) ) & 0x3F);
            p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pAttrIdtf->m_unIndex1);
            p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pAttrIdtf->m_unIndex2);
            break;
          
        case ATTR_TYPE_TWELVE_BIT_EXTENDED:
        {
            uint8 ucAIDForm = ((*p_pData) & 0x30) >> 4;
            
            p_pAttrIdtf->m_unAttrID = (uint16)(( (p_pData[0] & 0x0F) << 8 ) | p_pData[1]);
            p_pData += 2;
            
            switch (ucAIDForm)
            {
                case TWELVE_BIT_NO_INDEX:
                    //p_pAttrIdtf->m_ucAttrFormat = ATTR_NO_INDEX; // this is not needed by caller                    
                    break;
                    
                case TWELVE_BIT_ONE_DIM:
                    //p_pAttrIdtf->m_ucAttrFormat = ATTR_ONE_INDEX; // this is not needed by caller
                    p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pAttrIdtf->m_unIndex1);
                    break;
                case TWELVE_BIT_TWO_DIM:
                    //p_pAttrIdtf->m_ucAttrFormat = ATTR_TWO_INDEX; // this is not needed by caller
                    p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pAttrIdtf->m_unIndex1);
                    p_pData = ASLSRVC_extractExtensibleValue(p_pData, &p_pAttrIdtf->m_unIndex2);
                    break;
                case TWELVE_BIT_RESERVED:
                    // reserved type
                    p_pAttrIdtf->m_unAttrID = 0;
                    break;
            }
            break;      
        }
    }
    
  return p_pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Extracts an extensible value from a buffer
/// @param  p_pData - pointer to the data to be parsed
/// @param  p_pValue - (output)a pointer to the extensible value
/// @return pointer at end of extracted data
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * ASLSRVC_extractExtensibleValue( const uint8 * p_pData,
                                              uint16 * p_pValue)
{
  if ( !(*p_pData & 0x80) )
  {
      *p_pValue = (uint16)(*(p_pData++));
      return p_pData;
  }  
  *p_pValue = (uint16)(p_pData[0] << 8 | p_pData[1] ) & 0x7FFF;
  
  return (p_pData+2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Inserts (binarizes) an extensible value into a buffer
/// @param  p_pData     - (output) a pointer to the data buffer
/// @param  p_unValue   - the extensible value
/// @return pointer at end of inserted data
/// @remarks  don't check inside if value fits into 15 bits. this check is done by caller.
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * ASLSRVC_insertExtensibleValue(  uint8 * p_pData, 
                                        uint16  p_unValue)
{  
  if (p_unValue >= 0x80) // long format -> network order
  {
      *(p_pData++) = (uint8)(p_unValue >> 8) | 0x80;
  }
  
  *(p_pData++) = (uint8)p_unValue;
  return (p_pData);  
}
