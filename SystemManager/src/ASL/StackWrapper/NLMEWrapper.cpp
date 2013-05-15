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
 * NLMEWrapper.cpp
 *
 *  Created on: Mar 30, 2009
 *      Author: Andy
 */

#include "NLMEWrapper.h"
#include <porting.h>
#include <netinet/in.h>
#include <Common/logging.h>
#include <assert.h>

LOG_DEF("I.ASL.SW.NLMEWrapper");

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_AddRoute(
		const NE::Common::Address128& destination, const NE::Common::Address128& nextHop, Uint8 hopLimit,
		Stack_OutgoingInterfaceEnum oi
		)
{
	return (Isa100::Common::Objects::SFC::SFCEnum) NLME_AddRoute(destination.value, nextHop.value, hopLimit, oi);
}

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_DeleteRoute(
            const NE::Common::Address128& destination)
{
	return (Isa100::Common::Objects::SFC::SFCEnum)  NLME_delRoute(destination.value);
}


Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_AddContract(
		Uint16 contractID, const NE::Common::Address128& sourceAddress,
		const NE::Common::Address128& destAddress, Uint8 priority, bool includeContractFlag,
		Uint16 assignedMaxNSDUSize, Uint8 wndLen, Int16 comittedBurst, Int16 excessBurst)
{
	LOG_DEBUG("Add stack contract id=" << contractID << ", maxNSDU=" << assignedMaxNSDUSize << ", wndLen=" << (int)wndLen);
	assert(assignedMaxNSDUSize != 0);
//	assert(wndLen != 0);

	uint8 response = NLME_AddContract( contractID, sourceAddress.value, destAddress.value, priority, includeContractFlag,
										assignedMaxNSDUSize, wndLen, comittedBurst, excessBurst);

	LOG_DEBUG("Contract added with result=" << (int)response);

	return (Isa100::Common::Objects::SFC::SFCEnum) response;
}

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_DeleteContract(Uint16 contractID)
{
    uint8 response = NLME_DeleteContract(contractID);
	return (Isa100::Common::Objects::SFC::SFCEnum) response;
}

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_AddAddressTranslation (NE::Common::Address128& longAddress, Address16 shortAddress)
{
	return (Isa100::Common::Objects::SFC::SFCEnum)NLME_AddATT(longAddress.value,shortAddress);
}

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_DeleteAddressTranslation(
            const NE::Common::Address128& longAddress)
{
	return (Isa100::Common::Objects::SFC::SFCEnum) NLME_delATT(longAddress.value);
}

