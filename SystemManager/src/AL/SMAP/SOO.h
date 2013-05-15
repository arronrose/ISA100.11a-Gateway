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
 * SOO_H_.h
 */

#ifndef SOO_H_
#define SOO_H_

#include <time.h>

#include "AL/Isa100Object.h"
#include "AL/ObjectsIDs.h"
#include "Common/logging.h"
#include "Model/Operations/IEngineOperation.h"
#include "Model/Operations/IOperationsSender.h"
#include "Model/IDeletedDeviceListener.h"
#include <map>
#include <list>

namespace Isa100 {
namespace AL {
namespace SMAP {

/**
 * Send Operations Object.
 * @author Beniamin Tecar, radu.pop, catalin.pop
 */
class SOO : public Isa100Object,
            public NE::Model::Operations::IOperationsSender,
            public NE::Model::IDeletedDeviceListener {
    LOG_DEF("I.A.S.SOO");

    typedef Uint64 OperationReqID; //[Uint32](Address32) + [ReqID]
    typedef std::map<OperationReqID, NE::Model::Operations::IEngineOperationPointer> ReqIDsMap;


    ReqIDsMap reqIDsMap;


    public:

        SOO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~SOO();

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const {
            return Isa100::AL::ObjectID::ID_SOO;
        }

        void execute(Uint32 currentTime);

        bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication){
            return false;
        }

        bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        bool sendOperation(NE::Model::Operations::IEngineOperationPointer& operation);

        void deviceDeletedCallback(Address32 deletedDevAddr32, Uint16 deviceType);

    protected:

        bool isJobFinished() {
            return false;
        }

        void confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation);

        void processConfirm(const Address128& serverAddress32, AppHandle requestID,
                    Isa100::Common::Objects::SFC::SFCEnum feedbackCode, const BytesPointer& value);

    private:

        OperationReqID createOperationReqID(Address32 address32, AppHandle appHandle) const {
            OperationReqID operationReqID = address32;
            operationReqID = operationReqID << 32;
            operationReqID |= appHandle;
            return operationReqID;
        }
        void unmarshallReadResponse(const BytesPointer& value, NE::Model::Operations::IEngineOperationPointer operation);

        void setStackManagerSettings(NE::Model::Operations::IEngineOperationPointer& operation);

};

typedef boost::shared_ptr<SOO> SOOPointer;

}
}
}

#endif /* SOO_H_ */
