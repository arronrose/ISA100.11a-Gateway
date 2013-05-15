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
 * WriteResponsePDU.cpp
 *
 *  Created on: Oct 14, 2008
 *      Author: sorin.bidian
 */

#include "WriteResponsePDU.h"
#include "Misc/Convert/Convert.h"

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace NE::Misc::Convert;

namespace Isa100 {
namespace ASL {
namespace PDU {

WriteResponsePDU::WriteResponsePDU(bool forwardCongestionNotificationEcho_, SFC::SFCEnum feedbackCode_) :
    forwardCongestionNotificationEcho(forwardCongestionNotificationEcho_), feedbackCode(feedbackCode_) {
}

WriteResponsePDU::~WriteResponsePDU() {
}

Uint16 WriteResponsePDU::getSize(BytesPointer payload, Uint16 position) {
	return 1 //forwardCongestionNotificationEcho
		+ 1; // ServiceFeedbackCode
}

std::string WriteResponsePDU::toString() {
    std::ostringstream stream;
    stream << " forwardCongestionNotificationEcho: " << (int) forwardCongestionNotificationEcho;
    stream << " feedbackCode: " << (int) feedbackCode;

    return stream.str();
}


}
}
}

