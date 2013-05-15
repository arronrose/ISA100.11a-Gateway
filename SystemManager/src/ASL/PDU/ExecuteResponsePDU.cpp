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
 * ExecuteResponsePDU.cpp
 *
 *  Created on: Oct 14, 2008
 *      Author: sorin.bidian
 */

#include "ExecuteResponsePDU.h"
//#include "Common/Objects/ApduResponseControlData.h"
#include "Misc/Convert/Convert.h"

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace ASL {
namespace PDU {

ExecuteResponsePDU::ExecuteResponsePDU(bool forwardCongestionNotificationEcho_, SFC::SFCEnum feedbackCode_,
            BytesPointer parameters_) :
    forwardCongestionNotificationEcho(forwardCongestionNotificationEcho_), feedbackCode(feedbackCode_),
    parameters(parameters_) {
}

ExecuteResponsePDU::~ExecuteResponsePDU() {

}

Uint16 ExecuteResponsePDU::getSize(BytesPointer payload, Uint16 position) {

    Uint16 posBeforeParamsSize = position
		+ 1  // FCN
		+ 1; // ServiceFeedbackCode

    Uint8 value = payload->at(posBeforeParamsSize);
    Uint16 sizeParamsSize = 0;
    Uint16 paramsSize = 0;
    if ((value & 0x80) == 0x80) { // check if params size deal 1 or 2 bytes.
        paramsSize = (value & 0x7F);
        paramsSize = paramsSize << 8;
        value = payload->at(posBeforeParamsSize + 1);
        paramsSize += value;
        sizeParamsSize = 2;
    } else {
        paramsSize = (value & 0x7F);
        sizeParamsSize = 1;
    }

    return posBeforeParamsSize + sizeParamsSize + paramsSize - position;
}

std::string ExecuteResponsePDU::toString() {
    std::ostringstream stream;
    stream << " forwardCongestionNotificationEcho: " << (int) forwardCongestionNotificationEcho;
    stream << ", feedbackCode: " << (int) feedbackCode;
    stream << ", parameters: " << bytes2string(*parameters);

    return stream.str();
}

}
}
}

