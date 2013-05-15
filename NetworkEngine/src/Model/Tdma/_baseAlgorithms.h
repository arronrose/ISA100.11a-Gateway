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
 * _baseAlgorithms.cpp
 *
  *      Author: octavian, mulderul(Catalin Pop)
 */
#ifndef _BASEALGORITHMS_H_
#define _BASEALGORITHMS_H_
#include "Model/Subnet.h"

namespace NE {
namespace Model {
namespace Tdma {

/**
 * Verify if 2 repetitive links are overlapping. Each links is psecified by a starting slot (slot1) and a repetition period(period1).
 * @param slot1
 * @param perioada1
 * @param slot2
 * @param perioada2
 * @param lengthSuperframe
 * @return
 */
bool diofant (int slot1, int perioada1, int slot2, int perioada2, int lengthSuperframe);

struct AdvAlgorithmData{
    Uint16 advertiseStartSlot;
    Uint16 advertisePeriod;
    Uint16 txStartSlot;
    Uint16 rxStartSlot;
    Uint16 tx_rx_period;
    Uint8 superframeChannelBirth;
    Uint8 channelOffset;
    AdvAlgorithmData()
    :advertiseStartSlot(0),
    advertisePeriod(0),
    txStartSlot(0),
    rxStartSlot(0),
    tx_rx_period(0),
    superframeChannelBirth(0),
    channelOffset(0)
    {}
};

AdvAlgorithmData * obtainAdvAlgorithmData(Device * device, Subnet::PTR& subnet);

}
}
}

#endif /* _BASEALGORITHMS_H_ */
