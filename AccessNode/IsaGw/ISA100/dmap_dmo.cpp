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
/// Date:         December 2008
/// Description:  This file implements the DMO object of the DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>  //ntoh/hton
#include <stdio.h>
#include "dmap_dmo.h"
//#include "dmap_co.h"
#include "dmap_armo.h"
#include "dmap.h"
#include "aslsrvc.h"
#include "string.h"
#include "config.h"
#include "sfc.h"
#include "nlme.h"
#include "tlde.h"
#include "uap.h"
#include "porting.h"

// DMAP DMO Object Constant atributes

const uint8  c_ucNonVolatileMemoryCapable = 0;

const uint32 c_ulDevMemTotal       = 0x2000;  // units in octets

const uint8 c_ucCommSWMajorVer  = 0;          // assigned by ISA100
const uint8 c_ucCommSWMinorVer  = 0;          // assigned by ISA100
char c_aucSWRevInfo[16];		// assigned by LibIsaProvisionDMAP
uint8 c_ucSWRevInfoSize = 0;	// assigned by LibIsaProvisionDMAP

const uint8 c_aucVendorID[]   = {'N', 'I', 'V', 'I', 'S'};
char  c_aucModelID[16];			// assigned by LibIsaProvisionDMAP
const uint8 c_aucSerialNo[]   = {'1', '2', '3', '4'};

const uint8 c_ucDMAPObjCount = (uint8)DMAP_OBJ_NO;

// we will not internally use this data, it will be read by SM; store it as a stream;
const uint8 c_aucObjList[DMAP_OBJ_NO*5] = 
{
  (uint16)(DMAP_DMO_OBJ_ID) >> 8,   (uint8)(DMAP_DMO_OBJ_ID),   (uint8)(OBJ_TYPE_DMO),   0, 0, 
  (uint16)(DMAP_ARMO_OBJ_ID) >> 8,  (uint8)(DMAP_ARMO_OBJ_ID),  (uint8)(OBJ_TYPE_ARMO),  0, 0,
  (uint16)(DMAP_DSMO_OBJ_ID) >> 8,  (uint8)(DMAP_DSMO_OBJ_ID),  (uint8)(OBJ_TYPE_DSMO),  0, 0,
  (uint16)(DMAP_DLMO_OBJ_ID) >> 8,  (uint8)(DMAP_DLMO_OBJ_ID),  (uint8)(OBJ_TYPE_DLMO),  0, 0,
  (uint16)(DMAP_NLMO_OBJ_ID) >> 8,  (uint8)(DMAP_NLMO_OBJ_ID),  (uint8)(OBJ_TYPE_NLMO),  0, 0,
  (uint16)(DMAP_TLMO_OBJ_ID) >> 8,  (uint8)(DMAP_TLMO_OBJ_ID),  (uint8)(OBJ_TYPE_TLMO),  0, 0,
  (uint16)(DMAP_ASLMO_OBJ_ID) >> 8, (uint8)(DMAP_ASLMO_OBJ_ID), (uint8)(OBJ_TYPE_ASLMO), 0, 0,  
  (uint16)(DMAP_UDO_OBJ_ID) >> 8,   (uint8)(DMAP_UDO_OBJ_ID),   (uint8)(OBJ_TYPE_UDO),   0, 0,  
  (uint16)(DMAP_DPO_OBJ_ID) >> 8,   (uint8)(DMAP_DPO_OBJ_ID),   (uint8)(OBJ_TYPE_DPO),   0, 0,
  (uint16)(DMAP_HRCO_OBJ_ID) >> 8,  (uint8)(DMAP_HRCO_OBJ_ID),  (uint8)(OBJ_TYPE_HRCO),  0, 0
};

DMO_ATTRIBUTES g_stDMO;
CDmoContractsMap g_oDmoContractsMap;
#ifdef MULTICONTRACTS
CPendingContractsMap g_oPendingContractRspMap;

#else

static DMO_CONTRACT_ATTRIBUTE g_stPendingContractRsp;
static uint8  g_ucContractReqType;
#endif
static uint8  g_ucContractRspPending;
#ifdef MULTICONTRACTS
#else
static uint8  g_ucContractReqID = 1;
static uint32 g_unTimeout;
#endif

#ifdef MULTICONTRACTS
#define ANY_SERVICE_TYPE 255
static CDmoContractPtr DMO_IsPendingContractRequest(uint8	p_ucPendingState,
						const IPV6_ADDR	p_pDstAddr128,
						uint16		p_unDstTLPort,
						uint16		p_unSrcTLPort,
						uint8		p_ucSrvcType = ANY_SERVICE_TYPE,
						uint8*		p_pnRequestId = NULL);
static void DMO_TimeoutContractReqs(void);
#endif
static void DMO_readTAISec(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);
static void DMO_readContract(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize);

static uint8 DMO_parseContract( const uint8 *              p_pData,                          
                                DMO_CONTRACT_ATTRIBUTE *   p_pContract ); 

static uint8 DMO_deleteContract( uint16 p_unContractID );

static uint8 DMO_setContract( uint16        p_unDeleteIndex,
                              uint32        p_ulTaiCutover, 
                              const uint8 * p_pData, 
                              uint8         p_ucDataLen );

/*uint8 * DMO_InsertBandwidthInfo( uint8 *                         p_pData, 
                                        const DMO_CONTRACT_BANDWIDTH *  p_pBandwidth, 
                                        uint8                           p_ucServiceType);

static uint8 DMO_binarizeContract( const DMO_CONTRACT_ATTRIBUTE * p_pContract,
									uint8  * p_pRspBuf,
									uint16 * p_unBufLen);
*/

static void DMO_checkPowerStatusAlert(void);

static void DMO_generateRestartAlarm(void);

static void DM0_checkContractExpirationTimes(uint32 p_ulCrtTaiSec);

static uint8 DMO_sendContractRequest(DMO_CONTRACT_ATTRIBUTE * p_pContract,
                                     uint8                    p_ucRequestType);
  

// Need to write m_ucSize member at index 7, 8 and 22, cannot be const
DMAP_FCT_STRUCT c_aDMOFct[DMO_ATTR_NO] =
{
  { 0, 0                                              , DMAP_EmptyReadFunc     , NULL },     // just for protection; attributeID will match index in this table     
  { c_oEUI64BE,     sizeof(c_oEUI64BE)                , DMAP_ReadVisibleString , NULL },     // DMO_EUI64                   = 1,
  { ATTR_CONST(g_stDMO.m_unShortAddr)                 , DMAP_ReadUint16        , DMAP_WriteUint16 },        // DMO_16BIT_DL_ALIAS          = 2,
  { ATTR_CONST(g_stDMO.m_auc128BitAddr)               , DMAP_ReadVisibleString , DMAP_WriteVisibleString }, // DMO_128BIT_NWK_ADDR         = 3,   
  { ATTR_CONST(g_stDPO.m_unDeviceRole)                , DMAP_ReadUint16        , NULL },     // DMO_DEVICE_ROLE_CAPABILITY  = 4,
  { ATTR_CONST(g_stDMO.m_unAssignedDevRole)           , DMAP_ReadUint16        , DMAP_WriteUint16 },        // DMO_ASSIGNED_DEVICE_ROLE    = 5,  
  { ATTR_CONST(c_aucVendorID )                        , DMAP_ReadVisibleString , NULL },     // DMO_VENDOR_ID               = 6,
  { ATTR_CONST(c_aucModelID )                         , DMAP_ReadVisibleString , NULL },     // DMO_MODEL_ID                = 7,
  { ATTR_CONST(g_stDMO.m_aucTagName)                  , DMAP_ReadVisibleString , DMAP_WriteVisibleString }, // DMO_TAG_NAME                = 8,  
  { (void *)c_aucSerialNo , sizeof(c_aucSerialNo)     , DMAP_ReadVisibleString , NULL },     // DMO_SERIAL_NO               = 9,  
  { ATTR_CONST(g_stDMO.m_ucPWRStatus)                 , DMAP_ReadUint8         , DMAP_WriteUint8 },         // DMO_PWR_SUPPLY_STATUS       = 10,  
  { ATTR_CONST(g_stDMO.m_stPwrAlertDescriptor)        , DMAP_ReadAlertDescriptor, DMAP_WriteAlertDescriptor }, // DMO_PWR_CK_ALERT_DESCRIPTOR = 11,
  { ATTR_CONST(g_stDMO.m_ucDMAPState)                 , DMAP_ReadUint8         , NULL },     // DMO_DMAP_STATE              = 12,
  { ATTR_CONST(g_stDMO.m_ucJoinCommand)               , DMAP_ReadUint8         , DMAP_WriteUint8 },         // DMAP_JOIN_COMMAND           = 13,  
  { ATTR_CONST(g_stDMO.m_unStaticRevLevel)            , DMAP_ReadUint16        , NULL },     // DMO_STATIC_REV_LEVEL        = 14,
  { ATTR_CONST(g_stDMO.m_unRestartCount)              , DMAP_ReadUint16        , NULL },     // DMAP_RESTART_COUNT          = 15,
  { ATTR_CONST(g_stDMO.m_ulUptime)                    , DMAP_ReadUint32        , NULL },     // DMAP_UPTIME                 = 16,
  { (void*)&c_ulDevMemTotal, sizeof(c_ulDevMemTotal)  , DMAP_ReadUint32        , NULL },     // DMAP_DEV_MEM_TOTAL          = 17,
  { ATTR_CONST(g_stDMO.m_ulUsedDevMem)                , DMAP_ReadUint32        , NULL },     // DMAP_DEV_MEM_USED           = 18,
  { 0, 4                                              , DMO_readTAISec         , NULL },     // DMAP_TAI_TIME               = 19,
  {(void*)&c_ucCommSWMajorVer , sizeof(c_ucCommSWMajorVer), DMAP_ReadUint8     , NULL },     // DMAP_COMM_SW_MAJOR_VERSION  = 20,
  {(void*)&c_ucCommSWMinorVer , sizeof(c_ucCommSWMinorVer), DMAP_ReadUint8     , NULL },     // DMAP_COMM_SW_MINOR_VERSION  = 21,  
  {(void*)c_aucSWRevInfo , sizeof(c_aucSWRevInfo)     , DMAP_ReadVisibleString , NULL },     // DMAP_SW_REVISION_INFO       = 22,
  { ATTR_CONST(g_stDMO.m_aucSysMng128BitAddr)         , DMAP_ReadVisibleString , DMAP_WriteVisibleString }, // DMAP_SYS_MNG_128BIT_ADDR    = 23,
  { ATTR_CONST(g_stDMO.m_unSysMngEUI64)               , DMAP_ReadVisibleString , DMAP_WriteVisibleString }, // DMAP_SYS_MNG_EUI64          = 24, 
  { ATTR_CONST(g_stDMO.m_unSysMngShortAddr)           , DMAP_ReadUint16        , DMAP_WriteUint16 },        // DMAP_SYS_MNG_16BIT_ALIAS    = 25,
  { 0, DMO_CONTRACT_BUF_SIZE                          , DMO_readContract       , NULL },       // DMAP_CONTRACT_TABLE       = 26,
  { ATTR_CONST(g_stDMO.m_unContractReqTimeout)        , DMAP_ReadUint16        , DMAP_WriteUint16 },        // DMAP_CONTRACT_REQ_TIMEOUT   = 27,
  { ATTR_CONST(g_stDMO.m_ucMaxClntSrvRetries)         , DMAP_ReadUint8         , DMAP_WriteUint8 },         // DMAP_MAX_CLNT_SRV_RETRIES   = 28,
  { ATTR_CONST(g_stDMO.m_unMaxRetryToutInterval)      , DMAP_ReadUint16        , DMAP_WriteUint16 },        // DMAP_MAX_RETRY_TOUT_INTERVAL= 29
  {(void*)&c_ucDMAPObjCount, sizeof(c_ucDMAPObjCount) , DMAP_ReadUint8         , NULL},     // DMAP_OBJECTS_COUNT          = 30,
  {(void*)&c_aucObjList, sizeof(c_aucObjList)         , DMAP_ReadVisibleString , NULL },    // DMAP_OBJECTS_LIST           = 31,
  { 0, 4                                              , DMAP_ReadContractMeta  , NULL },    // DMAP_CONTRACT_METADATA      = 32,
  { ATTR_CONST(c_ucNonVolatileMemoryCapable)          , DMAP_ReadUint8         , NULL},     // DMAP_DMO_NON_VOLTATILE_MEM_CAPABLE       = 33,
  { ATTR_CONST(g_stDMO.m_unWarmRestartAttemptTout)    , DMAP_ReadUint16        , DMAP_WriteUint16} // DMO_WARM_RESTART_ATTEMPT_TIMEOUT = 34,
  
#ifdef BACKBONE_SUPPORT
  ,
  { ATTR_CONST(g_stDMO.m_unCrtUTCDrift)                 , DMAP_ReadUint16         , NULL },  // DMO_CRT_UTC_DRIFT           = 35,
  { ATTR_CONST(g_stDMO.m_ulNextDriftTAI)                , DMAP_ReadUint32         , NULL },  // DMO_NEXT_TIME_DRIFT         = 36,
  { ATTR_CONST(g_stDMO.m_unNextUTCDrift)                , DMAP_ReadUint32         , NULL }   // DMO_NEXT_UTC_DRIFT          = 37,
#endif 
};


void DMO_readTAISec(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
  uint32 ulTAI = MLSM_GetCrtTaiSec();
  *p_ucSize = 4;
  DMAP_ReadUint32( &ulTAI, p_pBuf, p_ucSize );   
}

void DMO_readContract(const void * p_pValue, uint8 * p_pBuf, uint8* p_ucSize)
{
    // tbd
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Parses an bandwidth data stream to a dmo bandwidth structure (part of a contract entry)
/// @param  p_pData - buffer containing bandwidth data
/// @param  p_pBandwidth - bandwith structure to be filled
/// @param  p_ucServiceType - type of service (periodic or aperiodic)
/// @return pointer to the next data to be parsed
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8 * DMO_ExtractBandwidthInfo( const uint8 *            p_pData, 
                                        DMO_CONTRACT_BANDWIDTH * p_pBandwidth,
                                        uint8                    p_ucServiceType )
{
  // following line does the same work for both periodic/aperiodic contracts (period or commited burst)
  p_pData = DMAP_ExtractUint16( p_pData, (uint16*)&p_pBandwidth->m_stPeriodic.m_nPeriod); 
  
  if ( p_ucServiceType == SRVC_PERIODIC_COMM )
  {      
      p_pBandwidth->m_stPeriodic.m_ucPhase = *(p_pData++);
      
      p_pData = DMAP_ExtractUint16( p_pData, &p_pBandwidth->m_stPeriodic.m_unDeadline);
  }
  else // non periodic communication service
  {      
      p_pData = DMAP_ExtractUint16( p_pData, (uint16*)&p_pBandwidth->m_stAperiodic.m_nExcessBurst);      
      p_pBandwidth->m_stAperiodic.m_ucMaxSendWindow = *(p_pData++);     
  }  
  return p_pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a bandwidth data structure to a stream(part of a contract entry)
/// @param  p_pData - buffer where to put data
/// @param  p_pBandwidth - pointer to the bandwith structure to be binarized
/// @param  p_ucServiceType - type of service (periodic or aperiodic)
/// @return pointer to the last written byte (actually to the next free available byte in stream)
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 * DMO_InsertBandwidthInfo(  uint8 *                         p_pData, 
                                  const DMO_CONTRACT_BANDWIDTH *  p_pBandwidth, 
                                  uint8                           p_ucServiceType)
{
  // following line does the same work for both periodic/aperiodic contracts (period or commited burst)
  p_pData = DMAP_InsertUint16( p_pData, *(uint16*)&p_pBandwidth->m_stPeriodic.m_nPeriod); 
  
  if ( p_ucServiceType == SRVC_PERIODIC_COMM )
  {      
      *(p_pData++) = p_pBandwidth->m_stPeriodic.m_ucPhase;
      
      p_pData = DMAP_InsertUint16( p_pData, p_pBandwidth->m_stPeriodic.m_unDeadline);
  }
  else // non periodic communication service
  {      
      p_pData = DMAP_InsertUint16( p_pData, *(uint16*)&p_pBandwidth->m_stAperiodic.m_nExcessBurst);      
       *(p_pData++) = p_pBandwidth->m_stAperiodic.m_ucMaxSendWindow;
  }    
  return p_pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Parses an contract data stream to a dmo contractdata structure
/// @param  p_pData     - pointer to the data to be parsed
/// @param  p_pContract - pointer to a dmo contract data structure, where to put data
/// @return service feedback code
/// @remarks
///      Access level: user level
///      Obs:   Check outside if buffer length is correct
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_parseContract( const uint8 *              p_pData,                          
                         DMO_CONTRACT_ATTRIBUTE *   p_pContract )
{  
  p_pData = DMAP_ExtractUint16(p_pData, &p_pContract->m_unContractID); 
  
  p_pContract->m_ucContractStatus = *(p_pData++);  
  p_pContract->m_ucServiceType    = *(p_pData++); 
  
  p_pData = DMAP_ExtractUint32(p_pData, &p_pContract->m_ulActivationTAI); 
  p_pData = DMAP_ExtractUint16(p_pData, &p_pContract->m_unSrcTLPort); 
  
  memcpy(p_pContract->m_aDstAddr128, p_pData, sizeof(p_pContract->m_aDstAddr128));
  p_pData += sizeof(p_pContract->m_aDstAddr128);
  
  p_pData = DMAP_ExtractUint16(p_pData, &p_pContract->m_unDstTLPort);     
  p_pData = DMAP_ExtractUint32(p_pData, &p_pContract->m_ulAssignedExpTime);
  
  p_pContract->m_ucPriority = *(p_pData++); 
  
  p_pData = DMAP_ExtractUint16(p_pData, &p_pContract->m_unAssignedMaxNSDUSize);     
  
  p_pContract->m_ucReliability = *(p_pData++);   
  
  p_pData = DMO_ExtractBandwidthInfo(  p_pData, 
                                       &p_pContract->m_stBandwidth, 
                                       p_pContract->m_ucServiceType );   
  return SFC_SUCCESS;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Binarizes a dmo contract data structure to a stream
/// @param  p_pContract  - pointer to the contract to be binarized
/// @param  p_pRspBuf    - pointer to buffer where to put data
/// @param  p_ucBufLen   - input: available length of output buffer; output: no of bytes put into bufer  
/// @return pointer to the next data to be parsed
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8 DMO_binarizeContract( const DMO_CONTRACT_ATTRIBUTE * p_pContract,  
                            uint8  * p_pRspBuf,
                            uint16 * p_unBufLen)
{
  uint8 * pucStart = p_pRspBuf;
  
  if ( *p_unBufLen < DMO_CONTRACT_BUF_SIZE )
      return SFC_DEVICE_SOFTWARE_CONDITION;
  
  p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, p_pContract->m_unContractID);  
  
  *(p_pRspBuf++) = p_pContract->m_ucContractStatus;    
  *(p_pRspBuf++) = p_pContract->m_ucServiceType;  
  
  p_pRspBuf = DMAP_InsertUint32( p_pRspBuf, p_pContract->m_ulActivationTAI);  
  p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, p_pContract->m_unSrcTLPort);    

  memcpy(p_pRspBuf, p_pContract->m_aDstAddr128, sizeof(p_pContract->m_aDstAddr128));
  p_pRspBuf +=  sizeof(p_pContract->m_aDstAddr128);
  
  p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, p_pContract->m_unDstTLPort);   
  p_pRspBuf = DMAP_InsertUint32(p_pRspBuf, p_pContract->m_ulAssignedExpTime);
  
  *(p_pRspBuf++) = p_pContract->m_ucPriority;  
  
  p_pRspBuf = DMAP_InsertUint16( p_pRspBuf, p_pContract->m_unAssignedMaxNSDUSize);     
  *(p_pRspBuf++) = p_pContract->m_ucReliability;
  
  p_pRspBuf = DMO_InsertBandwidthInfo(  p_pRspBuf,
                                        &p_pContract->m_stBandwidth,
                                        p_pContract->m_ucServiceType);  
  
  *p_unBufLen = p_pRspBuf - pucStart;
  
  return SFC_SUCCESS;   
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Adds a contract to the dmo contract table
/// @param  p_unDeleteIndex - index of the contract to be deleted
/// @param  p_ulTaiCutover  - TAI time when adding should be performed (not implemented)
/// @param  p_pData         - pointer to binarized contract data
/// @param  p_ucDataLen     - lenght of binarized contract data
/// @return - servie feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_setContract(  uint16        p_unDeleteIndex,
                        uint32        p_ulTaiCutover, 
                        const uint8 * p_pData, 
                        uint8         p_ucDataLen )
{
  
  
  if ( p_ucDataLen < DMO_CONTRACT_BUF_SIZE )
  {
		LOG_ISA100(LOGLVL_ERR, "DMO_setContract: ERROR: inavlid size: %d < %d", p_ucDataLen,  DMO_CONTRACT_BUF_SIZE );
		return SFC_INVALID_SIZE;
  }
 
  uint16 unIndex = ((uint16)p_pData[0] << 8) | p_pData[1];
  
  if ( p_unDeleteIndex != unIndex )
  {
      // index of write_row argument (or write srvc apdu header index) and index 
      // of contract data are different. perform delete operation on first index
      DMO_deleteContract( p_unDeleteIndex );
  } 
  
  // now find the second index contract  
  DMO_CONTRACT_ATTRIBUTE * pContract = DMO_FindContract( unIndex );
    
  if( !pContract ) // not found, add it
  {
      if( g_oDmoContractsMap.size() >= MAX_CONTRACT_NO )
      {
		  LOG_ISA100(LOGLVL_ERR, "DMO_setContract: ERROR: table full: %d ", g_oDmoContractsMap.size() );
          return SFC_INSUFFICIENT_DEVICE_RESOURCES;
      }
      
		CDmoContractPtr	pNewContract (new DMO_CONTRACT_ATTRIBUTE);
      
		g_oDmoContractsMap.insert(std::make_pair(unIndex,pNewContract));
		
		pContract = pNewContract.get();
		pContract->m_nUsageCount = 0;
  } 
  
  DMO_parseContract( p_pData, pContract );     
  
  // inform other DMAP concentrator about this new contract
//  CO_NotifyAddContract(pContract);
  // inform other UAPs about this new contract  
  LOG_ISA100(LOGLVL_INF, "DMO_setContract: contractID=%d DstAddr=%s SrcSAP=%d DstSAP=%d SrvcType=%d", pContract->m_unContractID, 
	  GetHex(pContract->m_aDstAddr128, sizeof(pContract->m_aDstAddr128)), pContract->m_unSrcTLPort, pContract->m_unDstTLPort, pContract->m_ucServiceType);
  UAP_NotifyAddContract(pContract);
  
// consistency check -> add the contract on NLME if missing
  NLME_AddDmoContract( pContract );
  
  return SFC_SUCCESS; 
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Deletes a contract from the dmo contract table
/// @param  pContract       - pointer to the contract in contract table that has to be deleted or NULL
///                           if not known
/// @param  p_unContractID  - contractID to be deleted (doesnt matter if pContract is valid)
/// @return servie feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_deleteContract( uint16 p_unContractID )
{
	CDmoContractsMap::iterator it = g_oDmoContractsMap.find(p_unContractID);
	
	if (it == g_oDmoContractsMap.end())
	{		
		LOG_ISA100(LOGLVL_ERR, "DMO_deleteContract: contractID=%d not found", p_unContractID );
		return SFC_INVALID_ELEMENT_INDEX;	
	}
	CDmoContractPtr pContract = it->second;
	LOG_ISA100(LOGLVL_INF, "DMO_deleteContract: contractID=%d DstAddr=%s SrcSAP=%d DstSAP=%d SrvcType=%d", pContract->m_unContractID, 
		GetHex(pContract->m_aDstAddr128, sizeof(pContract->m_aDstAddr128)), pContract->m_unSrcTLPort, pContract->m_unDstTLPort, pContract->m_ucServiceType);

	g_oDmoContractsMap.erase(it);
	
	// notify DMPA concentrator object about contract deletion
	//  CO_NotifyContractDeletion(p_unContractID);
	// notify other UAPs and objects about contract deletion
	UAP_NotifyContractDeletion(p_unContractID);

	NLME_DeleteContract(p_unContractID);

	return SFC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Searches a contract in the dmo contract table
/// @param  p_unContractID  - contract to be searched
/// @return pointer to contract entry in table or NULL if not found
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
DMO_CONTRACT_ATTRIBUTE * DMO_FindContract(uint16 p_unContractID)
{
	CDmoContractsMap::iterator it = g_oDmoContractsMap.find(p_unContractID);

	if (it == g_oDmoContractsMap.end())
	{
		return NULL;
	}

	return it->second.get();  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Searches a contract in the dmo contract table
/// @param  p_pDstAddr128  - pointer to the 128bit address of destination
/// @param  p_unDstTLPort     - destination TL port
/// @param  p_unSrcTLPort     - source TL port
/// @param  p_ucSrvcType   - service type (0 - periodic; 1 - aperiodic)
/// @return pointer to the contract structure or NULL if not found
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
DMO_CONTRACT_ATTRIBUTE * DMO_GetContract( const uint8 * p_pDstAddr128,
                                          uint16        p_unDstTLPort,
                                          uint16        p_unSrcTLPort,                        
                                          uint8         p_ucSrvcType )

{
	CDmoContractsMap::iterator it = g_oDmoContractsMap.begin();


  for( ; it != g_oDmoContractsMap.end(); it++ )
  {
	  CDmoContractPtr pContract = it->second;
      if( pContract->m_ucServiceType == p_ucSrvcType 
            && pContract->m_unDstTLPort == p_unDstTLPort
            && pContract->m_unSrcTLPort == p_unSrcTLPort            
            && !memcmp(p_pDstAddr128,
                       pContract->m_aDstAddr128,
                       sizeof(pContract->m_aDstAddr128)) )
      {
          return pContract.get(); 
      }       
  }
  
  //the contract was not found;       
  return NULL;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Reads a DMO attribute
/// @param  p_pAttrIdtf  - pointer to attribute identifier structure
/// @param  p_punBufLen   - in/out param; indicates available space in bufer / size of buffer read params
/// @param  p_pRspBuf     - pointer to response buffer
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_Read( ATTR_IDTF * p_pAttrIdtf,
                uint16 *    p_punBufLen,
                uint8 *     p_pRspBuf)
{   
  
  if ( DMO_CONTRACT_TABLE == p_pAttrIdtf->m_unAttrID )
  {
      DMO_CONTRACT_ATTRIBUTE * p_pContract = DMO_FindContract(p_pAttrIdtf->m_unIndex1); 
      
      if (p_pContract) // contract found
      {
          return DMO_binarizeContract( p_pContract, p_pRspBuf, p_punBufLen );                          
      }
      else
      {
          *p_punBufLen = 0;
          return SFC_INVALID_ELEMENT_INDEX; 
      }
  }
  return DMAP_ReadAttr( p_pAttrIdtf->m_unAttrID, p_punBufLen, p_pRspBuf, c_aDMOFct, DMO_ATTR_NO );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Performs execution of a DMO method
/// @param  p_ucMethodID  - method identifier
/// @param  p_unReqLen - input parameters length
/// @param  p_unReqBuf - input parameters buffer
/// @param  p_punRspLen - output parameters length
/// @param  p_pRspBuf - output parameters buffer
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_Execute( uint8          p_ucMethodID,
                   uint8          p_ucReqLen,
                   const uint8 *  p_pReqBuf,
                   uint16 *       p_punRspLen,
                   uint8 *        p_pRspBuf )                                      
{  
  uint16 unAttrID;
  uint32 ulSchedTAI = 0;
  uint8  ucSFC;
  
  *p_punRspLen = 0; // response of the execute request will contain only SFC and no data
   
  // if generic set/delete method extract attribute ID and TAI
  if( p_ucMethodID == DMO_MODIFY_CONTRACT ||  p_ucMethodID == DMO_TERMINATE_CONTRACT ) 
  {
      if ( p_ucReqLen < 6 )
            return SFC_INVALID_SIZE; 
            
      unAttrID = ((uint16)p_pReqBuf[0] << 8) | p_pReqBuf[1];
      
      if ( unAttrID != DMO_CONTRACT_TABLE )
            return SFC_INCOMPATIBLE_ATTRIBUTE;  // only contract table attribute is an indexed attribute
              
      ulSchedTAI = ((uint32)p_pReqBuf[0] << 24) |
                   ((uint32)p_pReqBuf[1] << 16) |
                   ((uint32)p_pReqBuf[2] << 8) |
                   p_pReqBuf[3];
      
      p_ucReqLen -= 6;
      p_pReqBuf += 6;
  }     
  
  switch( p_ucMethodID )
  {
    case DMO_MODIFY_CONTRACT:   
        if ( p_ucReqLen < 2 )
            return SFC_INVALID_SIZE;
        
        p_ucReqLen -= 2;        
        ucSFC = DMO_setContract( ((uint16)(p_pReqBuf[0]) << 8) | p_pReqBuf[1],
                                  ulSchedTAI, 
                                  p_pReqBuf+2, 
                                  p_ucReqLen ); 
        break;
        
    case DMO_TERMINATE_CONTRACT:  
        if ( p_ucReqLen != 2 )
            return SFC_INVALID_SIZE;
        
        ucSFC = DMO_deleteContract(((uint16)(p_pReqBuf[0]) << 8) | p_pReqBuf[1] );
        break;
        
    default:              
        ucSFC = SFC_INVALID_METHOD;
        break;
  }  
  
  return ucSFC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Issues a contract modification or renewal to the SCO of the system managee
/// @param  p_pContract - pointer to a DMO_CONTRACT_ATTRIBUTE structure that contains renew/modification info
/// @param  p_ucRequestType - specifies the request type: renew or modification;
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
/*
uint8 DMO_ModifyOrRenewContract ( DMO_CONTRACT_ATTRIBUTE * p_pContract,  
                                  uint8                    p_ucRequestType )
                                  
{
  if ( p_ucRequestType != CONTRACT_REQ_TYPE_MODIFY && p_ucRequestType != CONTRACT_REQ_TYPE_RENEW)
      return SFC_INVALID_ARGUMENT;
    
  // check if there is already a previously issued contract request
  if (g_ucContractRspPending || (g_ucJoinStatus != DEVICE_JOINED))                       
      return SFC_INSUFFICIENT_DEVICE_RESOURCES;  
    
  return DMO_sendContractRequest(p_pContract, p_ucRequestType);
}
*/


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Invocation of the SCO Contract Request method
/// @param  p_pDstAddr128 -  destination long address
/// @param  p_unDstTLPort - destination TL Port
/// @param  p_unSrcTLPort - source TL Port
/// @param  p_ulContractLife 
/// @param  p_ucSrvcType - type of service 
//			0 � periodic / scheduled communication
//			1 � non-periodic / unscheduled communication
/// @param  p_ucPriority - contract priority
/// @param  p_unMaxAPDUSize - maximum payload size
/// @param  p_ucRealibility 
/// @param  p_pBandwidth - bandwidth information structure
/// @param  p_ucContractRequestId - out - generated contract request ID (valid only when returned SFC_SUCCESS)
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CONTRACT_REQ_LEN  42

uint8 DMO_RequestNewContract( const uint8 * p_pDstAddr128,
                              uint16        p_unDstTLPort,
                              uint16        p_unSrcTLPort,
                              uint32        p_ulContractLife,
                              uint8         p_ucSrvcType,
                              uint8         p_ucPriority,
                              uint16        p_unMaxAPDUSize, 
                              uint8         p_ucReliability,                              
                              const DMO_CONTRACT_BANDWIDTH * p_pBandwidth,
				uint8*	p_pucContractRequestId
                                )
{   
  // just for safety, check if there is already a contract with these parameters
  DMO_CONTRACT_ATTRIBUTE * pExistContract = DMO_GetContract( p_pDstAddr128,
                                                        p_unDstTLPort,
                                                        p_unSrcTLPort,                        
                                                        p_ucSrvcType );
  if (pExistContract)
  {
	LOG_ISA100(LOGLVL_ERR,"DMO_RequestNewContract called for an existing contract: %d", pExistContract->m_unContractID);
	UAP_NotifyAddContract(pExistContract); //TODO: this does not belong here
      return SFC_DUPLICATE;
  }
  
  // check if there is already a previously issued contract request
#ifdef MULTICONTRACTS
	uint8	nRequestId;
	pExistContract = DMO_IsPendingContractRequest(CONTRACT_PENDING, p_pDstAddr128, p_unDstTLPort, p_unSrcTLPort, p_ucSrvcType, &nRequestId).get();
	if (pExistContract)
	{
		LOG_ISA100(LOGLVL_ERR,"WARNING: DMO_RequestNewContract called for a previously issued contract request (id:%u)", pExistContract->m_unContractID);
		return SFC_DUPLICATE;
	}
	if (g_ucJoinStatus != DEVICE_JOINED)
	{
		LOG_ISA100(LOGLVL_ERR,"WARNING: DMO_RequestNewContract called but GW is not joined");
		return SFC_INSUFFICIENT_DEVICE_RESOURCES;
	}
#else
  if (g_ucContractRspPending || (g_ucJoinStatus != DEVICE_JOINED))                       
  {
	LOG_ISA100(LOGLVL_ERR,"WARNING: DMO_RequestNewContract called for a previously issued contract request");
      return SFC_INSUFFICIENT_DEVICE_RESOURCES;
  }
#endif
  // ensure that there is at least one free entry in the contract table  
  if ( g_oDmoContractsMap.size() >= MAX_CONTRACT_NO )
  {
	LOG_ISA100(LOGLVL_ERR,"ERROR: DMO_RequestNewContract: contract table FULL");
      return SFC_INSUFFICIENT_DEVICE_RESOURCES; 
  }
  
  // save these values in a separate contract structure

#ifdef MULTICONTRACTS
	DMO_CONTRACT_ATTRIBUTE* pContract = new DMO_CONTRACT_ATTRIBUTE;

	memcpy( pContract->m_aDstAddr128, p_pDstAddr128, sizeof(pContract->m_aDstAddr128) );

	memcpy( &pContract->m_stBandwidth,
		p_pBandwidth,
	sizeof(DMO_CONTRACT_BANDWIDTH) );

	pContract->m_unSrcTLPort   = p_unSrcTLPort;
	pContract->m_unDstTLPort   = p_unDstTLPort;
	pContract->m_ucServiceType = p_ucSrvcType;
	pContract->m_ulAssignedExpTime = p_ulContractLife;
	pContract->m_ucPriority = p_ucPriority;
	pContract->m_unAssignedMaxNSDUSize = p_unMaxAPDUSize;
	pContract->m_ucReliability = p_ucReliability;
	pContract->m_unContractID = nRequestId; //m_unContractID is not initialized yet, we can use it for the ContractRequestID
	pContract->m_ulActivationTAI = MLSM_GetCrtTaiSec() + g_stDMO.m_unContractReqTimeout; //m_ulActivationTAI is not initialized yet, we can store our timeout there
	if(p_pucContractRequestId){
		*p_pucContractRequestId = nRequestId;
	}
#else
  //memset(g_stPendingContractRsp,0,sizeof(g_stPendingContractRsp));  //just in case
  
	
  memcpy( g_stPendingContractRsp.m_aDstAddr128, p_pDstAddr128, sizeof(g_stPendingContractRsp.m_aDstAddr128) );

  
  memcpy( &g_stPendingContractRsp.m_stBandwidth, 
          p_pBandwidth,
          sizeof(DMO_CONTRACT_BANDWIDTH) );

  g_stPendingContractRsp.m_unSrcTLPort   = p_unSrcTLPort;
  g_stPendingContractRsp.m_unDstTLPort   = p_unDstTLPort;     
  g_stPendingContractRsp.m_ucServiceType = p_ucSrvcType; 
  g_stPendingContractRsp.m_ulAssignedExpTime = p_ulContractLife;
  g_stPendingContractRsp.m_ucPriority = p_ucPriority;
  g_stPendingContractRsp.m_unAssignedMaxNSDUSize = p_unMaxAPDUSize;
  g_stPendingContractRsp.m_ucReliability = p_ucReliability;
  
#endif
  if( g_stDSMO.m_ucTLSecurityLevel != SECURITY_NONE )
  {
	// check if there is a valid key for this contract; if key not available, request 
	// a new key before the contract request is sent
	uint8 ucTmp;

	if (NULL == SLME_FindTxKey( p_pDstAddr128, p_unSrcTLPort, p_unDstTLPort, &ucTmp ) )
	{
		if (SFC_SUCCESS == SLME_GenerateNewSessionKeyRequest(p_pDstAddr128, p_unSrcTLPort, p_unDstTLPort))
		{
			LOG_ISA100(LOGLVL_INF, "DMO_RequestNewContract: key requested for DstAddr=%s SrcPort=%d DstPort=%d",
				GetHex(p_pDstAddr128, sizeof(IPV6_ADDR)), p_unSrcTLPort, p_unDstTLPort);
#ifdef MULTICONTRACTS
			g_ucContractRspPending++;
			pContract->m_ucContractStatus = CONTRACT_PENDING_NO_KEY;
			pContract->m_ulActivationTAI = MLSM_GetCrtTaiSec() + g_stDMO.m_unContractReqTimeout; //m_ulActivationTAI is not initialized yet, we can store our timeout there
			g_oPendingContractRspMap.erase ( nRequestId );
			g_oPendingContractRspMap.insert (std::make_pair(nRequestId, pContract));
#else
			g_ucContractRspPending = 1;
			g_unTimeout = MLSM_GetCrtTaiSec() + g_stDMO.m_unContractReqTimeout;
#endif
			return SFC_SUCCESS; // now wait for the key; 
		}
#ifdef MULTICONTRACTS
		else{
			delete pContract;
			return SFC_INSUFFICIENT_DEVICE_RESOURCES;
		}
#else
		else return SFC_INSUFFICIENT_DEVICE_RESOURCES;
#endif
	}
  }
#ifdef MULTICONTRACTS
	pContract->m_ucContractStatus = CONTRACT_PENDING;
	g_oPendingContractRspMap.erase ( nRequestId );
	g_oPendingContractRspMap.insert (std::make_pair(nRequestId, pContract));
	// the key is already available or no TL security ; send the contract request 
	return DMO_sendContractRequest(pContract, CONTRACT_REQ_TYPE_NEW);
#else
  
  // the key is already available or no TL security ; send the contract request 
  return DMO_sendContractRequest(&g_stPendingContractRsp, CONTRACT_REQ_TYPE_NEW);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Transforms a DMO_CONTRACT_ATTRIBUTE structure intor a SCO contract request
/// @param  p_pContract - pointer to a DMO_CONTRACT_ATTRIBUTE structure
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_sendContractRequest(DMO_CONTRACT_ATTRIBUTE * p_pContract,
                              uint8                    p_ucRequestType)
{  
  uint8 aContractReqBuf[MAX_CONTRACT_REQ_LEN];
  uint8 * pReqBuf = aContractReqBuf;    

#ifdef MULTICONTRACTS
  *(pReqBuf++) = (uint8)p_pContract->m_unContractID;
#else
  *(pReqBuf++) = g_ucContractReqID++; //TODO: this must be replaced with a mechanism to properly identify each separate contract request
#endif
  *(pReqBuf++) = p_ucRequestType;
  
  if (p_ucRequestType != CONTRACT_REQ_TYPE_NEW) // contrat renewal or modification
  {
      pReqBuf = DMAP_InsertUint16( pReqBuf, p_pContract->m_unContractID );
  }
  
  *(pReqBuf++) = p_pContract->m_ucServiceType; 
  
  pReqBuf = DMAP_InsertUint16( pReqBuf, p_pContract->m_unSrcTLPort);    
  
  // destination 128 bit address and destination port
  memcpy( pReqBuf, p_pContract->m_aDstAddr128, 16 );
  pReqBuf += 16;
  
  pReqBuf = DMAP_InsertUint16( pReqBuf, p_pContract->m_unDstTLPort);   
  
  *(pReqBuf++) = (uint8)CONTRACT_NEGOTIABLE_REVOCABLE; 
  
  pReqBuf = DMAP_InsertUint32(pReqBuf, p_pContract->m_ulAssignedExpTime);
  
  *(pReqBuf++) = p_pContract->m_ucPriority;  
  
  pReqBuf = DMAP_InsertUint16( pReqBuf, p_pContract->m_unAssignedMaxNSDUSize);  

  *(pReqBuf++) = p_pContract->m_ucReliability; // reliability and publish auto retransmit
  
  pReqBuf =  DMO_InsertBandwidthInfo(  pReqBuf, 
                                      &p_pContract->m_stBandwidth, 
                                      p_pContract->m_ucServiceType);    
  EXEC_REQ_SRVC stExecReq;
  stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
  stExecReq.m_unDstOID = SM_SCO_OBJ_ID;
  stExecReq.m_ucMethID = SCO_REQ_CONTRACT;  
  stExecReq.p_pReqData = aContractReqBuf;
  stExecReq.m_unLen    = pReqBuf - aContractReqBuf;

  LOG_ISA100(LOGLVL_INF, "DMO_RequestNewContract: requesting contract for DstAddr=%s SrcPort=%d DstPort=%d SrvcType=%d", 
	  GetHex(p_pContract->m_aDstAddr128, sizeof(IPV6_ADDR)), p_pContract->m_unSrcTLPort, p_pContract->m_unDstTLPort, p_pContract->m_ucServiceType);

  // verify that device has contract with SM, in order to issue the contract request
  DMO_CONTRACT_ATTRIBUTE * pContract = DMO_FindContract(g_unSysMngContractID);  
  
  if (!pContract) // contract with SM should always be available
  {
	  LOG_ISA100(LOGLVL_ERR, "DMO_RequestNewContract: ERROR: no contract to SM found g_unSysMngContractID=%d", g_unSysMngContractID);
	  return SFC_INCOMPATIBLE_MODE;
  }    

  if (SFC_SUCCESS == ASLSRVC_AddGenericObject( &stExecReq,
                                               SRVC_EXEC_REQ,
                                               pContract->m_ucPriority,
                                               pContract->m_unSrcTLPort - (uint16)ISA100_DMAP_PORT,
                                               pContract->m_unDstTLPort - (uint16)ISA100_DMAP_PORT,
                                               GET_NEW_APP_HANDLE(),
                                               NULL,
                                               pContract->m_unContractID,
                                               0,
						0 ) )
  {
#ifdef MULTICONTRACTS
	p_pContract->m_ucContractReqType = p_ucRequestType;
	g_ucContractRspPending++;
	p_pContract->m_ulActivationTAI = MLSM_GetCrtTaiSec() + g_stDMO.m_unContractReqTimeout; //m_ulActivationTAI is not initialized yet, we can store our timeout there
#else
      g_ucContractReqType = p_ucRequestType;
      g_ucContractRspPending = 1;
      g_unTimeout = MLSM_GetCrtTaiSec() + g_stDMO.m_unContractReqTimeout;
#endif
	  LOG_ISA100(LOGLVL_DBG, "DMO_RequestNewContract: g_stDMO.m_unContractReqTimeout=%d", g_stDMO.m_unContractReqTimeout);
      return SFC_SUCCESS;  
  }
  LOG_ISA100(LOGLVL_ERR,"ERROR: DMO_RequestNewContract: can't send request to ASL");
  return SFC_INSUFFICIENT_DEVICE_RESOURCES;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  
/// @param  
/// @return 
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_NotifyNewKeyAdded( const uint8* p_pucPeerIPv6Address, uint16 p_unUdpSPort, uint16 p_unUdpDPort )
{
	LOG_ISA100( LOGLVL_DBG, "DMO_NotifyNewKeyAdded: received a key for SPort:%X DPort:%X address:%s", p_unUdpSPort, p_unUdpDPort, GetHex(p_pucPeerIPv6Address, sizeof(IPV6_ADDR)));
  if (!g_ucContractRspPending)
  {
	LOG_ISA100(LOGLVL_DBG, "DMO_NotifyNewKeyAdded: no contracts pending!");
      return;  // nothing to do; 
  }
  
  // if g_ucContractRspPending is non-zero, we may have some saved data in the pending contracts map that
  // contains info about a new contract request that was not sent to the System Manager
#ifdef MULTICONTRACTS
	CDmoContractPtr pContract;
	while(pContract = DMO_IsPendingContractRequest( CONTRACT_PENDING_NO_KEY, p_pucPeerIPv6Address, p_unUdpDPort, p_unUdpSPort)){
		// we have a new valid key for our pending contract request; generate the contract request
		g_ucContractRspPending--;
		pContract->m_ucContractStatus = CONTRACT_PENDING;
		LOG_ISA100(LOGLVL_INF, "DMO_NotifyNewKeyAdded: request contract for SPort:%X DPort:%X address:%s (contract request id:%d)", p_unUdpSPort, p_unUdpDPort, GetHex(p_pucPeerIPv6Address, 16), pContract->m_unContractID);
		DMO_sendContractRequest(pContract.get(), CONTRACT_REQ_TYPE_NEW);
	};
#else
  DMO_CONTRACT_ATTRIBUTE * pContract = &g_stPendingContractRsp;
  
//  uint16 unSrcTLPort = 0xF0B0 | (p_ucUdpPorts >> 4);
//  uint16 unDstTLPort = 0xF0B0 | (p_ucUdpPorts & 0x0F);
  
  if( pContract->m_unSrcTLPort == p_unUdpSPort 
       && pContract->m_unDstTLPort == p_unUdpDPort 
       && !memcmp( pContract->m_aDstAddr128, p_pucPeerIPv6Address, 16 )
         )
  {
      // we have a new valid key for our pending contract request; generate the contract request 
      DMO_sendContractRequest(pContract, CONTRACT_REQ_TYPE_NEW);
  }else{
	LOG_ISA100(LOGLVL_ERR, "DMO_NotifyNewKeyAdded: can't use key for SPort:%X, DPort:%X, address:%s", p_unUdpSPort, p_unUdpDPort, GetHex(p_pucPeerIPv6Address, 16));
	LOG_ISA100(LOGLVL_ERR, "DMO_NotifyNewKeyAdded: mismatch with pending contract: SPort:%X DPort:%X address:%s", pContract->m_unSrcTLPort, pContract->m_unDstTLPort, GetHex(pContract->m_aDstAddr128, 16));
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Invocation of the SCO Contract Termiantion method on system manager
/// @param  p_unContractID - identifier of the contract that has to be terminated, deactivated or reactivated
/// @param  p_pucOperation - 0=contract_termination, 1=deactivation, 2=reactivation
/// @return service feedback code
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
uint8 DMO_RequestContractTermination( uint16 p_unContractID,
                                      uint8  p_ucOperation )
{
  if (g_ucJoinStatus != DEVICE_JOINED)
  {
	LOG_ISA100(LOGLVL_INF,"WARNING: DMO_RequestContractTermination called while GW is not joined");
      return SFC_INSUFFICIENT_DEVICE_RESOURCES;
  }
  
  // just for safety, check if there is already a contract with these parameters
  DMO_CONTRACT_ATTRIBUTE * pContract = DMO_FindContract(p_unContractID);
  if (!pContract)
  {
	LOG_ISA100(LOGLVL_ERR,"ERROR: DMO_RequestContractTermination: contractId %d not found", p_unContractID);
      return SFC_NO_CONTRACT;   
  }
  
  // now start building the request buffer
  uint8 aReqBuf[3];

  aReqBuf[0] = p_unContractID >> 8;
  aReqBuf[1] = p_unContractID;
  aReqBuf[2] = p_ucOperation;
 
  EXEC_REQ_SRVC stExecReq;
  stExecReq.m_unSrcOID = DMAP_DMO_OBJ_ID;
  stExecReq.m_unDstOID = SM_SCO_OBJ_ID;
  stExecReq.m_ucMethID = SCO_TERMINATE_CONTRACT;  
  stExecReq.p_pReqData = aReqBuf;
  stExecReq.m_unLen    = sizeof(aReqBuf);
  
  // verify that device has contract with SM, in order to issue the contract request
  pContract = DMO_FindContract(g_unSysMngContractID);  
  
  if ( !pContract ) // contract with SM should always be available
  {
	LOG_ISA100(LOGLVL_ERR,"ERROR: DMO_RequestContractTermination: we are joined but don't have contract with SM (id:%d)?!", g_unSysMngContractID);
      return SFC_INCOMPATIBLE_MODE;
  }

  LOG_ISA100(LOGLVL_INF,"DMO_RequestContractTermination: contractId=%d operation=%d", p_unContractID, p_ucOperation);
  return ASLSRVC_AddGenericObject( &stExecReq,
                                   SRVC_EXEC_REQ,
                                   pContract->m_ucPriority,
                                   pContract->m_unSrcTLPort - (uint16)ISA100_DMAP_PORT,
                                   pContract->m_unDstTLPort - (uint16)ISA100_DMAP_PORT,
                                   GET_NEW_APP_HANDLE(),
                                   NULL,
                                   pContract->m_unContractID,
                                   0,
					0);

}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Process the response to a SCO request contract invocation.
/// @param  p_pExecReq - pointer to the original request service structure
/// @param  p_pExecRsp - pointer to the response service structure
/// @return none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_ProcessContractResponse(EXEC_REQ_SRVC* p_pExecReq, EXEC_RSP_SRVC* p_pExecRsp)
{  
  const uint8* pucTemp = p_pExecRsp->p_pRspData;
  uint16 unContractID;
#ifdef MULTICONTRACTS
	CPendingContractsMap::iterator it;

	if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
	{
		//search the original contract request ID, reading from the request (the response does not include it)
		it = g_oPendingContractRspMap.find(p_pExecReq->p_pReqData[0]);
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: contract rsp ERROR sfc=%d",  p_pExecRsp->m_ucSFC  );
		if(it != g_oPendingContractRspMap.end())
		{
			it->second.get()->m_ulActivationTAI = MLSM_GetCrtTaiSec();
			it->second.get()->m_ucContractStatus = CONTRACT_PENDING_FAILED;
			UAP_NotifyContractFailure(it->second.get());
		}
		return;
	}

	it = g_oPendingContractRspMap.find(*(pucTemp++));
	if ( it == g_oPendingContractRspMap.end() )
	{
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: WARNING: contract request identifier %d does not match any of the expected responses", *(pucTemp-1) );
		return; // contract request identifiers does not match; it's not the expected response
	}
	CDmoContractPtr pContract = it->second;
	g_oPendingContractRspMap.erase(it);

	if(pContract->m_ucContractStatus == CONTRACT_PENDING_FAILED)
	{
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: WARNING: contract request identifier %d matches a request which failed %ds ago. Probably timeout.",
			*(pucTemp-1), MLSM_GetCrtTaiSec()-pContract->m_ulActivationTAI );
		return;
	}

	g_ucContractRspPending--;
#else
  DMO_CONTRACT_ATTRIBUTE * pContract = NULL;
  
  if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
  {
	LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: contract rsp ERROR sfc=%d",  p_pExecRsp->m_ucSFC  );
	g_unTimeout = MLSM_GetCrtTaiSec() + g_stDMO.m_unContractReqTimeout / 8;
	 return;	
  }      
  
  if ( *(pucTemp++) != g_ucContractReqID-1 )
  {
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: ERROR: contract request identifiers does not match; it's not the expected response" );
		return; // contract request identifiers does not match; it's not the expected response
  }
  
  //the next contract request must be sent with different Contract Request Id
  //g_ucContractReqID++;
  g_ucContractRspPending = 0;
  
#endif
  if ( *(pucTemp++) != CONTRACT_RSP_OK )
  {
	LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: ERROR: for now only Response_Code = 0 is implemented. Received=%d", *(pucTemp-1));
#ifdef MULTICONTRACTS
	pContract->m_ulActivationTAI = MLSM_GetCrtTaiSec();
	pContract->m_ucContractStatus = CONTRACT_PENDING_FAILED;
	UAP_NotifyContractFailure(pContract.get());
#endif
      return; // for now only Response_Code = 0 is implemented  
  }
  
  pucTemp = DMAP_ExtractUint16(pucTemp, &unContractID);   
  
#ifdef MULTICONTRACTS
	if ( pContract->m_ucContractReqType == CONTRACT_REQ_TYPE_NEW )
	{
		if ( g_oDmoContractsMap.size() >= MAX_CONTRACT_NO )
		{
			LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: ERROR: contract table is full size=%d. Possible inconsistency between our table and SM's",  g_oDmoContractsMap.size() );
			pContract->m_ulActivationTAI = MLSM_GetCrtTaiSec();
			pContract->m_ucContractStatus = CONTRACT_PENDING_FAILED;
			UAP_NotifyContractFailure(pContract.get());
			return; // contract table is full
		}
	}else{
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: m_ucContractReqType=%d NOT IMPLEMENTED!",pContract->m_ucContractReqType );
//		pContract = DMO_FindContract(unContractID);
	}
#else
  if ( g_ucContractReqType == CONTRACT_REQ_TYPE_NEW )
  {
	if ( g_oDmoContractsMap.size() >= MAX_CONTRACT_NO )
	{
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: ERROR: contract table is full size=%d",  g_oDmoContractsMap.size() );
		return; // contract table is full
	}
  
	pContract = new DMO_CONTRACT_ATTRIBUTE;
	*pContract = g_stPendingContractRsp;
  }
  else
  {
      pContract = DMO_FindContract(unContractID);
  }  
#endif

  if (!pContract)
  {
	LOG_ISA100(LOGLVL_ERR, "DMO_ProcessContractResponse: PROGRAMMER ERROR: at this point we must have a valid contract pointer!");
      return;  
  }
  pContract->m_unContractID     = unContractID;
  pContract->m_ucContractStatus = CONTRACT_RSP_OK;
  pContract->m_ucServiceType    = *(pucTemp++); 
  
  pContract->m_ulActivationTAI = 0; // not sent for response code = 0;
  pucTemp = DMAP_ExtractUint32(pucTemp, &pContract->m_ulAssignedExpTime);

  pContract->m_ucPriority = *(pucTemp++); 
  
  pucTemp = DMAP_ExtractUint16(pucTemp, &pContract->m_unAssignedMaxNSDUSize); 
  
  pContract->m_ucReliability = *(pucTemp++);
  
  pucTemp = DMO_ExtractBandwidthInfo(  pucTemp,
                                       &pContract->m_stBandwidth, 
                                       pContract->m_ucServiceType );     
  pContract->m_nUsageCount = 0;

  LOG_ISA100(LOGLVL_INF, "DMO_ProcessContractResponse: contractID=%d DstAddr=%s SrcSAP=%d DstSAP=%d SrvcType=%d", pContract->m_unContractID, 
		GetHex(pContract->m_aDstAddr128, sizeof(pContract->m_aDstAddr128)), pContract->m_unSrcTLPort, pContract->m_unDstTLPort, pContract->m_ucServiceType);
  // inform other DMAP concentrator about this new contract
//  CO_NotifyAddContract(pContract);
  // inform other UAPs about this new contract  

  g_oDmoContractsMap.insert (std::make_pair(pContract->m_unContractID, pContract));
#ifdef MULTICONTRACTS
//UAP_NotifyContractResponse( p_pExecRsp->p_pRspData, pucTemp - p_pExecRsp->p_pRspData ); do we need this???
  UAP_NotifyAddContract(pContract.get());
  
  // consistency check -> add the contract on NLME if missing
  NLME_AddDmoContract( pContract.get() );
  
  ARMO_NotifyAddContract(pContract.get());
#else
  UAP_NotifyAddContract(pContract);
  
  // consistency check -> add the contract on NLME if missing
  NLME_AddDmoContract( pContract );
  
  ARMO_NotifyAddContract(pContract);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Process the contract received in join response
/// @param  p_pExecRsp - pointer to the response service structure
/// @return none
/// @remarks
///      Access level: user level
///      Context: Called when a join response has been received
////////////////////////////////////////////////////////////////////////////////////////////////////

#define FIRST_CONTRACT_BUF_LEN  28

void DMO_ProcessFirstContract( EXEC_RSP_SRVC* p_pExecRsp )          
{
  
  if( SFC_SUCCESS != p_pExecRsp->m_ucSFC )
  {
	  LOG_ISA100(LOGLVL_ERR, "DMO_ProcessFirstContract: ERROR: got invalid EXEC_RSP sfc=%d", p_pExecRsp->m_ucSFC);
      return;
  }
  // just for safety; at this poin, contract table should be empty
  if (g_oDmoContractsMap.size() >= MAX_CONTRACT_NO)
  {
	LOG_ISA100(LOGLVL_ERR, "DMO_ProcessFirstContract:  ERROR: contract table full we expect to be empty");
      return;
  }
  // check the size of the buffer
  if( p_pExecRsp->m_unLen < FIRST_CONTRACT_BUF_LEN )
  {
	LOG_ISA100(LOGLVL_ERR, "DMO_ProcessFirstContract:  ERROR: SM sent us %d bytes instead of %d", p_pExecRsp->m_unLen, FIRST_CONTRACT_BUF_LEN);
      return;
  }
  // this is the first real contract; delete all previous fake contrats
  //BUT DON'T FORGET TO SAVE THE REQID!!!
  CNlmeContractPtr pNLJoinContract = g_stNlme.m_oContractsMap[JOIN_CONTRACT_ID];
  uint8 savedReqId = pNLJoinContract ? pNLJoinContract->m_ucReqID : 0;
  g_oDmoContractsMap.clear();
  g_stNlme.m_oContractsMap.clear();
  
  // add the first contract to the DMO contract table
  CDmoContractPtr pContract (new DMO_CONTRACT_ATTRIBUTE);

  const uint8* pucTemp = p_pExecRsp->p_pRspData;
  pucTemp = DMAP_ExtractUint16(pucTemp, &pContract->m_unContractID);
  //insert after the m_unContractID is extract
  g_oDmoContractsMap.insert (std::make_pair(pContract->m_unContractID,pContract));

  g_unSysMngContractID = pContract->m_unContractID;
  
  pContract->m_ucContractStatus = CONTRACT_RSP_OK;
  pContract->m_ucServiceType    = SRVC_APERIODIC_COMM;
  pContract->m_ulActivationTAI  = 0;
  pContract->m_unSrcTLPort      = ISA100_DMAP_PORT; 
  
  memcpy(pContract->m_aDstAddr128,
         g_stDMO.m_aucSysMng128BitAddr,
         sizeof(pContract->m_aDstAddr128));
  
  pContract->m_unDstTLPort        = ISA100_SMAP_PORT;
  pContract->m_ulAssignedExpTime  = (uint32)0xFFFFFFFF;  
  pContract->m_ucPriority         = (uint8)DMO_PRIORITY_BEST_EFFORT;
  
  pucTemp = DMAP_ExtractUint16(pucTemp, &pContract->m_unAssignedMaxNSDUSize);
  
  pucTemp = DMO_ExtractBandwidthInfo( pucTemp,
                                      &pContract->m_stBandwidth, 
                                      pContract->m_ucServiceType );   
  

  // add the first contract to NLMO contract table; 
  // access is through write_row; build write_row buffer, with doubled indexes
  uint8 aucTmpBuf[53];
  aucTmpBuf[0] = pContract->m_unContractID >> 8;
  aucTmpBuf[1] = pContract->m_unContractID;
  memcpy(aucTmpBuf+2, g_stDMO.m_auc128BitAddr, 16);
  memcpy(aucTmpBuf+18, aucTmpBuf, 2);
  memcpy(aucTmpBuf+20, g_stDMO.m_auc128BitAddr, 16);
  memcpy(aucTmpBuf+36, g_stDMO.m_aucSysMng128BitAddr, 16);
  aucTmpBuf[52] = (uint8)DMO_PRIORITY_BEST_EFFORT | ((*(pucTemp++) & 0x04) << 2); // b1b0 = priority; b3= include contract flag;
  if(pContract->m_unAssignedMaxNSDUSize<16)
  {
	LOG_ISA100(LOGLVL_ERR,"WARN: invalid AssignedMaxNSDUSize %u, resizing to %u", pContract->m_unAssignedMaxNSDUSize, MAX_DATAGRAM_SIZE);
	pContract->m_unAssignedMaxNSDUSize = MAX_DATAGRAM_SIZE;
  }
  LOG_ISA100(LOGLVL_INF, "DMO_ProcessFirstContract: contractID=%d DstAddr=%s SrcSAP=%d DstSAP=%d SrvcType=%d", pContract->m_unContractID, 
	  GetHex(pContract->m_aDstAddr128, sizeof(pContract->m_aDstAddr128)), pContract->m_unSrcTLPort, pContract->m_unDstTLPort, pContract->m_ucServiceType);

  if( SFC_SUCCESS == NLME_setContract( 0, aucTmpBuf, pContract->m_unAssignedMaxNSDUSize,
				pContract->m_ucServiceType == SRVC_APERIODIC_COMM ? pContract->m_stBandwidth.m_stAperiodic.m_ucMaxSendWindow:0,
				pContract->m_ucServiceType == SRVC_APERIODIC_COMM ? pContract->m_stBandwidth.m_stAperiodic.m_nComittedBurst:0,
				pContract->m_ucServiceType == SRVC_APERIODIC_COMM ? pContract->m_stBandwidth.m_stAperiodic.m_nExcessBurst:0) )
  {
    g_ucJoinStatus = DEVICE_SEND_SEC_CONFIRM_REQ;
  }
  pContract->m_nUsageCount = 0;
  //the Join Contract Response contains the NLMO Route Entry fileds but these are not used  
  //NL_Next_Hop(IPv6 address), NL_Next_Hop(1 byte), NL_Hops_Limit(1 byte)   
	//ugly hack to inherit the ReqID from the join fake contract to the first real contract. The SM requires it.
	NLME_CONTRACT_ATTRIBUTES * pNLFirstContract = NLME_FindContract(pContract->m_unContractID);
	if(pNLFirstContract){
		pNLFirstContract->m_ucReqID = savedReqId;
	}else{
		LOG_ISA100(LOGLVL_ERR, "DMO_ProcessFirstContract: PROGRAMMER ERROR: first contract: %p", pNLFirstContract);
	}
LOG_ISA100(LOGLVL_DBG, "DMO_ProcessFirstContract: complete");
}
             

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Initializes the DMO object
/// @param  - none
/// @return - none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_Init( void )
{
//save values initialized in porting.cpp
	uint8 ucMaxClntSrvRetries = g_stDMO.m_ucMaxClntSrvRetries;
	uint16 unMaxRetryToutInterval = g_stDMO.m_unMaxRetryToutInterval;
  memset( &g_stDMO.m_ulUptime, 0, sizeof(g_stDMO) - (uint32)&((DMO_ATTRIBUTES*)0)->m_ulUptime ); // set to 0 rest of struct
  g_stDMO.m_ulUsedDevMem = 0;
  
  g_ucContractRspPending = 0;
  
  g_stDMO.m_ucPWRStatus = 0; //line powered
    
  g_oDmoContractsMap.clear();
#ifdef MULTICONTRACTS
  g_oPendingContractRspMap.clear();
#endif
    
  // other initial default values
  g_stDMO.m_unContractReqTimeout    = (uint16)(ucMaxClntSrvRetries * unMaxRetryToutInterval) ;//(uint16)DMO_DEF_CONTRACT_TOUT;
  g_stDMO.m_ucMaxClntSrvRetries     = ucMaxClntSrvRetries;
  g_stDMO.m_unMaxRetryToutInterval  = unMaxRetryToutInterval;
  g_stDMO.m_ucJoinCommand           = DMO_JOIN_CMD_NONE;
  g_stDMO.m_unWarmRestartAttemptTout = (uint16)DMO_DEF_WARM_RESTART_TOUT; 
  
  // generate the warm restart alarm (only if NonVolatileMemoryCapability = 1)
  if (c_ucNonVolatileMemoryCapable)
  {
      DMO_generateRestartAlarm();
  }
  
  g_stDMO.m_unRestartCount++;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Performs operations that need one second timing
/// @param  - none
/// @return - none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_PerformOneSecondTasks( void )
{
  g_stDMO.m_ulUptime++;
  
  uint32 ulCrtTaiSec = MLSM_GetCrtTaiSec();
  
#ifdef MULTICONTRACTS
  DMO_TimeoutContractReqs();
#else
  if (g_ucContractRspPending && ulCrtTaiSec > g_unTimeout)
  {
	  LOG_ISA100(LOGLVL_ERR, "ERROR: DMO_PerformOneSecondTasks: timeout on contract req -> reset");
      g_ucContractRspPending = 0;   
  }
#endif
  
  switch( g_stDMO.m_ucJoinCommand )
  {
      case DMO_JOIN_CMD_RESET_FACTORY_DEFAULT:
      {
        uint16 unTemp;
        uint8 ucTemp;
        DPO_ResetToDefault(&unTemp, &ucTemp);  
        //no break
      }
      case DMO_JOIN_CMD_RESTART_AS_PROVISIONED:
      case DMO_JOIN_CMD_WARM_RESTART:
	LOG_ISA100(LOGLVL_INF,"DMO_JOIN_CMD: %d", g_stDMO.m_ucJoinCommand);
        DMAP_DLMO_ResetStack();
        break;

      case DMO_JOIN_CMD_START:
      default:;
  }
  
  if (g_ucJoinStatus == DEVICE_JOINED)
  {
      DMO_checkPowerStatusAlert();
      DM0_checkContractExpirationTimes(ulCrtTaiSec);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Verifies if a contract expiration time is elapsed. 
/// @param  - p_ulCrtTaiSec current TAI time in seconds
/// @return - none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DM0_checkContractExpirationTimes(uint32 p_ulCrtTaiSec)
{
//  DMO_CONTRACT_ATTRIBUTE * pContract = g_stDMO.m_aContractTbl;
//  DMO_CONTRACT_ATTRIBUTE * pEndContract = g_stDMO.m_aContractTbl+g_stDMO.m_ucContractNo;
//  
//  for( ; pContract < pEndContract; pContract++ )
//  {
//      if( p_ulCrtTaiSec >= pContract->m_ulAssignedExpTime )
//      {
//          DMO_deleteContract( pContract, pContract->m_unContractID );
//      }
//  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Tracks changes of the powerstatus attribute, and generates the power status alarm
/// @param  - none
/// @return - none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_checkPowerStatusAlert(void)
{  
  static uint8 ucLastPwrStatus = 0xFF;
  
  if ( ucLastPwrStatus != 0xFF && ucLastPwrStatus != g_stDMO.m_ucPWRStatus && g_stDMO.m_stPwrAlertDescriptor.m_bAlertReportDisabled)  
  {
      // generate the power status alarm;
      ALERT stAlert;
      uint8 aucAlertBuf[2]; 
      aucAlertBuf[0] = g_stDMO.m_ucPWRStatus;
      stAlert.m_unSize = 1;
      
      stAlert.m_ucPriority      = g_stDMO.m_stPwrAlertDescriptor.m_ucPriority;
      stAlert.m_unDetObjTLPort  = 0xF0B0; // DMO is DMAP port
      stAlert.m_unDetObjID      = DMAP_DMO_OBJ_ID; 
      stAlert.m_ucClass         = ALERT_CLASS_EVENT; 
      stAlert.m_ucDirection     = ALARM_DIR_RET_OR_NO_ALARM; 
      stAlert.m_ucCategory      = ALERT_CAT_COMM_DIAG; 
      stAlert.m_ucType          = DMO_POWER_STATUS_ALARM;              
  
      ARMO_AddAlertToQueue( &stAlert, aucAlertBuf );
  }
  
  ucLastPwrStatus = g_stDMO.m_ucPWRStatus;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @author NIVIS LLC, Eduard Erdei
/// @brief  Generates a waarm restart alarm
/// @param  - none
/// @return - none
/// @remarks
///      Access level: user level
////////////////////////////////////////////////////////////////////////////////////////////////////
void DMO_generateRestartAlarm(void)
{ 
  ALERT stAlert;
  uint8 aucAlertBuf[2]; // just for safety; no payload for restart alarms
  stAlert.m_unSize = 0;
  
  #warning "What is the value for priority field???"
  stAlert.m_ucPriority = 0;
  stAlert.m_unDetObjTLPort  = 0xF0B0; // DMO is DMAP port
  stAlert.m_unDetObjID      = DMAP_DMO_OBJ_ID; 
  stAlert.m_ucClass         = ALERT_CLASS_EVENT; 
  stAlert.m_ucDirection     = ALARM_DIR_RET_OR_NO_ALARM; 
  stAlert.m_ucCategory      = ALERT_CAT_COMM_DIAG; 
  stAlert.m_ucType          = DMO_DEVICE_RESTART_ALARM;              
  
  ARMO_AddAlertToQueue( &stAlert, aucAlertBuf );
}


void DMO_NotifyCongestion( uint8 p_ucCongestionDirection, APDU_IDTF * p_pstApduIdtf )
{
  // TBD 
}

void DMO_PrintContracts(void)
{	char ipv6[40];
	
	LOG_ISA100(LOGLVL_ERR, "   DMO CONTRACT TABLE: Total %d", g_oDmoContractsMap.size());
	LOG_ISA100(LOGLVL_ERR, "   ID         IPv6DstAddress                  S T Pri MaxNS SPort DPort Expire");
	LOG_ISA100(LOGLVL_ERR, "----- --------------------------------------- - - --- ----- ---- ---- --------");
	for(CDmoContractsMap::iterator it = g_oDmoContractsMap.begin(); it !=  g_oDmoContractsMap.end(); ++it )
	{
		CDmoContractPtr pContract = it->second;

		FormatIPv6( pContract->m_aDstAddr128, ipv6);
		LOG_ISA100(LOGLVL_ERR, "%5u %s %1u %1u %3u %5u %4X %4X %8d", pContract->m_unContractID, ipv6,
			pContract->m_ucContractStatus, pContract->m_ucServiceType, pContract->m_ucPriority,
			pContract->m_unAssignedMaxNSDUSize, pContract->m_unSrcTLPort, pContract->m_unDstTLPort,
			pContract->m_ulAssignedExpTime );
	}
	LOG_ISA100(LOGLVL_ERR, "----- --------------------------------------- - - --- ----- ---- ---- --------");
}

#ifdef MULTICONTRACTS
static CDmoContractPtr DMO_IsPendingContractRequest(uint8	p_ucPendingState,
						const IPV6_ADDR	p_pDstAddr128,
						uint16		p_unDstTLPort,
						uint16		p_unSrcTLPort,
						uint8		p_ucSrvcType,
						uint8*		p_pnRequestId)
{
	static uint8 s_ucContractReqID = 1;
	uint8 ucTentativeReqID = s_ucContractReqID;
	uint8 ucTentativeReqIDWrapped = 0;
	uint8 ucReclaimableReqID = rand() >> 8; //something random, to fix compiler complains about uninitialized vars :)
	bool bFoundReclaimableID = false;
	for(CPendingContractsMap::iterator it = g_oPendingContractRspMap.begin(); it != g_oPendingContractRspMap.end(); ++it )
	{
		CDmoContractPtr pContract = it->second;
		if( ucTentativeReqID == pContract->m_unContractID )
		{
			//find the next available ID
			++ucTentativeReqID;
		}
		if( ucTentativeReqIDWrapped == pContract->m_unContractID && ucTentativeReqIDWrapped < s_ucContractReqID )
		{
			//find the next available ID, if ucTentativeReqID wrapped over 255
			++ucTentativeReqIDWrapped;
		}
		if( CONTRACT_PENDING_FAILED == pContract->m_ucContractStatus )
		{
			//find the ID of a reclaimable failed request, if there are no available ID's
			if( !bFoundReclaimableID )
			{
				bFoundReclaimableID = true;
				ucReclaimableReqID = pContract->m_unContractID;
			}
			if( ucReclaimableReqID < s_ucContractReqID && pContract->m_unContractID >= s_ucContractReqID )
			{
				ucReclaimableReqID = pContract->m_unContractID;
			}
		}
		if( p_unDstTLPort == pContract->m_unDstTLPort
			&& p_unSrcTLPort == pContract->m_unSrcTLPort 
			&& !memcmp(p_pDstAddr128, pContract->m_aDstAddr128, sizeof(IPV6_ADDR)))
		{
			if( p_ucSrvcType == ANY_SERVICE_TYPE )
			{
				// They don't care which specific pending contract request they get, they just want one of them.
				// Therefore, they shouldn't mind if we don't give them a requestID.
				if( p_pnRequestId )
				{
					LOG_ISA100(LOGLVL_ERR, "DMO_IsPendingContractRequest PROGRAMMER ERROR: Can't give a contract requestID if the service type is unspecified!");
				}
				if( CONTRACT_PENDING_NO_KEY == pContract->m_ucContractStatus || p_ucPendingState == pContract->m_ucContractStatus)
				{
					//we always return the PENDING_NO_KEY contracts, but they may also want PENDING contracts
					return pContract;
				}
			}
			if( p_ucSrvcType == pContract->m_ucServiceType )
			{
				//got a contract with required addresses, ports and service type
				if(p_pnRequestId)
				{
					// If this is a PENDING or PENDING_NO_KEY contract, they want it and its contract request ID.
					// If this is a FAILED contract, they probably want to reinitiate that request, so they need this contract request ID
					*p_pnRequestId = (uint8)pContract->m_unContractID;
				}
				if( CONTRACT_PENDING_NO_KEY == pContract->m_ucContractStatus || p_ucPendingState == pContract->m_ucContractStatus )
				{
					//we always return the PENDING_NO_KEY contracts, but they may also want PENDING contracts
					return pContract;
				}
				//found a FAILED contract.
				//There is no need to search further. This contract request is not pending, and we've given them the ID if they wanted it.
				return CDmoContractPtr();
			}
		}
	}
	if(p_pnRequestId)
	{
		if( p_ucSrvcType == ANY_SERVICE_TYPE )
		{
			LOG_ISA100(LOGLVL_ERR, "DMO_IsPendingContractRequest PROGRAMMER ERROR: Can't give a contract requestID if the service type is unspecified!");
			return CDmoContractPtr();
		}
		//we didn't find their contract request, so we give them a contract request ID.
		if(ucTentativeReqID >= s_ucContractReqID)
		{
			*p_pnRequestId = ucTentativeReqID++;
			s_ucContractReqID = ucTentativeReqID;
			return CDmoContractPtr();
		}
		if(ucTentativeReqIDWrapped < s_ucContractReqID)
		{
			*p_pnRequestId = ucTentativeReqIDWrapped++;
			s_ucContractReqID = ucTentativeReqIDWrapped;
			return CDmoContractPtr();
		}
		//reclaim the contract request...
		//TODO: determine whether this is good enough or maybe we should reclaim the oldest request?
		--g_ucContractRspPending;
		CDmoContractPtr pContract = g_oPendingContractRspMap.find(ucReclaimableReqID)->second;
		pContract->m_ucContractStatus = CONTRACT_PENDING_FAILED;
		LOG_ISA100(LOGLVL_ERR, "apparently, there are no available contract request id's, we must reclaim one of them...");
		UAP_NotifyContractFailure( pContract.get() );
		*p_pnRequestId = ucReclaimableReqID++;
		s_ucContractReqID = ucReclaimableReqID;
		if(!bFoundReclaimableID)
		{
			LOG_ISA100(LOGLVL_ERR, "WARNING:DMO_IsPendingContractRequest: all contract request ID's are in use, nothing was reclaimable! We reclaimed PENDING request %d", *p_pnRequestId);
		}
	}
	return CDmoContractPtr();
}

static void DMO_TimeoutContractReqs(void)
{
	uint32 nNow = MLSM_GetCrtTaiSec();
//	LOG_ISA100(LOGLVL_DBG, "DMO_TimeoutContractReqs: nNow=%d", nNow);
	for(CPendingContractsMap::iterator it = g_oPendingContractRspMap.begin(); it != g_oPendingContractRspMap.end(); ++it )
	{
//		LOG_ISA100(LOGLVL_DBG, "DMO_TimeoutContractReqs: checking pending ContractReqId=%d (timeout deadline=%d)", it->second->m_unContractID, it->second->m_ulActivationTAI);
		if(it->second->m_ulActivationTAI < nNow && it->second->m_ucContractStatus != CONTRACT_PENDING_FAILED) //in g_oPendingContractRspMap, m_ulActivationTAI is really the deadline to switch status to FAILED
		{
			--g_ucContractRspPending;
			it->second->m_ucContractStatus = CONTRACT_PENDING_FAILED;
			LOG_ISA100(LOGLVL_INF, "DMO_TimeoutContractReqs: timeout for pending ContractReqId=%d", it->second->m_unContractID);
			UAP_NotifyContractFailure( it->second.get() );
		}
	}
}
#endif
