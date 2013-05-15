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

/***************************************************************************
                          Common.h  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    email                : marcel.ionescu@nivis.com
 ***************************************************************************/

//lint -library

#ifndef _COMMON_H_
#define _COMMON_H_

#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <stdint.h>
#include <cstdio>

#include "AnPaths.h"
#include "UtilsSolo.h"

#include "log_callback.h"

#ifdef CALLBACK_LOG	// support for callback logging


/// compatibility hack. c_log actually does not log perror
/// TODO: implement c_logerr
#define LOG_ERR c_log_errno_lvlerr
#define LOG c_log_lvlerr
#define LOG_HEX c_loghex_lvlerr

#else	// normal logging


//include log defines like LOG, LOG_HEX, NLOG_ERR, etc
#include "LogDefs.h"

const char* NLogLevelGetName (NLOG_LVL p_nLogLevel);
NLOG_LVL NLogLevelGetType (const char*  p_szLogLevel);

#endif //CALLBACK_LOG

class CANEvent;

//define all modules names
#define HISTORY "hm"
#define CC_COMM "cc"
#define RA "ra"
#define NC "nc"

// typedefs
struct rf_address{
    unsigned char id[4];
}__attribute__((packed));

typedef rf_address CMeshAddress;

struct event_code{
	unsigned char id[2];
}__attribute__((packed));

struct net_address
{
    unsigned long m_nIP;
    unsigned short m_dwPortNo;
}__attribute__((packed));

#define IPv6_ADDR_LENGTH 16	///< The length of a IPv6 Address.
struct TIPv6Address
{
	const static int nIPv6PrintSize = 4 * 8 + 7 + 1; // 8 groups of 2 bytes and 7 ':'and '\0'
	uint8_t m_pu8RawAddress[IPv6_ADDR_LENGTH];

	TIPv6Address();

	int FillIPv6 (char* p_szBuff, int p_nMaxSize) const ;

	const char* GetPrintIPv6();
};

struct events
{
	int nId;
	short shEnabled;
	short shAlarm;
}__attribute__((packed));


inline clock_t GetClockTicks( void ) {	struct tms tms_buf;	return times( &tms_buf );}

int CheckAndFixWrapClockTicks( clock_t p_nCrtTime, clock_t *p_pnTimeout );
int TrackPointTimeout (int p_nDuration, const char *p_szFile, const char* p_szFunc, int p_nLine);

#ifdef CYG
	int IGN_FCT1( ... );
	#define stime IGN_FCT1
	#define setpriority IGN_FCT1
	#define PRIO_PROCESS 0
	#define PIPE_BUF 4096
//	#define PATH_MAX 512
//#define INT_MAX 2000000000
	//const void * memmem( const void * p_pBuffer, unsigned int p_nBufferLen, const void * p_pNeed,  unsigned int p_nNeedLen );
#endif



#if __GNUC__ < 3 || CYG
//const void * memmem( const void * p_pBuffer, unsigned int p_nBufferLen, const void * p_pNeed,  unsigned int p_nNeedLen );
#endif

const char* JumpOverBlank(const char *p_szString);
bool EscapeString(char *p_szString);

char* RTrim( const char* p_szString );
int ReadField( char* p_szString, char* p_pField, int p_nLen, int sens = 0 );

int GetAbsDiffFromCrtTime(  unsigned char year,
                    unsigned char month,
                    unsigned char day,
                    unsigned char hour,
                    unsigned char minute,
                    unsigned char second
                   );


int GetFileLen( int fd );
int GetFileLen( const char * p_szFile );

int WriteToFile( const char* p_szFile, const char* p_pBuff, bool p_nTrunc = false, int p_nPos = -1 );
int WriteToFile( const char* p_szFile, const char* p_pBuff, int p_nLen, bool p_nTrunc = false, int p_nPos = -1 );
int GetFileData( const char* p_pFileName, char *& p_pData, int& p_rLen );

int GetSYSTime( unsigned char* pByteData );

int CheckAgainstAnTime( unsigned char* p_pTime );

/** convert a rf_address structure into a string statically stored
  TAKE CARE: DO NOT use this twice in the same function call. string is statically
  stored and will be overwritten!
*/
const char * Id2string( const rf_address & p_stAddr );
const char * Id2string( const unsigned int p_unAddr );


int IsAcceptedMsg( int p_nMsgSrc, unsigned char p_cCmd );
int EscapeYsiSdi12( u_char * p_pData, int p_nDataLen );



extern clock_t	g_nLastTimedLog;
//needed by CCC for obscure reasons
//#define DEBUG_LOG

enum
{
    PARAM_TYPE_CHAR = 1,
    PARAM_TYPE_SHORT,
    PARAM_TYPE_INT,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_STRING
};

/* offsetof macro */
#ifndef offsetof
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif


#if !defined(SAFE_DELETE)
#	define SAFE_DELETE(x)	if (x) {delete x; x = NULL;}
#endif

//file used by remote_access module to store execution results.
#define RA_TMP_FILE     "/tmp/RA.tmp"

#define BUFFER_FILE_DISK     		ACTIVITY_DATA_PATH"RMP_HM.bin"
#define BUFFER_FILE_FORCE_RESIZE	NIVIS_TMP"que_force_resize"

//serialisation file for PipeToRA. Needed when restarting
#define SER_PIPE2RA        ACTIVITY_DATA_PATH"PIPE2RA.ser11"

#define FROM_DN_000     	0x00
#define FROM_DN_001     	0x01
#define FROM_DN_010     	0x02
#define FROM_DN_011     	0x03
#define FROM_DN_1XX     	0x04

#define UNSET_DEST				-1
#define TO_CC_COMM				0
#define TO_HISTORY				1
#define TO_LOC_COMMANDER		2

#define FROM_CC_COMM    	        0x11
#define FROM_REMOTE_ACCESS         	0x12
#define FROM_SCHEDULER     	        0x13
#define FROM_HISTORY    	        0x14
#define FROM_LOC_COMMANDER			0x1A
#define FROM_LOC_ACT				0x20
#define FROM_EML_NC					0x21
#define FROM_REPEATER				0x22
#define FROM_CGIDB					0x23

#define TREAT_AS_EP_EVENT				0
#define TREAT_AS_EP_CMD					1
#define TREAT_AS_LOC_ACT				2


#define INTREFACE_LOCAL_ARM		0x00

#define INTERFACE_MESH		0x01
#define INTERFACE_LOCALDN	0x02

#define INTERFACE_SPI		0x80
#define INTERFACE_UNK		0xff






//DN GetData cmd codes
#define CMD_ACK                 	0X01
#define CMD_NACK                	0X02
#define CMD_GET_DATA 				0X03
#define CMD_GET_STATUS				0x05
#define CMD_GET_DEV_STATUS			0x06
#define CMD_RESTART					0x07
#define CMD_DEV_COMMAND				0x08
#define CMD_ACT_NEW_CONF			0x09
#define CMD_GENERIC_TEXT			0x0A
#define CMD_SET_REG_VAL         	0x0C
#define CMD_GET_REG_VAL         	0x0D
#define CMD_GET_DEV_STATUS_BUFFER	0x0E
#define CMD_GET_EVT_WITH_TIME		0x0F

#define CMD_READ_FROM_FLASH         0x10
#define CMD_WRITE_IN_FLASH          0x11
//#define CMD_DATA_BUFFER             0x12
#define CMD_REQ_SONDE_CFG           0x13
#define CMD_SONDE_REMOTE_CMD		0x14
#define CMD_ACTIVATE_SONDE_CFG      0x15

#define CMD_NEW_DEV_STATUS			0x16
#define CMD_GET_START_REASON        0x17
#define CMD_POWER_MANAGEMENT        0x18
#define CMD_SDI12_PNP               0x19
#define CMD_FORCE_SONDE_DETECTION	0x1A
#define CMD_SET_WAKE_UP_TIME		0x1B
#define CMD_KEEP_ALIVE				0x1C

#define CMD_YSI_BATCH				0x20



#define CMD_LOCALMSP_GET_VER		0x22
#define CMD_LOCALMSP_UPTIME			0x23

#define CMD_CANCEL_CMD				0x60
#define CMD_PROGRESS_CMD			0x61
//0x62 -- meshdebug
#define CMD_CHANGE_LAN_KEYS			0x63
#define CMD_DELAY					0x64

//MASK ALL COMMANDS WITH THIS - for real command code. First bit meaning: 0 - pass answer to CC, 1 - don't pass to CC
#define MASK_CMD	0x7F

//when this is found in Evt_file.cfg, command code is actually in data field (linked command)
#define CMD_LINKED              0xFF

//CC -> AN COMMAND RESPONSE: DN COMMAND
#define CC_CMD_DNCMD            0X0001

//restart event from DN - used to determine duplicate events
#define DN_EVT_ALARMCODE_RESTART    0x000C

//AN WAKEUP reasons:
//wakeup because DN waas powered on
#define START_REASON_DN_START			0x01
//wakeup because an DN event have the critical bit set, requesting AN wakeup
#define START_REASON_CRITICAL_EVT		0x02
//wakeup because a box open was detected (CURRENTLY DISABLED)
#define START_REASON_BOX_OPEN			0x04
//wakeup because AN requested to be restarted (PARAMETER: number of seconds of downtime requested)
#define START_REASON_AN_REQUEST			0x08
//wakeup because previous attempt to start AN failed (MUST be together with START_REASON_DN_WATCHDOG)
// PARAMETER: number of succesive failed attempts to start AN
#define START_REASON_PREV_WAKEUP_FAILED	0x10
//wakeup because DN watchdog has detected NO SPI activity. The sequence is 10 nim, 30 min, 30 min
//(except for START_REASON_DN_START set, when sequence is 10 min, 10 min, 30 min, 30 min ...)
#define START_REASON_DN_WATCHDOG		0x20

//length of time (6 bytes with current time)
#define TIME_SIZE	6

#define MIN_MESSAGE_ID 1000000000
#define MAX_MESSAGE_ID 2000000000

//MESSAGE ID  for for async DN request for critical alarms (self-generated requests)
//should be -1 (but this is an unsigned int...)
#define SELF_GENERATED_REQ_MSG_ID 0xFFFFFFFF

#define MAX_RS232_SIZE      1024

//size (2bytes) of the history contained message= msg_type(2)+data
#define MAX_HM_CONTAINED_MSG_SIZE  65533



#define MSG_ID_NOT_SET				  0xFFFFFFFF

//msg types to be used in comm AN <--> CC
#define RESPONSE_MSG_TYPE				3
#define INNER_EVENT_MSG_TYPE			4
#define DATA_NODE_VALUESEVENT_MSG_TYPE	5
#define DATA_NODE_MAPEVENT_MSG_TYPE		6
#define DATA_NODE_BUFFER_MSG_TYPE		7
#define DATA_NODE_HISTORY_MSG_TYPE		8
#define DATA_NODE_YSI_EVENT				9
#define DATA_NODE_GENERIC_VALUES    0x0A
#define DATA_NODE_SDI12_PNP         0x0B
#define BEGIN_TCP_CONNECTION		0x0C
#define PROTOCOL_ACK				0x0E
//#define END_TCP_CONNECTION			0x0D	//this is NOT sent by AN... for now
#define EXTENDED_AN_EVENT			0x0F
#define DATA_NODE_AMR_EVT			0x10
#define PROXY_PING					0x11
#define MSGTYPE_GET_ENCRYPTION_KEY	0x00C0
#define MSGTYPE_COMPACT_RESPONSE	0x00C1
#define AN_REG_CONFIRMATION				0x00C2


#define EMAIL_SENDER_MSG_ALARM			0x00D1
#define EMAIL_SENDER_MSG_FILE_DEL		0x00D2
#define EMAIL_SENDER_MSG_FILE_NO_DEL	0x00D3

//DN link type

#define DNLINKTYPE_LOCAL_ARM	1
#define DNLINKTYPE_SPI			2	//spi
#define DNLINKTYPE_LOCAL_MSP	3
#define DNLINKTYPE_RF_MESH		4


#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))
#define ZeroMemory RtlZeroMemory

#define TIMESHIFT_FILENAME "history_ccc_timeshift.dat"

enum
{
	YEAR = 0,
	MONTH,
	DAY,
	HOUR,
	MINUTE,
	SECOND
};


#define TIME_TO_BYTES( timeUpdate, pTime )     \
{                                              \
    pTime[YEAR] = ( timeUpdate ).tm_year - 100;\
    pTime[MONTH] = ( timeUpdate ).tm_mon + 1;  \
    pTime[DAY] = ( timeUpdate ).tm_mday;       \
    pTime[HOUR] = ( timeUpdate ).tm_hour;      \
    pTime[MINUTE] = ( timeUpdate ).tm_min;     \
    pTime[SECOND] = ( timeUpdate ).tm_sec;     \
}

#define BYTES_TO_TIME( pByteData, timeUpdate )      \
{                                                   	\
    ( timeUpdate ).tm_year  = (pByteData[YEAR] + 100) %256; \
    ( timeUpdate ).tm_mon   = ( int )pByteData[MONTH] - 1; \
    ( timeUpdate ).tm_mday  = ( int )pByteData[DAY];       \
    ( timeUpdate ).tm_hour  = ( int )pByteData[HOUR];      \
    ( timeUpdate ).tm_min   = ( int )pByteData[MINUTE];    \
    ( timeUpdate ).tm_sec   = ( int )pByteData[SECOND];    \
}


///HM - CC module specific
#define READ_HMDATA_COMMAND		0x07
#define GET_HMDATA_COMMAND		0x08
#define DEL_HMDATA_COMMAND		0x09
inline const char * HmdataCommandText( int _cmd_ )
{
switch (_cmd_ )
    {
        case READ_HMDATA_COMMAND:   return "READ";
        case GET_HMDATA_COMMAND:    return "GET";
        case DEL_HMDATA_COMMAND:    return "DEL";
        default: return "<unknown>";
    }
}

#define LOG_TIME( _szMsg_, _timeStart_ )                \
LOG( _szMsg_,                                                               \
2000+( int )_timeStart_[0], ( int )_timeStart_[1], ( int )_timeStart_[2],   \
     ( int )_timeStart_[3], ( int )_timeStart_[4], ( int )_timeStart_[5] );   \

#define LOG_TIME_INTERVAL( _szMsg_, _timeStart_, _timeEnd_ )                \
{                                                                           \
LOG( _szMsg_,                                                               \
2000+( int )_timeStart_[0], ( int )_timeStart_[1], ( int )_timeStart_[2],   \
     ( int )_timeStart_[3], ( int )_timeStart_[4], ( int )_timeStart_[5],   \
2000+( int )_timeEnd_[0],   ( int )_timeEnd_[1],   ( int )_timeEnd_[2],     \
     ( int )_timeEnd_[3],   ( int )_timeEnd_[4],   ( int )_timeEnd_[5] );   \
}


#ifndef _Max
#define _Max( x, y ) ( ( x ) > ( y ) ? x : y )
#endif

#ifndef _Min
#define _Min( x, y ) ( ( x ) < ( y ) ? x : y )
#endif


static inline void delay( int sec, int nsec = 0 )
{
//     struct timespec tv = {
//         tv_sec          : sec,
//         tv_nsec         : nsec,
//     };

    // Change initialization to stop generation of Error 40 by lint
    struct timespec tv;
    tv.tv_sec = sec;
    tv.tv_nsec = nsec;

    while( nanosleep(&tv, &tv) )
    {
        if( errno != EINTR )
        {
            //LOG_ERR("delay: nanosleep ERROR");
            break;
        }
    }
}

const char* GetSourceAsString(int p_nSource);

//execute shell command with timeout and variable number of parameters
int systemf_to(int timeout, const char *cmd,...);

//execute shell command with timeout
int system_to( int timeout, const char *cmd );

//execute connect with timeout
int connect_to(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen, int timeout);

//get free space (bytes) on the partition containing the file given as parameter
// valid only until free space is aroung 2 GB.
unsigned long GetFreeFlashSpace( const char * p_szFile );




//////////////////////////////////////////////////////////////////////////////
/// @brief       Copy file sections from roff to woff. Sections can overlap.
/// @author      Marius Negreanu
/// @param [in]  p_fhFile   File handle.
/// @param [in]  woff       Destination file offset.
/// @param [in]  roff       Source file offset.
/// @param [in]  n          Bytes to copy from roff to woff. If n=0 then will
///                         read until EOF.
/// @retval true            All n bytes were copied.
/// @retval false           Operation failed.
//////////////////////////////////////////////////////////////////////////////
bool fbmove( FILE* p_fhFile, off_t woff, off_t roff, size_t n ) ;

void ShiftTime( char* pTime, time_t timeShift );
int	 ShiftTimeForEvent( int p_nMsgType, unsigned char* p_pData, time_t p_nTime );

#define FLAG( i ) ( 1 << i )


// The following defines are related to user interface
// Variables will be exported to a file that is in
// ini format
#define SHARED_VAR_FILE			NIVIS_TMP"RMP_Shared.txt"
#define SHARED_VAR_FILE_FLASH	ACTIVITY_DATA_PATH"RMP_Shared.txt"
#define SHARED_VAR_GROUP "SHARED"

#define VAR_RF_LEVEL     "RF_LEVEL"
#define VAR_RF_COUNT     "RF_COUNT"
#define VAR_RF_STATUS    "RF_STATUS"
#define VAR_SENDQ_COUNT  "SENDQ_COUNT"
#define VAR_SENDQ_BYTES  "SENDQ_BYTES"
#define VAR_HIST_BYTES   "HIST_BYTES"
#define VAR_HIST_UNSENT  "HIST_UNSENT"
#define VAR_HIST_MSGS    "HIST_MSGS"
#define VAR_CONN_STATUS  "CONN_STATUS"
#define VAR_TRANS_TIME   "TRANS_TIME"

#define MAX_VALUE_SIZE 64

void ExportVariable( const char* filename, const char* group, const char* varname, int value);
void ExportVariable( const char* filename, const char* group, const char* varname, const char* value);

void ImportVariable( const char* filename, const char* group, const char* varname, int* value);
void ImportVariable( const char* filename, const char* group, const char* varname, char** value);

bool Creat( const char * p_szFilename );
bool FileIsExecutable( const char * p_szFilename );
int		FileExist (const char * p_szFile);
int		AttachDatetime( const char* p_szName, char* p_szResult = NULL ) ;

void alrm_handler(int) ;

#ifdef CYG
inline int getline(char**, size_t*, FILE* ) { return 0; }
#endif

#endif //COMMON_H
