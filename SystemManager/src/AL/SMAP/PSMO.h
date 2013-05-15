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

#ifndef PSMO_H_
#define PSMO_H_

#include <boost/noncopyable.hpp>
#include "Common/logging.h"
#include "AL/Isa100Object.h"
#include "Security/SecuritySymJoinRequest.h"
#include "Security/SecuritySymJoinResponse.h"
#include <Security/SecurityNewSessionRequest.h>
#include <Security/SecurityKeyAndPolicies.h>
#include <ASL/PDU/ExecuteRequestPDU.h>
#include "Model/SecurityKey.h"
#include <Common/NEAddress.h>

namespace Isa100 {
namespace AL {
namespace SMAP {

/**
 * Proxy security management object (PSMO).
 * This object acts as a proxy for the security manager.
 * Conceptually, the system manager can be viewed as including a proxy security management object (PSMO).
 * The PSMO forwards all security related messages between the security manager and the devices in the
 * network that are compliant with this standard.
 * The PSMO can be used by the security manager to access information from other system management objects,
 * such as current TAI time, if necessary.
 * The security manager does not have a valid address as defined by this standard;
 * thus, devices that wish to communicate with the security manager can do by communicating with the PSMO.
 * @author Beniamin Tecar, sorin.bidian
 * @version 1.0
 *
 * @version 2.0 - September 2009 - New SystemManager Design
 */
class PSMO: public Isa100Object, boost::noncopyable {
        LOG_DEF("I.A.S.PSMO");

    private:
        enum PSMOState {
            Initial,
            WaitingForSecManagerJoinResp,
            WaitingForSecJoinConfirmReq,
            WaitingForDMSOConfirm,
            Finished,
            WaitingForSetKeyTo,
            WaitingForSetKeyFrom
        };

        /**
         * The current processed join request.
         */
        Isa100::Security::SecuritySymJoinRequest joinRequest;
        //        Isa100::Model::SecuritySymJoinResponse joinResponse;

        Isa100::Security::SecurityNewSessionRequest newSessionRequest;

        /**
         * Used at new session establishment.
         */
        Isa100::Security::SecurityKeyAndPolicies sessionKeySettingsFrom;
        Isa100::Security::SecurityKeyAndPolicies sessionKeySettingsTo;

        /**
         * Used to send the response when available.
         */
        Isa100::ASL::Services::PrimitiveIndicationPointer joinRequestPrimitive;
        Isa100::ASL::Services::PrimitiveIndicationPointer joinConfirmReqPrimitive;

        Isa100::ASL::Services::PrimitiveIndicationPointer dmsoForwardPrimitive;

        NE::Common::Address128 joiningDeviceAddress128;

        Isa100::ASL::Services::PrimitiveRequestPointer setKeyA;
        bool confirmedSetKeyA;
        Isa100::ASL::Services::PrimitiveRequestPointer setKeyB;
        bool confirmedSetKeyB;
        bool errorOnConfirmKeys;

        Isa100::ASL::Services::PrimitiveRequestPointer setKeyGW;
        bool expectingSetKeyGWResponse;

        PSMOState currentState;

    public:

        PSMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~PSMO();


        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const {
            return Isa100::AL::ObjectID::ID_PSMO;
        }

        virtual bool isJobFinished();

        virtual bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        virtual bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void execute(Uint32 currentTime);

        /**
         *
         */
        void processSecuritySymJoinRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer& executeRequest);

        /**
         * As part of the last step of the join process, the new device uses this method for sending
         * a security confirmation to the security manager.
         */
        void processSecurityConfirmRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer& executeRequest);

        //obsolete
        void processJoinConfirmation(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer& executeRequest);

        void NotifyOfJoinRequest(NE::Common::Address64& joiningAddress);

        void processDMSOFinishOnConfirm();

        void processSecurityNewSessionRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer& executeRequest);

        void processSessionKeyUpdateRequest(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    ASL::PDU::ExecuteRequestPDUPointer& executeRequest);

        // void processConfirmSecurityNewSession(bool keysSetWithSuccess);

        std::string toString();

    protected:

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);


    private:

        void sendResponse(Isa100::ASL::Services::PrimitiveIndicationPointer indication, BytesPointer joinResponsePayload);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseJoinSecurity(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseSecurityConfirm(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseSecurityNewSession(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

        void securitySymJoinResponse(bool status, Isa100::Security::SecuritySymJoinResponse response, int failureReason);

        Isa100::Common::Objects::SFC::SFCEnum sendSessionKey(const Address128& to,
                    Isa100::Security::SecurityKeyAndPolicies& sessionKey, Isa100::ASL::Services::PrimitiveRequestPointer &setKey);


        std::string stateString();
};

}
}
}
#endif /*PSMO_H_*/
