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
 * BlacklistChannels.h
 *
 *  Created on: Jun 25, 2009
 *      Author: Andy
 */

#ifndef BLACKLISTCHANNELS_H_
#define BLACKLISTCHANNELS_H_

#include <nlib/log.h>

#include "Common/NEAddress.h"
#include "Model/IDevicesProvider.h"
#include "Model/Tdma/ChannelDiagnostics.h"



namespace NE {
namespace Model {

/**
 * Handles blacklisting channels that are not usable.
 *
 */

struct BlacklistChannel {
        Uint8 ChannelNo;
        Uint32 BlacklistedTime;

        // redefined for std::remove
        bool operator==(Uint32 value) {
            return BlacklistedTime == value;
        }
};

class BlacklistChannels {
    public:
        LOG_DEF("NE.M.BlacklistChannels")
        ;

        BlacklistChannels() { }

         void getBlacklistedChannels(std::vector<Uint8>& blackListedChannels);

         std::vector<BlacklistChannel>& getBlacklistedChannels() {
             return blacklistedChannels;
         }


         void removeUnselectedChannels();

         void createChannelMap(Uint16 &channelMapMask);


        bool IsBlacklistedChannel(Uint8 channelID);

        void modifyTimeToReset(Uint8 channelID, Uint32 channelBlacklistingKeepPeriod);

        void insertChannel(BlacklistChannel blChannel) {
            blacklistedChannels.push_back(blChannel);
        }



    private:


        std::vector<BlacklistChannel> blacklistedChannels;

        bool enabled;


};

}
}

#endif /* BLACKLISTCHANNELS_H_ */
