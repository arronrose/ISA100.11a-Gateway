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
 * MessageDispatcher.cpp
 *
 *  Created on: Mar 16, 2009
 *      Author: Andy
 */

#include "MessageDispatcher.h"
#include <Common/SmSettingsLogic.h>
#include "ProcessesProvider.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/PDU/ClientServerPDU.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "ASL/PDUUtils.h"
#include <Common/NETypes.h>
#include <Stats/Cmds.h>
///-------------------------
#include "Common/NEException.h"
#include "Common/Utils/ContractUtils.h"
#include "Model/Isa100EngineOperations.h"
#include "ASL/PDU/ClientServerPDU.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "ASL/StackWrapper.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>


MessageDispatcher::MessageDispatcher() :
    requestCallback(boost::bind(&Stack_EnqueueMessage_request, _1)), //
                responseCallback(boost::bind(&Stack_EnqueueMessage_response, _1)), //
                alertAckCallback(boost::bind(&Stack_EnqueueMessage_alertAck, _1)),
                alertReportCallback(boost::bind(&Stack_EnqueueMessage_alertReport, _1)) {

}

MessageDispatcher::MessageDispatcher(StackRequestCallback requestCallback_, StackResponseCallback responseCallback_,
            StackAlertACKCallback alertAckCallback_, StackAlertReportCallback alertReportCallback_) :
    requestCallback(requestCallback_), responseCallback(responseCallback_),
    alertAckCallback(alertAckCallback_), alertReportCallback(alertReportCallback_) {

}
void MessageDispatcher::Request(Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer acknowledgeRequest) {

    alertAckCallback(acknowledgeRequest);
}

void MessageDispatcher::Request(Isa100::ASL::Services::ASL_AlertReport_RequestPointer alertReportRequest) {
    alertReportCallback(alertReportRequest);
}

/**
 * Accepts request from the Application layer.
 * The request will be sent down the stack, or back to the AL (depending on the address).
 */
void MessageDispatcher::Request(Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequest) {
    //if (IsForMe(primitiveRequest->serviceContract)) {
    if (IsForMe(primitiveRequest->destination)) {
        Isa100::ASL::Services::PrimitiveIndicationPointer indication(
                    new Isa100::ASL::Services::PrimitiveIndication(0, TransmissionDetailedTime(), //
                    TransmissionTime(), //
                    0, // forwardCongestionNotification_
                    primitiveRequest->serverTSAP_ID, //
                    primitiveRequest->serverObject, //
                    SmSettingsLogic::instance().managerAddress128, //
                    primitiveRequest->clientTSAP_ID, //
                    primitiveRequest->clientObject, //
                    primitiveRequest->apduRequest //
                    ));

        LoopbackIndicate(indication);
    } else {
        requestCallback(primitiveRequest);
    }
}

/**
 * Accepts response from the Application layer.
 * The request will be sent down the stack, or back to the AL (depending on the address).
 */
void MessageDispatcher::Response(Isa100::ASL::Services::PrimitiveResponsePointer primitiveResponse) {
    //if (IsForMe(primitiveResponse->serviceContract)) {
    if (IsForMe(primitiveResponse->destination)) {
        Isa100::ASL::Services::PrimitiveConfirmationPointer confirm(
                    new Isa100::ASL::Services::PrimitiveConfirmation(0, TransmissionDetailedTime(), //
                    TransmissionTime(), //
                    0, // forwardCongestionNotification_
                    primitiveResponse->forwardCongestionNotificationEcho, //
                    SmSettingsLogic::instance().managerAddress128, //
                    primitiveResponse->serverTSAP_ID, //
                    primitiveResponse->serverObject, //
                    primitiveResponse->clientTSAP_ID, //
                    primitiveResponse->clientObject, //
                    primitiveResponse->apduResponse //
                    ));

        LoopbackConfirm(confirm);
    } else {
        responseCallback(primitiveResponse);
    }
}

void MessageDispatcher::LoopbackIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indicate) {
    Isa100::Stats::Cmds::logInfo(indicate, "LOOPBACK");
    requestsQueue.push(indicate);
}

void MessageDispatcher::LoopbackConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    Isa100::Stats::Cmds::logInfo(confirm, "LOOPBACK");
    responseQueue.push(confirm);
}


void MessageDispatcher::Execute() {
    while (!requestsQueue.empty() || !responseQueue.empty()) {
        if (!requestsQueue.empty()) {
            Isa100::ASL::Services::PrimitiveIndicationPointer indication = requestsQueue.front();
            requestsQueue.pop();

            ProcessPointer & process = Isa100::AL::ProcessesProvider::getProcessByTSAP_ID(indication->serverTSAP_ID);
            if (process) {
                process->indicate(indication);
            }
        } else if (!responseQueue.empty()) {
            Isa100::ASL::Services::PrimitiveConfirmationPointer confirm = responseQueue.front();
            responseQueue.pop();

            ProcessPointer & process = Isa100::AL::ProcessesProvider::getProcessByTSAP_ID(confirm->clientTSAP_ID);
            if (process) {
                process->confirm(confirm);
            }
        }
    }
}
