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

#define UPGRADE_WEB		NIVIS_TMP"upgrade_web/"
#define VERSION_FILE	NIVIS_FIRMWARE"version"

#include "MiscImpl.h"

#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <sys/types.h>

#include <sys/resource.h>
#include <signal.h>

#include "../../../Shared/Utils.h"
#include "../../../Shared/IniParser.h"
#include "../../../Shared/FileLock.h"
#include <fcntl.h> // O_RDRW
#include <sys/file.h> // flock

#define RESOLV_CONF                     "/etc/resolv.conf.eth"

enum {
  NENOFILE,
  NEVALIDATIONERROR
};

#define EMAIL_TMP_FILE	NIVIS_TMP"email_tmp_file.txt"
#define UPGRADE_LOCK	NIVIS_TMP"fw_upgrade.lock"
#define DISABLE_DHCP_FLAG_FILE "/etc/config/no_dhcp"

uint32_t CValidator::Ip(const char* p_szIp, uint32_t p_uiMask) 	 
{
	uint32_t ip32 ; 	 
	if ( strlen(p_szIp) > 15 ) { FLOG("Inalid IP length");return 0 ;} 	 

	unsigned int a1,a2,a3,a4; 	 
	a1=a2=a3=a4=0; 	 
	sscanf( p_szIp, "%3u.%3u.%3u.%3u", &a1,&a2,&a3,&a4) ; 	 
        if ( a1>255 || a2>255 || a3>255 || a4>255 ) { FLOG("Invalid ip group");return 0 ;} 	 
        ip32 = a1<<24 | a2<<16 | a3<<8 | a4 ; 	 
        if ( ip32==0xFFFFFFFF || !ip32 ) {FLOG("[%X](%s) is 00000000/FFFFFFFF",ip32,p_szIp);return  0;} 	 
        if ( (ip32& ~p_uiMask)==0 ) { FLOG("[%X] doesn't match the NetMask[%X]");return 0 ;} 	 
        return ip32 ; 	 
}

uint32_t CValidator::Mask( const char *p_szMask ) 	 
{
        int r=0; 	 
        uint32_t mask32 = Ip ( p_szMask,0x00000000U ); 	 
 	 
        if ( !mask32 || mask32&1 ||   0xFFFFFFFF == mask32 ) { FLOG("NetMask[%X] is 00000000 or FFFFFFFF or has 1 on right",mask32);return 0 ;} 	 
                //http://www-graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup 	 
        r = MultiplyDeBruijnBitPosition[((mask32 & -mask32) * 0x077CB531UL) >> 27]; 	 
        if ( 0xFFFFFFFF != (mask32 | (0xFFFFFFFF>>(32-r)) ) ) 	 
        { 	 
                FLOG("Interspersed 0/1 found in NetMask[%X]",mask32); 	 
                return 0 ; 	 
        } 	 
        return mask32 ; 	 
}

uint32_t CValidator::Gateway( const char *p_szGw, uint32_t mask, uint32_t ip32 ) 	 
{
        uint32_t gw32 = Ip(p_szGw,mask); 	 
        if (    !gw32 	 
                                 ||   (gw32 & mask) != (ip32 & mask) 	 
                                 ||   ~mask == (gw32 & ~mask) ) 	 
        { 	 
                 FLOG("Invalid Gateway[%X](%s):mask[%X]:ip[%X]",gw32,p_szGw, mask, ip32); 	 
                 return 0; 	 
        } 	 
        return gw32; 	 
}

bool CMiscImpl::unpackTgz(const char *p_szScript, std::vector<char*> & p_vOut)
{
	if ( !lockUpgradeFile() )
	{
		WARN("Unable to lock:%s", p_szScript);
		return false ;
	}
	if ( !p_szScript || access(p_szScript, R_OK|X_OK) )
	{
		unlockUpgradeFile();
		WARN("Wrong permissions on:%s", p_szScript);
		return false;
	}
	FILE *script_pipe = popen(p_szScript, "r");
	if ( script_pipe==NULL )
	{
		unlockUpgradeFile();
		ERR("Unable to run: %s", p_szScript);
		return false ;
	}
	ssize_t ls;
	char *lineptr(0);
	size_t n(0);
	while(true){
		ls = getline(&lineptr, &n, script_pipe);
		if(ferror(script_pipe) || feof(script_pipe) || ls == -1)
			break;

		if(lineptr)
			p_vOut.push_back(strdup(lineptr));

	}
	free(lineptr);
	int ret_code = pclose(script_pipe);
	if ( ret_code==-1 )
	{
		ERR("Unable to close pipe: %s", p_szScript);
		unlockUpgradeFile();
		return false;
	}
	if ( WEXITSTATUS(ret_code) != EXIT_SUCCESS )
	{
		ERR("Command failed: %s:%d ", p_szScript,  WEXITSTATUS(ret_code));
		unlockUpgradeFile();
		return false;
	}
	unlockUpgradeFile();
	return true;
}

char *CMiscImpl::getVersion(void)
{
	char *line = new char[256 * sizeof (char)];
	FILE* f=fopen(VERSION_FILE, "r");
	if ( !f )
	{
		ERR("Unable to open "VERSION_FILE);
		delete [] line ;
		return NULL ;
	}
	if ( !fgets( line, 256, f) )
	{
		ERR("Unable to get version");
		delete [] line ;
		return NULL ;
	}
	return line;
}


bool getImageSize(const char *fn, int *x,int *y)
{
	FILE *f = fopen(fn, "rb");
	if (f == 0)
		return false;

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (len < 24)
	{
		fclose(f);
		return false;
	}

	// Strategy:
	// reading GIF dimensions requires the first 10 bytes of the file
	// reading PNG dimensions requires the first 24 bytes of the file
	// reading JPEG dimensions requires scanning through jpeg chunks
	// In all formats, the file is at least 24 bytes big, so we'll read that always
	unsigned char buf[24];
	size_t br = fread(buf, 1, 24, f);
	if (br < 24)
	{
		if ( ferror(f) )
			FPERR("getImageSize: failed read(%s)", fn);
		else
			LOG_ERR("getImageSize: failed read(%s)", fn);
		fclose(f);
		return false;
	}

	// For JPEGs, we need to read the first 12 bytes of each chunk.
	// We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
	if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE0 &&
		buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I' && buf[9] == 'F')
	{
		long pos = 2;
		while (buf[2]==0xFF) {
			if (buf[3] == 0xC0 || buf[3] == 0xC1 || buf[3] == 0xC2 || buf[3] == 0xC3 ||
				buf[3] == 0xC9 || buf[3] == 0xCA || buf[3] == 0xCB)
				break;
			pos += 2 + (buf[4] << 8) + buf[5];
			if (pos + 12 > len)
				break;
			fseek(f, pos, SEEK_SET);
			br = fread(buf + 2, 1, 12, f);
			if (br < 12)
			{
				if ( ferror(f) )
					FPERR("getImageSize: failed read(%s)", fn);
				else
					LOG_ERR("getImageSize: failed read(%s)", fn);
				fclose(f);
				return false;
			}
		}
	}
	fclose(f);
	// JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
	if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
	{
		*y = (buf[7] << 8) + buf[8];
		*x = (buf[9] << 8) + buf[10];
		return true;
	}
	// GIF: first three bytes say "GIF", next three give version number. Then dimensions
	if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
	{
		*x = buf[6] + (buf[7] << 8);
		*y = buf[8] + (buf[9] << 8);
		return true;
	}
	// PNG: the first frame is by definition an IHDR frame, which gives dimensions
	if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' &&
		buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A && buf[7] == 0x0A &&
		buf[12] == 'I' && buf[13] == 'H' && buf[14] == 'D' && buf[15] == 'R')
	{
		*x = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + (buf[19] << 0);
		*y = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + (buf[23] << 0);
		return true;
	}
	return false;
}

bool CMiscImpl::fileUpload(const char *p_szScript, const char*p_kszScriptParam, char *& p_szScriptRsp) {
	size_t a= p_szScript?strlen(p_szScript):0 ;
	size_t b= p_kszScriptParam?strlen(p_kszScriptParam):0;

	if ( ! FileExist(p_szScript) )
	{
		WARN("No upload script:[%s]", p_szScript );
		return false ;
	}
	if(!lockUpgradeFile())
	{
		return false;
	}
	char * cmd = new char[ a+b+2 ] ;
	sprintf( cmd, "%s %s", p_szScript?p_szScript:"", p_kszScriptParam?p_kszScriptParam:"");
	LOG("fileUpload: sh=%s",cmd);

	bool bRet = execCmd(cmd, p_szScriptRsp);
	delete [] cmd ;
	LOG("fileUpload:DONE");
	unlockUpgradeFile();
	return bRet;
}


bool CMiscImpl::fileDownload(const char *p_szScript, const char*p_kszScriptParam, char *& p_szScriptRsp)
{
	size_t a= p_szScript?strlen(p_szScript):0 ;
	size_t b= p_kszScriptParam?strlen(p_kszScriptParam):0;

	if ( !FileExist(p_szScript) )
	{
		return false ;
	}

	char * cmd = new char[ a+b+2 ] ;
	sprintf( cmd, "%s %s", p_szScript?p_szScript:"", p_kszScriptParam?p_kszScriptParam:"");
	LOG("fileDownload: sh=%s",cmd);
	bool bRet = execCmd(cmd, p_szScriptRsp);
	delete [] cmd ;
	return bRet;
}

/// TAKE CARE: p_szCmdRsp memory must be deleted by the caller
bool CMiscImpl::execCmd(const char *p_szCmd, char *& p_szCmdRsp)
{
	FLOG("cmd[%s]",p_szCmd);
	char szTmpFileSH[128];
	char szTmpFileOut[128];
	int nTime = time(NULL);
	sprintf(szTmpFileSH,NIVIS_TMP"web_ra_%d.sh", nTime);
	sprintf(szTmpFileOut,NIVIS_TMP"web_ra_%d.out", nTime);
	WriteToFile(szTmpFileSH, "#!/bin/sh\n.  /etc/profile\n set -x\n", true);
	WriteToFile(szTmpFileSH, p_szCmd);
	systemf_to(600, "chmod a+x %s >%s 2>&1", szTmpFileSH, szTmpFileOut);
	int nRet = systemf_to(600, "%s >%s 2>&1", szTmpFileSH, szTmpFileOut);
	int nRspLen = 0;
	GetFileData(szTmpFileOut, p_szCmdRsp, nRspLen);
	unlink(szTmpFileOut);
	unlink(szTmpFileSH);
	return !nRet;
}
bool CMiscImpl::lockUpgradeFile(void)
{
	m_nFd = open( UPGRADE_LOCK , O_RDWR | O_CREAT, 0666 );
	if (m_nFd < 0)
	{	LOG_ERR("lockUpgradeFile: failed open(%s)", UPGRADE_LOCK);
		return false;
	}
	if ( ::flock( m_nFd, LOCK_NB|LOCK_EX )!=0)
	{
		LOG_ERR("lockUpgradeFile: failed flock(%s)", UPGRADE_LOCK);
		struct stat buf;
		if( stat(UPGRADE_LOCK , &buf) == 0 )
		{
			int nCrtTime = time(NULL);
			if(  nCrtTime < buf.st_mtime || nCrtTime - buf.st_mtime >= 300 )
			{
				LOG("lockUpgradeFile: lock(%s) hold too long -> unlink", UPGRADE_LOCK);
				if (unlink(UPGRADE_LOCK ) ) LOG_ERR("lockUpgradeFile: failed unlink(%s)", UPGRADE_LOCK);
			}
		}
		return false;
	}
	ssize_t bw = write(m_nFd, "1", 1);
	if (bw < 1)
	{
		FPERR("lockUpgradeFile: failed write(%s)", UPGRADE_LOCK);
		unlockUpgradeFile();
		close(m_nFd);
		return false;
	}
	return true;
}

bool CMiscImpl::unlockUpgradeFile(void)
{
	if (flock( m_nFd, LOCK_UN)!=0)
	{
		LOG_ERR("unlockUpgradeFile: failed flock(%s)", UPGRADE_LOCK);
		return false;
	}
	return true;
}

#define RC_NET_INFO "/etc/rc.d/rc.net.info"

bool CMiscImpl::getGatewayNetworkInfo( GatewayNetworkInfo& p_rGatewayNetworkInfo )
{
	CIniParser oP;

	if ( ! oP.Load(RC_NET_INFO) )
	{
		ERR("getGateway: unable to load "RC_NET_INFO);
		return false;
	}
	oP.GetVar( 0, "ETH0_IP",	p_rGatewayNetworkInfo.ip,	sizeof(p_rGatewayNetworkInfo.ip)    );
	oP.GetVar( 0, "ETH0_MASK",	p_rGatewayNetworkInfo.mask,	sizeof(p_rGatewayNetworkInfo.mask)  );
	oP.GetVar( 0, "ETH0_GW",	p_rGatewayNetworkInfo.defgw,sizeof(p_rGatewayNetworkInfo.defgw) );
	p_rGatewayNetworkInfo.mac0[0] = p_rGatewayNetworkInfo.mac1[0]=0;	/// compatibility with systems without MAC
	oP.GetVar( 0, "ETH0_MAC",	p_rGatewayNetworkInfo.mac0,	sizeof(p_rGatewayNetworkInfo.mac0)  );
	oP.GetVar( 0, "ETH1_MAC",	p_rGatewayNetworkInfo.mac1,	sizeof(p_rGatewayNetworkInfo.mac1)  );
	p_rGatewayNetworkInfo.bDhcpEnabled = access( DISABLE_DHCP_FLAG_FILE, R_OK );

	FILE * f;
	if( (f = fopen( RESOLV_CONF, "r")) )
	{
		flock( fileno(f), LOCK_EX );
		int i;
		for( i=0; (!feof( f )) && i<(int)sizeof(p_rGatewayNetworkInfo.nameservers) ; )
		{	// Match 15 in the format with DNS_SERVER_SIZE-1
			char tmp[512];
			if(fscanf( f, " nameserver %15[0-9.] ", tmp) == 1 )
			{
				p_rGatewayNetworkInfo.nameservers[i] = strdup(tmp);
				LOG("getGateway DNS[%d]=[%s]", i, p_rGatewayNetworkInfo.nameservers[i] );
				i++;
			}
			else
			{
				char* l(NULL);
				size_t s(0);
				ssize_t cr = getline(&l, &s, f) ; // ignore malformed lines
				if (l) free(l);
				if (cr == -1)
				{
					FPERR("getGateway: failed getline (%s)", RESOLV_CONF);
					break;
				}
			}
		}
		p_rGatewayNetworkInfo.nameservers[i]=NULL;
		flock( fileno(f), LOCK_UN );
		fclose(f);
	}	// open file errors are ok: report no DNS servers

	oP.Release() ;

	return true;
}


// you will see the same code in ElmShared.
// [Marcel] no longer true
bool CMiscImpl::setGatewayNetworkInfo( GatewayNetworkInfo& p_rGatewayNetworkInfo, int*status)
{
	CValidator isValid ;
	uint32_t mask32 = isValid.Mask( p_rGatewayNetworkInfo.mask );
	if ( !mask32 )
	{
		ERR("setGatewayNetworkInfo: Invalid mask [%s]", p_rGatewayNetworkInfo.mask);
		if (status) *status = NEVALIDATIONERROR ;
		return false ;
	}

	uint32_t ip32 = isValid.Ip( p_rGatewayNetworkInfo.ip, mask32 );
	if ( !ip32 )
	{
		ERR("setGatewayNetworkInfo: Invalid ip [%s]", p_rGatewayNetworkInfo.ip);
		if (status) *status = NEVALIDATIONERROR ;
		return false ;
	}

	if ( ! isValid.Gateway( p_rGatewayNetworkInfo.defgw, mask32, ip32))
	{
		ERR("setGatewayNetworkInfo: Invalid gateway [%s]", p_rGatewayNetworkInfo.defgw);
		if (status) *status = NEVALIDATIONERROR ;
		return false ;
	}

	mask32 = isValid.Mask( "255.255.255.0" );

	// try to use the ETH_NAME that already exists.
	CIniParser oP;
	char ethName[MAX_LINE_LEN]={0};
	char eth0MAC[MAX_LINE_LEN]={0};
	char eth1MAC[MAX_LINE_LEN]={0};
	if( p_rGatewayNetworkInfo.bUpdateMAC )
	{	strncpy( eth0MAC, p_rGatewayNetworkInfo.mac0, sizeof(eth0MAC) );/// user input takes precedence. it will override whatever was previously there
		strncpy( eth1MAC, p_rGatewayNetworkInfo.mac1, sizeof(eth1MAC) );/// user input takes precedence. it will override whatever was previously there
		eth0MAC[ sizeof(eth0MAC)-1 ] = eth1MAC[ sizeof(eth1MAC)-1 ] = 0;
	}

	if ( ! oP.Load(RC_NET_INFO) )
	{
		ERR("getGateway: unable to load "RC_NET_INFO" will use ETH_NAME=eth0");
	}
	else
	{
		oP.GetVar( 0, "ETH_NAME",	ethName,	sizeof(ethName) );
		if( !p_rGatewayNetworkInfo.bUpdateMAC )/// update not requested by user, read & use current value
		{	oP.GetVar( 0, "ETH0_MAC",	eth0MAC,	sizeof(eth0MAC) );
			oP.GetVar( 0, "ETH1_MAC",	eth1MAC,	sizeof(eth1MAC) );
		}
		oP.Release();
	}

	FILE*f=fopen( RC_NET_INFO, "w");
	if ( !f )
	{
		LOG_ERR("ERROR setGatewayNetworkInfo: fopen(" RC_NET_INFO ",w) failed");
		return false ;
	}
	flock( fileno(f), LOCK_EX );
	fputs("#AUTOMATICALLY GENERATED.\n", f );
	fprintf( f, "ETH0_IP=%s\n",   p_rGatewayNetworkInfo.ip ) ;
	fprintf( f, "ETH0_MASK=%s\n", p_rGatewayNetworkInfo.mask ) ;
	fprintf( f, "ETH0_GW=%s\n",   p_rGatewayNetworkInfo.defgw ) ;
	if ( 0==ethName[0] ) fprintf( f, "ETH_NAME=eth0\n") ;
	else fprintf( f, "ETH_NAME=%s\n", ethName ) ;

	/// user request update OR value previously set
	if( p_rGatewayNetworkInfo.bUpdateMAC || eth0MAC[0] )
	{
		fprintf( f, "ETH0_MAC=%s\n", eth0MAC ) ;
	}
	/// else (update not requested AND previous value inexistent): do not set at all, fallback to IP-based MAC

	if( p_rGatewayNetworkInfo.bUpdateMAC || eth1MAC[0] )
	{
		fprintf( f, "ETH1_MAC=%s\n", eth1MAC ) ;
	}
	/// else (update not requested AND previous value inexistent): do not set at all, fallback to IP-based MAC

	flock( fileno(f), LOCK_UN );
	fclose(f);

	bool bPreviouslyDisabled = !access( DISABLE_DHCP_FLAG_FILE, R_OK );

	if(p_rGatewayNetworkInfo.bDhcpEnabled && bPreviouslyDisabled )
	{	// delete disable flag file
		unlink( DISABLE_DHCP_FLAG_FILE );
	}
	if( !p_rGatewayNetworkInfo.bDhcpEnabled && !bPreviouslyDisabled )
	{	// create disable flag file
		int rv ;
		rv = open( DISABLE_DHCP_FLAG_FILE , O_CREAT, 0644 );
		if ( -1 != rv )
		{
			close( rv );
		}
	}

	if ( NULL == p_rGatewayNetworkInfo.nameservers[0] ) return true ;

	f=fopen( RESOLV_CONF, "w");
	if ( !f )
	{
		LOG_ERR("ERROR: setGatewayNetworkInfo: fopen(" RESOLV_CONF ") failed");
		return false ;
	}
	flock( fileno(f), LOCK_EX );
	for ( int i=0; p_rGatewayNetworkInfo.nameservers[i] != NULL;++i )
	{
		fprintf( f, "nameserver %s\n", p_rGatewayNetworkInfo.nameservers[i] );
	}
	flock( fileno(f), LOCK_UN );
	fclose(f);

	return true;
}

#define NTP_CONF "/etc/ntp.conf"
bool CMiscImpl::getNtpServers( std::vector<char*>& servers )
{
	char *dline=NULL, srv[512];
	size_t n;
	ssize_t rv ;
	FILE* df;
	df=fopen(NTP_CONF,"r");
	if ( !df ) return false ;
	while ( -1 != (rv=getline(&dline, &n, df)) )
	{
		rv=sscanf( dline, "server %511s", srv );
		if ( EOF==rv || rv < 1 ) continue ;
		LOG("ntp-server:[%s]", srv);
		servers.push_back(strdup(srv));
	}
	if (dline) free(dline);
	fclose(df);
	return true ;
}


/// Set the server lines to those inside servers vector.
/// leave the unknown lines intact
bool CMiscImpl::setNtpServers( const std::vector<const char*>& servers )
{
	char *dline=NULL, srv[512];
	FILE *df,*out;
	ssize_t rv;
	size_t n;
	df=fopen(NTP_CONF,"r");
	if ( !df ) return false ;

	out=fopen(NTP_CONF".halfway","w");
	if ( !out ) return false ;
	while ( -1 != (rv=getline(&dline, &n, df)) )
	{
		srv[0] = 0;
		rv=sscanf( dline, "server %511s", srv);
		if ( rv == EOF || rv <0 ) break ;
		if ( (rv == 0) || !strncmp(srv,"127.127.1.0", 11) )/// unknown lines OR server 127.127.1.0
		{
			fprintf( out, "%s",dline);
			continue ;
		}
	}
	for ( unsigned i=0; i<servers.size(); ++i)
	{
		if ( strncmp(servers[i],"127.127.1.0", 11) )
		{
			LOG("set:%s", servers[i]);
			fprintf( out, "server %s iburst\n",servers[i]);
		}
	}

	fclose(df);
	fclose(out);
	rename(NTP_CONF".halfway",NTP_CONF);
	return true ;
}

bool CMiscImpl::restartApp( const char * p_szAppName, const char * p_szAppParams )
{
	log2flash("CGI: restartApp[%s]", p_szAppName);

	/// TODO: ADD SOME MINIMAL PROTECTION: illegal chars: & ; and space
	/// This should not be implemented as restartApp, but rather as a wrapper
	/// for start.sh ApplicationName. This way we are not aware of the release
	/// specific application list nor it's permissions.
//#warning MAKE/CONVINCE restartApp to use start.sh
	systemf_to( 600, "exec 0>&- ; exec 1>&- ; exec 2>&- ; . /etc/profile; killall %s; sleep 1; killall -9 %s; usleep 500000; %s %s &", p_szAppName, p_szAppName, p_szAppName, p_szAppParams?p_szAppParams:"");
	return true;
}

void detach()
{
	// HTTP server won't exit until we close all stds
	// so close them as fast as possible
	close(0) ;
	close(1) ;
	close(2) ;

	// I haven't found out why ( renice -n 0 $$ ; ... ) was not enough to
	// bring the processes to normal priority, so I'm using setpriority
	setpriority(PRIO_PGRP,0,0);
	setsid() ;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Restart all applications using start.sh
/// @retval false fork failed.
/// @retval true  only the parent returns true; the child stays alive to run start.sh
////////////////////////////////////////////////////////////////////////////////
bool CMiscImpl::softwareReset( )
{
	log2flash("CGI: %s", __FUNCTION__ );

	pid_t pid = vfork() ;
	if ( pid < 0 ) return false ;

	if ( pid > 0 ) return true ;

	if ( pid == 0 )
	{
		detach() ;
		// wait for the parent to return the response
		// then it's safe to kill the http server(from start.sh)
		sleep(2) ;

//#warning ALWAYS SOURCE /etc/profile BEFORE RUNNING FIRMWARE SCRIPTS
		return execlp("/bin/sh", "sh", "-c", ". /etc/profile && "NIVIS_FIRMWARE"start.sh >/dev/null 2>&1 && "NIVIS_FIRMWARE"log2flash 'CGI: softwareReset done'", NULL );

		//return execlp("/bin/sh", "sh", "-c", ". /etc/profile && "NIVIS_FIRMWARE"web_start.sh 5 && "NIVIS_FIRMWARE"log2flash 'CGI: softwareReset done'", NULL );
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Reboot the device
/// @retval false fork failed.
/// @retval true  only the parent returns true; the child stays alive to run stop.sh
////////////////////////////////////////////////////////////////////////////////
bool CMiscImpl::hardReset( )
{
	log2flash("CGI: %s", __FUNCTION__ );

	pid_t pid = vfork() ;
	if ( pid < 0 ) return false ;

	if ( pid > 0 ) return true ;

	if ( pid == 0 )
	{
		detach() ;
		// wait for the parent to return the response
		// then it's safe to kill the http server(from start.sh)
		sleep(2) ;

//		#warning ALWAYS SOURCE /etc/profile BEFORE RUNNING FIRMWARE SCRIPTS
		execlp("/bin/sh", "sh", "-c", ". /etc/profile && "NIVIS_FIRMWARE"stop.sh ; reboot", NULL );
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @brief   Apply configuration file changes (signal the module to reload configurations)
/// @param   p_szModule Module to signal
/// @retval  false cannot signal the module
/// @retval  true  module signalled ok
/// @note    This opens yet another security hole, since the user can kill any
///          process he wishes.
////////////////////////////////////////////////////////////////////////////////
bool CMiscImpl::applyConfigChanges( const char * p_szModule )
{	/// send HUP to specified module
	#define READ_BUF_SIZE 50

	DIR *dir;
	struct dirent *next;

	if ( strcmp(p_szModule, "init")==0)
	{
		LOG("Can't kill init") ;
		return false ;
	}

	dir = opendir("/proc");
	if (!dir)
	{
		LOG("Cannot open /proc");
		return false ;
	}

	while ((next = readdir(dir)) != NULL)
	{
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, p_szModule) == 0) {
			kill(strtol(next->d_name, NULL,0), SIGHUP );
			return true ;
		}
	}
	WARN("Process %s not found", p_szModule );
	return false ;
}
