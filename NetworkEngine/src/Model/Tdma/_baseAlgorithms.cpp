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
#include "_baseAlgorithms.h"
#include "TdmaTypes.h"
#include <cmath>
#include "Common/logging.h"

namespace NE {
namespace Model {
namespace Tdma {



bool diofant (int slot1, int perioada1, int slot2, int perioada2, int lengthSuperframe) {
	//## solve the equation
	//## perioada1*X1+perioada2*X2 = X2 - X1
    if (slot1 == slot2){
        return true;
    }

	int r1 = perioada1;
	int r2 = perioada2;
	int c = slot1 - slot2;

	std::queue<int> r1Int;
	std::queue<int> r2Int;

	int a,b,r;

	while (r2 > 1) {

		if (r1 > r2) {
			a = r1;
			b = r2;
		} else {
			a = r2;
			b = r1;
		}

		r1Int.push(a);
		r2Int.push(b);

		r = a % b;
		r1 = b; r2 = r;
	}

	int fl_mltp = 0;
	if (r2 == 0) {
		r1 = r1Int.front();
		r1Int.pop();
		r2 = r2Int.front();
		r2Int.pop();

		fl_mltp = r2;
		int k = r1 / r2;
		if (c % r2 != 0) {
			return false;
		} else {
			c = c / r2;
			r1 = k;
		}
	}

		int a1 = 1; int b1 = 0;
		int a2 = -r1; int b2 = c;

		if (fl_mltp != 0) {
			c = c * fl_mltp;
		}

		r = r2;

		while (!r1Int.empty()) {
			r1 = r1Int.front();
			r1Int.pop();
			r2 = r2Int.front();
			r2Int.pop();

			int q = (r1 - (r1 % r2)) / r2;
			if (r > r2) {
				a1 = a1; b1 = b1;
				a2 = a2 - q * a1;
				b2 = b2 - q * b1;
			} else {
				int ac = a1;
				int bc = b1;
				a1 = a2;
				b1 = b2;
				a2 = ac - q * a2;
				b2 = bc - q * b2;
			}

		}
        a = a2;
        b = b2;
        int s = slot2;
        int p = perioada2;

		int n = int (-b/a);
		int ps = a * n + b;

		if (ps <= 0) {
			if (a < 0) {
				n = n - 1;
			} else {
				n = n+1;
			}
		}

		ps = s + p * (a * n + b);
		if (ps > lengthSuperframe) {
			return false;
		} else {
			return true;
		}
}

AdvAlgorithmData * obtainAdvAlgorithmData(Device * device, Subnet::PTR& subnet){
    const SubnetSettings& subnetSettings = subnet->getSubnetSettings();

    if (device->capabilities.isBackbone()){
        AdvAlgorithmData * data = new AdvAlgorithmData();
        data->advertiseStartSlot = 1;
        data->advertisePeriod = subnetSettings.join_bbr_adv_period * subnetSettings.getSlotsPerSec();
        data->txStartSlot = 5;
        data->rxStartSlot = 9;
        data->tx_rx_period = subnetSettings.getSlotsPerSec();
        data->superframeChannelBirth = 1;//used to force the advertise on channels 8 3 5 11
        data->channelOffset = 0;
        return data;
    }

    // on reload config file (SIGHUP);
    // clear old device allocation from advertiseChuncksTable in order to allocate a new advertise chunk
    subnet->removeDeviceFromAdvertiseChuncksTable(device);

    for (Uint8 advPeriods = 0;
            (advPeriods < MAX_NR_OF_ADV_PERIODS);
            ++advPeriods){

        for (Uint8 advStartSlotIndex = 0; advStartSlotIndex < MAX_NR_OF_ADV_BANDWIDTH_CHUNCKS; ++advStartSlotIndex){
            if (subnet->getAdvertiseChuncksTableElement(advStartSlotIndex, advPeriods) == 0){
                Uint8 startSlotAdv;
                AdvAlgorithmData * data = new AdvAlgorithmData();
                switch(advStartSlotIndex){
                case 0: startSlotAdv = 3;  data->txStartSlot = 7;  data->rxStartSlot = 11; data->superframeChannelBirth = 3; break;
                case 1: startSlotAdv = 13; data->txStartSlot = 17; data->rxStartSlot = 21; data->superframeChannelBirth = 13;break;
                case 2:
                default:
                    startSlotAdv = 15; data->txStartSlot = 19; data->rxStartSlot = 23; data->superframeChannelBirth = 15;break;
                }
                data->advertiseStartSlot = (advPeriods % subnet->getSubnetSettings().join_adv_period) * subnetSettings.getSlotsPerSec() + startSlotAdv;
                data->advertisePeriod = subnet->getSubnetSettings().join_adv_period * subnetSettings.getSlotsPerSec();
                data->tx_rx_period = subnet->getSubnetSettings().join_rxtx_period * subnetSettings.getSlotsPerSec();
                data->channelOffset = ((int)advPeriods / (int)subnet->getSubnetSettings().join_adv_period) * 4;//must be 4 multiply to preserve channels

                subnet->setAdvertiseChuncksTableElement(advStartSlotIndex, advPeriods, Address::getAddress16(device->address32));
                return data;
            }
        }
    }
    return NULL;//means that no advertise slots are available
}


}
}
}
