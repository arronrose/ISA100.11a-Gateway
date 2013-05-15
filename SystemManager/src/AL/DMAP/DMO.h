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

#ifndef DMO_H_
#define DMO_H_

#include "AL/Isa100Object.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "Common/NETypes.h"
#include "Common/logging.h"
#include "Model/Isa100EngineOperations.h"
#include "Model/SmJoinRequest.h"

namespace Isa100 {
namespace AL {
namespace DMAP {

/**
 * Device management object (DMO).
 * This object facilitates the management of the device's general device-wide functions.
 * DMO is only a proxy object for the BR | GW join. An instance of DMO will be kept alive only
 * while a request & response sequence is taking place. After the response is received the object can be deleted.
 *
 * @author  Radu Pop, sorin.bidian, beniamin.tecar
 * @version 1.0, 01.16.2008
 *
 * @version 2.0 - September 2009 - New SystemManager Design
 */
class DMO: public Isa100Object {
        LOG_DEF("I.A.D.DMO");

    private:

        /**
         * This address will be set when the first security join is received and is used
         * to match the following requests (join & contract).
         */
        Address64 initialRequestAddress64;

        Isa100::ASL::Services::PrimitiveIndicationPointer responseExpectingIndication;

        bool expectingJoinResponse;

        bool expectingContractJoinResponse;

    public:

        DMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~DMO();

        virtual bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        virtual bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        /**
         * Returns the id of the current object.
         */
        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const;

        /**
         * Sets the values for the attributes from the request list.
         */
        void execute(Uint32 currentTime);

        /**
         * Method that handles the 1st join step - security join request of a new device (Backbone or Gateway).
         */
        void proxySecuritySymJoin(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Method that handles the 2nd join step - join request of a new device (Backbone or Gateway).
         */
        void proxySystemManagerJoin(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Method that handles the 3rd join step - join contract request of a new device (Backbone or Gateway).
         */
        void proxySystemManagerContract(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer executeRequest);

    protected:

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer response);

    private:
        /**
         * Callback function invoked by Physical Model.
         */
        void responseJoinSystemManager(Address32 deviceAddress, int requestID, int status);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseJoinContract(Address32 deviceAddress, int requestID, int status);

        void forwardJoinResponse(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation);

    private:

        /**
         * Flag that is set to mark that the DMO is waiting for a response from PSMO in the security join step.
         */
        bool expectingProxySecurityJoinResponse;

        /**
         * Holds the join request received from a device.
         */
        Isa100::Model::SmJoinRequest smJoinRequest;

};

}
}
}

#endif /*DMO_H_*/
