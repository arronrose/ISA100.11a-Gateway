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

#ifndef CMDS_H_
#define CMDS_H_

#include "ASL/Services/ASL_Service_PrimitiveTypes.h"
#include "ASL/Services/ASL_Publish_PrimitiveTypes.h"
#include "ASL/Services/ASL_AlertReport_PrimitiveTypes.h"
#include "ASL/Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "Common/NEAddress.h"
#include "ASL/PDU/ClientServerPDU.h"
#include "Common/smTypes.h"
#include "Common/Objects/SFC.h"
#include "Common/SmSettingsLogic.h"

using namespace Isa100::ASL::PDU;

namespace Isa100 {
namespace Stats {

/**
 * Logs ASL services commands information.
 * @author Sorin Bidian, radu.pop, catalin.pop, beniamin.tecar
 * @version 1.0
 */
class Cmds {
        LOG_DEF("I.S.Cmds");

    public:

        Cmds();

        ~Cmds();

        /**
         * Log ASL_Service traffic information. Primitive ...
         */
        static void logInfo(Isa100::ASL::Services::PrimitiveIndicationPointer primitiveIndication, std::string mesg = "");

        static void logInfo(Isa100::ASL::Services::PrimitiveConfirmationPointer primitiveConfirmation, std::string mesg = "");

        static void logInfo(Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequest);

        static void logInfo(Isa100::ASL::Services::PrimitiveResponsePointer primitiveResponse);

        /**
         * Log ASL_Publish_IndicationPointer traffic information.
         */
        static void logInfo(Isa100::ASL::Services::ASL_Publish_IndicationPointer publishIndication, std::string mesg = "");

        /**
         * Log ASL_AlertReport traffic information.
         */
        static void logInfo(Isa100::ASL::Services::ASL_AlertReport_IndicationPointer alertIndication, std::string mesg = "");

        static void logInfo(Isa100::ASL::Services::ASL_AlertReport_RequestPointer alertRequest, std::string mesg = "");

        static void logInfo(Isa100::ASL::Services::ASL_AlertAcknowledge_IndicationPointer alertAcknowledgeIndication);

        static void logInfo(Isa100::ASL::Services::ASL_AlertAcknowledge_RequestPointer alertAcknowledgeRequest);

        /**
         * Log duplicate requests. These requests are discarded.
         */
        //        static void logInfo(std::vector<Isa100::AL::ObjectRequest>& request);

    private:

        static void getObjectName(ObjectID::ObjectIDEnum objectID, Uint8 tsap, std::string &objectName);

        static void logFeedbackCode(std::ostringstream& stream, Isa100::Common::Objects::SFC::SFCEnum feedbackCode);

        /**
         * Gets the string representation of euiAddress64 from the capabilities of device with address32.
         */
        static void logAddress(std::ostringstream& stream, Address32 address);

        /**
         * Gets the string representation of euiAddress64 from the capabilities of device with address128.
         * If the device is not found, the string representation of the address128 is returned.
         */
        static void logAddress(std::ostringstream& stream,const NE::Common::Address128& address128);

        //        static std::string getObjectName(ObjectID::ObjectIDEnum objectID, Uint8 tsap);
};

}
}
#endif /*CMDS_H_*/
