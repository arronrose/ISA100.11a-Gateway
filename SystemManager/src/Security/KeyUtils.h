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
 * KeyUtils.h
 *
 *  Created on: Sep 15, 2009
 *      Author: Sorin.Bidian, beniamin.tecar
 */

#ifndef KEYUTILS_H_
#define KEYUTILS_H_

#include <Model/EngineProvider.h>
#include <Model/Device.h>
#include <Common/smTypes.h>
#include "Common/ClockSource.h"
#include <limits>
#include "Common/SmSettingsLogic.h"

namespace Isa100 {
namespace Security {

/**
 * Class containing static functions for keys-related processing.
 * It is called by the Security Manager and it accesses the sessions keys in the Physical Model.
 */
class KeyUtils {

        LOG_DEF("I.S.KeyUtils");

    public:

        static Uint32 GetOffsetInSeconds(Common::SmSettingsLogic &smSettingsLogic, const NE::Model::Policy& sessionKeyPolicy) {
            return smSettingsLogic.keysHardLifeTime * GetGranularityInSeconds(sessionKeyPolicy);
        }

        static Uint32 GetGranularityInSeconds(const NE::Model::Policy& sessionKeyPolicy) {
            Uint32 offset = 1;
            switch (sessionKeyPolicy.Granularity) {
                case 0x01:
                    offset = 60; //softlifetime in minutes
                break;
                case 0x02:
                    offset = 3600; //softlifetime in hours
                break;
                case 0x03:
                    offset = 86400; //softlifetime in days
                break;
                default:
                break;
            }
            return offset;
        }

        static NE::Model::PhySessionKey getAsPhySessionKey(const NE::Model::SecurityKey& securityKey,
                    const NE::Common::Address64& from64, const NE::Common::Address64& to64,
                    const NE::Common::Address128& from128, const NE::Common::Address128& to128,
                    Isa100::Common::TSAP::TSAP_Enum fromTSAP, Isa100::Common::TSAP::TSAP_Enum toTSAP,
                    const NE::Model::Policy& sessionPolicy, Uint16 keyID, Uint16 index) {

            NE::Model::PhySessionKey phySessionKey;
            phySessionKey.index = index;
            phySessionKey.keyID = keyID;
            phySessionKey.destination64 = to64;
            phySessionKey.source64 = from64;
            phySessionKey.destination128 = to128;
            phySessionKey.source128 = from128;
            phySessionKey.destinationTSAP = toTSAP;
            phySessionKey.sourceTSAP = fromTSAP;
            //phySessionKey.lastKeyId = keyID;
            phySessionKey.key = securityKey;
            phySessionKey.sessionKeyPolicy = sessionPolicy;

            Uint32 offset = GetOffsetInSeconds(Common::SmSettingsLogic::instance(), sessionPolicy);
            if (sessionPolicy.NotValidBefore == 0) {
                Uint32 currentTAI = NE::Common::ClockSource::getTAI(Common::SmSettingsLogic::instance());
                phySessionKey.softLifeTime = currentTAI + offset / 2;
                phySessionKey.hardLifeTime = currentTAI + offset;
            } else {
                phySessionKey.softLifeTime = sessionPolicy.NotValidBefore + offset / 2;
                phySessionKey.hardLifeTime = sessionPolicy.NotValidBefore + offset;
            }

            return phySessionKey;
        }

        static int GetGranularityMultiplier(Uint8 granularity) {
            switch (granularity) {
                case 0x00:
                    return 1;
                case 0x01:
                    return 60;
                case 0x02:
                    return 3600;
                case 0x03:
                    return 86400;
                default:
                    return 1;
            }
        }

        static void updateKey(Uint32 currentTAI, Common::SmSettingsLogic &smSettingsLogic, NE::Model::PhySessionKey *key) {

            //compute NotValidBefore as the softLifeTime of expiring key + 1 hour
            //softLifeTime of expiring key is current hour + 1 (detection of expiring is done before actual expiring)
            //so +1 to obtain the hour of softLifeTime and +1 to set NotValidBefore one hour in the future
            Uint32 granularityInSeconds = GetGranularityInSeconds(key->sessionKeyPolicy);
            //key->sessionKeyPolicy.NotValidBefore = (currentTAI - currentTAI % granularityInSeconds) + 2 * granularityInSeconds;
            key->sessionKeyPolicy.NotValidBefore = (currentTAI - currentTAI % granularityInSeconds) + 3 * granularityInSeconds; //varianta cu minim 6 lifetime

            Uint32 offset = GetOffsetInSeconds(smSettingsLogic, key->sessionKeyPolicy);
            key->softLifeTime = key->sessionKeyPolicy.NotValidBefore + offset / 2;
            key->hardLifeTime = key->sessionKeyPolicy.NotValidBefore + offset;
        }

        //used for master keys and DL keys; lifetime for these keys is always expressed in hours (internally kept in seconds)
        static void updateKey(Uint32 currentTAI, Common::SmSettingsLogic &smSettingsLogic, NE::Model::PhySpecialKey *key) {
            //key->policy.NotValidBefore = (currentTAI - currentTAI % 3600) + 2 * 3600;
            key->policy.NotValidBefore = (currentTAI - currentTAI % 3600) + 3 * 3600; //varianta cu minim 6 lifetime

            Uint32 offset = GetOffsetInSeconds(smSettingsLogic, key->policy);
            key->softLifeTime = key->policy.NotValidBefore + offset / 2;
            key->hardLifeTime = key->policy.NotValidBefore + offset;
        }

        /**
         * Searches the Physical Model for keys (source, tsap) -> (destination, tsap). If there are more keys found,
         * the one with the greatest lifetime is chosen.
         * Returns false if no key is found.
         */
        static bool findKey(const Address32& source, const Address32& destination,
                    Isa100::Common::TSAP::TSAP_Enum sourceTSAP, Isa100::Common::TSAP::TSAP_Enum destinationTSAP,
                    NE::Model::SecurityKey& securityKey, /*Uint16& outKeyID,*/ NE::Model::Policy& sessionKeyPolicy) {

            NE::Model::Device * device = Isa100::Model::EngineProvider::getEngine()->getDevice(source);
            if (!device) {
                std::ostringstream stream;
                stream << "Device " << Address::toString(source) << " not found.";
                LOG_ERROR(stream.str());
                throw NEException(stream.str());
            }

            NE::Model::Device * destinationDevice = Isa100::Model::EngineProvider::getEngine()->getDevice(destination);
            if (!destinationDevice) {
                std::ostringstream stream;
                stream << "Device " << Address::toString(destination) << " not found.";
                LOG_ERROR(stream.str());
                throw NEException(stream.str());
            }

            Uint32 greatestSoft = 0;
            bool foundKey = false;

            Uint32 currentTAI = NE::Common::ClockSource::getTAI(Common::SmSettingsLogic::instance());

            for (NE::Model::SessionKeyIndexedAttribute::iterator it = device->phyAttributes.sessionKeysTable.begin(); it
                        != device->phyAttributes.sessionKeysTable.end(); ++it) {

                if (it->second.currentValue
                            && it->second.currentValue->destination64 == destinationDevice->address64
                            && it->second.currentValue->destinationTSAP == destinationTSAP
                            && it->second.currentValue->sourceTSAP == sourceTSAP) {

                    if (greatestSoft < it->second.currentValue->softLifeTime) {

                        greatestSoft = it->second.currentValue->softLifeTime;
                        //outKeyID = it->second.currentValue->keyID;
                        securityKey = it->second.currentValue->key;
                        sessionKeyPolicy = it->second.currentValue->sessionKeyPolicy;
                        sessionKeyPolicy.KeyHardLifetime = (it->second.currentValue->hardLifeTime - currentTAI) / GetGranularityInSeconds(sessionKeyPolicy);
                        foundKey = true;
                    }
                }
            }
            return foundKey;
        }


};

}
}

#endif /* KEYUTILS_H_ */
