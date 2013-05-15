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

/********************************************************************
	created:	2004/10/04
	created:	4:10:2004   15:20
	filename: 	C:\home\AccessNode\Shared\Utils.cpp
	file path:	C:\home\AccessNode\Shared
	file base:	Utils
	file ext:	cpp
	author:		Claudiu Hobeanu

	purpose:
*********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/vfs.h>

#ifndef CYG
#include <sys/sysinfo.h>
#endif

#include "Utils.h"
#include "Common.h"
#include "Socket.h"

// we don't need to access the pidfiles at every function call, only once every PIDFILES_TIMEOUT seconds
#define PIDFILES_TIMEOUT 60
// only delete the pidfiles once every PIDFILES_TIMEOUT * PIDFILES_FACTOR seconds.
#define PIDFILES_FACTOR 15


int GetBinData( const char* p_szInput, char* p_pData, int* p_pLen )
{
    switch ( p_szInput[0] )
    {
    case 's' :
    case 'S':
		*p_pLen = sprintf(p_pData, p_szInput+2) + 1;
        //strcpy( p_pData, p_szInput + 2 );
        //LOG("Sz data: %s", p_pData );
        //*p_pLen = strlen(p_pData)+1;
        break;
    case 'x':
    case 'X':
        {
            *p_pLen = Hex2Bin( p_szInput+2, p_pData );
            break;
        }
    }

    return 1;
}

//	exec single shell cmd like 'ps' ( not $ps;ls ) and redirect output to file p_szFile
//  echo  '$ps' 1>>tmp.file 2>>tmp.file; ps 1>>tmp.file 2>>tmp.file;
int ExecSimpleBash2File( const char* p_szCmd, const char* p_szFile )
{
	return ExecSimpleBash2File(p_szCmd, strlen(p_szCmd), p_szFile );
}

int ExecSimpleBash2File( const char* p_pCmd, int p_nLen, const char* p_szFile )
{
	char	szBuff[1024];

	sprintf( szBuff, "echo -e '\n$%.*s' >>%s; %.*s 1>>%s 2>>%s",
		p_nLen, p_pCmd, p_szFile, p_nLen, p_pCmd, p_szFile, p_szFile );
//	LOG("ExecBash: %s", szBuff );
	return system_to( 600, szBuff ) == 0;
}

// exec multiple shell cmd like $ps;ls
int ExecBash2File( const char* p_szCmd, const char* p_szFile )
{
	return ExecBash2File(p_szCmd, strlen(p_szCmd), p_szFile);
}

int ExecBash2File( const char* p_pCmd, int p_nLen, const char* p_szFile )
{
	int ret = 0;
	int nPos = 0;

	while( (nPos < p_nLen) && p_pCmd[nPos] )
	{
		int nPosDelim = NextToken( p_pCmd + nPos, p_nLen - nPos );
		if( nPosDelim == 0 )
		{	nPos++;
			continue;
		}

		ret |= ExecSimpleBash2File(p_pCmd + nPos, nPosDelim, p_szFile );

		nPos += nPosDelim + 1;
	}
	return ret;
}


// returns no of Kbytes of free memory of a type (types: "MemFree:", "Buffers:", "Cached:" )
// 0 if nothing found
int	GetFreeMemFor( const char* p_szMemInfo, const char* pNameType )
{
	const char *p = strstr(p_szMemInfo, pNameType) ;
	if( !p )
	{	return 0;
	}

	p += strlen(pNameType);

	return atoi(p);
}


int FileRead(const char* p_szFile, char* p_pBuff, int p_nBuffLen )
{
	int fd = open( p_szFile,  O_RDONLY);

	if (fd < 0)
	{	LOG_ERR( "FileRead: error at opening file %s",  p_szFile);
		return 0;
	}


	int nLen = read(fd, p_pBuff, p_nBuffLen);

	if (nLen<=0)
	{
		LOG_ERR( "FileRead: error at reading from file %s",  p_szFile);
	}
	close(fd);
	return nLen;
}

//return sys uptime in seconds
//	on error return <0
float SysGetUpTime()
{
	float dTime;

	char szData[128];

	int nLen = FileRead("/proc/uptime", szData, sizeof(szData) - 1);

	if (nLen<=0)
	{	return -1;
	}

	szData[nLen] = 0;

	if (sscanf(szData, "%f",&dTime) <= 0)
	{
		return -1;
	}

	return dTime;
}

// returns no of bytes of free memory  conform to sysinfo call
// return 0 if error occured
//we need /proc/meminfo for cached, which is not reported by sysinfo
int GetSysFreeMem( void )
{
#ifdef CYG
	return 0;
#else

	char szData[1024];

	int nLen = FileRead("/proc/meminfo", szData, sizeof(szData) - 1);

	if (nLen<=0)
	{	return -1;
	}
	szData[nLen] = 0;

	struct sysinfo inf;
	if ( sysinfo( &inf) )
	{
		LOG_ERR("ERROR GetSysFreeMem");
		inf.freeram=0;
	}

	struct statfs buf;
	long long nTmpUsed = 0;
	if( statfs( "/tmp", &buf)) {
		LOG_ERR( "GetSysFreeMem: statfs( \"tmp\") failed");
	}else{
		nTmpUsed= (buf.f_blocks-buf.f_bavail) * (long long) buf.f_bsize;
	}

	return inf.freeram * inf.mem_unit +
		( GetFreeMemFor( szData, "Buffers:") + GetFreeMemFor( szData, "Cached:")) * 1024 - nTmpUsed;

#endif
}

//return free memory, expressed in bytes
int GetSysFreeMemK( void )
{
	return GetSysFreeMem()/1024;
}

//return true if a memory low condition is detected
//TAKE CARE: this detection is faulty. Use this only as a warning
//the requested memory is expressed in kB
bool IsSysMemoryLow( int p_nKBRequested )
{
	if( GetSysFreeMemK() < (p_nKBRequested + FREE_MEM_MIN_REQ))
	{
		LOG("ERROR MEMORY LOW, possible failure (%d < %d + %d kB)",
			GetSysFreeMemK(), p_nKBRequested, FREE_MEM_MIN_REQ );
		return true;
	}

	return false;
}

//convert hex string in binary data
//suppose that p_pOutput has allocated (strlen(p_szInput)+2)/2
//return the len of the bin buffer
int Hex2Bin( const char* p_szInput, char* p_pOutput )
{
	return Hex2Bin(p_szInput, strlen(p_szInput), p_pOutput );
}

int Hex2Bin( const char* p_pInput, int n, char* p_pOutput )
{
	int i,j =0;

	for( i = 0; i < n; j++ )
	{	
		while ( i < n && ( isspace(p_pInput[i]) || p_pInput[i] == ':' ) )
		{
			i++;
		}

		if (i>=n)
		{
			break;
		}
		
		int nCrt = 0;
		int nValue;
		if (sscanf( p_pInput +i,"%2x%n", &nValue, &nCrt ) < 1 )
		{	break;
		}
		p_pOutput[j] = (char)nValue;
		i += nCrt;

	}
	return j;
}

//////////////////////////////////////////////////////////////////////////////////
// Description : compute no. of p_cDelim char in p_pData in first p_nLen bytes
//				or when it was encounter '\0'
// Parameters  :
//				const char *  p_pData      	-  	input   the data block
//				int			p_nLen			-	input
//				char		p_cDelim		- 	input	char to look for
// Return      :
//				return no. of chars p_cDelim
//////////////////////////////////////////////////////////////////////////////////
int NoOfDelim( const char* p_pData, const int p_nLen, const char p_cDelim )
{
	if( !p_pData )
		return -1;

	int nCount = 0;
	for( int i =0; i < p_nLen && p_pData[i]; i++ )
	{	if( p_pData[i] == p_cDelim )
			nCount++;
	}
	return nCount;
}


//////////////////////////////////////////////////////////////////////////////////
// Description : compute the position of first p_cDelim or of end of data in p_pData in first p_nLen bytes
//				or when it was encounter '\0'
// Parameters  :
//				const char * 	 p_pData      	-  	input   the data block
//				const int		p_nLen			-	input
//				const char		p_cDelim		- 	input	char to look for
// Return      :
//				return no. of chars p_cDelim
//////////////////////////////////////////////////////////////////////////////////
int NextToken( const char* p_pData, const int p_nLen, const char p_cDelim )
{
	if( !p_pData )
		return -1;

	int nPos = 0;

	for( int i = 0; i < p_nLen && p_pData[i]; i++ )
	{
		if( p_pData[i] == p_cDelim )
			return nPos;

		nPos++;
	}
	return nPos;
}


//////////////////////////////////////////////////////////////////////////////////
// Description : replace in string all space characters(' ','\n','\r','\f','\t','\v') with p_cChar character
// Parameters  :
//				char *  p_pString		      	-  input,output   string wich will be modified
//				const char p_cChar					-  char that will be used for replace
// Return      :
//				none
//////////////////////////////////////////////////////////////////////////////////
void ReplaceSpaces( char* p_pString, const char /*p_cChar*/ )
{
	while( *p_pString )
	{	if( isspace(*p_pString) )
			*p_pString = ' ';
		p_pString++;
	}
}



int UpdateLastTransmissionTime()
{
	time_t tlastTransmissionTime = ::time( NULL );

	if( ((time_t)-1) == tlastTransmissionTime )
	{	LOG_ERR( "UpdateLastTransmissionTime: TIME routine failed" );
		return 0;
	}

//	LOG( "update last transmission time (for UI)" );

	struct tm * pTransmissionTime = ::gmtime( &tlastTransmissionTime );
	char pTime[6];
	TIME_TO_BYTES( *pTransmissionTime, pTime );

    char szTime[128];
	sprintf( szTime, "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2dUTC",
             pTime[1], pTime[2], pTime[0], pTime[3], pTime[4], pTime[5] );

    ::ExportVariable( SHARED_VAR_FILE_FLASH, SHARED_VAR_GROUP, VAR_TRANS_TIME, szTime );

	return 0;
}


// copy src file to dest file in p_nStep bytes chunks
// always creats dest file ( needed in saving mesh net)
int Copy( const char* p_szSrc, const char* p_szDest, int p_nStep /*= 4096*/ )
{
	int nFdSrc, nFdDest;

	nFdDest = open( p_szDest, O_CREAT | O_WRONLY | O_TRUNC,  0666 );
	if( nFdDest < 0 )
	{	LOG_ERR( "Copy: can't creat file %s", p_szDest );
		return 0;
	}

	nFdSrc = open( p_szSrc,  O_RDONLY);
	if( nFdSrc < 0 )
	{	LOG_ERR( "Copy: can't open file %s", p_szSrc );
		close(nFdDest);
		return 0;
	}

	char* pBuff = new char[p_nStep]; //throws exceptions if error
	int nCount = 0;

	for(;;)
	{	nCount = read( nFdSrc, pBuff, p_nStep );

		if ( nCount < 0 )
		{	LOG_ERR( "Copy: can't read from file %s", p_szSrc );
			break;
		}

		if (nCount == 0)
		{	break;
		}

		nCount = write( nFdDest, pBuff, nCount );
		if ( nCount < 0 )
		{	LOG_ERR( "Copy: can't write to file %s", p_szDest );
			break;
		}
	}

	delete []pBuff;
	close(nFdDest);
	close(nFdSrc);
	return nCount >= 0;
}


//computes CRC for only one byte, based on the previous one.
unsigned short crc( unsigned short addedByte,
                                  unsigned short genpoli,
                                  unsigned short current )
{
    register int i;

    addedByte <<= 8;
    for (i=8; i>0; i--){
        if ((addedByte^current) & 0x8000)
            current = (current << 1) ^ genpoli;
        else
            current <<= 1;
        addedByte <<= 1;
    }
    return current;
}

//'escapes' special chars: STX, ETX, CHX which might appear inside data
//ie, send a CHX and then 0xFF - char instead of char
//in case of normal char, send it unchanged
void EscapeChar(  const u_char * p_psPacket, u_int* p_pnIndexPacket,
                  u_char       * p_psBuffer, u_int* p_pnIndexBuffer )
{
    switch( p_psPacket[*p_pnIndexPacket] ) {
        case STX:
        case ETX:
        case CHX:
            p_psBuffer[ (*p_pnIndexBuffer)++ ] = CHX;
            p_psBuffer[ (*p_pnIndexBuffer)++ ] = 0xFF - p_psPacket[ (*p_pnIndexPacket)++ ];
            break;
        default :
            p_psBuffer[ (*p_pnIndexBuffer)++ ] = p_psPacket[ (*p_pnIndexPacket)++ ];
    }
}



//computes CRC for packet and return the result in crc field
int ComputeCRC( const u_char * p_pPacket, u_int p_nPacketLength, u_char p_pCrc[CRC_LENGTH], unsigned short p_usCrcStart )
{
    unsigned int i;
    unsigned short current = p_usCrcStart;
    unsigned short *pCrc = (unsigned short*)p_pCrc;

    for( i = 0; i < p_nPacketLength; i++ ){
        current = crc( (unsigned short)p_pPacket[i], CRCCCITT, current );
    }
    *pCrc = htons(current);
    for( i=0; i<2; i++ )
        switch( p_pCrc[ i ] ) {
            case STX:
            case ETX:
            case CHX:
                p_pCrc[ i ] = 0xFF - p_pCrc[ i ];
             //   LOG( "computeCRC: special char %x Substracted from 0xFF", p_pCrc[ i ]);
        }
    return 1;
}


int EscapePacket(u_char *p_pSrc, int p_nSrcLen, u_char* p_pDest, int* p_pDestLen )
{
	if (p_nSrcLen > *p_pDestLen - 2 )
	{	LOG( "CNivisPacket::EscapePacket: dest buffer too small" );
		return 0;
	}
	int i = 0, j = 1;

    while( i < p_nSrcLen )
    {   EscapeChar(p_pSrc, (unsigned int*)&i, p_pDest, (unsigned int*)&j );
		if (j >= *p_pDestLen)
		{	return 0;
		}
    }

	if ( *p_pDestLen <= j )
	{	LOG( "CNivisPacket::EscapePacket: dest buffer too small" );
		return 0;
	}

	p_pDest[0] = STX;
	p_pDest[j++] = ETX;
	*p_pDestLen = j;

	return 1;
}

int UnescapePacket(u_char *p_pBuff, int *p_pLen)
{
	int i = 1, j = 0;

	for(; i < *p_pLen; i++ )
	{
		if (p_pBuff[i] == ETX)
		{	break;
		}
		switch(p_pBuff[i])
		{
		case STX:
			return 0;
		case CHX:
			p_pBuff[j++] = 0xff - p_pBuff[++i];
			break;
		default:
			p_pBuff[j++] = p_pBuff[i];
		}
	}
	*p_pLen = j;
	return 1;
}



int ExtractBoundedPacket(u_char* p_pRcvBuffer, int* p_pCrtPosition, u_char *p_pBuff, int *p_pLen)
{
	if (!*p_pCrtPosition)
	{	return 0;
	}

    u_char *pStx, *pEtx;

    if( ( pStx = (unsigned char *) memchr( p_pRcvBuffer, STX, *p_pCrtPosition ) ) == NULL )
    {   // STX not found  -> discard all chars
		// log only if something else that 0 is recv
// 		int i;
// 		for( i = 0; i < *p_pCrtPosition && p_pRcvBuffer[i] == 0 ; i++)
// 			;
// 		if ( i < *p_pCrtPosition )
// 		{	//LOG( "ExtractBoundedPacket: STX not found -> discard all chars");
// 		}

        *p_pCrtPosition = 0;
        return 0;
    }

	pEtx = (u_char*)memchr( pStx, ETX, *p_pCrtPosition - (pStx - p_pRcvBuffer));

//	if ( !pEtx && *p_pCrtPosition < MAX_RS232_SIZE *3/4 )  //no etx and plenty of space in read buff, do nothing
//	{	return 0;
//	}

	//keep only the last packet
	pStx = pEtx ? pEtx : p_pRcvBuffer + *p_pCrtPosition -1;
	for( ; *pStx != STX; pStx-- ) ; //exists at least one STX

	int nResult = 0;

	if (pEtx)
	{	if (pEtx-pStx+1 <= *p_pLen)
		{	*p_pLen = pEtx-pStx+1;
			memcpy( p_pBuff, pStx, *p_pLen );
			nResult = 1;
		}
		pStx = pEtx + 1; //on this position is expected a STX
	}

	*p_pCrtPosition -= (pStx - p_pRcvBuffer);
	memmove( p_pRcvBuffer, pStx, *p_pCrtPosition );
	return nResult;
}

///////////////////////////////////////////////////////////////////////////////
// Name:	GetLoadAvg
// Author:	Mihai Buha (mihai.buha@nivis.com)
// Description:	uses sysinfo() to calculate the integer part of the 1-minute
//		load average
// Parameters:	none
// Returns:	the integer part of the first number in `cat /proc/loadavg`
///////////////////////////////////////////////////////////////////////////////
#define FSHIFT          16              /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)
int GetLoadAvg( void )
{
#ifndef CYG
	struct sysinfo info;
	info.loads[0] = 0;
	sysinfo(&info);
	return LOAD_INT( info.loads[0] );
#else
	return 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Name:	TouchPidFile
// Author:	Mihai Buha (mihai.buha@nivis.com)
// Description:	creates a file containing the PID
// Parameters:	p_szName - input - name of the file (ex: modulename.pid)
// Returns:	true if successful, false if failed
///////////////////////////////////////////////////////////////////////////////
bool TouchPidFile( const char* p_szName )
{
	static clock_t last_checked;
	static long n_SC_CLK_TCK;
	clock_t timestamp;
	int nFd;
	static char pid[6];
	bool status = true;
	if(!n_SC_CLK_TCK) n_SC_CLK_TCK = sysconf( _SC_CLK_TCK);

	timestamp = GetClockTicks();
	if( timestamp < last_checked){ // large uptime overflowed the clock_t
		last_checked = timestamp;
	}
	if( timestamp - last_checked < PIDFILES_TIMEOUT * n_SC_CLK_TCK)
	{	return status;
	}
	last_checked = timestamp;

	nFd = open( p_szName, O_CREAT | O_RDWR,  0666 );
	if( nFd < 0 )
	{	LOG_ERR( "TouchPidFile: can't create pidfile %s", p_szName );
		return false;
	}

	if(!pid[0])
	{	snprintf( pid, 5, "%d", getpid() );
	}

// 	if( lseek( nFd, 0, SEEK_SET ) < 0)
// 	{	LOG_ERR( "TouchPidFile: can't write to pidfile %s", p_szName );
// 		status = false;
// 	}

	if( write( nFd, pid, strlen( pid) ) < 0)
	{	LOG_ERR( "TouchPidFile: can't write to pidfile %s", p_szName );
		status = false;
	}

	if( close( nFd))
	{	LOG_ERR( "TouchPidFile: can't close pidfile %s", p_szName );
		status = false;
	}
	return status;
}

///////////////////////////////////////////////////////////////////////////////
// Name:	CheckDelPidFiles
// Author:	Mihai Buha (mihai.buha@nivis.com)
// Description:	removes all the pidfiles specified, but not too frequently
// Parameters:	p_pszNames - input - null-terminated array of filenames
//		p_szMissNames - output - if not null, fill with names of
//			missing files. Take care to allocate enough memory for
//			"Watchdog: <all filenames in p_pszNames> " or else bad
//			things will happen.
// Returns:	true if all pidfiles existed or timeout not expired
//		false if some pidfile was missing (controlling app was dead -
//			did not create a file during latest 2 intervals)
///////////////////////////////////////////////////////////////////////////////
bool CheckDelPidFiles (const char** p_pszNames, char* p_szMissNames)
{
	static clock_t last_checked;
	static long n_SC_CLK_TCK;
	static bool last_existed = true;
	clock_t timestamp;
	bool status = true;
	bool exists = true;
	if(!n_SC_CLK_TCK) n_SC_CLK_TCK = sysconf( _SC_CLK_TCK);

	timestamp = GetClockTicks();
	if( timestamp < last_checked){ // large uptime overflowed the clock_t
		last_checked = timestamp;
	}
	if( timestamp - last_checked < PIDFILES_FACTOR * PIDFILES_TIMEOUT * n_SC_CLK_TCK){
		return status;
	}
	last_checked = timestamp;
	int i;
	for( i=0; p_pszNames[i]; ++i)
	{
		int nFileLen = GetFileLen(p_pszNames[i]);
		if ( nFileLen > 0)
		{	unlink( p_pszNames[i]);
			continue;
		}
		if (nFileLen == 0)
		{	LOG("CheckDelPidFiles: file %s len==0",  p_pszNames[i]);
			unlink( p_pszNames[i]);
		}

		LOG_ERR( "CheckDelPidFiles: pidfile %s missing!", p_pszNames[i]);
		if( exists && p_szMissNames){
			sprintf( p_szMissNames, "Watchdog: ");
		}
		exists = false;
		if( p_szMissNames){
			strcat( p_szMissNames, p_pszNames[i]);
			strcat( p_szMissNames, " ");
		}
		system_to( 60, NIVIS_TMP"take_system_snapshot.sh "ACTIVITY_DATA_PATH"snapshot_warning.txt &");

	}
	status = exists || last_existed;
	last_existed = exists;
	return status;
}

///////////////////////////////////////////////////////////////////////////////
// Name:	GetProxyString
// Description:	creates a string containig ProxyIP ProxyPort
// Parameters:	p_oProxy - input - proxy which will be converted to string
// Returns:	-string containing the proxy
///////////////////////////////////////////////////////////////////////////////
const char* GetProxyString(net_address p_oProxy)
{
	static char szProxyString[50];
	in_addr s;

	s.s_addr = p_oProxy.m_nIP;
	sprintf(szProxyString, "%s:%d",inet_ntoa(s), ntohs(p_oProxy.m_dwPortNo) );

	return szProxyString;

}


////////////////////////////////////////////////////////////////////////////////
// Name:        SetCloseOnExec
// Author:		Claudiu Hobeanu
// Description: wrapper for setting the FD_CLOEXEC flag on a file descriptor
// Parameters:  int fd  -- file descriptor
//
// Return:      success/error
////////////////////////////////////////////////////////////////////////////////
int SetCloseOnExec( int fd )
{
	int flags;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
	{
		LOG_ERR("SetCloseOnExec: fcntl(F_GETFD)");
		return 0;
	}

	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1)
	{
		LOG_ERR("SetCloseOnExec: fcntl(F_SETFD)");
		return 0;
	}

	return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Name:        WriteToFile
// Author:		Claudiu Hobeanu
// Description:
// Parameters:
//
// Return:      success/error
////////////////////////////////////////////////////////////////////////////////
int WriteToFileAtOffset( const char* p_szFile, int p_nOffset, char* p_pBuff, int p_nLen )
{
	int flags = O_RDWR;

	//if writing at start or end creat file not exist
	if (p_nOffset <= 0)
	{	flags |= O_CREAT;
	}

	int fd = open( p_szFile, flags );
	if (fd<0)
	{	LOG_ERR("WriteToFile: opening file %s", p_szFile );
		return 0;
	}

	if (p_nOffset<0)
	{	lseek( fd, 0, SEEK_END );
	}
	else
	{	lseek(fd,p_nOffset,SEEK_SET);
	}

	int ret =  write( fd, p_pBuff, p_nLen );

	if (ret <0)
	{	LOG_ERR("WriteToFile: writing in file %s", p_szFile );
	}
	close(fd);

	return ret >= 0;
}


time_t GetLastModificationOfFile(const char* p_szFile)
{
	struct stat buf;

	if (stat(p_szFile,&buf))
	{
		//LOG_ERR("GetLastModificationOfFile");
		return -1;
	}

	return buf.st_mtime;
}


#define CODE_START_OFFSET 0x2202
#define FLASH_UPDATE	0x21
#define ACTIVATE_FLASH	0x22
#define BROADCAST		0x23

#define MAKE_ON_DEMAND_CMD(p_ucCmd) ( p_ucCmd | 0x80 )
#define IS_ON_DEMAND_CMD(p_ucCmd) ( p_ucCmd & 0x80 )

const unsigned short crcRF[256]={
    	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
		0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
		0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
		0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
		0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
		0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
		0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
		0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
		0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
		0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
		0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
		0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
		0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
		0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
		0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
		0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
		0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
		0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
		0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
		0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
		0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
		0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
		0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};


 const unsigned short  crcAnsi[256]={
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78 };


#define COMPUTE_RF_CRC(nAcc,chBuff)   (((nAcc) << 8) ^ crcRF[((nAcc) >> 8) ^ (chBuff)])
#define COMPUTE_ANSI_CRC(nAcc,chBuff) (((nAcc) >> 8) ^ crcAnsi[((nAcc) ^ (chBuff)) & 0x00ff])

unsigned short crcAnsiBlock(unsigned char * p_pBlock, int p_nBlockLen )
{
	unsigned short nCRC = 0;
	for( ; p_nBlockLen > 0; p_nBlockLen --, p_pBlock++ )
	{
		nCRC = COMPUTE_ANSI_CRC( nCRC, *p_pBlock );

	}
	return htons(nCRC);
}

unsigned short crcRFBlock(unsigned char * p_pBlock, int p_nBlockLen )
{
	unsigned short nCRC = 0;
	for( ; p_nBlockLen > 0; p_nBlockLen --, p_pBlock++ )
	{
		nCRC = COMPUTE_RF_CRC( nCRC, *p_pBlock );

	}
	return htons(nCRC);
}

int hex2bin( char * p_sHex, unsigned char * p_pBin, int p_nBinSize )
{
	int nDataLen = (int)(strlen(p_sHex) / 2);
	if( nDataLen > p_nBinSize )
		return -1;


	for( p_nBinSize = nDataLen; p_nBinSize; p_nBinSize--, p_sHex += 2 )
	{
		int nByte;
		sscanf( p_sHex, "%2X", &nByte );
		*(p_pBin++) = nByte;

	}

	return nDataLen;
}



/////////////////////////////////////////////////////////////////////////////
// Description:
//		Create a stream for upload firmware commands
//
// Note:
//		This function is used by the interface members
//
// Parameters:
//		p_ucCmd		        - command code (broadcast/flashupdate + on-demand)
//		p_bstrMetaHeaders	- The string contains the possible sub-headers (hex format)
//                            (ex: expiration and/or filter in this order if both present)
//		p_bstrFirmwareFile	- The string contains firmware file (intel-standard format)
//		p_shFirmwareTarget	- The firmware target version, 0xFFFF means any software version
//		p_nDNATarget		- The DNA target version, 0xFFFFFFFF means any DNA target
//		p_shWriteFlag		- SenseNode action after download completion
//								0 - just check the flash
//								1 - check the flash and update the flash
//								2 - check the flash, update the flash and restart the SN
//                           if p_shWriteFlag > 0x0100
//                              the code is not downloaded
//                              is executed only the last action
//		p_shMaxPachetLen	- The max packet data (code) size (in bytes)
//		p_pnErrorCode		- result code
//								0 - Success
//								1 - Out of Memory
//								2 - Cannot open tmp file
//								3 - Invalid Firmware file
//								4 - Invalid packet size
//								5 - Invalid filter len or format
//		p_pbstrCommandsStream	-  The result contains the commands stream HEX encoded
//
// Return:
//		none
/////////////////////////////////////////////////////////////////////////////
int MakeStream(unsigned char   p_ucCmd,
						const char*	p_bstrMetaHeaders,
						const char*	p_bstrFirmwareFile,
						unsigned short	p_shFirmwareTarget,
						unsigned int	p_nDNATarget,
						short	p_shWriteFlag,
						short	p_shMaxPacketLen,
						char**	/*p_pbstrCommandsStream*/,
						int *	p_lFirmwareVersion,
						int *	p_lFirmwareBuild,
						int *	p_lFirmwareDNA )
{
	p_nDNATarget = htonl(p_nDNATarget);

	const char* bstrMetaHeaders = p_bstrMetaHeaders ;
	char szTempFile[256];
	//_bstr_t bstrFirmwareFile(p_bstrFirmwareFile);
	FILE* hex;
	FILE* fws;
	//int i;

	unsigned char anPacket[124];
	unsigned char anActivatePacket[sizeof(anPacket)];
	unsigned char m_anMemoryBuffer[MEMORY_SIZE];

	//*p_pbstrCommandsStream = SysAllocString(L"");


	unsigned char aMetaHeaders[128];
	int nMetaHeadersLen = hex2bin( (char*)bstrMetaHeaders, aMetaHeaders, sizeof(aMetaHeaders) );

	/*if( p_shMaxPacketLen+14+2+5+nMetaHeadersLen > sizeof(anPacket) )
	{
		*p_pnErrorCode = 4;
		return;
	}*/

	strcpy(szTempFile, p_bstrFirmwareFile);
	strcat(szTempFile, ".fws");
	hex = fopen(p_bstrFirmwareFile, "r");
	//if(hex != NULL)
	//	printf("file %s was opened ok\n", p_bstrFirmwareFile);
	fws = fopen(szTempFile, "w+");
	if( hex == NULL || fws == NULL )
	{
		printf("cannot open source or dest file\n");
		return 2;
	}

/*	fread(szFileData, 1, p_shMaxPacketLen, hex);
	fwrite(p_bstrFirmwareFile, sizeof(char), strlen(p_bstrFirmwareFile), hex);
	rewind(hex);*/


	// init
	memset(m_anMemoryBuffer, 0xFF, sizeof(m_anMemoryBuffer));

	int kActivate = 0;
	*(unsigned short*)(anActivatePacket+kActivate) = htons(p_shFirmwareTarget);
	kActivate += sizeof(p_shFirmwareTarget);
	anActivatePacket[kActivate++] = (unsigned char)(p_shWriteFlag & 0xFF);

	bool bWritten = true;
	int nLastOffset = CODE_START_OFFSET;
	int nLastContiguous = CODE_START_OFFSET;
    unsigned char ucCmd = 0;
	unsigned short nCmdLen = 0;

	int k = 0;
	*(unsigned short*)(anPacket+k) = htons(p_shFirmwareTarget);
	k += sizeof(p_shFirmwareTarget);
	*(unsigned short*)(anPacket+k) = htons((unsigned short)nLastOffset);
	k += sizeof(short);

	// firmware
	char string[100];
	unsigned char anData[50];

	*p_lFirmwareVersion = 0;
    *p_lFirmwareBuild = 0;
	*p_lFirmwareDNA = 0;

	//fgets(string, sizeof(string), hex);
	//printf("string read is %s\n",string);

	while( fgets(string, sizeof(string), hex)  )
	{
		switch( string[0] )
		{
		case ':':
			{
				if( hex2bin( string+1, anData, sizeof(anData) ) < 0 )
				{
					// Invalid Firmware file
					printf("invalid firm file\n");
					return 3;
				}
				//printf("andata is %s\n",anData);

				if( anData[3]  ) // must be 0
					break;

                if(anData[1] == 0xFC )
				{
					switch(  anData[2] )
					{
					case 0x00: // REG 01 (DNA)
//:20 FC00 00 FFFFFFFF FFFFF811 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF9
						*p_lFirmwareDNA = (((unsigned long)anData[10]) << 8 ) + anData[11];
                        break;

					case 0x20: // REG 0E (VERSION NO)
//:20 FC20 00 FFFFFFFF FFFFFFFF 00010A32 00000708 00000005 00000003 01000003 FFFFFFFF78
						((char*)p_lFirmwareBuild)[0] = anData[31];
						((char*)p_lFirmwareBuild)[1] = anData[30];
						((char*)p_lFirmwareVersion)[0] = anData[29];
						((char*)p_lFirmwareVersion)[1] = anData[28];

						break;
					}

				}

				int nNoOfBytes = anData[0];
				int nAddress = 0;
				((char*)&nAddress)[1] = anData[1];
				((char*)&nAddress)[0] = anData[2];
				//nAddress -= OFFSET;

				// validity check
				if( nAddress < 0 || nAddress > MEMORY_SIZE )
				{
					// Invalid Firmware file
					return 3;
				}

                // ignore lines before CODE_START_OFFSET
                if ( nAddress < CODE_START_OFFSET )
                    break;

				memcpy(m_anMemoryBuffer+nAddress, anData+4, nNoOfBytes);
				if( (k + nNoOfBytes) > p_shMaxPacketLen || (nAddress != nLastOffset) )
				{
                    if( p_shWriteFlag < 0x0100 ) // upload FW required
                    {
						if( p_ucCmd == MAKE_ON_DEMAND_CMD(BROADCAST) )
						{
							nCmdLen = htons( (unsigned short)(k + 1 + 5) + nMetaHeadersLen );
							fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
							fwrite(&p_ucCmd, sizeof(p_ucCmd), 1, fws);
							fwrite(&p_nDNATarget, sizeof(p_nDNATarget), 1, fws);
							if( nMetaHeadersLen )
							{
								fwrite(aMetaHeaders, nMetaHeadersLen, 1, fws);
							}
                            ucCmd = FLASH_UPDATE;
						}
						else
						{
							nCmdLen = htons( (unsigned short)(k + 1) );
							fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
                            ucCmd = p_ucCmd;
						}
						fwrite(&ucCmd, sizeof(ucCmd), 1, fws);
						fwrite(anPacket, 1, k, fws);
                    }
					bWritten = true;

					// put CRC into activate packet
					if( nAddress != nLastOffset )
					{
						if( kActivate < p_shMaxPacketLen )
						{
							// dst offset
							// dst offset becomes ANSI crc
							*(unsigned short*)(anActivatePacket+kActivate) = crcAnsiBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous );
							kActivate += sizeof(short);
							// src offset
							*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)nLastContiguous);
							kActivate += sizeof(short);
							// src len
							*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)(nLastOffset-nLastContiguous));
							kActivate += sizeof(short);
							// crc
							*(unsigned short*)(anActivatePacket+kActivate) = crcRFBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous );
							kActivate += sizeof(short);
						}
						else
						{
							// clean
							// invalid max packet len
							return 4;
						}
						nLastContiguous = nAddress;
					}

					nLastOffset = nAddress;
					k = 0;
					*(unsigned short*)(anPacket+k) = htons(p_shFirmwareTarget);
					k += sizeof(p_shFirmwareTarget);
					*(unsigned short*)(anPacket+k) = htons((unsigned short)nLastOffset);
					k += sizeof(short);
				}
				memcpy(anPacket+k, anData+4, nNoOfBytes);
				bWritten = false;
				k += nNoOfBytes;
				nLastOffset += nNoOfBytes;
				//printf("offset %d\n",nLastOffset);
				break;
			}
		case '\r':
		case '\n':
		case '\0':
				break;
		default :
				printf("default values this means error.exiting\n");
				return 3;
		}
	}

	//printf("offset %d\n",nLastOffset);
	if( nLastOffset == CODE_START_OFFSET ) // empty file
	{
		// Invalid Firmware file
		printf("invalid file\n");
		return 3;
	}

    if( !bWritten && p_shWriteFlag < 0x0100) { // upload FW required
        if( p_ucCmd == MAKE_ON_DEMAND_CMD(BROADCAST) )
        {
			nCmdLen = htons( (unsigned short)( k + 1 + 5 ) + nMetaHeadersLen );
			fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
			fwrite(&p_ucCmd, sizeof(p_ucCmd), 1, fws);
			fwrite(&p_nDNATarget, sizeof(p_nDNATarget), 1, fws);
			if( nMetaHeadersLen )
			{
				fwrite(aMetaHeaders, nMetaHeadersLen, 1, fws);
			}
            ucCmd = FLASH_UPDATE;
		}
		else
        {
			nCmdLen = htons( (unsigned short)(k+1) );
			fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
            ucCmd = p_ucCmd;
		}
		fwrite(&ucCmd, sizeof(ucCmd), 1, fws);
		fwrite(anPacket, k, 1, fws);
	}

	if( kActivate >= p_shMaxPacketLen )
	{
		// invalid max packet len
		return 4;
	}

	// dst offset becomes ANSI crc
	*(unsigned short*)(anActivatePacket+kActivate) = crcAnsiBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous);
	kActivate += sizeof(short);

	// src offset
	*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)nLastContiguous);
	kActivate += sizeof(short);

	// src len
	*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)(nLastOffset-nLastContiguous));
	kActivate += sizeof(short);

	// crc
	*(unsigned short*)(anActivatePacket+kActivate) = crcRFBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous );
	kActivate += sizeof(short);

	// write activate
	if( p_ucCmd == MAKE_ON_DEMAND_CMD(BROADCAST) )
	{
		nCmdLen = htons( (unsigned short)(kActivate + 5 + 1) + nMetaHeadersLen );
		fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
		fwrite(&p_ucCmd, sizeof(p_ucCmd), 1, fws);
		fwrite(&p_nDNATarget, sizeof(p_nDNATarget), 1, fws);
		if( nMetaHeadersLen )
		{
			fwrite(aMetaHeaders, nMetaHeadersLen, 1, fws);
		}
        ucCmd = ACTIVATE_FLASH;
	}
	else
    {
		nCmdLen = htons( (unsigned short)(kActivate + 1) );
		fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
        ucCmd = ( IS_ON_DEMAND_CMD(p_ucCmd) ? MAKE_ON_DEMAND_CMD(ACTIVATE_FLASH) : ACTIVATE_FLASH );
	}
	fwrite(&ucCmd, sizeof(ucCmd), 1, fws);
	fwrite(anActivatePacket, kActivate, 1, fws);

	/*rewind(fws);
	long nFileLength = filelength(fileno(fws));
	char* pwchOutputStream = new char[2*nFileLength+1];
	if( pwchOutputStream == NULL )
	{
		*p_pnErrorCode = 1;
		return;
	}
	pwchOutputStream[nFileLength] = 0;
	for(i = 0; i < nFileLength; i++) {
		unsigned char byte;
		fread(&byte, sizeof(byte), 1, fws);
		sprintf(pwchOutputStream+2*i, L"%02X", (unsigned int)byte);
	}
	//SysFreeString( *p_pbstrCommandsStream );
	// *p_pbstrCommandsStream = SysAllocString(pwchOutputStream);
	delete pwchOutputStream;*/
	fclose(hex);
	fclose(fws);
	return 0;
}


#define MEMORY_SIZE_900 0xEE00
#define OFFSET 0x1200
int MakeStream900(	bool	p_bBroadcast,
								char*	p_bstrFilter,
								char*	p_bstrFirmwareFile,
								unsigned short	p_shFirmwareTarget,
								unsigned int	p_nDNATarget,
								short	p_shWriteFlag,
								short	p_shMaxPacketLen,
								char**	/*p_pbstrCommandsStream*/,
								int*	p_lFirmwareVersion,
								int*	p_lFirmwareBuild,
								int*	p_lFirmwareDNA )
{
	p_nDNATarget = htonl(p_nDNATarget);

	char* bstrFiler =  p_bstrFilter;
	//int i;
	FILE* hex;
	FILE* fws;
	char szTempFile[128];

	unsigned char anPacket[256];
	unsigned char anActivatePacket[sizeof(anPacket)];
	unsigned char m_anMemoryBuffer[MEMORY_SIZE_900];


	unsigned char aFilter[128];
	int nFilterLen = hex2bin( bstrFiler, aFilter, sizeof(aFilter) );

	/*if( nFilterLen ) // have filter
	{
		// check for invalid filter len or format
		if( nFilterLen < 2
			|| aFilter[0] != 0x24
			|| (int)(aFilter[1])*3 + 2 != nFilterLen )
		{
			*p_pnErrorCode = 5;
			return 5;
		}
	}*/

	if( p_shMaxPacketLen+25+nFilterLen >= (int)sizeof(anPacket) )
	{
		return 4;
	}

	strcpy(szTempFile, p_bstrFirmwareFile);
	strcat(szTempFile, ".fws");
	hex = fopen(p_bstrFirmwareFile, "r");
	fws = fopen(szTempFile, "w+");

	if( hex == NULL || fws == NULL )
	{
		printf("cannot open source or dest file\n");
		return 2;
	}

	// init
	memset(m_anMemoryBuffer, 0xFF, sizeof(m_anMemoryBuffer));

	int kActivate = 0;
	*(unsigned short*)(anActivatePacket+kActivate) = htons(p_shFirmwareTarget);
	kActivate += sizeof(p_shFirmwareTarget);
	anActivatePacket[kActivate++] = (unsigned char)(p_shWriteFlag & 0xFF);

	bool bWritten = true;
	int nLastOffset = 0;
	int nLastContiguous = 0;
	unsigned char nCmdCode = 0;
	unsigned short nCmdLen = 0;

	int k = 0;
	*(unsigned short*)(anPacket+k) = htons(p_shFirmwareTarget);
	k += sizeof(p_shFirmwareTarget);
	*(unsigned short*)(anPacket+k) = htons((unsigned short)nLastOffset);
	k += sizeof(short);

	// firmware
	char string[100];
	unsigned char anData[50];

	*p_lFirmwareVersion = 0;
    *p_lFirmwareBuild = 0;
	*p_lFirmwareDNA = 0;

	while( fgets(string, 100, hex)  )
	{
		switch( string[0] )
		{
		case ':':
			{
				if( hex2bin( string+1, anData, sizeof(anData) ) < 0 )
				{
					return 3;
				}

				if( anData[3]  ) // must be 0
					break;

                if(anData[1] == 0xEC )
				{
					switch(  anData[2] )
					{
					case 0x00: // REG 01 (DNA)
//:10EC0000FFFFFFFFFFFF  0021  FFFFFFFFFFFFFFFFF1
						*p_lFirmwareDNA = (((unsigned long)anData[10]) << 8 ) + anData[11];
						break;

					case 0x30: // REG 0E (VERSION NO)
//:10EC300005000080030000  0003  0000  010A0000043A

						((char*)p_lFirmwareBuild)[0] = anData[12];
						((char*)p_lFirmwareBuild)[1] = anData[13];
						((char*)p_lFirmwareVersion)[0] = anData[14];
						((char*)p_lFirmwareVersion)[1] = anData[15];
						break;
					}

				}

				int nNoOfBytes = anData[0];
				int nAddress = 0;
				((char*)&nAddress)[1] = anData[1];
				((char*)&nAddress)[0] = anData[2];
				nAddress -= OFFSET;

				// validity check
				if( nAddress < 0 || nAddress > MEMORY_SIZE_900 )
				{
					return 3;
				}

				memcpy(m_anMemoryBuffer+nAddress, anData+4, nNoOfBytes);
				if( (k + nNoOfBytes) > p_shMaxPacketLen || (nAddress != nLastOffset) )
				{
                    if( p_shWriteFlag < 0x0100 ) // upload FW required
                    {
						if( p_bBroadcast )
						{
							nCmdLen = htons( (unsigned short)(k + 1 + 5) + nFilterLen );
							fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
							nCmdCode = BROADCAST;
							fwrite(&nCmdCode, sizeof(nCmdCode), 1, fws);
							fwrite(&p_nDNATarget, sizeof(p_nDNATarget), 1, fws);

							if( nFilterLen )
							{
								fwrite(aFilter, nFilterLen, 1, fws);
							}
						}
						else
						{
							nCmdLen = htons( (unsigned short)(k + 1) );
							fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
						}
						nCmdCode = FLASH_UPDATE;
						fwrite(&nCmdCode, sizeof(nCmdCode), 1, fws);
						fwrite(anPacket, 1, k, fws);
                    }
					bWritten = true;

					// put CRC into activate packet
					if( nAddress != nLastOffset )
					{
						if( kActivate < p_shMaxPacketLen )
						{
							// dst offset
							// dst offset becomes ANSI crc
							*(unsigned short*)(anActivatePacket+kActivate) = crcAnsiBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous );
							kActivate += sizeof(short);
							// src offset
							*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)nLastContiguous);
							kActivate += sizeof(short);
							// src len
							*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)(nLastOffset-nLastContiguous));
							kActivate += sizeof(short);
							// crc
							*(unsigned short*)(anActivatePacket+kActivate) = crcRFBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous );
							kActivate += sizeof(short);
						}
						else
						{
							// clean
							return 4;
						}
						nLastContiguous = nAddress;
					}

					nLastOffset = nAddress;
					k = 0;
					*(unsigned short*)(anPacket+k) = htons(p_shFirmwareTarget);
					k += sizeof(p_shFirmwareTarget);
					*(unsigned short*)(anPacket+k) = htons((unsigned short)nLastOffset);
					k += sizeof(short);
				}
				memcpy(anPacket+k, anData+4, nNoOfBytes);
				bWritten = false;
				k += nNoOfBytes;
				nLastOffset += nNoOfBytes;
				break;
			}
		case '\r':
		case '\n':
		case '\0':
				break;
		default :
				return 3;
		}
	}

	if( ! nLastOffset ) // empty file
	{
		return 3;
	}

    if( !bWritten && p_shWriteFlag < 0x0100) { // upload FW required
		if( p_bBroadcast ) {
			nCmdLen = htons( (unsigned short)( k + 1 + 5 ) + nFilterLen );
			fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
			nCmdCode = BROADCAST;
			fwrite(&nCmdCode, sizeof(nCmdCode), 1, fws);
			fwrite(&p_nDNATarget, sizeof(p_nDNATarget), 1, fws);
			if( nFilterLen )
			{
				fwrite(aFilter, nFilterLen, 1, fws);
			}
		}
		else {
			nCmdLen = htons( (unsigned short)(k+1) );
			fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
		}
		nCmdCode = FLASH_UPDATE;
		fwrite(&nCmdCode, sizeof(nCmdCode), 1, fws);
		fwrite(anPacket, k, 1, fws);
	}

	if( kActivate >= p_shMaxPacketLen )
	{
		return 4;
	}

	// dst offset becomes ANSI crc
	*(unsigned short*)(anActivatePacket+kActivate) = crcAnsiBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous);
	kActivate += sizeof(short);

	// src offset
	*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)nLastContiguous);
	kActivate += sizeof(short);

	// src len
	*(unsigned short*)(anActivatePacket+kActivate) = htons((unsigned short)(nLastOffset-nLastContiguous));
	kActivate += sizeof(short);

	// crc
	*(unsigned short*)(anActivatePacket+kActivate) = crcRFBlock( m_anMemoryBuffer+nLastContiguous, nLastOffset-nLastContiguous );
	kActivate += sizeof(short);

	// write activate
	if( p_bBroadcast )
	{
		nCmdLen = htons( (unsigned short)(kActivate + 5 + 1) + nFilterLen );
		fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
		nCmdCode = BROADCAST;
		fwrite(&nCmdCode, sizeof(nCmdCode), 1, fws);
		fwrite(&p_nDNATarget, sizeof(p_nDNATarget), 1, fws);
		if( nFilterLen )
		{
			fwrite(aFilter, nFilterLen, 1, fws);
		}
	}
	else {
		nCmdLen = htons( (unsigned short)(kActivate + 1) );
		fwrite(&nCmdLen, sizeof(nCmdLen), 1, fws);
	}
	nCmdCode = ACTIVATE_FLASH;
	fwrite(&nCmdCode, sizeof(nCmdCode), 1, fws);
	fwrite(anActivatePacket, kActivate, 1, fws);
	fclose(fws);
	fclose(hex);

/*	rewind(fws);
	long nFileLength = filelength(fileno(fws));
	WCHAR* pwchOutputStream = new WCHAR[2*nFileLength+1];
	if( pwchOutputStream == NULL )
	{
		*p_pnErrorCode = 1;
		return;
	}
	pwchOutputStream[nFileLength] = 0;
	for(i = 0; i < nFileLength; i++) {
		unsigned char byte;
		fread(&byte, sizeof(byte), 1, fws);
		swprintf(pwchOutputStream+2*i, L"%02X", (unsigned int)byte);
	}
	SysFreeString( *p_pbstrCommandsStream );
	*p_pbstrCommandsStream = SysAllocString(pwchOutputStream);
	delete pwchOutputStream;*/

	return 0;
}


//assume that p_szIpv4 has at least 13 bytes reserved
bool AdjustIPv6 (TIPv6Address* p_pIpv6, const char* p_szIpv4, int p_nPort)
{
	unsigned int nIPv4 = INADDR_NONE;

	if ( strcmp(p_szIpv4,"0.0.0.0") == 0)
	{
		nIPv4 = CWrapSocket::GetLocalIp();				
	}
	else
	{
		nIPv4 = inet_addr(p_szIpv4);
	}

	if (nIPv4 == INADDR_NONE)
	{
		LOG("AdjustIPv6: ERROR IPv4 <%s> %d invalid", p_szIpv4, nIPv4  );
		return false;
	}

	memcpy( p_pIpv6->m_pu8RawAddress + 12, &nIPv4, sizeof(nIPv4));

	unsigned short usPort = htons((unsigned short)p_nPort);
	memcpy(p_pIpv6->m_pu8RawAddress + 10, &usPort, sizeof(usPort));
	return true;
}
