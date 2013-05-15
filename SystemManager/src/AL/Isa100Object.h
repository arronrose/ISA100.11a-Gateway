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

/**
 * @authors catalin.pop, beniamin.tecar, sorin.bidian, andrei.petrut
 */
#ifndef ISA100OBJECT_H_
#define ISA100OBJECT_H_

#include "ObjectsIDs.h"
#include "MessageDispatcher.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "ASL/Services/ASL_Publish_PrimitiveTypes.h"
#include "ASL/Services/ASL_AlertReport_PrimitiveTypes.h"
#include "Common/logging.h"
#include "Common/Objects/SFC.h"
#include "Common/NETypes.h"
#include "Common/smTypes.h"
#include "Model/IEngine.h"
#include "Model/Isa100EngineOperations.h"
#include "Security/SecurityManager.h"
#include "Common/SmSettingsLogic.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace AL {

// Object instance
#define LOG_OI "I:" << objectId << " "

class Isa100Object {
        LOG_DEF("I.A.Isa100Object");

    protected:
        /**
         * Unique ID assigned to each object.
         */
        int objectId;

        /**
         * The time of the last received operation request.
         * Reset to 0 when there is no current operation.
         */
        time_t indicateTimestamp;

        /**
         * The counter of retried times.
         */
        Uint8 indexRetryOperations;

        Isa100::ASL::Services::PrimitiveIndicationPointer indication;

        bool jobFinished;

        /**
         * Set to true when an error occurs (timeout, error response, device rejoin).
         */
//        bool isRollBack;

        int creationTime;

        /**
         * Tsap ID of the process this object is running in.
         */
        Isa100::Common::TSAP::TSAP_Enum const tsap;

    public:
        MessageDispatcher::Ptr messageDispatcher;
        NE::Model::IEngine* engine;
        Isa100::Security::SecurityManager* securityManager;

    public:

        Isa100Object(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~Isa100Object();

        virtual Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const = 0;
        virtual Isa100::Common::TSAP::TSAP_Enum getObjectTSAPId() const{
            return tsap;
        }
        virtual void getObjectIDString(std::string &objectId) const;

        virtual void execute(Uint32 currentTime) = 0;

        virtual void cleanupOnError(){
//            LOG_WARN(LOG_OI << "CleanupOnError() - called from base class! obj=" << getObjectID() << ", objId=" << objectId);
        }

        // indicate
        virtual void indicateObject(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        virtual void indicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        virtual void indicate(Isa100::ASL::Services::ASL_Publish_IndicationPointer indication);
        virtual void indicate(Isa100::ASL::Services::ASL_AlertReport_IndicationPointer indication);
        virtual void indicate(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer indication);
        virtual bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication){
            return false;
        }

        void forwardToObject(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::Common::TSAP::TSAP_Enum destinationTSAP_ID, ObjectID::ObjectIDEnum destinationObject,
                    Isa100::Common::TSAP::TSAP_Enum sourceTSAP_ID, ObjectID::ObjectIDEnum sourceObject);

        /**
         * Method called by Process to ask the current instance of object if it is expecting
         * the current confirmation (response) that was received from a device.
         * Each implementation of Isa100Object should overwrite this method in order to receive confirmations (responses)
         * to request that it sends to devices. In the actual implementation they must verify if this confirmation matches
         * the requests that it sent (based on request ID).
         * If an object will respond with true for a confirm packet, the next method that will be called by Process will be
         * confirm() to give the current instance the chance to consume the confirmation packet. It the object will respond
         * with false the confirm() method will not be called.
         * @param confirm
         * @return
         */
        virtual bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm){
            return false;
        }

        /**
         * Method to pass to an ISA object a confirmation packet that must be consumed by this object.
         * The packet will be passed to real ISA object by calling confirmExecute, confirmWrite or confirmRead
         * that must be implemented by ISA objects.
         * The ISA objects SHOULD NOT overwrite this method.
         * @param confirm
         */
        virtual void confirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        virtual bool isJobFinished(){
            return jobFinished;
        }

        // virtual void executeObject();

        bool isLifeTimeExpired(){
            return (time(NULL) > creationTime + SmSettingsLogic::instance().objectLifeTime);
        }

        /**
         * This method is called by Process when the countdown timer expired for this instance of the object.
         * This is the last method that will be called on this instance before automatically destroy the instance.
         * This method should be overwritten by Objects in order to do some last minute processing before distroing.
         */
        virtual void lifeTimeExpired();

    protected:
        virtual void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        virtual void indicateWrite(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        virtual void indicateRead(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        virtual void confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);
        virtual void confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);
        virtual void confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void sendExecuteResponseToRequester(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::Common::Objects::SFC::SFCEnum feedbackCode, BytesPointer payload, bool jobFinished_);
        void sendReadResponseToRequester(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::Common::Objects::SFC::SFCEnum feedbackCode, BytesPointer payload, bool jobFinished_);
        void sendWriteResponseToRequester(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::Common::Objects::SFC::SFCEnum feedbackCode, bool jobFinished_);

        /**
         * This method should be called by all objects that have lifetime greater than default timeout.
         * By calling this method repeatedly the Process will no automatically destroy this object until it respond with
         * true on the isJobFihished() method.
         */
        void resetLifeTime(Uint32 &currentTime){
            creationTime = currentTime;
        }

        /**
         * Send a response timeout to requester if timeout period elapses.
         */
        virtual void sendTimeoutToRequester();

        friend std::ostream& operator<<(std::ostream& stream, const Isa100Object& object);

};
std::ostream& operator<<(std::ostream& stream, const Isa100Object& object);


typedef boost::shared_ptr<Isa100Object> Isa100ObjectPointer;

}
}

#endif /*ISA100OBJECT_H_*/
