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


#include "GwUtil.h"
#include "Service.h"

const char* GetAlertCategoryName (unsigned int p_nCategory)
{
	static const char * pUNK =
		"Unknown  ";
	static const char* sAlertCategoryNames[] = {
		"Dev Diag ",
		"Comm Diag",
		"Security ",
		"Process  " };
	unsigned int nAlertCategoryNamesSize = sizeof(sAlertCategoryNames)/sizeof(sAlertCategoryNames[0]);
	
	return (p_nCategory < nAlertCategoryNamesSize) ? sAlertCategoryNames[p_nCategory] : pUNK;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief GSAP service name (GW/USER), without direction (REQ/IND/CNF/RSP)
/// @param p_ucServiceType service type
/// @return the service name
////////////////////////////////////////////////////////////////////////////////
const char * getGSAPServiceNameNoDir( uint8 p_ucServiceType )
{	static const char * sServiceName[] = {
		"UNKNOWN     ",
		"SESSION_MGMT",		//SESSION_MANAGEMENT
		"LEASE_MGMT  ",		//LEASE_MANAGEMENT
		"DEVICE_LIST ",		//DEVICE_LIST_REPORT
		"TOPOLOGY    ",		//TOPOLOGY_REPORT
		"SCHEDULE    ",		//SCHEDULE_REPORT
		"DEVICEHEALTH",		//DEVICE_HEALTH_REPORT
		"NEIGHBHEALTH",		//NEIGHBOR_HEALTH_REPORT
		"NETWRKHEALTH",		//NETWORK_HEALTH_REPORT
		"TIME        ",
		"CLIENTSERVER",		//CLIENTSERVER
		"PUBLISH     ",
		"SUBSCRIBE   ",
		"PUBLISH_TMR ",
		"SUBSCRIB_TMR",
		"WATCHDOG_TMR",
		"BULK_OPEN   ",		//BULK_TRANSFER_OPEN
		"BULK_TRNSFER",		//BULK_TRANSFER_TRANSFER
		"BULK_CLOSE  ",		//BULK_TRANSFER_CLOSE
		"ALERT_SUBSCR",
		"ALERT_NOTIFY",
		"GW_CFG_READ ",
		"GW_CFG_WRITE",
		"DEVCFG_READ ",
		"DEVCFG_WRITE",	//x18
		"UNKNOWN     ", //x19
		"UNKNOWN     ", //x1A
		"UNKNOWN     ", //x1B
		"UNKNOWN     ", //x1C
		"UNKNOWN     ", //x1D
		"UNKNOWN     ", //x1E
		"UNKNOWN     ", //x1F
		"UNKNOWN     ", //x20
		"NET_RESOURCE"	//x21
	};

	uint8 u8Idx = REQUEST(p_ucServiceType);
	if( ( u8Idx < 1) || ( u8Idx >= (sizeof(sServiceName) / sizeof(sServiceName[0])) ) )
		return sServiceName[ 0 ];
	else
		return sServiceName[ u8Idx ];
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief GSAP service name (GW/USER), including direction (REQ/IND/CNF/RSP)
/// @param p_ucServiceType service type
/// @return the service name
////////////////////////////////////////////////////////////////////////////////
const char * getGSAPServiceName( uint8 p_ucServiceType )
{
	static char sBuf[ 64 ];
	const char *pExtra="";
	
	if(       IS_REQUEST(p_ucServiceType) )		pExtra = " REQ"; ///REQUEST;
	else if ( IS_INDICATION(p_ucServiceType) )	pExtra = " IND"; ///INDICATION;
	else if ( IS_CONFIRM(p_ucServiceType) )		pExtra = " CNF"; ///CONFIRM;
	else if ( IS_RESPONSE(p_ucServiceType) )	pExtra = " RSP"; ///RESPONSE";
	else pExtra = " UNK";
	
	sprintf( sBuf, "%s%s", getGSAPServiceNameNoDir( p_ucServiceType ) , pExtra );
	return sBuf;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief GSAP status name
/// @param p_ucServiceType service type
/// @param p_ucStatus status
/// @return the status name (service-type dependent)
////////////////////////////////////////////////////////////////////////////////
const char * getGSAPStatusName( uint8_t p_ucProtoVersion, uint8_t p_ucServiceType, uint8_t p_ucStatus )
{
	size_t nSize;
	static const char * pUnimplemented =
		"UNIMPLEMNTED";
	static const char * pUNK =
		"UNKNOWN     ";
	static const char ** pStat = &pUNK;
	static const char * sSession[] ={
		"SUCCESS     ",
		"SUCCESS_REDU",
		"NOT_EXIST   ",
		"NOT_AVAIL   ",
		"FAIL_OTHER  " };
	static const char * sLease[] = {
		"SUCCESS     ",
		"SUCCESS_REDU",
		"NOT_EXIST   ",
		"NOT_AVAIL   ",
		"NO_DEVICE   ",
		"INVALID_TYPE",
		"INVALID_INFO",
		"FAIL_OTHER  " };
	static const char * sSystemReport[] = {
		"SUCCESS     ",
  		"FAILURE     " };
	static const char * sTime[] = {
		"SUCCESS     ",
		"NOT_ALLOWED ",
  		"FAIL_OTHER  " };
	static const char * sClientServer[] = {
		"SUCCESS     ",
		"NOT_AVAIL   ",
		"BUFFER_INVAL",
		"LEASE_EXIPRE",
		"FAIL_OTHER  " };
	static const char * sPublishSubscribe[] = {
		"SUCCESS_FRSH",
		"SUCCESS_STAL",
		"LEASE_EXPIRE",
		"FAIL_OTHER  " };
	static const char * sBulkOpen[] =  {
		"SUCCESS     ",
		"FAIL_EXCEED_LIM",
		"FAIL_UNK_RES",
		"FAIL_INVAL_MODE",
		"FAIL_OTHER  " };
	static const char * sBulkTransfer[] = {
		"SUCCESS     ",
		"FAIL_COMM   ",
		"FAIL_ABORT  ",
		"FAIL_OTHER  " };
	static const char * sAlertSubscribe[] = {
		"SUCCESS     ",
		"FAIL_CAT    ",
		"FAIL_ID     ",
		"FAIL_OTHER  " };
	static const char * sBulkClose[] =  {
		"SUCCESS     ",
		"FAIL        " }; //This value do not exist in ISA100 standard
	static const char * sNetworkResource[] =  {
		"SUCCESS     ",	  //This value do not exist in ISA100 standard
		"FAIL        " }; //This value do not exist in ISA100 standard
	static const char * sYGsap[] =  {
		"YSUCCESS     ",	//YGS_SUCCESS
		"YSUCCESS_REDU",	//YGS_SUCCESS_REDUCED
		"YSUCCESS_STAL",	//YGS_SUCCESS_STALE
		"YGS_RESERVED ", 	// 3 reserved
		"YGS_RESERVED ", 	// 4 reserved
		"YTAG_NOTFOUND",	//YGS_TAG_NOT_FOUND
		"YEXCEED_LIM  ",	//YGS_LIMIT_EXCEEDED
		"YINVALID_TYPE",	//YGS_INVALID_TYPE
		"YINVALID_TYPE_INF",	//YGS_INVALID_TYPE_INFO
		"YINVALID_SESS",	//YGS_INVALID_SESSION
		"YINVALID_MODE",	//YGS_INVALID_MODE
		"YINVALID_CAT ",	//YGS_INVALID_CATEGORY
		"YINVALID_ALRT",	//YGS_INVALID_ALERT
		"YNOT_ALLOWED ",	//YGS_NOT_ALLOWED
		"YNOT_ACCESS  ",	//YGS_NOT_ACCESSIBLE
		"YCACHE_MISS  ",	//YGS_CACHE_MISS
		"YCOMM_FAILED ",	//YGS_COMM_FAILED
		"YINVALID_LEASE",	//YGS_INVALID_LEASE
		"YABORTED     "	//YGS_ABORTED	//18
	};
	static const char *  pYFail = "YGS_FAILURE  ";	//YGS_FAILURE //250

	if( IsYGSAP(p_ucProtoVersion) )
	{
		if ( YGS_FAILURE == p_ucStatus )	//YGS_FAILURE
			return pYFail;

		pStat = sYGsap; 	nSize = sizeof(sYGsap);
	}
	else	// else is PROTO_VERSION_GSAP
	{
		switch( REQUEST(p_ucServiceType) )
		{
			case SESSION_MANAGEMENT:	pStat = sSession; 	nSize = sizeof(sSession);	break;
			case LEASE_MANAGEMENT: 		pStat = sLease; 	nSize = sizeof(sLease); 	break;
			case DEVICE_LIST_REPORT:
			case TOPOLOGY_REPORT:
			case SCHEDULE_REPORT:
			case DEVICE_HEALTH_REPORT:
			case NEIGHBOR_HEALTH_REPORT:
			case NETWORK_HEALTH_REPORT: pStat = sSystemReport;	nSize = sizeof(sSystemReport);	break;
			case TIME_SERVICE:			pStat = sTime; nSize = sizeof(sTime); break;
			case CLIENT_SERVER:			pStat = sClientServer;	nSize = sizeof(sClientServer);	break;
			case PUBLISH:
			case SUBSCRIBE:
			case PUBLISH_TIMER:
			case SUBSCRIBE_TIMER:
			case WATCHDOG_TIMER:		pStat = sPublishSubscribe;nSize = sizeof(sPublishSubscribe);break;
			case BULK_TRANSFER_OPEN:	pStat = sBulkOpen;		nSize = sizeof(sBulkOpen);		break;
			case BULK_TRANSFER_TRANSFER:pStat = sBulkTransfer;	nSize = sizeof(sBulkTransfer);	break;
			case BULK_TRANSFER_CLOSE:	pStat = sBulkClose;		nSize = sizeof(sBulkClose);		break;
			case ALERT_SUBSCRIBE:		pStat = sAlertSubscribe;		nSize = sizeof(sAlertSubscribe);		break;
			//case ALERT_NOTIFY:
			//case GATEWAY_CONFIGURATION_READ:
			//case GATEWAY_CONFIGURATION_WRITE:
			//case DEVICE_CONFIGURATION_READ:
			//case DEVICE_CONFIGURATION_WRITE:
			case NETWORK_RESOURCE_REPORT:	pStat = sNetworkResource; nSize = sizeof(sNetworkResource); break;
			default:					return pUNK;
		}
	}
	if(G_STATUS_UNIMPLEMENTED == p_ucStatus )
	{
		return pUnimplemented;
	}

	return (p_ucStatus < (nSize/sizeof(pStat[0]))) ? pStat[ p_ucStatus ] : pUNK;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief ISA100 service name (ASL_SERVICE_TYPE)
/// @param p_ucServiceType service type
/// @return the service name
/// @note do not mistake for getGSAPServiceName which represent GW/USER communication
////////////////////////////////////////////////////////////////////////////////
const char * getISA100SrvcTypeName( uint8 p_ucServiceType )
{	static const char * pUNK =
		"UNKNOWN";
	static const char * sName[] ={
		"PUBLISH ",
		"ALERTREP",
		"ALERTACK",
		"READ ",
		"WRITE",
		"EXEC ",
		"TUNNEL  " };
	return (p_ucServiceType < (sizeof(sName)/sizeof(sName[0]))) ? sName[ p_ucServiceType ] : pUNK;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief ISA100 device capabilities
/// @param p_ushDeviceType device type
/// @return the device capabilities, text
////////////////////////////////////////////////////////////////////////////////
const char * getCapabilitiesText( uint16 p_ushDeviceType )
{	static char szBuf[ 64 ];// it is enough
	szBuf[ 0 ] = 0;
	if( p_ushDeviceType & 0x01 ) strcat(szBuf, "IO ");
	if( p_ushDeviceType & 0x02 ) strcat(szBuf, "ROUTER ");
	if( p_ushDeviceType & 0x04 ) strcat(szBuf, "BBR ");
	if( p_ushDeviceType & 0x08 ) strcat(szBuf, "GW ");
	if( p_ushDeviceType & 0x10 ) strcat(szBuf, "SM ");
	if( p_ushDeviceType & 0x20 ) strcat(szBuf, "SYS_MGR ");
	if( p_ushDeviceType & 0x40 ) strcat(szBuf, "TIME_SRC ");
	if( p_ushDeviceType & 0x80 ) strcat(szBuf, "PROVISIONING ");
	return szBuf;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief device join status
/// @param p_unJoinStatus device join status
/// @return the device capabilities, text
////////////////////////////////////////////////////////////////////////////////
const char * getJoinStatusText( byte p_unJoinStatus )
{
	switch (p_unJoinStatus)
	{	case 4: return "SEC_JOIN_Req";		//Security join request received
		case 5: return "SEC_JOIN_Rsp";		//Security join response sent
		case 6: return "NETWORK_Req";		// Network join request received (System_Manager_Join method)
		case 7: return "NETWORK_Rsp";		//Network join response sent
		case 8: return "CONTRACT_Req";		//Join contract request received (System_Manager_Contract method)
		case 9: return "CONTRACT_Rsp";		//Join contract response sent (System_Manager_Contract method)
		case 10: return "SEC_CNFRM_Req";	// Security join confirmation received (Security_Confirm)
		case 11: return "SEC_CNFRM_Rsp";	// Security join confirmation response sent (Security_Confirm)
		case 20: return "FULL_JOIN"; 		// Joined & Configured & All info available. THIS IS FULLY JOINED
		default: return "unknown";
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief JOIN Fail ALERT reason
/// @param p_u8Reason join fail reason
/// @return the join fail reason text
////////////////////////////////////////////////////////////////////////////////
const char* getJoinFailAlertReasonText (uint8_t p_u8Reason)
{
	switch( p_u8Reason )
	{	case 1: return "timeout";
		case 2: return "re-join";
		case 3: return "parent_left";
		case 4: return "provisioning_removed";
		case 5: return "not_provisioned";
		case 6: return "invalid_join_key";
		case 7: return "challenge_check_failure";
		case 8: return "insufficient_parent_resources";
		case 9: return "subnet_provisioning_mismatch";
		default: return "unknown";
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief JOIN Leave ALERT reason
/// @param p_u8Reason join leave reason
/// @return the join leave reason, text
////////////////////////////////////////////////////////////////////////////////
const char* getJoinLeaveAlertReasonText (uint8_t p_u8Reason)
{
	switch( p_u8Reason )
	{	case 1: return "timeout";
		case 2: return "re-join";
		case 3: return "parent_left";
		case 4: return "provisioning_removed";
		default: return "unknown";
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  UDO Progress End ALERT error code
/// @param p_u8ErrCode error code
/// @return the error code, text
////////////////////////////////////////////////////////////////////////////////
const char* getUDOTransferEndAlertReasonText (uint8_t p_u8ErrCode )
{
	switch( p_u8ErrCode )
	{	case 0: return "ok";
		case 1: return "cancel";
		case 2: return "fail";
		case 3: return "invalid_firmware";	// signature does not match target type
		default: return "unknown";
	}
}






