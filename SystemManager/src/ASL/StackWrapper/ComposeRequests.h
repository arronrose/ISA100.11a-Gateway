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
 * ComposeRequests.h
 *
 *  Created on: Mar 11, 2009
 *      Author: Andy
 */

#ifndef COMPOSEREQUESTS_H_
#define COMPOSEREQUESTS_H_

#include "../Services/ASL_Service_PrimitiveTypes.h"
#include "../Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "../Services/ASL_AlertReport_PrimitiveTypes.h"
#include <Common/NETypes.h>
//#include "../TSDUUtils.h"
#include "../PDUUtils.h"
#include <Common/logging.h>

#include <aslde.h>



std::string StreamToString2(Uint8* input, Uint16 len)
{
	std::ostringstream stream;
	for (int i = 0; i < len; i++)
	{
		stream << std::hex << ::std::setw(2) << ::std::setfill('0') << (int)*(input + i) << " ";
	}

	return stream.str();
}


void* GetGenericObject(const Isa100::ASL::Services::PrimitiveResponsePointer& message, GENERIC_ASL_SRVC& service,
		Isa100::ASL::PDU::ReadResponsePDUPointer& readRespBuf, Isa100::ASL::PDU::WriteResponsePDUPointer& writeRespBuf,
		Isa100::ASL::PDU::ExecuteResponsePDUPointer& execRespBuf)
{
	switch (message->apduResponse->serviceInfo)
	{
	case Isa100::Common::ServiceType::read:
		{
			Isa100::ASL::PDU::ReadResponsePDUPointer readResp = Isa100::ASL::PDUUtils::extractReadResponse(message->apduResponse);
			service.m_stSRVC.m_stReadRsp.m_pRspData = (uint8*)readResp->value->data();
			service.m_stSRVC.m_stReadRsp.m_unLen = (uint16)readResp->value->size();
			service.m_stSRVC.m_stReadRsp.m_unSrcOID = (uint16)message->serverObject;
			service.m_stSRVC.m_stReadRsp.m_unDstOID = (uint16)message->clientObject;
			service.m_stSRVC.m_stReadRsp.m_ucReqID = (uint8)message->apduResponse->appHandle;
			service.m_stSRVC.m_stReadRsp.m_ucFECCE = (uint8)message->forwardCongestionNotificationEcho;
			service.m_stSRVC.m_stReadRsp.m_ucSFC = (uint8)readResp->feedbackCode;

			readRespBuf = readResp;
		}
		return (void*)&service.m_stSRVC.m_stReadRsp;

	case Isa100::Common::ServiceType::write:
		{
			Isa100::ASL::PDU::WriteResponsePDUPointer writeResp = Isa100::ASL::PDUUtils::extractWriteResponse(message->apduResponse);
			service.m_stSRVC.m_stWriteRsp.m_unSrcOID = (uint16)message->serverObject;
			service.m_stSRVC.m_stWriteRsp.m_unDstOID = (uint16)message->clientObject;
			service.m_stSRVC.m_stWriteRsp.m_ucSFC = writeResp->feedbackCode;
			service.m_stSRVC.m_stWriteRsp.m_ucReqID = (uint8)message->apduResponse->appHandle;
			service.m_stSRVC.m_stWriteRsp.m_ucFECCE = (uint8)message->forwardCongestionNotificationEcho;

			writeRespBuf = writeResp;
		}
		return (void*)&service.m_stSRVC.m_stWriteRsp;

	case Isa100::Common::ServiceType::execute:
		{
			Isa100::ASL::PDU::ExecuteResponsePDUPointer executeResp = Isa100::ASL::PDUUtils::extractExecuteResponse(message->apduResponse);
			service.m_stSRVC.m_stExecRsp.m_ucFECCE = (uint8)message->forwardCongestionNotificationEcho;
			service.m_stSRVC.m_stExecRsp.m_ucReqID = (uint8)message->apduResponse->appHandle;
			service.m_stSRVC.m_stExecRsp.m_ucSFC = executeResp->feedbackCode;
			service.m_stSRVC.m_stExecRsp.p_pRspData = (uint8*)executeResp->parameters->data();
			service.m_stSRVC.m_stExecRsp.m_unLen = (uint16)executeResp->parameters->size();
			service.m_stSRVC.m_stExecRsp.m_unSrcOID = (uint16)message->serverObject;
			service.m_stSRVC.m_stExecRsp.m_unDstOID = (uint16)message->clientObject;

			execRespBuf = executeResp;
		}
		return (void*)&service.m_stSRVC.m_stExecRsp;

	default:
		return (void*)NULL;
	}
}

void* GetGenericObject(const Isa100::ASL::Services::PrimitiveRequestPointer& message, GENERIC_ASL_SRVC& service,
		Isa100::ASL::PDU::ReadRequestPDUPointer& readReqBuf, Isa100::ASL::PDU::WriteRequestPDUPointer& writeReqBuf,
				Isa100::ASL::PDU::ExecuteRequestPDUPointer& execReqBuf)
{
	switch (message->apduRequest->serviceInfo)
	{
	case Isa100::Common::ServiceType::read:
		{
			Isa100::ASL::PDU::ReadRequestPDUPointer readReq = Isa100::ASL::PDUUtils::extractReadRequest(message->apduRequest);
			service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_ucAttrFormat = (uint8)readReq->targetAttribute.attributeType;
			service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID = (uint16)readReq->targetAttribute.attributeID;
			service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex1 = (uint16)readReq->targetAttribute.oneIndex;
			service.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex2 = (uint16)readReq->targetAttribute.twoIndex;
			service.m_stSRVC.m_stReadReq.m_unSrcOID = (uint16)message->clientObject;
			service.m_stSRVC.m_stReadReq.m_unDstOID = (uint16)message->serverObject;
			service.m_stSRVC.m_stReadReq.m_ucReqID = (uint8)message->apduRequest->appHandle;
			readReqBuf = readReq;
		}
		return (void*)&service.m_stSRVC.m_stReadReq;

	case Isa100::Common::ServiceType::write:
		{
			Isa100::ASL::PDU::WriteRequestPDUPointer writeReq = Isa100::ASL::PDUUtils::extractWriteRequest(message->apduRequest);
			service.m_stSRVC.m_stWriteReq.m_unSrcOID = (uint16)message->clientObject;
			service.m_stSRVC.m_stWriteReq.m_unDstOID = (uint16)message->serverObject;
			service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_ucAttrFormat = (uint8)writeReq->targetAttribute.attributeType;
			service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unAttrID = (uint16)writeReq->targetAttribute.attributeID;
			service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex1 = (uint16)writeReq->targetAttribute.oneIndex;
			service.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex2 = (uint16)writeReq->targetAttribute.twoIndex;
			service.m_stSRVC.m_stWriteReq.m_ucReqID = (uint8)message->apduRequest->appHandle;
			service.m_stSRVC.m_stWriteReq.p_pReqData = (uint8*)writeReq->value->data();
			service.m_stSRVC.m_stWriteReq.m_unLen = (uint16)writeReq->value->size();
			writeReqBuf = writeReq;
		}
		return (void*)&service.m_stSRVC.m_stWriteReq;

	case Isa100::Common::ServiceType::execute:
		{
			Isa100::ASL::PDU::ExecuteRequestPDUPointer execReq = Isa100::ASL::PDUUtils::extractExecuteRequest(message->apduRequest);
			service.m_stSRVC.m_stExecReq.m_ucReqID = (uint8)message->apduRequest->appHandle;
			service.m_stSRVC.m_stExecReq.m_ucMethID = (uint8)execReq->methodID;
			service.m_stSRVC.m_stExecReq.p_pReqData = (uint8*)execReq->parameters->data();
			service.m_stSRVC.m_stExecReq.m_unLen = (uint16)execReq->parameters->size();
			service.m_stSRVC.m_stExecReq.m_unSrcOID = (uint16)message->clientObject;
			service.m_stSRVC.m_stExecReq.m_unDstOID = (uint16)message->serverObject;
			execReqBuf = execReq;
		}
		return (void*)&service.m_stSRVC.m_stExecReq;

	default:
		return (void*)NULL;
	}
}

void* GetGenericObject(const Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer& message, GENERIC_ASL_SRVC& service)
{
	service.m_stSRVC.m_stAlertAck.m_ucAlertID = (uint8)message->alertAcknowledge->appHandle;
	service.m_stSRVC.m_stAlertAck.m_unSrcOID = (uint16)message->sourceObject;
	service.m_stSRVC.m_stAlertAck.m_unDstOID = (uint16)message->destinationObject;
	return (void*)&service.m_stSRVC.m_stAlertAck;

	//return (void*)NULL;
}

void* GetGenericObject(const Isa100::ASL::Services::ASL_AlertReport_RequestPointer& message, GENERIC_ASL_SRVC& service,
            Isa100::ASL::PDU::AlertReportPDUPointer& alertReport)
{
    service.m_stSRVC.m_stAlertRep.m_unSrcOID = (uint16)message->armoObject;
    service.m_stSRVC.m_stAlertRep.m_unDstOID = (uint16)message->sinkObject;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ulNextSendTAI = 0; // TODO ???
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unDetObjTLPort = alertReport->detectingObjectTransportLayerPort;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unDetObjID = alertReport->detectingObjectID;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_stDetectionTime.m_ulSeconds = alertReport->detectionTime.currentTAI;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_stDetectionTime.m_unFract = alertReport->detectionTime.fractionalTAI;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucID = alertReport->alertID;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucClass = alertReport->alertClass;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucDirection = alertReport->alarmDirection;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucCategory = alertReport->alertCategory;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucType = alertReport->alertType;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucPriority = alertReport->alertPriority;
    service.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_unSize = alertReport->alertValueSize;
    service.m_stSRVC.m_stAlertRep.m_pAlertValue = (uint8*) alertReport->alertValue->data();

    return (void*)&service.m_stSRVC.m_stAlertRep;
}

uint8 GetGenericObjectService(const Isa100::ASL::Services::PrimitiveResponsePointer& message)
{
	switch (message->apduResponse->serviceInfo)
	{
	case Isa100::Common::ServiceType::read:
		return SRVC_READ_RSP;

	case Isa100::Common::ServiceType::write:
		return SRVC_WRITE_RSP;

	case Isa100::Common::ServiceType::execute:
		return SRVC_EXEC_RSP;

	default:
		return 0;
	}
}

uint8 GetGenericObjectService(const Isa100::ASL::Services::PrimitiveRequestPointer& message)
{
	switch (message->apduRequest->serviceInfo)
	{
	case Isa100::Common::ServiceType::read:
		return SRVC_READ_REQ;

	case Isa100::Common::ServiceType::write:
		return SRVC_WRITE_REQ;

	case Isa100::Common::ServiceType::execute:
		return SRVC_EXEC_REQ;

	default:
		return 0;
	}
}

uint8 GetGenericObjectService(const Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer& message)
{
	return SRVC_ALERT_ACK;
}

uint8 GetGenericObjectService(const Isa100::ASL::Services::ASL_AlertReport_RequestPointer& message)
{
    return SRVC_ALERT_REP;
}

#endif /* COMPOSEREQUESTS_H_ */
