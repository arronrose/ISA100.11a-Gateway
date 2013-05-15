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
 * MessageDispatcher.h
 *
 *  Created on: Mar 16, 2009
 *      Author: Andy
 */

#ifndef MESSAGEDISPATCHER_H_
#define MESSAGEDISPATCHER_H_

#include <ASL/Services/ASL_Service_PrimitiveTypes.h>
#include <ASL/Services/ASL_AlertAcknowledge_PrimitiveTypes.h>
#include <ASL/Services/ASL_AlertReport_PrimitiveTypes.h>
#include <Model/Isa100EngineOperations.h>
#include <Model/EngineProvider.h>
#include <queue>
#include <Common/logging.h>
#include <boost/function.hpp>

/*
 * Responsible with dispatching a message request to the appropriate destination (the same stack,
 * or down the stack to another destination).
 * Messages that should be looped back to the application layer are queued up and only fired on execute.
 */
class MessageDispatcher {
    LOG_DEF("I.A.MessageDispatcher");

    public:

        typedef boost::shared_ptr<MessageDispatcher> Ptr;

        typedef boost::function1<void, Isa100::ASL::Services::PrimitiveRequestPointer> StackRequestCallback;
        typedef boost::function1<void, Isa100::ASL::Services::PrimitiveResponsePointer> StackResponseCallback;
        typedef boost::function1<void, Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer> StackAlertACKCallback;
        typedef boost::function1<void, Isa100::ASL::Services::ASL_AlertReport_RequestPointer> StackAlertReportCallback;

        MessageDispatcher();
        MessageDispatcher(StackRequestCallback requestCallback, StackResponseCallback responseCallback,
        		StackAlertACKCallback alertAckCallback, StackAlertReportCallback alertReportCallback);

        /**
         * Accepts alert acknowledge from the Application layer.
         * The request will be sent down the stack.
         */
        void Request(Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer acknowledgeRequest);

        /**
         * Accepts alert report from the Application layer.
         * The request will be sent down the stack.
         */
        void Request(Isa100::ASL::Services::ASL_AlertReport_RequestPointer alertReportRequest);

        /**
         * Accepts request from the Application layer.
         * The request will be sent down the stack, or back to the AL (depending on the address).
         */
        void Request(Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequest);

        /**
         * Accepts response from the Application layer.
         * The request will be sent down the stack, or back to the AL (depending on the address).
         */
        void Response(Isa100::ASL::Services::PrimitiveResponsePointer primitiveResponse);

        /**
         *
         */
        void LoopbackIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indicate);

        /**
         *
         */
        void LoopbackConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        /**
         * Executes tasks, sending queued messages back to the stack.
         */
        void Execute();


    private:

        bool IsForMe(Address32 destination){
            return destination == Isa100::Model::EngineProvider::getEngine()->getManagerAddress32();
        }

        std::queue<Isa100::ASL::Services::PrimitiveIndicationPointer> requestsQueue;

        std::queue<Isa100::ASL::Services::PrimitiveConfirmationPointer> responseQueue;

        StackRequestCallback requestCallback;
        StackResponseCallback responseCallback;
        StackAlertACKCallback alertAckCallback;
        StackAlertReportCallback alertReportCallback;

};

#endif /* MESSAGEDISPATCHER_H_ */
