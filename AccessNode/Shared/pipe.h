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
                          pipe.h  -  description
                           -------------------
    begin                : Fri Apr 12 2002
    email                : marcel.ionescu@nivis.com
 ***************************************************************************/

//lint -library

#ifndef _PIPE_H_
#define _PIPE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>


#include "Common.h"

// Structures and Classes defined in this file
//   THeaderToDNC
//   THeaderToCC
//   THeaderToRA
//   THeaderSim
//   TRFToCCMsgHeader
//   TCCToRFMsgHeader
//   THeaderHMCC
//   TTimeInterval
//   TCCToHMData
//   THMCCMsg
//   THeaderToCommMgr
//   CPipe

class CPipe
{
public:

class THeader
{
public:
    int       m_nLength;
} __attribute__((packed));


class THeaderToDNC
{
public:
    int			    m_nLength;
    short 			m_nFrom;
	short			m_nMsgType;	
    int			    m_nMsgId;
    short           m_nAppId;
	unsigned char   m_cAnsPath;	

	union
	{   rf_address	    m_stNodeId;
		unsigned int	m_nNodeId;		
	};
  
} __attribute__((packed));

class THeaderToCC
{
public:
    int       m_nLength;
    short	  m_nFrom;
	short	  m_nMsgType;			
    int       m_nMsgId;
    short     m_nAppId;
} __attribute__((packed));

class THeaderToRA
{
public:
    int       m_nLength;
    short	  m_nFrom;
	short	  m_nMsgType;			    
    int       m_nMsgId;
    short     m_nAppId;
}__attribute__((packed));

typedef THeaderToRA THeaderToEmailSender;
typedef THeaderToRA THeaderFromEmailSender;

class THeaderToCommMgr
{
public:
    int       m_nLength;
	short	  m_nFrom;	
	short	  m_shMsgType;
    int       m_nMsgId;
    short     m_shAppId;
} __attribute__((packed));


class THeaderSim
{
public:
    int			    m_nLength;
    unsigned char   m_cCmdCode;
} __attribute__((packed));

enum
{
	HM_EVENT,
	HM_RESPONSE
};


class THeaderRFCC
{
public:
    int       m_nLength;
    short	  m_nFrom;
    int       m_nMsgId;
    short     m_nAppId;
} __attribute__((packed));


typedef struct //_tRFToCCMsgHeader
{
	unsigned short	shMessageVersion;
	unsigned short	shMessageType;
	unsigned int	unANID;

}TRFToCCMsgHeader, *LPTRFToCCMsgHeader;

typedef struct _tCCToRFMsgHeader
{
	unsigned short	shMessageVersion;
	unsigned short	shMessageType;
	unsigned int	unANID;

}TCCToRFMsgHeader, *LPTCCToRFMsgHeader;


class DNCAnswDetails
{
public:
	unsigned short  m_nCommand;
	unsigned int    m_nErrorCode;
	union
	{	rf_address      m_stNodeId;
		unsigned int	m_nNodeId;
	};
	unsigned char   m_cLastCommandId;
	unsigned char   m_nComandSuccessfullFlag;
}__attribute__((packed));



class THeaderDNAnswerToCC
{
public:
	THeaderToCC    m_stHeader;
	DNCAnswDetails m_stAnswer;
} __attribute__((packed));


///////////////////////////////////////////////////////////////////////////
//
// Messages Exchanged between CC and HM
//
///////////////////////////////////////////////////////////////////////////

// This is the header for messages passed between the 
// HM and CC_COMM
//
// HdrLength = number of byte in the message, including header but excluding this byte
// shType = HM_EVENT | HM_RESPONSE
// 
// for HM_EVENT, msgID = 0xFFFC
//
struct THeaderHMCC
{
public:
	int				nHdrLength;	
    short			shType;
	short			shAppID;
	unsigned int	unMsgID;
}__attribute__ ((packed));

struct TTimeInterval
{
	char	startTime[6];
	char	endTime[6];
}__attribute__ ((packed));;

struct THmMsgInfo
{
	unsigned int	m_unStartPos;
	unsigned int	m_unLen;
}__attribute__ ((packed));;

//
// unCmd - can be one of the following
//   DEL_HMDATA_COMMAND
//   DEL_HMDATA_COMMAND | FLAG(16) - Delete data and send more if available
//   READ_HMDATA_COMMAND 
//   GETHMDATA_COMMAND
//
struct TCCToHMData
{
	unsigned int	unCmd;
	unsigned int	unErrCode;
	union
	{
		THmMsgInfo		m_unHmMsgInfo;
		TTimeInterval	timeInterval;
		time_t			timeShift;
	};
}__attribute__ ((packed));


//
// A message passed between history module and cc_comm
// Is made up of THeaderHMCC and TCCToHMData
//
struct THMCCMsg : public THeaderHMCC
{
	TCCToHMData		hmCustomHMCC;
}__attribute__ ((packed));


///////////////////////////////////////////////////////////////////////////
//
// End Messages Exchanged between HM and CC
//
///////////////////////////////////////////////////////////////////////////




public:
        CPipe();
        ~CPipe();

        int  Open( const char * p_sPipeName = "/tmp/pipe", int  /*p_nFlag*/ = O_TRUNC | O_RDWR,
                            bool p_bLock = true );
        void Close( );

        int  GetFd() const { return m_nFd; } ;
        int  HaveMsg( int p_nTime = 10 );

        int  WriteMsg( const void * p_pHeader, int m_nHeaderSize, const char *  p_pData , int p_nDataLen);
        int  ReadMsg( void * p_pHeader, int m_nHeaderSize, const char **  p_ppData , int * p_nDataLen);
		
		//release internal buffer, after use. This is needed because data size can be pretty large
		//don't close the pipe; all operation will continue normally after this call; just don't use data pointer, now invalid
		//TODO: check this can be safely called by HaveMsg
		void ReleaseBuffer( void );

private:
        int  writeBlock( const char *  p_pBlock, int p_nBlockLen );
        int  readBlock( char * p_pBlock, int p_nBlockLen );

        void lock() { if(m_bLock)   flock( m_nFd, LOCK_EX ); }
        void unlock() { if(m_bLock)   flock( m_nFd, LOCK_UN ); }

		int reset( void );
        int    m_nFd;
        char * m_pBuffer;
        bool   m_bLock;
		char	m_szPipeName[ 256 ];

			
protected:
	int readStartMarker();
};

#endif //PIPE_H
