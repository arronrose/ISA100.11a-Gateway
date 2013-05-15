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
 * BlacklistChannels.cpp
 *
 *  Created on: Jun 29, 2009
 *      Author: Andy
 */

#include "BlacklistChannels.h"

#include <Model/NetworkEngine.h>
#include "SMState/SMStateLog.h"
#include "Model/Operations/OperationsProcessor.h"

namespace NE {
namespace Model {

using namespace NE::Model::Operations;



bool BlacklistChannels::IsBlacklistedChannel(Uint8 channelID)
{
	for (std::vector<BlacklistChannel>::iterator it = blacklistedChannels.begin(); it != blacklistedChannels.end(); ++it)
	{
		if (it->ChannelNo == channelID)
			return true;
	}

	return false;
}

void BlacklistChannels::modifyTimeToReset(Uint8 channelID, Uint32 currentTime) {
    for (std::vector<BlacklistChannel>::iterator it = blacklistedChannels.begin(); it != blacklistedChannels.end(); ++it)
    {
        if (it->ChannelNo == channelID) {
            it->BlacklistedTime = currentTime;
        }

    }

}

void BlacklistChannels::removeUnselectedChannels() {
    blacklistedChannels.erase(std::remove(blacklistedChannels.begin(), blacklistedChannels.end(), 0), blacklistedChannels.end());

}


void BlacklistChannels::createChannelMap( Uint16 &channelMapMask) {

    for (std::vector<BlacklistChannel>::iterator it = blacklistedChannels.begin(); it != blacklistedChannels.end(); ++it)
    {
        channelMapMask &=  ~(1 << it->ChannelNo);
    }


}



void BlacklistChannels::getBlacklistedChannels(std::vector<Uint8>& blackListedChannels) {
	for (std::vector<BlacklistChannel>::iterator it = blacklistedChannels.begin(); it != blacklistedChannels.end(); ++it) {
	    blackListedChannels.push_back(it->ChannelNo);
	}


}

}
}
