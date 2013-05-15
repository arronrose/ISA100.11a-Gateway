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
 * NLMEWrapper.h
 *
 *  Created on: Mar 30, 2009
 *      Author: Andy
 */

#ifndef NLMEWRAPPER_H_
#define NLMEWRAPPER_H_

#include <Common/Address128.h>
#include <Common/smTypes.h>
#include <cstring>
#include <Common/Objects/SFC.h>
//#include <Model/Contract.h>

#define OUTGOING_INTERFACE 1

	enum Stack_OutgoingInterfaceEnum
	{
		soiDL = 0,
		soiBackbone = 1
	};

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_AddRoute(
		const NE::Common::Address128& destination, const NE::Common::Address128& nextHop, Uint8 hopLimit,
		Stack_OutgoingInterfaceEnum oi);

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_DeleteRoute(
		const NE::Common::Address128& destination);

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_AddContract(
		Uint16 contractID, const NE::Common::Address128& sourceAddress,
		const NE::Common::Address128& destAddress, Uint8 priority, bool includeContractFlag,
		Uint16 assignedMaxNSDUSize, Uint8 wndLen, Int16 comittedBurst, Int16 excessBurst);

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_DeleteContract(Uint16 contractID);

//void Stack_CheckContractsSyncro(NE::Model::MapSourceContracts& smContracts);

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_AddAddressTranslation(
		NE::Common::Address128& longAddress, Address16 shortAddress);

Isa100::Common::Objects::SFC::SFCEnum Stack_NLME_DeleteAddressTranslation(
		const NE::Common::Address128& longAddress);


#endif /* NLMEWRAPPER_H_ */
