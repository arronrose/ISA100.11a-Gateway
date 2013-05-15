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
/// Date:         December 2007
/// Description:  This file holds definitions of the data link layer - dlme module
/// Changes:      Created
/// Revisions:    1.0
/// @endverbatim
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NIVIS_DLME_H_
#define _NIVIS_DLME_H_

/*===========================[ public includes ]=============================*/
#include "typedef.h"
#include "config.h"

/*==========================[ public datatypes ]=============================*/

typedef uint8 DLL_SCHEDULE[16];

typedef struct
{
  uint8 m_mfDllJoinBackTimeout; //xxxxyyyy
                                //xxxx - maximum extent of exponential backoff on joining
                                //yyyy - timeout to receive a system manager response to a join request
  uint8 m_mfDllJoinLinksType;   //
                                //b7b6 - schedule type for the Join Request Tx link; range 0-2
                                //b5b4 - schedule type for the Join Response Rx link; range 0-2
                                //b3 - indicate if "m_stDllJoinAdvRx" is valid and transmited
                                //b2 - indicate the type of advertisement( = 1 - burst advertisement; = 0 - one advertisment/slot)
                                //   - if b3=0, b2 is meaningless                              
                                //b1b0 - schedule type for the Advertisement Rx link; range 0-2
                                //     - if bit b3=0, this is also meaningless
  DLL_SCHEDULE m_stDllJoinTx;   //schedule for Join Request Tx link
  DLL_SCHEDULE m_stDllJoinRx;   //schedule for Join Response Rx link
  DLL_SCHEDULE m_stDllJoinAdvRx;//schedule for Advertisement Rx link
} DLL_MIB_ADV_JOIN_INFO;


#define DLL_MASK_JOIN_BACKOFF   0xf0
#define DLL_MASK_JOIN_TIMEOUT   0x0f

#define DLL_MASK_JOIN_TX_SCHEDULE   0xc0
#define DLL_MASK_JOIN_RX_SCHEDULE   0x30
#define DLL_MASK_ADV_RX_FLAG        0x08
#define DLL_MASK_ADV_RX_BURST       0x04
#define DLL_MASK_ADV_RX_SCHEDULE    0x03

#define DLL_JOIN_TX_SCHD_OFFSET     0x06
#define DLL_JOIN_RX_SCHD_OFFSET     0x04

#define SEC_KEY_LEN             16

typedef struct
{
  uint16  m_unQueueCapacity; 
  uint16  m_unChannelMap; 
  uint16  m_unAckTurnaround;
  uint16  m_unNeighDiagCapacity;
  uint8   m_ucClockAccuracy;   
  uint8   m_ucDLRoles;    
  uint8   m_ucOptions;
} DLL_MIB_DEV_CAPABILITIES;

typedef struct
{
  int16   m_nEnergyLife;
  uint16  m_unListenRate;
  uint16  m_unTransmitRate;
  uint16  m_unAdvRate;  
}DLL_MIB_ENERGY_DESIGN;

#endif /* _NIVIS_DLME_H_ */

