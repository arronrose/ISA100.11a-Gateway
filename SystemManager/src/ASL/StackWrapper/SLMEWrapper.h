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
 * SLMEWrapper.h
 *
 *  Created on: Mar 30, 2009
 *      Author: Andy
 */

#ifndef SLMEWRAPPER_H_
#define SLMEWRAPPER_H_

#include <Common/Address128.h>
#include <Common/NETypes.h>
#include <Common/NEAddress.h>
#include <Model/SecurityKey.h>
#include <Common/Objects/SFC.h>

Isa100::Common::Objects::SFC::SFCEnum Stack_SLME_SetKey(const NE::Common::Address128& peerAddress,
		Uint16 udpSourcePort, Uint16 udpDestinationPort,
		Uint8 keyID, NE::Model::SecurityKey key,
		NE::Common::Address64 issuerEUI64, Uint32 notValidBefore, Uint32 softLifeTime, Uint32 hardLifeTime,
		Uint8 type, Uint8 policy);

Isa100::Common::Objects::SFC::SFCEnum Stack_SLME_DeleteKey(const NE::Common::Address128& peerAddress,
		Uint16 udpSourcePort, Uint16 udpDestinationPort, Uint8 keyID, Uint8 keyType);

#endif /* SLMEWRAPPER_H_ */
