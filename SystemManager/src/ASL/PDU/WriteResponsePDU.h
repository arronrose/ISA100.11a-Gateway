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
 * WriteRequestPDU.h
 *
 *  Created on: Oct 13, 2008
 *      Author: beniamin.tecar
 */

#ifndef WRITERESPONSEPDU_H_
#define WRITERESPONSEPDU_H_

#include "Common/NETypes.h"
#include "Common/Objects/SFC.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

class WriteResponsePDU {
public:
        bool forwardCongestionNotificationEcho;
        Isa100::Common::Objects::SFC::SFCEnum feedbackCode;

    public:
        WriteResponsePDU(bool forwardCongestionNotificationEcho, Isa100::Common::Objects::SFC::SFCEnum feedbackCode);
        ~WriteResponsePDU();

        static Uint16 getSize(BytesPointer payload, Uint16 position);
        std::string toString();
};

typedef boost::shared_ptr<WriteResponsePDU> WriteResponsePDUPointer;

} // namespace PDU
} // namespace ASL
} // namespace Isa100

#endif /* WRITERESPONSEPDU_H_ */
