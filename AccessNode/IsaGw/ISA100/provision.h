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
/// Date:         February 2008
/// Description:  This file holds the provisioning data
/// Changes:      Rev. 1.0 - created
/// Revisions:    Rev. 1.0 - by author
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_PROVISION_H_
#define _NIVIS_PROVISION_H_

#include "config.h"
#include "dlme.h"
#include "typedef.h"

#define MAX_DLL_PAYLOAD_SIZE  100
#define JOIN_BACKOFF_BASE     5   // 500 msec

#define VENDOR_ID_SIZE  5
#define MODEL_ID_SIZE   9
#define TAG_NAME_SIZE   4
#define PWR_SUPPLY_SIZE 3
#define MEM_INFO_SIZE   6

#include "dmap_dpo.h"

extern EUI64_ADDR   c_oEUI64BE; // nwk order EUI64ADDR

#define   c_oSecManagerEUI64BE g_stDPO.m_aTargetSecurityMngrEUI
  
extern uint8 c_aucJoinKey[SEC_KEY_LEN];

extern const uint8 c_ucMaxDllPayload;  
extern const DLL_MIB_DEV_CAPABILITIES c_stCapability;
extern const DLL_MIB_ENERGY_DESIGN    c_stEnergyDesign; 

  
#ifdef _BATTERY_OPERATED_DEVICE_  
  #define BATTERY 0x10
#else    
  #define BATTERY 0    
#endif // _BATTERY_OPERATED_DEVICE_


#if ( _DEMOAPP_TYPE == 4 )
  #define ACQUISITION  0x20  
#else   
  #define ACQUISITION 0
#endif //  _DEMOAPP_TYPE     

#define CAPABILITY (ACQUISITION | BATTERY)  
  
#ifdef BACKBONE_SUPPORT
  #define DLL_DL_ROLE   0x04
#elif ROUTING_SUPPORT
  #define DLL_DL_ROLE   0x02
#else
  #define DLL_DL_ROLE   0x01
#endif  
  
uint8 PROVISION_AddCmdToProvQueue( uint16  p_unObjId,
                                   uint8   p_ucAttrId,
                                   uint8   p_ucSize,
                                   uint8*  p_pucPayload );
void PROVISION_ValidateProvQueue( void );
void PROVISION_ExecuteProvCmdQueue( void );   

    #define g_ucProvisioned 1
        
void ReadPersistentData( uint8 *p_pucDst, uint32 p_uAddr, uint16 p_unSize );
void WritePersistentData( const uint8 *p_pucSrc, uint32 p_uAddr, uint16 p_unSize );

  
  
#endif // _NIVIS_PROVISION_H_
