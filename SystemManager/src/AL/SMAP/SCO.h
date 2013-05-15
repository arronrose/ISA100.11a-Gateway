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

#ifndef SCO_H_
#define SCO_H_

#include "AL/Isa100Object.h"
#include "Common/NEException.h"
#include "Model/Contracts/ContractResponse.h"
#include "Model/ContractRequest.h"
#include "Model/Isa100EngineOperations.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace AL {
namespace SMAP {

namespace SCOActionId {
enum SCOActionId {
    SendContract, // expected response from device
    DeleteContract,
    Set_DllTbl_Route_Entry
};
}

namespace SCOContractOperation {
enum SCOContractOperation {
    Termination = 0, Deactivation = 1, Reactivation = 2
};
}

/**
 * System Configuration Object (SCO).
 * This object facilitates the configuration of the system
 * including contract establishment, modification and termination.
 * @author Ioan v. Pocol, Catalin Pop, flori.parauan, radu.pop, Beniamin Tecar, sorin.bidian
 * @version 2.0, 03.19.2008
 * @version 3.0, D2A
 *
 * @version 4.0 - September 2009 - New SystemManager Design
 */
class SCO: public Isa100Object {
        LOG_DEF("I.A.S.SCO");

    private:

        NE::Model::ContractRequest contractRequest;

    private:

        bool checkContractRequestConstraints(const ContractRequest& contractRequest);

        void createNewContract(const ContractRequest& contractRequest);

        /**
         * Callback function invoked by Physical Model.
         */
        void responseCreateNewContract(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

        void modifyContract(const ContractRequest& contractRequest);
        void contractRenewal(const ContractRequest& contractRequest);
        void terminateContract(const Uint16 contractID);
        /**
         * Send Contract Response after all operations are confirmed.
         */
        void sendContractResponse();


        void sendResponseToRequesterOfContractTermination(Isa100::Common::Objects::SFC::SFCEnum serviceFeedbackCode);

        void setSourceDeviceRoute(const Address128& sourceAddress128, const Address128& destinationAddress128);

    public:

        SCO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~SCO();

        void execute(Uint32 currentTime){}

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const{
            return Isa100::AL::ObjectID::ID_SCO;
        }

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm){
            return false;
        }

    protected:
        void removeContractOfDevice(const Address128& deviceAddress, const ContractId id);
};

}
}
}

#endif /*SCO_H_*/
