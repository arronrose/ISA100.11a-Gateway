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
 *
 *      Author: mulderul@y(Catalin Pop)
 */
#ifndef APPALLOCATION_H_
#define APPALLOCATION_H_

#include "Model/ContractRequest.h"
#include "Model/Device.h"
#include "Model/IEngine.h"
#include "Model/Routing/Edge.h"
#include "Model/Tdma/TdmaTypes.h"

namespace NE {
namespace Model {
namespace Tdma {

inline Uint16 getSlot(SlotFreq slotFreq) {
    return (slotFreq >> 4);
}

inline Uint8 getFreq(SlotFreq slotFreq){
    return (slotFreq & 0xF);
}

void searchPreferredEdges(Device* destinationDevice, PhyRoute * route, Subnet::PTR& subnet, DoubleExitEdges&  preferredEdges, Uint16& graphID);

bool isFreeSlot(Uint16 slot, Uint16 period, Device& device, int superframeLength);

Uint32 reserveFreeSlot(AppSlots& appSlots, Uint16 startSlot, Uint16 & maxSlotDelay, Uint16 period, Device& sourceDevice, Device& destinationDevice, const int superframeLengthAPP,  Uint8 FreqCount);

bool isOverlapping(int slot1, int perioada1, int slot2, int perioada2, int lengthSuperframe);

}
}
}

#endif /* APPALLOCATION_H_ */
