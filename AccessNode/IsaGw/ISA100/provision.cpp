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

#include <string.h>

#include "provision.h"
//#include "../isa100/dmap_dmo.h"
//#include "../isa100/dlmo_utils.h"
//#include "../isa100/dmap.h"
//#include "../isa100/mlde.h"
//#include "../isa100/mlsm.h"


  EUI64_ADDR c_oEUI64BE; // nwk order EUI64ADDR
  EUI64_ADDR c_oEUI64LE; // little endian order
  
#if defined(at91sam7x512)
  uint16 c_unPort; 
  uint32 c_ulMASK;   //
  uint32 c_ulGWY;    //
  uint32 c_ulIP4LOG;        //
  uint16 c_usPort4LOG;      //
  uint8 c_ucVerboseLOG[NUMBER_OF_MODULES];     //
#ifdef WCI_SUPPORT  
  uint32 c_ulIP4LOGAck;     //  
  uint16 c_usPort4LOGAck;   //
  uint8 c_ucVerboseLOGAck[NUMBER_OF_MODULES];     //
#endif  // WCI_SUPPORT  
#endif  // at91sam7x512

uint8 g_ucPAValue;

const DLL_MIB_DEV_CAPABILITIES c_stCapability = 
{
  DLL_MSG_QUEUE_SIZE_MAX, // m_unQueueCapacity
  DLL_CHANNEL_MAP,        // m_unChannelMap
  DLL_ACK_TURNAROUND,     // m_unAckTurnaround
  200,                    // m_unNeighDiagCapacity
  DLL_CLOCK_ACCURACY,     // m_ucClockAccuracy;
  DLL_DL_ROLE,            // m_ucDLRoles;
  (BIT0 | BIT2)           // m_ucOptions;  
};

const DLL_MIB_ENERGY_DESIGN c_stEnergyDesign = 
{
#ifdef BACKBONE_SUPPORT
  
  0x7FFF, //  days capacity, 0x7FFF stands for permanent power supply
    3600, //  3600 seconds per hour Rx listen capacity
     600, //  600 seconds per hour Rx listen capacity
     120  //  120 Advertises/minute rate 
  
#else 
    #ifdef ROUTING_SUPPORT
      180,    // 180 days battery capacity
      360,    // 360 seconds per hour Rx listen capacity
      100,    // 100 DPDUs/minute transmit rate
       60     //  60 Advertises/minute rate          
    #else
      180,    // 180 days battery capacity
      360,    // 360 seconds per hour Rx listen capacity
      100,    // 100 DPDUs/minute transmit rate
       60     //  60 Advertises/minute rate     
  #endif // ROUTING_SUPPORT 
#endif // BACKBONE_SUPPORT
};

#define CODE_CAN 124
#define CODE_JPN 392
#define CODE_MEX 484
#define CODE_USA 840
    
  void PROVISION_Overwrite(void)
  {
//#define OVERWRITE_MANUFACTURING_INFO  // if this line is uncommented will overwrite the provisioned info     
#ifdef OVERWRITE_MANUFACTURING_INFO
    
    // manufacturing info
    const struct 
    {
        uint16 m_unFormatVersion; 
        uint8  m_aMAC[8];              
        uint16 m_unVRef;    
        uint8  m_ucMaxPA;        
        uint8  m_ucCristal;
        uint8  m_aProvKEY[16];
        uint8  m_aProvSecManager[8];
        uint8  m_aDeviceTag[16];
        uint8  m_aPAProfile0[17];
        uint8  m_aPAProfile1[17];
        uint8  m_aPAProfile2[17];
        uint8  m_aPAProfile3[17];
        uint16 m_unCountryCode[16];
    } c_stManufacturing = {1, 
      #if defined( PROVISIONING_DEVICE )
                        { 0xFD, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, _ISA100_DEVICE_ID }, // dev addr
      #elif defined( BACKBONE_SUPPORT )
                        { 0xFB, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, _ISA100_DEVICE_ID }, // dev addr
                        
      #elif defined( ROUTING_SUPPORT )
                        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0B, _ISA100_DEVICE_ID }, // dev addr

      #else
                        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, _ISA100_DEVICE_ID }, // dev addr
                        
      #endif                            
                        0xC409,
                        0xE7,
                        0x00,
                        { 0x00,0x49,0x00,0x53,0x00,0x41,0x00,0x20,0x00,0x31,0x00,0x30,0x00,0x30,0x00,0x00 }, // m_aProvKEY 
                        { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }, // m_aProvSecManager 
                        { 'I', 'N', 'T', 'E', 'G', 'R', 'A', 'T', 'I', 'O', 'N', ' ', 'K', 'I', 'T', ' ' }, // m_aDeviceTag
                        { 0x00,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7 }, // m_aPAProfile0
                        { 0x00,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7 }, // m_aPAProfile1
                        { 0x00,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7 }, // m_aPAProfile2
                        { 0x00,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7 }, // m_aPAProfile3
                        { CODE_USA, CODE_CAN, CODE_MEX, CODE_JPN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
                        };
    
    EraseSector( MANUFACTURING_SECTOR_NO );
    WritePersistentData( (uint8*)&c_stManufacturing, MANUFACTURING_START_ADDR, sizeof(c_stManufacturing) );

  #endif // OVERWRITE_MANUFACTURING_INFO
  }

  
  void PROVISION_Init(void)
  {        
//   if( !g_stDPO.m_ucStructSignature ) 
     {
         PROVISION_Overwrite();
     }        
  }
  
