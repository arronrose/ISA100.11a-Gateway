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
 * @author beniamin.tecar, sorin.bidian
 */
#ifndef ARMO_H_
#define ARMO_H_

#include "AL/Isa100Object.h"
#include "ASL/Services/ASL_AlertReport_PrimitiveTypes.h"
#include "ASL/Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "Model/Operations/IAlertSender.h"
#include "Common/logging.h"

namespace Isa100 {
namespace AL {
namespace SMAP {

class ARMO: public Isa100::AL::Isa100Object, public NE::Model::Operations::IAlertSender {
        LOG_DEF("I.A.D.ARMO");

    private:
        std::vector<NE::Model::Operations::AlertOperationPointer> alertList;

        Uint32 lastAlertTime;

    public:

        ARMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~ARMO();

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const {
            return ObjectID::ID_ARMO;
        }

        /**
         * Always return false - there is only one instance created and it remains active.
         */
        bool isJobFinished() {
            return false;
        }

        void execute(Uint32 currentTime);

        void indicate(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer indication);

        /**
         * Adds the AlertOperation to an alert list from where the alerts are forwarded to the Gateway.
         */
        void sendAlert(NE::Model::Operations::AlertOperationPointer& alertOperation);

    private:

        /**
         * Dispatches the alerts to the Gateway.
         */
        void dispatchAlert(std::vector<NE::Model::Operations::AlertOperationPointer>& alertOperations, int alertType);

        void dispatchAlert(NE::Model::Operations::AlertOperationPointer& alertOperation);
};

}
}
}

//
//class ARMO;
//typedef boost::shared_ptr<ARMO> ARMOPointer;
//
///**
// * Alert reporting management object (ARMO).
// * This object facilitates the management of the device's alert reporting functions.
// * @author Sorin Bidian
// * @version 1.0
// */
//class ARMO: public Isa100Object {
//        LOG_DEF("I.A.D.ARMO")
//
//    public:
//        //ARMO attributes
//        //TODO initialize with default values
//        //device diagnostics
//        Uint16 alertMasterDevice;
//        Uint16 maxAlertQueueDevice;
//        Uint16 currentAlertQueueDevice;
//        Uint8 confirmationTimeoutDevice;
//        Uint8 alertsDisableDevice;
//        bool alarmRegenDevice;
//        //communication diagnostics
//        Uint16 alertMasterComm;
//        Uint16 maxAlertQueueComm;
//        Uint16 currentAlertQueueComm;
//        Uint8 confirmationTimeoutComm;
//        Uint8 alertsDisableComm;
//        bool alarmRegenComm;
//        //security
//        Uint16 alertMasterSecurity;
//        Uint16 maxAlertQueueSecurity;
//        Uint16 currentAlertQueueSecurity;
//        Uint8 confirmationTimeoutSecurity;
//        Uint8 alertsDisableSecurity;
//        bool alarmRegenSecurity;
//        //process
//        Uint16 alertMasterProcess;
//        Uint16 maxAlertQueueProcess;
//        Uint16 currentAlertQueueProcess;
//        Uint8 confirmationTimeoutProcess;
//        Uint8 alertsDisableProcess;
//        bool alarmRegenProcess;
//
//    public:
//        ARMO(Isa100::Common::TSAP::TSAP_Enum tsap);
//
//        virtual ~ARMO();
//
////        virtual bool
////                    isAttributeReadOnly(const Isa100::Common::Objects::ExtensibleAttributeIdentifier& attributeID) const;
//
//        /**
//         * Returns the if the current object.
//         */
//        virtual Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const;
//
//
//        virtual void execute( Uint32 currentTime  );
//
//        virtual bool isAvailable();
//
////        friend ARMOPointer getARMOInstance();
//};
//
////ARMOPointer getARMOInstance();
//
//}
//}
//}

#endif /*ARMO_H_*/
