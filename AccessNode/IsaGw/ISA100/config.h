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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Name:         config.h
// Author:       Nivis, LLC
// Date:         March 2008
// Description:  Define main ISA config parameters for different layers
// Changes:
// Revisions:
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NIVIS_ISA100_CONFIG_H_
#define _NIVIS_ISA100_CONFIG_H_

#include "global.h"

  // DMAP DMO initial default defines
  // defines for inital default dmo attributes values
  #define DMO_DEF_DMAP_STATE          1
  #define DMO_DEF_CONTRACT_TOUT       30
  #define DMO_DEF_MAX_CLNT_SRV_RETRY  3
  #define DMO_DEF_MAX_RETRY_TOUT_INT  64//30  //64 sec - maximum acceptable by TL layer validation(message TTL)
  #define DMO_DEF_WARM_RESTART_TOUT   60

  #define ARMO_QUEUE_SIZE         16384

  // contract establishment timeouts
  #define CONTRACT_WAIT_TIMEOUT   (DMO_DEF_MAX_CLNT_SRV_RETRY * DMO_DEF_MAX_RETRY_TOUT_INT)   // units in seconds     
  #define CONTRACT_RETRY_TIMEOUT  (DMO_DEF_MAX_CLNT_SRV_RETRY * DMO_DEF_MAX_RETRY_TOUT_INT)   // units in seconds

// Application configuration
#define MAX_SIMULTANEOUS_JOIN_NO  1
#define COMMON_APDU_BUF_SIZE		65535

  #define MAX_APDU_CONF_ENTRY_NO    5   // maximum TL confirmation list size
  #define MIN_JOIN_FREE_SPACE       600 // minimum rx/tx APDU buffer to accept a join request

  /* time to live (seconds) of an APDU in the queue             */
  #define APP_QUE_TTL_SEC    60 //6  // 20 //todo: this could be the joinTimeout....

// Application sublayer configuration
    #define MAX_PARAM_SIZE        130
    #define MAX_RSP_SIZE          220
    #define MAX_GENERIC_VAL_SIZE  100

// Network layer configuration
    #define MAX_DATAGRAM_SIZE     (65535-20-8-40) // 65535(max ip4 len) - 20(ip4 hdr) - 8(udp hdr) - 40(ip6 hdr)
    #define MAX_APDU_SIZE         (MAX_DATAGRAM_SIZE-8-4-MIC_SIZE) // MAX_DATAGRAM_SIZE - 8(udp hdr) - 4(sec hdr) - 4(MIC)
    #define MAX_RCV_TIME_MULTIPLE_PK  10 // seconds // timeout for a multipacket RX message

    #define MAX_ROUTES_NO				5000
    #define MAX_CONTRACT_NO				5000
    #define MAX_ADDR_TRANSLATIONS_NO	5000
    #define MAX_SLME_KEYS_NO			MAX_CONTRACT_NO
    #define MAX_ASLMO_COUNTER_N0		8000   // counts malformed APDUs for maximum 8000 communication endpoints

// Data Link Layer configuration
#ifdef BACKBONE_SUPPORT
    #define DLL_MAX_SUPERFRAMES_NO  4  //+2
    #define DLL_MAX_CHANNELS_NO     8  //6
    #define DLL_MAX_TIMESLOTS_NO    4  //+2
    #define DLL_MAX_LINKS_NO        25
    #define DLL_MAX_GRAPHS_NO       2  //+1
    #define DLL_MAX_ROUTES_NO       10 //+6
    #define DLL_MAX_NEIGHBORS_NO    10 //+6
    #define DLL_MAX_NEIGH_DIAG_NO   2

    #define DLL_MAX_FUTURE_SUPERFRAMES_NO  2
    #define DLL_MAX_FUTURE_CHANNELS_NO     2 //1
    #define DLL_MAX_FUTURE_TIMESLOTS_NO    2
    #define DLL_MAX_FUTURE_LINKS_NO        4
    #define DLL_MAX_FUTURE_GRAPHS_NO       1
    #define DLL_MAX_FUTURE_ROUTES_NO       2
    #define DLL_MAX_FUTURE_NEIGHBORS_NO    2

    #define DLL_ADVERTISE_RATE             1000   // signed int 16 - capacity to transmit advertismenst/minute
    #define DLL_IDLE_LISTEN                1000   // signed int 16 - capacity for idle listening (seconds pe hour)

#elif defined( ROUTING_SUPPORT )
    #define DLL_MAX_SUPERFRAMES_NO  4  //+2
    #define DLL_MAX_CHANNELS_NO     8  //6  //+4
    #define DLL_MAX_TIMESLOTS_NO    4  //+2
    #define DLL_MAX_LINKS_NO        25
    #define DLL_MAX_GRAPHS_NO       2  //+1
    #define DLL_MAX_ROUTES_NO       10
    #define DLL_MAX_NEIGHBORS_NO    10
    #define DLL_MAX_NEIGH_DIAG_NO   2

    #define DLL_MAX_FUTURE_SUPERFRAMES_NO  2
    #define DLL_MAX_FUTURE_CHANNELS_NO     1  //2
    #define DLL_MAX_FUTURE_TIMESLOTS_NO    2
    #define DLL_MAX_FUTURE_LINKS_NO        5
    #define DLL_MAX_FUTURE_GRAPHS_NO       1
    #define DLL_MAX_FUTURE_ROUTES_NO       2
    #define DLL_MAX_FUTURE_NEIGHBORS_NO    2

    #define DLL_ADVERTISE_RATE             100 // signed int 16 - capacity to transmit advertismenst/minute
    #define DLL_IDLE_LISTEN                0   // signed int 16 - capacity for idle listening (seconds pe hour)

#else // non router support (end node)
    #define DLL_MAX_SUPERFRAMES_NO  4 //  6
    #define DLL_MAX_CHANNELS_NO     8 //6
    #define DLL_MAX_TIMESLOTS_NO    4 // 8
    #define DLL_MAX_LINKS_NO        8 // 14
    #define DLL_MAX_GRAPHS_NO       2 // 4
    #define DLL_MAX_ROUTES_NO       4 // 10
    #define DLL_MAX_NEIGHBORS_NO    3 // 25
    #define DLL_MAX_NEIGH_DIAG_NO   1

    #define DLL_MAX_FUTURE_SUPERFRAMES_NO  2
    #define DLL_MAX_FUTURE_CHANNELS_NO     1
    #define DLL_MAX_FUTURE_TIMESLOTS_NO    2
    #define DLL_MAX_FUTURE_LINKS_NO        5  //at least 3 links needed inside PrepareSMIBS after DAUX received
    #define DLL_MAX_FUTURE_GRAPHS_NO       1
    #define DLL_MAX_FUTURE_ROUTES_NO       2
    #define DLL_MAX_FUTURE_NEIGHBORS_NO    2

    #define DLL_ADVERTISE_RATE            0   // signed int 16 - capacity to transmit advertismenst/minute
    #define DLL_IDLE_LISTEN               0   // signed int 16 - capacity for idle listening (seconds pe hour)

#endif //defined( BACKBONE_SUPPORT )

    #define MAX_TX_NO_TO_ONE_NEIHBOR      4
    #define RECOVERY_TIME                 200     // recovery delay for NACK1 acknowledge defined as nr. of slots, aprox. 2sec
    #define DLL_MSG_QUEUE_SIZE_MAX        4       // DLL message queue size
    #define DLL_MAX_CANDIDATE_NO          4
    #define DLL_CHANNEL_MAP               0xFFFF  // all channels available
    #define DLL_FORWARD_RATE              100     // device's energy capacity to forward DPDUs on behalf of its neighbors, in number of DPDUs per minute
    #define DLL_ACK_TURNAROUND            100     // time needed by the device to process incomming DPDU and respond to it with ACK/NACK, in units of 2^-15
    #define DLL_CLOCK_ACCURACY            8       // nominal clock accuracy in ppm ( DS32khz datasheet specifies 7.5 ppm for -40 to 85 degrees celsius )

    #define ARMO_ALERT_ACK_TIMEOUT        180    // units in seconds; reset device if no ACK is received for an alert within ARMO_ALERT_ACK_TIMEOUT seconds   
    #define MAX_NB_OF_PORTS   16

#endif // _NIVIS_ISA100_CONFIG_H_
