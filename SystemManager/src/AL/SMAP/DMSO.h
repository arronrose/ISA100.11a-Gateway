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

#ifndef DMSO_H_
#define DMSO_H_

#include <boost/noncopyable.hpp>
#include "AL/Isa100Object.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "Common/logging.h"
#include "Model/SmJoinRequest.h"
#include "Model/SmJoinResponse.h"
//#include "Model/Isa100EngineOperations.h"
#include "Model/SmJoinContractRequest.h"
#include "Model/Contracts/NewDeviceContractResponse.h"
#include "Common/NETypes.h"

namespace Isa100 {
namespace AL {
namespace SMAP {

/**
 * Device management service object (DMSO).
 * This object facilitates device's join process.
 * The join process consists of 3 major steps : security join request, join request and contract request.
 * The last 2 request are processed by DMSO.<BR>
 *
 * @version 2.0 - September 2009 - New SystemManager Design
 * @author radu.pop, sorin.bidian, beniamin.tecar, catalin.pop
 */
class DMSO: public Isa100Object, boost::noncopyable {
        LOG_DEF("I.A.S.DMSO");

    private:

        /**
         * Flag that is set to mark that the DMO is waiting for a response from PSMO in the security join step.
         */
        bool expectingProxySecurityJoinResponse;

        /**
         * The time of the join process started.
         */
        time_t joinStartTime;

        /**
         * The current processed join request.
         */
        Isa100::Model::SmJoinRequest smJoinRequest;

        /**
         * The current processed contract join request.
         */
        Isa100::Model::SmJoinContractRequest smContractRequest;

        /**
         * Used to send the response when available.
         */
        Isa100::ASL::Services::PrimitiveIndicationPointer joinRequestPrimitive;

        /**
         * Used to send the response when available.
         */
        Isa100::ASL::Services::PrimitiveIndicationPointer joinContractPrimitive;

        /**
         * Used to send the response when available.
         */
        Isa100::ASL::Services::PrimitiveIndicationPointer joinPsmoConfirmPrimitive;

    public:

        DMSO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~DMSO();

        /**
         * This object expects an indicate if there is no previous indicate received
         * or if it has received another indicate from the same Address64.
         */
        bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
            return false;
        }

        /**
         *
         */
        bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm){
            return (expectingProxySecurityJoinResponse
                        ? (confirm->apduResponse->appHandle == indication->apduRequest->appHandle)
                        : false);
        }

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const{
            return Isa100::AL::ObjectID::ID_DMSO;
        }

        void execute(Uint32 currentTime);

        /**
         * Method that handles the 2nd join step - join request of a new device.
         */
        void processSystemManagerJoinRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Method that handles the 3rd join step - join contract request of a new device.
         */
        void processSystemManagerContractRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer executeRequest);

    protected:

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

    private:

        void forwardSecurityJoinResponse(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseJoinSystemManager(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseJoinContract(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

};

}
}
}

#endif /*DMSO_H_*/
