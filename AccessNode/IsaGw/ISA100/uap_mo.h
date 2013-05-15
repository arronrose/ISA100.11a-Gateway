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
/// Author:       Nivis LLC, Mircea Vlasin 
/// Date:         June 2009
/// Description:  This file holds the definitions of the MO object of the UAP
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "typedef.h"
#include "aslde.h"
#include "dmap_utils.h"

enum
{
    UAPMO_RESERVED,
    UAPMO_VERSION,
    UAPMO_STATE,
    UAPMO_COMMAND,
    UAPMO_MAXRETRIES,
    UAPMO_NUM_UNSCHED_COORESPONDENT,
    UAPMO_TABLE_UNSCHED_COORESPONDENT,
    UAPMO_TABLE_CONTRACT_UNSCHED_COORESPONDENT,
    UAPMO_NUM_OBJECTS,
    UAPMO_TABLE_OBJECTS,
    UAPMO_STATIC_REVISION,

    UAPMO_ATTR_NO                  
}; // UAPMO_ATTRIBUTES

enum UAP_STATES {
    UAP_INACTIVE,
    UAP_ACTIVE,
    UAP_FAILED
};

typedef struct
{
    IPV6_ADDR m_pucIPv6Addr;
    uint16    m_unTLPort;
} UNSCHEDULED_CORRESPONDENT;

typedef struct
{
    uint16 m_unObjectId;
    uint8  m_ucObjectType;
    uint8  m_ucObjSubType;
    uint8  m_ucVendorSubType;
}OBJECT_ID_AND_TYPE;

typedef struct{
    uint16  m_unContractID;
    uint8   m_ucContractStatus;
    uint16  m_unActualPhase;
}COMMUNICATION_CONTRACT_DATA;


//#define DEFAULT_UAP_RETRY_TIMEOUT  20   //not specified by standard    (but not needed on the dynamic-retries-enabled common stack)

    extern uint8 g_ucMaxUAPRetries;
    extern const DMAP_FCT_STRUCT c_aUapMoAttributes[UAPMO_ATTR_NO];

    #define UAPMO_Read(p_unAttrID,p_punBufferSize,p_pucRspBuffer) \
                DMAP_ReadAttr(p_unAttrID,p_punBufferSize,p_pucRspBuffer,c_aUapMoAttributes,UAPMO_ATTR_NO)
    
    #define UAPMO_Write(p_unAttrID,p_ucBufferSize,p_pucBuffer)   \
                DMAP_WriteAttr(p_unAttrID,p_ucBufferSize,p_pucBuffer,c_aUapMoAttributes,UAPMO_ATTR_NO)
