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
 * ReadRequestPDU.h
 *
 *  Created on: Oct 13, 2008
 *      Author: beniamin.tecar
 */

#ifndef READRESPONSEPDU_H_
#define READRESPONSEPDU_H_

#include "Common/NETypes.h"
#include "Common/Objects/SFC.h"

namespace Isa100 {
namespace ASL {
namespace PDU {

/*
 * This is the container class for ReadResponse packet.
 * For marshaling see the in standard: 13.22.2.6 chapter.
 */
class ReadResponsePDU {
    public:
        bool forwardCongestionNotificationEcho;
        Isa100::Common::Objects::SFC::SFCEnum feedbackCode;
        BytesPointer value;

    public:
        ReadResponsePDU(bool forwardCongestionNotificationEcho,
                    Isa100::Common::Objects::SFC::SFCEnum feedbackCode, BytesPointer value);
        ~ReadResponsePDU();

        static Uint16 getSize(BytesPointer payload, Uint16 position);
        std::string toString();
};

typedef boost::shared_ptr<ReadResponsePDU> ReadResponsePDUPointer;

}
} // namespace ASL
} // namespace Isa100

#endif /* READRESPONSEPDU_H_ */
