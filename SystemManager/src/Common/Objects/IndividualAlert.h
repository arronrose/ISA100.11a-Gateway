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

#ifndef INDIVIDUALALERT_H_
#define INDIVIDUALALERT_H_

#include "Common/Objects/AlertType.h"
#include "Common/Objects/TAINetworkTimeValue.h"
//#include "Common/Objects/IndividualAlertID.h"
#include "Misc/Convert/Convert.h"
#include "AL/ObjectsIDs.h"
//#include "Common/Objects/ExtensibleIdentifier.h"

using namespace NE::Misc::Marshall;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace Common {
namespace Objects {

/**
 * @author Sorin Bidian
 * @version 1.0
 *
 * Updated - september draft ; sorin.bidian
 *
 */
class IndividualAlert {
    public:
        Uint8 alertID;
        Uint16 detectingObjectTransportLayerPort;
        Uint16 detectingObjectID; //ObjectID::ObjectIDEnum fromInt(Isa100::Common::Uint16 value)
        TAINetworkTimeValue detectionTime;
        Uint8 alertClass;
        /*
         * Direction: into alarm, or not an alarm condition (i.e., event, or return to normal)
         */
        Uint8 alarmDirection;
        Uint8 alertCategory;
        Uint8 alertPriority;
        AlertType alertType; //TODO sorin - de revazut
        Uint16 alertValueSize;
        std::vector<Uint8> alertValue;

    public:
        IndividualAlert() : alertID(0), detectingObjectTransportLayerPort(0), detectingObjectID(0),
                detectionTime(),
                alertClass(0), alarmDirection(0), alertCategory(0), alertPriority(0),
                alertValueSize(0), alertValue(0) {

        }

        virtual ~IndividualAlert() {
        }

        void marshall(OutputStream& stream) {
            Uint8 octet;
            stream.write(alertID);
            stream.write(detectingObjectTransportLayerPort);
            stream.write(detectingObjectID);
//            detectionTime.marshall(stream);

            octet = alertClass << 7;
            octet |= alarmDirection << 6;
            octet |= alertCategory << 4;
            octet |= alertPriority;
            stream.write(octet);

            alertType.marshall(stream, alertCategory);

            stream.write(alertValueSize);
            for (Uint8 i = 0; i < alertValueSize; ++i) {
                stream.write(alertValue[i]);
            }
        }

        void unmarshall(InputStream& stream) {
            Uint8 octet;
            stream.read(alertID);
            stream.read(detectingObjectTransportLayerPort);
            stream.read(detectingObjectID);
            detectionTime.unmarshall(stream);

            stream.read(octet);
            alertClass = octet >> 7;
            alarmDirection = (octet & 0x40) >> 6;
            alertCategory = (octet & 0x30) >> 4;
            alertPriority = octet & 0x0F;

            alertType.unmarshall(stream, alertCategory);

            stream.read(alertValueSize);
            for (Uint8 i = 0; i < alertValueSize; ++i) {
                stream.read(octet);
                alertValue.push_back(octet);
            }
        }

        std::string toString() {
            std::ostringstream stream;
            stream << "IndividualAlert {";
            stream << "alertID=" << std::hex << (int)alertID;
            stream << ", detectingObjTransLayerPort=" << (int)detectingObjectTransportLayerPort;
            stream << ", detectingObjID=" << (int)detectingObjectID;
            stream << ", detectionTime=" << detectionTime.toString();
            stream << ", alertClass=" << (int)alertClass;
            stream << ", alarmDirection=" << (int)alarmDirection;
            stream << ", alertCategory=" << (int)alertCategory;
            stream << ", alertPriority=" << (int)alertPriority;
            stream << ", alertType=" << alertType.toString(alertCategory);
            stream << ", alertValueSize=" << (int)alertValueSize << ", alertValue=[ ";
            for (std::vector<Uint8>::iterator it = alertValue.begin(); it != alertValue.end(); ++it) {
                stream << (int)*it << " ";
            }
            stream << "]" << "}";

            return stream.str();
        }

//        //get alertClass value
//        Uint8 alertClass() {
//            Uint8 val = classDirectionCategoryPriority;
//            return (val & 0x80) >> 7;
//        }
//
//        //set alertClass value
//        void alertClass(Uint8 val) {
//            classDirectionCategoryPriority &= 0x7f;
//            classDirectionCategoryPriority |= (val << 7);
//        }
//
//        //get alarmDirection value
//        Uint8 alarmDirection() {
//            Uint8 val = classDirectionCategoryPriority;
//            return (val & 0x40) >> 6;
//        }
//
//        //set alarmDirection value
//        void alarmDirection(Uint8 val) {
//            classDirectionCategoryPriority &= 0xbf;
//            classDirectionCategoryPriority |= (val << 6);
//        }
//
//        //get alertCategory value
//        Uint8 alertCategory() {
//            Uint8 val = classDirectionCategoryPriority;
//            return (val & 0x30) >> 4;
//        }
//
//        //set alertCategory value
//        void alertCategory(Uint8 val) {
//            classDirectionCategoryPriority &= 0xcf;
//            classDirectionCategoryPriority |= (val << 4);
//        }
//
//        //get alertPriority value
//        Uint8 alertPriority() {
//            Uint8 val = classDirectionCategoryPriority;
//            return val & 0x0f;
//        }
//
//        //set alertPriority value
//        void alertPriority(Uint8 val) {
//            classDirectionCategoryPriority &= 0xf0;
//            classDirectionCategoryPriority |= val;
//        }

};

typedef boost::shared_ptr<IndividualAlert> IndividualAlertPointer;

}//namespace
}
}

#endif /*INDIVIDUALALERT_H_*/
