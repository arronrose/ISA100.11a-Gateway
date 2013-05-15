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
 * AlertReportPDU.h
 *
 *  Created on: Dec 11, 2008
 *      Author: sorin.bidian
 */

#ifndef ALERTREPORTPDU_H_
#define ALERTREPORTPDU_H_

#include "Common/NETypes.h"
#include "Common/Objects/TAINetworkTimeValue.h"
#include "Common/NEAddress.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

class AlertReportPDU {

    public:

        Uint8 alertID;

        Uint16 detectingObjectTransportLayerPort;

        Uint16 detectingObjectID;

        Isa100::Common::Objects::TAINetworkTimeValue detectionTime;

        // 1 bit; This parameter indicates if this is an event (stateless) or alarm (state-oriented) type of alert.
        Uint8 alertClass;

        // 1 bit; Direction: into alarm, or not an alarm condition (i.e., event, or return to normal).
        Uint8 alarmDirection;

        // 2 bit;
        Uint8 alertCategory;

        // 4 bits;
        Uint8 alertPriority;

        Uint8 alertType;

        Uint16 alertValueSize; //ExtensibleInteger (msb 0/1)

        BytesPointer alertValue;

    public:

        AlertReportPDU(Uint8 alertID, Uint16 detectingObjectTransportLayerPort, Uint16 detectingObjectID,
                    Isa100::Common::Objects::TAINetworkTimeValue detectionTime, Uint8 alertClass, Uint8 alarmDirection,
                    Uint8 alertCategory, Uint8 alertPriority, Uint8 alertType, BytesPointer alertValue);

        ~AlertReportPDU();

        static Uint16 getSize(BytesPointer payload, Uint16 position);

        std::string toString();
};

typedef boost::shared_ptr<AlertReportPDU> AlertReportPDUPointer;

struct CascadedAlertReportPDU {
    NE::Common::Address128 originalDeviceAddress;
    AlertReportPDU alertReportPDU;

    CascadedAlertReportPDU(NE::Common::Address128 originalDeviceAddress, AlertReportPDU alertReportPDU);

    std::string toString();

};

typedef boost::shared_ptr<CascadedAlertReportPDU> CascadedAlertReportPDUPointer;

}
}
}

#endif /* ALERTREPORTPDU_H_ */
