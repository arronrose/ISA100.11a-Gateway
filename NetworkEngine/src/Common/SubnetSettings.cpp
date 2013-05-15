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
 * SubnetSettings.cpp
 *
 *  Created on: Mar 31, 2010
 *      Author: Catalin Pop
 */

#include "SubnetSettings.h"

namespace NE {
namespace Common {

void SubnetSettings::randomizeChannelList() {
    Uint16 i, t, k;
    for (i = 2; i < channel_list.size(); ++i) {
        k = rand() % i;
        t = channel_list[i - 1];
        channel_list[i - 1] = channel_list[k];
        channel_list[k] = t;
    }
}

string SubnetSettings::getChannelListAsString() {
    std::ostringstream stream;
    // "8, 1, 9, 13, 5, 12, 7, 3, 10, 0, 4, 11, 6, 2";
    for (std::size_t i = 0; i < channel_list.size(); ++i) {
        stream << (int) channel_list[i];
        if (i + 1 < channel_list.size()) {
            stream << ",";
        }
    }
    return stream.str();
}

string SubnetSettings::getNeighborDiscoveryListAsString() {
    std::ostringstream stream;
    // "11,8,5,3,11,8,5" when channel list is complete / "7,9,11,8,7,9,11" for reduced channel hopping sequence
    for (std::size_t i = 0; i < neigh_disc_channel_list.size(); ++i) {
        stream << (int) neigh_disc_channel_list[i];
        if (i + 1 < neigh_disc_channel_list.size()) {
            stream << ",";
        }
    }
    return stream.str();
}

string SubnetSettings::getReducedChannelHoppingListAsString() {
    std::ostringstream stream;
    // "7, 10, 8, 11, 9, 7, 10, 8, 11, 9, 7, 10, 8, 11, 9 ,7";
    for (std::size_t i = 0; i < reduced_hopping.size(); ++i) {
        stream << (int) reduced_hopping[i];
        if (i + 1 < reduced_hopping.size()) {
            stream << ",";
        }
    }
    return stream.str();
}

string SubnetSettings::getQueuePrioritiesListAsString() {
    std::ostringstream stream;
    for (std::size_t i = 0; i < queuePriorities.size(); ++i) {
    	Uint8 priorityNo = queuePriorities[i] >> 8;
    	Uint8 qMax = queuePriorities[i] ;
        stream << (int) priorityNo<<":"<<  (int) qMax;
        if (i + 1 < queuePriorities.size()) {
            stream << ",";
        }
    }
    return stream.str();
}

void SubnetSettings::createChannelListWithReducedHopping() {
	channel_list.clear();
	for (Uint8  i = 0; i < 14; ++i) {
		channel_list.push_back(reduced_hopping[i]);
	}
}

void SubnetSettings::detectNumberOfUsedFrequencies() {
	std::set<Uint8> usedFrequencies;
	for (std::size_t i = 0; i < reduced_hopping.size(); ++i) {
		usedFrequencies.insert(reduced_hopping[i]);
	}
	numberOfFrequencies = usedFrequencies.size();
}

std::ostream& operator<<(std::ostream& stream, const SubnetSettings& entity) {
    stream << "channel_list={";
    for (std::vector<Uint8>::const_iterator it = entity.channel_list.begin(); it != entity.channel_list.end(); ++it) {
        stream << (int) (*it) << ", ";
    }
    stream << "}";
    stream << ", join_reserved_set=" << (int) entity.join_reserved_set;
    stream << ", join_adv_period=" << (int) entity.join_adv_period;
    stream << ", join_bbr_adv_period=" << (int) entity.join_bbr_adv_period;
    stream << ", join_rxtx_period=" << (int) entity.join_rxtx_period;
    stream << ", mng_link_r_out=" << (int) entity.mng_link_r_out;
    stream << ", mng_link_r_in=" << (int) entity.mng_link_r_in;
    stream << ", mng_link_s_out=" << (int) entity.mng_link_s_out;
    stream << ", mng_link_s_in=" << (int) entity.mng_link_s_in;
    stream << ", mng_r_in_band=" << (int) entity.mng_r_in_band;
    stream << ", mng_s_in_band=" << (int) entity.mng_s_in_band;
    stream << ", mng_r_out_band=" << (int) entity.mng_r_out_band;
    stream << ", mng_s_out_band=" << (int) entity.mng_s_out_band;
    stream << ", mng_alloc_band=" << (int) entity.mng_alloc_band;
    stream << ", mng_disc_bbr=" << (int) entity.mng_disc_bbr;
    stream << ", mng_disc_r=" << (int) entity.mng_disc_r;
    stream << ", mng_disc_s=" << (int) entity.mng_disc_s;
    stream << ", retry_proc=" << (int) entity.retry_proc;
    stream << ", slotsNeighborDiscovery={";
    for (int i = 0; i < SubnetSettings::SLOTS_NEIGHBOR_DISCOVERY_LENGTH; i++) {
        stream << (int) entity.slotsNeighborDiscovery[i] << ", ";
    }
    stream << "}";
    stream << ", neigh_disc_channel_list={";
    for (std::vector<Uint8>::const_iterator it = entity.neigh_disc_channel_list.begin(); it
                != entity.neigh_disc_channel_list.end(); ++it) {
        stream << (int) (*it) << ", ";
    }
    stream << "}";
    stream << ", enableAlertsForGateway=" << entity.enableAlertsForGateway;
    stream << ", enableMultiPath=" << entity.enableMultiPath;
    return stream;

}

}
}
