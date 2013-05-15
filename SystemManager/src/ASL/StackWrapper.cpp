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

/*
 * StackWrapper.cpp
 *
 *  Created on: Mar 9, 2009
 *      Author: Andy
 */


#include "StackWrapper.h"

#include <porting.h>
LOG_DEF("I.A.StackWrapper");

#include "StackWrapper/ComposeRequests.h"
#include "StackWrapper/ParseResponses.h"
//#include "TSDUUtils.h"
#include "PDUUtils.h"

#include <Common/SmSettingsLogic.h>
#include <Model/EngineProvider.h>
#include <Model/ContractsHelper.h>
#include <Common/NEException.h>
#include <Common/Address128.h>
#include <Stats/Cmds.h>
#include <map>

#define MAX_PACKET_RETRIES 2
#define RETRY_TIMEOUT 35

extern DSMO_ATTRIBUTES g_stDSMO;
extern NLME_ATRIBUTES g_stNlme;
extern SLME_KEY        g_aKeysTable[MAX_SLME_KEYS_NO];
extern uint16 g_nSLMEKeysNo;

// is iterated in WaitForAPDU, so watch out if multiple threads access it
std::vector<Isa100::ASL::Services::PrimitiveConfirmationPointer> timeouts;


void CallbackLog(LOGLVL p_eLogLvl, const char* p_szMsg )
{
	switch (p_eLogLvl)
	{
	case LOGLVL_ERR: LOG_ERROR( p_szMsg ); break;
	case LOGLVL_INF: LOG_INFO( p_szMsg ); break;
	case LOGLVL_DBG:
	default: LOG_DEBUG( p_szMsg );break;
	}
}

void Stack_ContractsPktsContor(){
    CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.begin();

    for( ; it !=  g_stNlme.m_oContractsMap.end(); it++ ) {
        CNlmeContractPtr pContract = it->second;
        pContract->m_unPktCount = 0;
    }
}

void Stack_PrintContracts(std::ostringstream& stream)
{
	long delaySec;
	int i;
	char ipv6[40];
	char buffer[500];
	char tmp[3];
	MLSM_GetCrtTaiSec();
	stream << " ID           IPv6DstAddress                 RTry Wnd MaxNS CtdBst MaxWnd Delay Pkts" << std::endl;
	stream << "----- --------------------------------------- --- --- ----- ------ ------ ----- ----" << std::endl;

	CNlmeContractsMap::iterator it = g_stNlme.m_oContractsMap.begin();

	for( ; it !=  g_stNlme.m_oContractsMap.end(); it++ )
	{
		CNlmeContractPtr pContract = it->second;

		ipv6[0] = 0;
		for(i=0; i<16; ++i){
			sprintf(tmp,"%02X", pContract->m_aDestAddress[i]);
			strcat(ipv6, tmp);
			if(i%2 && i<15){
				strcat(ipv6,":");
			}
		}
		delaySec = pContract->m_stSendNoEarlierThan.tv_sec - tvNow.tv_sec;
		if(delaySec < 0){
			delaySec = 0;
		}
		sprintf(buffer, "%5u %s %3lu %3u %5u %6d %6u %6lu %6lu", pContract->m_unContractID, ipv6,
			pContract->m_nRTO, pContract->m_nCrtWndSize,
			pContract->m_unAssignedMaxNSDUSize, pContract->m_nComittedBurst,
			pContract->m_ucMaxSendWindow, delaySec, pContract->m_unPktCount);

		stream << (char *)buffer << std::endl;
	}
}

void Stack_PrintRoutes(std::ostringstream& stream)
{
	int  i;
	char buffer[500];
	char ipv61[40],ipv62[40];
	char tmp1[3], tmp2[3];
	stream << "        IPv6DstAddress                          IPv6NextHopAddress              TTL If" << std::endl;
	stream << "--------------------------------------- --------------------------------------- --- --" << std::endl;

	CNlmeRoutesMap::iterator it = g_stNlme.m_oNlmeRoutesMap.begin();

	for( ; it != g_stNlme.m_oNlmeRoutesMap.end(); it++ )
	{
		CNlmeRoutePtr pRoutePtr = it->second;
		ipv61[0] = ipv62[0] = 0;
		for(i=0; i<16; ++i){
			sprintf(tmp1,"%02X", pRoutePtr->m_aDestAddress[i]);
			sprintf(tmp2,"%02X", pRoutePtr->m_aNextHopAddress[i]);
			strcat(ipv61, tmp1);
			strcat(ipv62, tmp2);
			if(i%2 && i<15){
				strcat(ipv61,":");
				strcat(ipv62,":");
			}
		}
		sprintf(buffer, "%s %s %3u %u", ipv61, ipv62,
			pRoutePtr->m_ucNWK_HopLimit, pRoutePtr->m_bOutgoingInterface);

		stream << (char *)buffer << std::endl;
	}
}

void Stack_PrintKeys(std::ostringstream& stream)
{
	int i = 0;
	char ipv6[40], key[40];
	char tmp1[3], tmp2[3];
	char buffer[500];
	stream << "        IPv6PeerAddress                 SPort DPort  ID U P                   Key                   ValidNotBefore"<< std::endl;
	stream << "--------------------------------------- ----- ----- --- - - --------------------------------------- --------------"<< std::endl;

	extern CSlmeKeysCategoryMap g_oSlmeKeysCatMap;
	CSlmeKeysCategoryMap::iterator itMap = g_oSlmeKeysCatMap.begin();

	for (; itMap != g_oSlmeKeysCatMap.end(); itMap++)
	{
		CSlmeKeysListPtr pList = itMap->second;
		CSlmeKeysList::iterator itList = pList->begin();

		for (; itList != pList->end(); itList++ )
		{
			CSlmeKeyPtr pKey = *itList;
			ipv6[0] = key[0] = 0;
			for(i=0; i<16; ++i){
				sprintf(tmp1,"%02X", pKey->m_aPeerIPv6Address[i]);
				sprintf(tmp2,"%02X", pKey->m_aKey[i]);
				strcat(ipv6, tmp1);
				strcat(key, tmp2);
				if(i%2 && i<15){
					strcat(ipv6,":");
					strcat(key,":");
				}
			}
			sprintf(buffer, "%s %5d %5d %3d %1d %1d %s %08lX",
				ipv6, pKey->m_unUdpSPort, pKey->m_unUdpDPort, pKey->m_ucKeyID, pKey->m_ucUsage, pKey->m_ucPolicy, key, pKey->m_ulValidNotBefore);

			stream << (char *)buffer << std::endl;

			LOG_DEBUG("PrintKeys:" << buffer);
		}
	}
}


void CommandTimeout(uint16 p_ushAppHandle, uint8 p_ucSrcSAP, uint8 p_ucDestSAP, uint8 p_ucSFC)
{
	LOG_DEBUG("TIMEOUT received on message with appHandle=" << (int)p_ushAppHandle <<"!");
	APDU_IDTF idtf;
	uint8 * result = ASLDE_GetMyOriginalTxAPDU(p_ushAppHandle, p_ucSrcSAP, &idtf );

	Isa100::AL::ObjectID::ObjectIDEnum serverObject;
	Isa100::AL::ObjectID::ObjectIDEnum clientObject;

	if (result)
	{
		GENERIC_ASL_SRVC originalService;
		ASLSRVC_GetGenericObject(result, idtf.m_unDataLen, &originalService, NULL);

		Isa100::ASL::PDU::ClientServerPDUPointer apdu;

		switch (originalService.m_ucType)
		{
		case SRVC_READ_REQ:
		{
			NE::Misc::Marshall::NetworkOrderStream stream;
			stream.write((uint8)0); // fecce
			stream.write(p_ucSFC);	// sfc
			stream.write((uint8)0);	// size of response
			serverObject = (Isa100::AL::ObjectID::ObjectIDEnum)originalService.m_stSRVC.m_stReadReq.m_unDstOID;
			clientObject = (Isa100::AL::ObjectID::ObjectIDEnum)originalService.m_stSRVC.m_stReadReq.m_unSrcOID;


			BytesPointer bPtr(new Bytes());
			bPtr->assign(stream.ostream.str().data(), stream.ostream.str().size());

			apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
					Isa100::Common::PrimitiveType::response,
						Isa100::Common::ServiceType::read,
						0,
						0,
						p_ushAppHandle,
						bPtr
					));
			break;
		}
		case SRVC_WRITE_REQ:
		{
			NE::Misc::Marshall::NetworkOrderStream stream;
			stream.write((uint8)0); // fecce
			stream.write(p_ucSFC);	// sfc
			serverObject = (Isa100::AL::ObjectID::ObjectIDEnum)originalService.m_stSRVC.m_stWriteReq.m_unDstOID;
			clientObject = (Isa100::AL::ObjectID::ObjectIDEnum)originalService.m_stSRVC.m_stWriteReq.m_unSrcOID;

			BytesPointer bPtr(new Bytes());
			bPtr->assign(stream.ostream.str().data(), stream.ostream.str().size());

			apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
					Isa100::Common::PrimitiveType::response,
						Isa100::Common::ServiceType::write,
						0,
						0,
						p_ushAppHandle,
						bPtr
					));
			break;
		}
		case SRVC_EXEC_REQ:
		{
			NE::Misc::Marshall::NetworkOrderStream stream;
			stream.write((uint8)0); // fecce
			stream.write(p_ucSFC);	// sfc
			stream.write((uint8)0);	// size of response
			serverObject = (Isa100::AL::ObjectID::ObjectIDEnum)originalService.m_stSRVC.m_stExecReq.m_unDstOID;
			clientObject = (Isa100::AL::ObjectID::ObjectIDEnum)originalService.m_stSRVC.m_stExecReq.m_unSrcOID;

			BytesPointer bPtr(new Bytes());
			bPtr->assign(stream.ostream.str().data(), stream.ostream.str().size());

			apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
					Isa100::Common::PrimitiveType::response,
						Isa100::Common::ServiceType::execute,
						0,
						0,
						p_ushAppHandle,
						bPtr
					));
			break;
		}
		default:
			LOG_WARN("Got unexpected timeout for service type=" << (int)originalService.m_ucType << ".");
			return;
		}


		Address128 destAddress;
		destAddress.loadBinary(idtf.m_aucAddr);

		Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation(new Isa100::ASL::Services::PrimitiveConfirmation(
		        0,
                TransmissionDetailedTime(), //
                0,//transmissionTime,
				0,//forwardCongestionNotification,
				0,//forwardCongestionNotificationEcho,
				//destAddr,
				destAddress, //device->address128,
				(Isa100::Common::TSAP::TSAP_Enum)p_ucDestSAP,//serverTSAP_ID,
				serverObject,//serverObject,
				(Isa100::Common::TSAP::TSAP_Enum)p_ucSrcSAP,//clientTSAP_ID,
				clientObject,//clientObject,
				apdu
		));

		timeouts.push_back(confirmation);
	}
	else
	{
		assert(result && "Should have found the request that has timeouted!!!");
	}
}

void Stack_Init(const NE::Common::Address128& myIPv6, Uint16 port, const NE::Common::Address64& myEUI64,
		const NE::Common::Address64& secManagerEUI64)
{
	////////////////////////////////////////////////////////////////////////////////
	/// C++ linkage methods

	/// Initializes the global variables from this file to the proper values
	/// Start listening on p_ushPort UDP
	/// Return true if init was successfull,
	/// Return false on error (example: bind error)
	/// LibIsaInit failure is FATAL error, the application should stop in this case
	//	bool LibIsaInit(const uint8* p_aOwnIPv6, const uint8* p_aSMIPv6, unsigned short p_ushPort, uint8 p_ucPingTimeoutConf);
	LogCallback_SetFunc( &CallbackLog );	// The callback should be initialized before any LOG

	if( SmSettingsLogic::instance().logLevelStack > 0 )	/// use config.ini [SYSTEM_MANAGER].
	{	if( SmSettingsLogic::instance().logLevelStack >=3){
			LogCallback_SetLevel( LOGLVL_DBG );
		} else {
			LogCallback_SetLevel( (LOGLVL)SmSettingsLogic::instance().logLevelStack );
		}
	} else if ( LOG_DEBUG_ENABLED()) {
		LogCallback_SetLevel(LOGLVL_DBG);
	} else if (LOG_INFO_ENABLED()){
		LogCallback_SetLevel(LOGLVL_INF);
	} else {
		LogCallback_SetLevel(LOGLVL_ERR);
	}

	if (!LibIsaInit((const uint8*)myIPv6.value, NULL, (unsigned short)port, myEUI64.value, secManagerEUI64.value, NULL, 0, DEVICE_TYPE_SM, 0, SmSettingsLogic::instance().currentUTCAdjustment))	{
		LOG_ERROR("CANNOT START STACK on port " << port << "!!!");
		exit(2);
	}

	LibIsaConfig(
	            0, /// @param p_ucSMLinkTimeoutConf - the detection timeout limit for UNJOINing the stack's DMAP. The detection timeout is measured but cannot get lower than this.
	            3, /// @param p_ucMaxRetries - how many retries should the stack do when trying to send reliable requests
	            3 /// @param p_unRetryTimeout - retry timer to use when something happens to the RTO in the contract (not really useful: no contract = no sending !)
                );

	LibIsaRegisterCallbacks(CommandTimeout);

	SLME_Init();

	if (Isa100::Common::SmSettingsLogic::instance().disableTLEncryption){
		Stack_SetTLSecurityLevel(TLSecurityNone);
	} else{
		Stack_SetTLSecurityLevel(TLSecurityENC_MIC_32);
	}

	LOG_DEBUG("Initialized UDP Stack with IPv6=" << myIPv6.toString() <<", port=" << port);
}

void Stack_Release()
{
	LibIsaShutdown();
	LOG_DEBUG("Released UDP Stack...");
}

void Stack_SetTLSecurityLevel(TLSecurityLevel securityLevel)
{
	switch (securityLevel)
	{
	case TLSecurityNone: g_stDSMO.m_ucTLSecurityLevel = SECURITY_NONE; break;
	case TLSecurityMIC_32: g_stDSMO.m_ucTLSecurityLevel = SECURITY_MIC_32; break;
	case TLSecurityENC_MIC_32: g_stDSMO.m_ucTLSecurityLevel = SECURITY_ENC_MIC_32; break;
	}
}


Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_response(const Isa100::ASL::Services::PrimitiveResponsePointer& message)
{
    Isa100::Stats::Cmds::logInfo(message);
	GENERIC_ASL_SRVC service;
	//HACK:[andy] - hold reference to the PDU pointers until Stack copies message content
	Isa100::ASL::PDU::ReadResponsePDUPointer readRespBuf;
	Isa100::ASL::PDU::WriteResponsePDUPointer writeRespBuf;
	Isa100::ASL::PDU::ExecuteResponsePDUPointer execRespBuf;

	uint8 result = 0xFF;
	if (message->forceUnencrypted)
	{
		result = ASLSRVC_AddGenericObjectPlain(GetGenericObject(message, service, readRespBuf, writeRespBuf, execRespBuf),
				GetGenericObjectService(message),
				(uint8)message->priority,
				(uint8)message->serverTSAP_ID,
				(uint8)message->clientTSAP_ID,	//TODO check if these are ok reversed, since it is a response
//				MAX_PACKET_RETRIES,
//				RETRY_TIMEOUT,
				message->apduResponse->appHandle,
				(const uint8*) NULL,
				(uint16)message->contractID,
				0,	// bin size = 0, create binary in stack
				Isa100::Common::SmSettingsLogic::instance().obeyContractBandwidth ? 1 : 0, // obey contract bandwith
                0, //p_ucNoRspExpected
                Isa100::Common::SmSettingsLogic::instance().maxASLTimeout //MAX_ASL_TIMEOUT //p_unAPDULifetime
                );
	}
	else
	{

		result = ASLSRVC_AddGenericObject(GetGenericObject(message, service, readRespBuf, writeRespBuf, execRespBuf),
				GetGenericObjectService(message),
				(uint8)message->priority,
				(uint8)message->serverTSAP_ID,
				(uint8)message->clientTSAP_ID,	//TODO check if these are ok reversed, since it is a response
//				MAX_PACKET_RETRIES,
//				RETRY_TIMEOUT,
				message->apduResponse->appHandle,
				(const uint8*) NULL,
				(uint16)message->contractID,
				0,	// bin size = 0, create binary in stack
				Isa100::Common::SmSettingsLogic::instance().obeyContractBandwidth ? 1 : 0, // obey contract bandwith
                0, //p_ucNoRspExpected
                Isa100::Common::SmSettingsLogic::instance().maxASLTimeout //MAX_ASL_TIMEOUT //p_unAPDULifetime
                );
	}
	LOG_DEBUG("Enqueued message in stack with result=" << (int)result);

	return (Isa100::Common::Objects::SFC::SFCEnum)result;
}

Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_request(const Isa100::ASL::Services::PrimitiveRequestPointer& message)
{
    Isa100::Stats::Cmds::logInfo(message);
	GENERIC_ASL_SRVC service;
	//hold reference to the PDU pointers until Stack copies message content
	Isa100::ASL::PDU::ReadRequestPDUPointer readReqBuf;
	Isa100::ASL::PDU::WriteRequestPDUPointer writeReqBuf;
	Isa100::ASL::PDU::ExecuteRequestPDUPointer execReqBuf;

	LOG_DEBUG("AppHandle passed to stack is:" << (int) message->apduRequest->appHandle);

	uint8 result = 0xFF;
	if (message->forceUnencrypted)
	{
		result = ASLSRVC_AddGenericObjectPlain(GetGenericObject(message, service, readReqBuf, writeReqBuf, execReqBuf),
				GetGenericObjectService(message),
				(uint8)message->priority,
				(uint8)message->clientTSAP_ID,
				(uint8)message->serverTSAP_ID,
//				MAX_PACKET_RETRIES,
//				RETRY_TIMEOUT,
				message->apduRequest->appHandle,
				(const uint8*) NULL,
				(uint16)message->contractID,
				0,	// bin size = 0, create binary in stack
				Isa100::Common::SmSettingsLogic::instance().obeyContractBandwidth ? 1 : 0, // obey contract bandwith
				0, //p_ucNoRspExpected
				Isa100::Common::SmSettingsLogic::instance().maxASLTimeout //MAX_ASL_TIMEOUT //p_unAPDULifetime
                );
	}
	else
	{
		result = ASLSRVC_AddGenericObject(GetGenericObject(message, service, readReqBuf, writeReqBuf, execReqBuf),
				GetGenericObjectService(message),
				(uint8)message->priority,
				(uint8)message->clientTSAP_ID,
				(uint8)message->serverTSAP_ID,
//				MAX_PACKET_RETRIES,
//				RETRY_TIMEOUT,
				message->apduRequest->appHandle,
				(const uint8*) NULL,
				(uint16)message->contractID,
				0,	// bin size = 0, create binary in stack
				Isa100::Common::SmSettingsLogic::instance().obeyContractBandwidth ? 1 : 0, // obey contract bandwith
                0, //p_ucNoRspExpected
                Isa100::Common::SmSettingsLogic::instance().maxASLTimeout //MAX_ASL_TIMEOUT //p_unAPDULifetime
                );
	}
	LOG_DEBUG("Enqueued message in stack with result=" << (int)result);

	return (Isa100::Common::Objects::SFC::SFCEnum)result;
}

Isa100::Common::Objects::SFC::SFCEnum Stack_EnqueueMessage_alertAck(const Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer& message)
{
    Isa100::Stats::Cmds::logInfo(message);
	GENERIC_ASL_SRVC service;

	uint8 result = ASLSRVC_AddGenericObject(GetGenericObject(message, service),
			GetGenericObjectService(message),
			(uint8)message->priority,
			(uint8)message->sourceTSAP,
			(uint8)message->destinationTSAP,
//			MAX_PACKET_RETRIES,
//			RETRY_TIMEOUT,
			message->alertAcknowledge->appHandle,
			(const uint8*) NULL,
			(uint16)message->contractID,
			0,	// bin size = 0, create binary in stack
			Isa100::Common::SmSettingsLogic::instance().obeyContractBandwidth ? 1 : 0, // obey contract bandwith
            0, //p_ucNoRspExpected
            Isa100::Common::SmSettingsLogic::instance().maxASLTimeout //MAX_ASL_TIMEOUT //p_unAPDULifetime
            );

	LOG_DEBUG("Enqueued message in stack with result=" << (int)result);

	return (Isa100::Common::Objects::SFC::SFCEnum)result;
}

Isa100::Common::Objects::SFC::SFCEnum
    Stack_EnqueueMessage_alertReport(const Isa100::ASL::Services::ASL_AlertReport_RequestPointer& message) {

    Isa100::Stats::Cmds::logInfo(message);
    GENERIC_ASL_SRVC service;

    Isa100::ASL::PDU::AlertReportPDUPointer alertReport = Isa100::ASL::PDUUtils::extractAlertReport(message->alertReport);

    uint8 result = ASLSRVC_AddGenericObject(GetGenericObject(message, service, alertReport),
            GetGenericObjectService(message),
            (uint8)message->priority,
            (uint8)message->armoTSAP,
            (uint8)message->sinkTSAP,
//          MAX_PACKET_RETRIES,
//          RETRY_TIMEOUT,
            message->alertReport->appHandle,
            (const uint8*) NULL,
            (uint16)message->contractID,
            0,  // bin size = 0, create binary in stack
            Isa100::Common::SmSettingsLogic::instance().obeyContractBandwidth ? 1 : 0, // obey contract bandwith
            0, //p_ucNoRspExpected
            Isa100::Common::SmSettingsLogic::instance().maxASLTimeout //MAX_ASL_TIMEOUT //p_unAPDULifetime
            );

	if( result != SFC_SUCCESS )
	{
		LOG_ERROR("ASLSRVC_AddGenericObject retcode " << (int)result);
	}

    return (Isa100::Common::Objects::SFC::SFCEnum)result;
}

void Stack_Run()
{
	ASLDE_ASLTask();
}

void Stack_OneSecondTasks()
{
	ASLDE_PerformOneSecondOperations();
}


#define MICROSEC_IN_SEC 1000000
// Return the uSec difference between the two parameters
static inline double elapsed_usec( struct timeval& p_tStart, struct timeval& p_tEnd )
{
    return (double)MICROSEC_IN_SEC * (p_tEnd.tv_sec - p_tStart.tv_sec) + (double)(p_tEnd.tv_usec - p_tStart.tv_usec);
}

void Stack_ParseResponse(APDU_IDTF& idtf, const GENERIC_ASL_SRVC& service, Isa100::AL::ProcessMessages &processMessages,
            const Isa100::AL::ProcessPointer &processPointer, int nElapsedMSec, Uint32 currentTime)
{
    Address128 networkAddress;
    networkAddress.loadBinary(idtf.m_aucAddr);

    TransmissionDetailedTime detailedTxTime;
    detailedTxTime.tv_sec = idtf.m_tvTxTime.tv_sec;
    detailedTxTime.tv_usec = idtf.m_tvTxTime.tv_usec;

	TransmissionTime transmissionTime = idtf.m_ucTransportTime;
	Uint8 forwardCongestionNotification = 0;
	Uint8 forwardCongestionNotificationEcho = 0;
	Isa100::Common::TSAP::TSAP_Enum serverTSAP_ID;
	Isa100::Common::TSAP::TSAP_Enum clientTSAP_ID;
	ObjectID::ObjectIDEnum serverObject;
	ObjectID::ObjectIDEnum clientObject;
	Isa100::ASL::PDU::ClientServerPDUPointer apdu;

	BytesPointer bPtr(new Bytes());

	uint8 reqID;

	//record device activity
	Address32 destAddress32 = Isa100::Model::EngineProvider::getEngine()->getAddress32(networkAddress);
	Device * destDevice = Isa100::Model::EngineProvider::getEngine()->getDevice(destAddress32);
	if (destDevice) {
//	    destDevice->lastPacketTAI = NE::Common::ClockSource::getTAI(Isa100::Common::SmSettingsLogic::instance());
		//Can use aproximated time for speed. lastPacketTAI is used for detection of inactice devices. It permits +/- 1 sec.
	    destDevice->lastPacketTAI = NE::Common::ClockSource::getAproxTAI(Isa100::Common::SmSettingsLogic::instance());
	} else {
	    LOG_WARN("received packet for unknown device: " << networkAddress.toString());
	}

	switch (service.m_ucType)
	{
	case (uint8)SRVC_PUBLISH:	// ok, send through indicate
	{

        //check for too large packets - discard them
        if (service.m_stSRVC.m_stPublish.m_unSize > 0x7FFF) { //data size is on max 15 bits
            LOG_ERROR("SRVC_PUBLISH fn: " << (int) service.m_stSRVC.m_stPublish.m_ucFreqSeqNo << " - data size="
                        << (int) service.m_stSRVC.m_stPublish.m_unSize << " too large. Packet discarded.");
            return;
        }

		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stPublish.m_unPubOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stPublish.m_unSubOID;

		bPtr->assign(service.m_stSRVC.m_stPublish.m_pData, service.m_stSRVC.m_stPublish.m_unSize);

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::request,
					Isa100::Common::ServiceType::publish,
					serverObject,
					clientObject,
					service.m_stSRVC.m_stPublish.m_ucFreqSeqNo,
					bPtr
				));

		Isa100::ASL::Services::ASL_Publish_IndicationPointer publish(
				new Isa100::ASL::Services::ASL_Publish_Indication(
				            nElapsedMSec,
				            transmissionTime,
                            clientTSAP_ID,
                            clientObject,
                            networkAddress,
                            serverTSAP_ID,
                            serverObject,
                            apdu));

		processMessages.IndicatePublishCallbackFn(publish, processPointer, currentTime);
		return;
	}
	break;
	case (uint8)SRVC_READ_REQ:	// ok
	{
		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stReadReq.m_unDstOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stReadReq.m_unSrcOID;

		Isa100::Common::Objects::ExtensibleAttributeIdentifier attrId(
				(Isa100::Common::Objects::AttributeType)service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_ucAttrFormat,
				(Uint16)service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID,
				(Uint16)service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex1,
				(Uint16)service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex2
				);

		//HACK serialize so that somebody can unserialize the above attrId
		NE::Misc::Marshall::NetworkOrderStream stream;
		attrId.marshall(stream);
		bPtr->assign(stream.ostream.str().data(), stream.ostream.str().size());

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::request,
					Isa100::Common::ServiceType::read,
					clientObject,
					serverObject,
					service.m_stSRVC.m_stReadReq.m_ucReqID,
					bPtr
				));
	}
	break;
	case (uint8)SRVC_READ_RSP:	// ok
	{

        //check for too large packets - discard them
        if (service.m_stSRVC.m_stReadRsp.m_unLen > 0x7FFF) { //data size is on max 15 bits
            LOG_ERROR("SRVC_READ_RSP reqID: " << (int) service.m_stSRVC.m_stReadRsp.m_ucReqID << " - data size="
                        << (int) service.m_stSRVC.m_stReadRsp.m_unLen << " too large. Packet discarded.");
            return;
        }

		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stReadRsp.m_unSrcOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stReadRsp.m_unDstOID;
		forwardCongestionNotificationEcho = service.m_stSRVC.m_stReadRsp.m_ucFECCE;


		bPtr->assign(&service.m_stSRVC.m_stReadRsp.m_ucFECCE, 2); // fecce & sfc
		WriteCompressedSize(service.m_stSRVC.m_stReadRsp.m_unLen, bPtr);
		bPtr->append(service.m_stSRVC.m_stReadRsp.m_pRspData, service.m_stSRVC.m_stReadRsp.m_unLen);

		uint16 originalLen = 0;
		uint16 originalHandle = 0;

		uint8* result = ASLDE_SearchOriginalRequest(service.m_stSRVC.m_stReadRsp.m_ucReqID, serverObject, clientObject, &idtf, &originalLen, &originalHandle);


		if (!result)
		{
			LOG_WARN("Original request for read response with reqID=" << (int)service.m_stSRVC.m_stReadRsp.m_ucReqID <<" not found!");
			return;
		}

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::response,
					Isa100::Common::ServiceType::read,
					serverObject,
					clientObject,
					originalHandle,
					bPtr
				));
	}
	break;
	case (uint8)SRVC_WRITE_REQ:	// ok
	{

        //check for too large packets - discard them
        if (service.m_stSRVC.m_stWriteReq.m_unLen > 0x7FFF) { //data size is on max 15 bits
            LOG_ERROR("SRVC_WRITE_REQ reqID: " << (int) service.m_stSRVC.m_stWriteReq.m_ucReqID << " - data size="
                        << (int) service.m_stSRVC.m_stWriteReq.m_unLen << " too large. Packet discarded.");
            return;
        }

		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stWriteReq.m_unDstOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stWriteReq.m_unSrcOID;

		Isa100::Common::Objects::ExtensibleAttributeIdentifier attrId(
				(Isa100::Common::Objects::AttributeType)service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_ucAttrFormat,
				(Uint16)service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unAttrID,
				(Uint16)service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex1,
				(Uint16)service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex2
				);

		//HACK serialize so that somebody can unserialize the above attrId
		NE::Misc::Marshall::NetworkOrderStream stream;
		attrId.marshall(stream);

		WriteCompressedSize(service.m_stSRVC.m_stWriteReq.m_unLen, stream);

		stream.ostream.write(service.m_stSRVC.m_stWriteReq.p_pReqData, service.m_stSRVC.m_stWriteReq.m_unLen);
		bPtr->assign(stream.ostream.str().data(), stream.ostream.str().size());

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::request,
					Isa100::Common::ServiceType::write,
					clientObject,
					serverObject,
					service.m_stSRVC.m_stWriteReq.m_ucReqID,
					bPtr
				));
	}
	break;
	case (uint8)SRVC_WRITE_RSP:	// ok
	{
		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stWriteRsp.m_unSrcOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stWriteRsp.m_unDstOID;
		reqID = service.m_stSRVC.m_stWriteRsp.m_ucReqID;
		forwardCongestionNotificationEcho = service.m_stSRVC.m_stWriteRsp.m_ucFECCE;

		bPtr->assign((unsigned char*)(&service.m_stSRVC) + 5 , sizeof(service.m_stSRVC.m_stWriteRsp) - 5);

		uint16 originalLen = 0;
		uint16 originalHandle = 0;

		uint8* result = ASLDE_SearchOriginalRequest(service.m_stSRVC.m_stWriteRsp.m_ucReqID, serverObject, clientObject, &idtf, &originalLen, &originalHandle);

		if (!result)
		{
			LOG_WARN("Original request for write response with reqID=" << (int)service.m_stSRVC.m_stWriteRsp.m_ucReqID <<" not found!");
			return;
		}

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::response,
					Isa100::Common::ServiceType::write,
					serverObject,
					clientObject,
					originalHandle,
					bPtr
				));
	}
	break;
	case (uint8)SRVC_EXEC_REQ:	// ok
	{
		//check for too large packets - discard them
	    if (service.m_stSRVC.m_stExecReq.m_unLen > 0x7FFF) { //data size is on max 15 bits
	        LOG_ERROR("SRVC_EXEC_REQ reqID: " << (int) service.m_stSRVC.m_stExecReq.m_ucReqID << " - data size="
	                    << (int) service.m_stSRVC.m_stExecReq.m_unLen << " too large. Packet discarded.");
	        return;
	    }

	    serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stExecReq.m_unDstOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stExecReq.m_unSrcOID;
		forwardCongestionNotificationEcho = service.m_stSRVC.m_stExecRsp.m_ucFECCE;

		bPtr->assign(&service.m_stSRVC.m_stExecReq.m_ucMethID, 1); // methodID
		WriteCompressedSize(service.m_stSRVC.m_stExecReq.m_unLen, bPtr); //append data size
		bPtr->append(service.m_stSRVC.m_stExecReq.p_pReqData, service.m_stSRVC.m_stExecReq.m_unLen);

		LOG_DEBUG("SRVC_EXEC_REQ m_unLen=" << (int) service.m_stSRVC.m_stExecReq.m_unLen);

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::request,
					Isa100::Common::ServiceType::execute,
					clientObject,
					serverObject,
					service.m_stSRVC.m_stExecReq.m_ucReqID,
					bPtr
				));
	}
	break;
	case (uint8)SRVC_EXEC_RSP:	// ok
	{
        //check for too large packets - discard them
        if (service.m_stSRVC.m_stExecRsp.m_unLen > 0x7FFF) { //data size is on max 15 bits
            LOG_ERROR("SRVC_EXEC_RSP reqID: " << (int) service.m_stSRVC.m_stExecRsp.m_ucReqID << " - data size="
                        << (int) service.m_stSRVC.m_stExecRsp.m_unLen << " too large. Packet discarded.");
            return;
        }

		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stExecRsp.m_unSrcOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stExecRsp.m_unDstOID;


		bPtr->assign(&service.m_stSRVC.m_stExecRsp.m_ucFECCE, 2); // fecce & sfc
		WriteCompressedSize(service.m_stSRVC.m_stExecRsp.m_unLen, bPtr); //append data size
		bPtr->append(service.m_stSRVC.m_stExecRsp.p_pRspData, service.m_stSRVC.m_stExecRsp.m_unLen);

		uint16 originalLen = 0;
		uint16 originalHandle = 0;

		uint8* result = ASLDE_SearchOriginalRequest(service.m_stSRVC.m_stExecRsp.m_ucReqID, serverObject, clientObject, &idtf, &originalLen, &originalHandle);

		if (!result)
		{
			LOG_WARN("Original request for execute response with reqID=" << (int)service.m_stSRVC.m_stExecRsp.m_ucReqID <<" not found!");
			return;
		}

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::response,
					Isa100::Common::ServiceType::execute,
					serverObject,
					clientObject,
					originalHandle,
					bPtr
				));
	}
	break;
	case (uint8)SRVC_ALERT_REP:	// ok
	{
        //check for too large packets - discard them
        if (service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unSize > 0x7FFF) { //data size is on max 15 bits
          LOG_ERROR("SRVC_ALERT_REP reqID: " << (int) service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucID << " - data size="
                      << (int) service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unSize << " too large. Packet discarded.");
          return;
        }

		serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
		clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stAlertRep.m_unDstOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stAlertRep.m_unSrcOID;


		bPtr->assign(&service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucID, 1); //alert id
		Write(service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unDetObjTLPort, bPtr); //detectingObjectTransportLayerPort
		Write(service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unDetObjID, bPtr); //detectingObjectID
		Write((uint32) service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_stDetectionTime.m_ulSeconds, bPtr); //detectionTime
		Write(service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_stDetectionTime.m_unFract, bPtr); //detectionTime

		Uint8 octet = service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucClass << 7;
		octet |= service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucDirection << 6;
		octet |= service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucCategory << 4;
		octet |= service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucPriority;
		bPtr->append(1, octet); //alertClass + alarmDirection + alertCategory + alertPriority

		bPtr->append(&service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucType, 1); //alertType
		WriteCompressedSize(service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unSize, bPtr); //alertValueSize
		bPtr->append(service.m_stSRVC.m_stAlertRep.m_pAlertValue, service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unSize);

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::request,
					Isa100::Common::ServiceType::alertReport,
					clientObject,
					serverObject,
					service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucID,
					bPtr
				));

		Isa100::ASL::Services::ASL_AlertReport_IndicationPointer alert(
				new Isa100::ASL::Services::ASL_AlertReport_Indication(
				            nElapsedMSec,
				            transmissionTime,
				            clientTSAP_ID,
				            clientObject,
				            networkAddress,
				            serverTSAP_ID,
				            serverObject,
				            apdu));

		processMessages.IndicateAlertReportCallbackFn(alert, processPointer);
		return;
	}
	break;
	case (uint8)SRVC_ALERT_ACK:	// ok
	{
	    serverTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucDstTSAPID;
	    clientTSAP_ID = (Isa100::Common::TSAP::TSAP_Enum)idtf.m_ucSrcTSAPID;
		serverObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stAlertAck.m_unDstOID;
		clientObject = (Isa100::AL::ObjectID::ObjectIDEnum) service.m_stSRVC.m_stAlertAck.m_unSrcOID;

		apdu.reset(new Isa100::ASL::PDU::ClientServerPDU(
				Isa100::Common::PrimitiveType::request,
					Isa100::Common::ServiceType::alertAcknowledge,
					serverObject,
					clientObject,
					service.m_stSRVC.m_stAlertAck.m_ucAlertID,
					bPtr //empty
				));

	    Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer alertAck(
	                new Isa100::ASL::Services::ASL_AlertAcknowledge_Indication(
	                            transmissionTime,
	                            networkAddress,
	                            serverTSAP_ID,
	                            serverObject,
	                            clientTSAP_ID,
	                            clientObject,
	                            apdu));

        processMessages.IndicateAlertAckCallbackFn(alertAck, processPointer);
        return;
	}
	break;
	default:
		LOG_ERROR("Unable to parse response with service type=" << (int)service.m_ucType);
		return;

	}

	if (Stack_isRequest(service)) {

		Isa100::ASL::Services::PrimitiveIndicationPointer indication(new Isa100::ASL::Services::PrimitiveIndication(
		        nElapsedMSec,
                detailedTxTime,
				transmissionTime,
				forwardCongestionNotification,
				serverTSAP_ID,
				serverObject,
				networkAddress,
				clientTSAP_ID,
				clientObject,
				apdu
		));

		processMessages.IndicateCallbackFn( indication, processPointer, currentTime);
	}
	else {

		Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation(new Isa100::ASL::Services::PrimitiveConfirmation(
		        nElapsedMSec,
                detailedTxTime,
				transmissionTime,
				forwardCongestionNotification,
				forwardCongestionNotificationEcho,
				networkAddress,
				serverTSAP_ID,
				serverObject,
				clientTSAP_ID,
				clientObject,
				apdu
		));



		processMessages.ConfirmCallbackFn(confirmation, processPointer);

	}
}


void Stack_WaitForAPDU(int tsapID, int timeoutUsec, Isa100::AL::ProcessMessages &processMessages,  const Isa100::AL::ProcessPointer &processPointer, Uint32 currentTime)
{
	APDU_IDTF apduIdentifier;

	uint8 * result = ASLDE_GetMyAPDU(tsapID,
			timeoutUsec,
			&apduIdentifier);

	if (result)	// have APDU
	{
		LOG_DEBUG("Packet received...");

		uint8* currentPosition = result;
		uint16 dataLen = apduIdentifier.m_unDataLen;
		const uint8* newPosition = 0;

		GENERIC_ASL_SRVC service;

		memset(&service, 0, sizeof(GENERIC_ASL_SRVC));

	    //compute travel time micro seconds
	    struct timeval now;
	    gettimeofday(&now, NULL);
	    apduIdentifier.m_tvTxTime.tv_sec -= ClockSource::get_UTC_TAI_Offset(); /// The stack returns TAI in this field. Convert it to unix time
	    int nElapsedMSec = (int) elapsed_usec(apduIdentifier.m_tvTxTime, now) / 1000;

	    char szTravel[32];
	    const char * szPrefix = "";
	    int nWide = 2;
	    if( nElapsedMSec < 0 )  /// Have the sign in the right place: before the dot
	    {   nElapsedMSec = -nElapsedMSec;
	        szPrefix="-";
	        nWide = 1;
	    }
	    sprintf( szTravel, "%s%*d.%03d", szPrefix, nWide, nElapsedMSec/1000, nElapsedMSec%1000);
	    //    LOG("APDU(s):%s:%u Dtsap:%d Prio+Flg:%02X Len:%3d Travel:%s", GetHex( apduIdentifier.m_aucAddr, sizeof(apduIdentifier.m_aucAddr)),
	    //                apduIdentifier.m_ucSrcTSAPID, apduIdentifier.m_ucDstTSAPID, apduIdentifier.m_ucPriorityAndFlags, apduIdentifier.m_unDataLen, szTravel );
	    Address128 networkAddress;
	    networkAddress.loadBinary(apduIdentifier.m_aucAddr);
	    LOG_INFO("Packet from " << networkAddress.toString() << " TSAP: " << (int) apduIdentifier.m_ucSrcTSAPID << "->" << (int) apduIdentifier.m_ucDstTSAPID
	                << " travel: " << szTravel);

	    if( apduIdentifier.m_tvTxTime.tv_usec > 1000000 ) {
	        //LOG_WARN("WARNING APDU TX uSec too BIG: %u usec", apduIdentifier.m_tvTxTime.tv_usec);
	        LOG_WARN("APDU TX uSec too BIG:" << apduIdentifier.m_tvTxTime.tv_usec << "usec");
	    }
	    //

		while (dataLen > 0 && (newPosition = ASLSRVC_GetGenericObject(currentPosition, dataLen, &service, NULL)))
		{
			dataLen -= newPosition - currentPosition;
			currentPosition += newPosition - currentPosition;

			try
			{
				Stack_ParseResponse(apduIdentifier, service, processMessages, processPointer, nElapsedMSec, currentTime);
			}
			catch (std::exception& ex)
			{
				LOG_WARN("Exception occured! ex=" << ex.what());
			}
		}

	}

	for (std::vector<Isa100::ASL::Services::PrimitiveConfirmationPointer>::iterator it = timeouts.begin();
		it != timeouts.end();)
	{
		if ((*it)->clientTSAP_ID == tsapID)
		{
			try
			{
				processMessages.ConfirmCallbackFn(*it, processPointer);
			}
			catch (std::exception& ex)
			{
				LOG_ERROR("Unable to confirm timeout! err=" << ex.what());
			}

			it = timeouts.erase(it);
		}
		else
		{
			++it;
		}
	}
}
