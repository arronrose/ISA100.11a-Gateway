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
/// Date:         February 2009
/// Description:  This file implements the device provisioning object in DMAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "dmap_dpo.h"
#include "uap.h"
//#include "uap_data.h"
#include "dmap.h"
//#include "dlmo_utils.h"
#include "dmap_dmo.h"
//#include "dmap_dlmo.h"
//#include "dmap_co.h"
//#include "mlsm.h"
#include "dlme.h"
#include "slme.h"

/*#ifdef PROVISIONING_DEVICE
  #undef BACKBONE_SUPPORT
#endif*/

    // provision info   
#ifndef FILTER_BITMASK        
    #define FILTER_BITMASK 0xFFFF //specific network only      
#endif

#ifndef FILTER_TARGETID        
    #define FILTER_TARGETID 0x0001 // default provisioning network      
#endif

#define DPO_SIGNATURE              0xD4 
#define DPO_DEFAULT_PROV_SIGNATURE 0xFF

#define ISA100_GW_UAP       2



DPO_STRUCT  g_stDPO;
void DPO_WriteTargetNwkId(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_ReadDLConfigInfo(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
void DPO_WriteDLConfigInfo(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

void DPO_readAssocEndp(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
void DPO_writeAssocEndp(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_readAttrDesc(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize);
void DPO_writeAttrDesc(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_WriteDeviceRole(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);
void DPO_WriteUAPCORevision(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize);

const uint16  c_unDefaultNetworkID      = 1; //id for the default network
#define   c_aDefaultSYMJoinKey c_aulWellKnownISAKey
#define   c_aOpenSYMJoinKey    c_aulWellKnownISAKey

const uint16  c_unDefaultFrequencyList  = 0xFFFF; //list of frequencies used by the adv. routers of the default network

// has to start with write dll config, has to end with write ntw id

  #define DPO_writeUint8          NULL
  #define DPO_writeUint16         NULL
  #define DPO_writeVisibleString  NULL

const DMAP_FCT_STRUCT c_aDPOFct[DPO_ATTR_NO] = {
	{ 0,   0,                                              DMAP_EmptyReadFunc,     NULL },
	{ ATTR_CONST(c_unDefaultNetworkID),                    DMAP_ReadUint16,        NULL },
	{ ATTR_CONST(c_aDefaultSYMJoinKey),                    DMAP_ReadVisibleString, NULL },
	{ ATTR_CONST(c_aOpenSYMJoinKey),                       DMAP_ReadVisibleString, NULL },
	{ ATTR_CONST(c_unDefaultFrequencyList),                DMAP_ReadUint16,        NULL } ,
	{ ATTR_CONST(g_stDPO.m_ucJoinMethodCapability),        DMAP_ReadUint8,         DPO_writeUint8 },
	{ ATTR_CONST(g_stDPO.m_ucAllowProvisioning),           DMAP_ReadUint8,         DPO_writeUint8 },
	{ ATTR_CONST(g_stDPO.m_ucAllowOverTheAirProvisioning), DMAP_ReadUint8,         DPO_writeUint8 },
	{ ATTR_CONST(g_stDPO.m_ucAllowOOBProvisioning),        DMAP_ReadUint8,         DPO_writeUint8 },
	{ ATTR_CONST(g_stDPO.m_ucAllowResetToFactoryDefaults), DMAP_ReadUint8,         NULL },
	{ ATTR_CONST(g_stDPO.m_ucAllowDefaultJoin),            DMAP_ReadUint8,         DPO_writeUint8 },
	{ ATTR_CONST(g_stDPO.m_unTargetNwkID),                 DMAP_ReadUint16,        DPO_WriteTargetNwkId },
	{ ATTR_CONST(g_stDPO.m_unTargetNwkBitMask),            DMAP_ReadUint16,        DPO_writeUint16 },
	{ ATTR_CONST(g_stDPO.m_ucTargetJoinMethod),            DMAP_ReadUint8,         DPO_writeUint8 },
	{ ATTR_CONST(g_stDPO.m_aTargetSecurityMngrEUI),        DMAP_ReadVisibleString, DPO_writeVisibleString },
	{ ATTR_CONST(g_stDMO.m_aucSysMng128BitAddr),           DMAP_ReadVisibleString, DPO_writeVisibleString },
	{ ATTR_CONST(g_stDPO.m_unTargetFreqList),              DMAP_ReadUint16,        DPO_writeUint16 },
	{ NULL, MAX_GENERIC_VAL_SIZE,                          DPO_ReadDLConfigInfo,   DPO_WriteDLConfigInfo },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
#ifdef BACKBONE_SUPPORT
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
	{ NULL, 0,                                             DMAP_EmptyReadFunc,     NULL },
#else
	{ ATTR_CONST(g_stDPO.m_nCurrentUTCAdjustment),         DMAP_ReadUint16,        DMAP_WriteUint16 },
	{ ATTR_CONST(g_stDPO.m_stDAQPubCommEndpoint),          CO_readAssocEndp,       DPO_writeAssocEndp },
	{ ATTR_CONST(g_stDPO.m_aAttrDescriptor),               DPO_readAttrDesc,       DPO_writeAttrDesc },
	{ ATTR_CONST(g_stDPO.m_unDeviceRole),                  DMAP_ReadUint16,        DPO_WriteDeviceRole },
	{ ATTR_CONST(g_stDPO.m_ucUAPCORevision),               DMAP_ReadUint8,         DPO_WriteUAPCORevision },
#endif
};

//from the joining device point of view
#define ROUTER_JREQ_ADV_RX_BASE   1
#define ROUTER_JREQ_TX_OFF        3
#define ROUTER_JRESP_RX_OFF       5

#ifdef BACKBONE_SUPPORT
  DLL_MIB_ADV_JOIN_INFO c_stDefJoinInfo =
  {
    0x24,     // Join timeout is 2^4 = 16 sec, Join backof is 2^2 = 4 times
    DLL_MASK_ADV_RX_FLAG,   //all join links type Offset only 
	{ ROUTER_JREQ_TX_OFF,      0, },
	{ ROUTER_JRESP_RX_OFF,     0, },
	{ ROUTER_JREQ_ADV_RX_BASE, 0, }
  };
#endif
  
#ifdef BACKBONE_SUPPORT  
  void DPO_Init(void)
  {
      // Necesary for initialization of JoinBackoff and JoinTimeout
//      DLME_SetMIBRequest(DL_ADV_JOIN_INFO, &c_stDefJoinInfo);
      
      g_stDPO.m_unDeviceRole = DEVICE_ROLE;
//      g_unDllSubnetId = g_stFilterTargetID;
  }
  
  void DPO_ReadDLConfigInfo(const void* p_pValue, uint8* p_pBuf, uint8* p_ucSize)
  {
  }

  void DPO_WriteDLConfigInfo(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {    
  }

  void DPO_WriteTargetNwkId(void* p_pValue, const uint8* p_pBuf, uint8 p_ucSize)
  {    
  }
    
  
  uint8 DPO_ResetToDefault(uint16* p_pRspSize,
                           uint8*  p_pRspBuf)
  {
    *p_pRspSize = 0;    
    return SFC_INCOMPATIBLE_MODE;
  }
  
  uint8 DPO_WriteSYMJoinKey(uint16  p_unReqSize, 
                            uint8*  p_pReqBuf,
                            uint16* p_pRspSize,
                            uint8*  p_pRspBuf)
  {
    uint8 ucSFC = SFC_OBJECT_STATE_CONFLICT;
    *p_pRspSize = 0;
    return ucSFC;
  }
  

  uint8 DPO_Execute(uint8   p_ucMethID,
                     uint16  p_unReqSize, 
                     uint8*  p_pReqBuf,
                     uint16* p_pRspSize,
                     uint8*  p_pRspBuf)
  {
    uint8 ucSFC = SFC_OBJECT_STATE_CONFLICT;
    *p_pRspSize = 0;
    return ucSFC;
  }
      
#endif  // ! BACKBONE_SUPPORT
