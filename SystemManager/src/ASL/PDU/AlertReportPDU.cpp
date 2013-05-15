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
 * AlertReportPDU.cpp
 *
 *  Created on: Dec 11, 2008
 *      Author: sorin.bidian
 */

#include "AlertReportPDU.h"
#include "Common/NEException.h"
#include "Misc/Convert/Convert.h"

using namespace Isa100::Common;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace ASL {
namespace PDU {

AlertReportPDU::AlertReportPDU(Uint8 alertID_, Uint16 detectingObjectTransportLayerPort_, Uint16 detectingObjectID_,
            Isa100::Common::Objects::TAINetworkTimeValue detectionTime_, Uint8 alertClass_, Uint8 alarmDirection_,
            Uint8 alertCategory_, Uint8 alertPriority_, Uint8 alertType_, BytesPointer alertValue_) {
    alertID = alertID_;
    detectingObjectTransportLayerPort = detectingObjectTransportLayerPort_;
    detectingObjectID = detectingObjectID_;
    detectionTime = detectionTime_;
    if (alertClass_ > 0x01) {
        ostringstream msg;
        msg << "AlertReportPDU-alertClass can only be 0 or 1! value=" << (int) alertClass_;
        throw NE::Common::NEException(msg.str());
    } else {
        alertClass = alertClass_;
    }
    if (alarmDirection_ > 0x01) {
        ostringstream msg;
        msg << "AlertReportPDU-alarmDirection can only be 0 or 1! value=" << (int) alarmDirection_;
        throw NE::Common::NEException(msg.str());
    } else {
        alarmDirection = alarmDirection_;
    }
    if (alertCategory_ > 0x03) {
        ostringstream msg;
        msg << "AlertReportPDU-alertCategory value not allowed! value=" << (int) alertCategory_;
        throw NE::Common::NEException(msg.str());
    } else {
        alertCategory = alertCategory_;
    }
    if (alertPriority_ > 0x0F) {
        ostringstream msg;
        msg << "AlertReportPDU-alertPriority value not allowed! value=" << (int) alertPriority_;
        throw NE::Common::NEException(msg.str());
    } else {
        alertPriority = alertPriority_;
    }
    alertType = alertType_;
    alertValue = alertValue_;
    alertValueSize = (Uint16) alertValue->size();
}

AlertReportPDU::~AlertReportPDU() {
}

Uint16 AlertReportPDU::getSize(BytesPointer payload, Uint16 position) {
    // LOG_DEBUG ("getSize payload: " << bytes2string(*payload));
    Uint16 size = position + 1 // alertID
                + 2 // detectingObjectTransportLayerPort
                + 2 // detectingObjectID
                + 6 // detectionTime
                + 1 // alertClass + alarmDirection + alertCategory + alertPriority
                + 1; // alertType
    Uint8 value = payload->at(size);
    Uint16 paramsSize = 0;
    if ((value & 0x80) == 0x80) { //params size = 2 bytes
        paramsSize = (value & 0x7F);
        paramsSize = paramsSize << 8;
        ++size;
        value = payload->at(size);
        paramsSize += value;
        ++size;
    } else {
        paramsSize = (value & 0x7F);
        ++size;
    }

    return size + paramsSize - position;
}

std::string AlertReportPDU::toString() {
    std::ostringstream stream;
    stream << " alertID: " << (int) alertID;
    stream << ", detectingObjTranspLayerPort: " << (int) detectingObjectTransportLayerPort;
    stream << ", detectingObjID: " << (int) detectingObjectID;
    stream << ", detectionTime: " << detectionTime.toString();
    stream << ", alertClass: " << (int) alertClass;
    stream << ", alarmDirection: " << (int) alarmDirection;
    stream << ", alertCategory: " << (int) alertCategory;
    stream << ", alertPriority: " << (int) alertPriority;
    stream << ", alertType: " << (int) alertType;
    stream << ", alertValue: " << bytes2string(*alertValue);

    return stream.str();
}

CascadedAlertReportPDU::CascadedAlertReportPDU(NE::Common::Address128 originalDeviceAddress_, AlertReportPDU alertReportPDU_):
    originalDeviceAddress(originalDeviceAddress_), alertReportPDU(alertReportPDU_) {

}

std::string CascadedAlertReportPDU::toString() {
    return "original device=" + originalDeviceAddress.toString() + alertReportPDU.toString();
}

}
}
}
