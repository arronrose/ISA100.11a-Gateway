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
 * ParseResponses.h
 *
 *  Created on: Mar 11, 2009
 *      Author: Andy
 */

#ifndef PARSERESPONSES_H_
#define PARSERESPONSES_H_

#include <boost/function.hpp>

#include "../Services/ASL_Service_PrimitiveTypes.h"
#include "../Services/ASL_AlertAcknowledge_PrimitiveTypes.h"
#include "../Services/ASL_AlertReport_PrimitiveTypes.h"
#include "../Services/ASL_Publish_PrimitiveTypes.h"
#include "../../AL/ProcessMessages.h"
#include <Common/NETypes.h>
//#include "../TSDUUtils.h"

#include <Common/Objects/ExtensibleAttributeIdentifier.h>
#include <Misc/Marshall/NetworkOrderStream.h>
#include "Types.h"

#include <map>
#include <porting.h>

//LOG_DEF("ParseResponses");

//HACK[andy]
//extern std::map<Uint16, NE::Common::Address128> destinationAddresses;
extern std::map<Uint16, Address32> destinationAddresses;

inline
bool Stack_isRequest(const GENERIC_ASL_SRVC& service)
{
	return !(service.m_ucType & (SRVC_RESPONSE << 7));
}

inline
void WriteCompressedSize(uint16 size, NE::Misc::Marshall::NetworkOrderStream& stream)
{
	if (size > 0x7F)
	{
		stream.write((Uint8)(0x80 | ((size & 0x7F00) >> 8)));
	}
	stream.write((Uint8)(size & 0xFF));
}

inline
void WriteCompressedSize(uint16 size, BytesPointer& bPtr) {
    if (size > 0x7F) {
        bPtr->append(1, (Uint8) (0x80 | ((size & 0x7F00) >> 8)));
    }
    bPtr->append(1, (Uint8) (size & 0xFF));
}

inline void Write(uint16 value, BytesPointer& bPtr) {
    bPtr->append(1, (Uint8) ((value & 0xFF00) >> 8));
    bPtr->append(1, (Uint8) (value & 0x00FF));
}

inline void Write(uint32 value, BytesPointer& bPtr) {
    bPtr->append(1, (Uint8) ((value & 0xFF000000) >> 24));
    bPtr->append(1, (Uint8) ((value & 0x00FF0000) >> 16));
    bPtr->append(1, (Uint8) ((value & 0x0000FF00) >> 8));
    bPtr->append(1, (Uint8) (value & 0x000000FF));
}



#endif /* PARSERESPONSES_H_ */
