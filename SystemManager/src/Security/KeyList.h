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
 * KeyList.h
 *
 *  Created on: Mar 23, 2009
 *      Author: Andy, beniamin.tecar
 */

#ifndef KEYLIST_H_
#define KEYLIST_H_

#include <Common/Address128.h>
#include <Common/smTypes.h>
#include <Model/SecurityKey.h>
#include <Model/Policy.h>
#include "Common/ClockSource.h"
#include "Common/SmSettingsLogic.h"
#include <list>
#include <limits>

namespace Isa100 {
namespace Security {

struct Key {
        NE::Model::SecurityKey key;
        NE::Model::Policy sessionKeyPolicy;
        Uint32 softLifeTime;
        Uint32 hardLifeTime;

};

typedef std::map<NE::Model::KeyID, Key> KeyMapT;

struct SecKey {
        NE::Common::Address128 destination;
        NE::Common::Address128 source;
        Common::TSAP::TSAP_Enum destinationTSAP;
        Common::TSAP::TSAP_Enum sourceTSAP;
        KeyMapT keyMap;
        NE::Model::KeyID lastKeyId;

        bool sentToDevice;

        SecKey(NE::Common::Address128 source_, NE::Common::Address128 destination_, Common::TSAP::TSAP_Enum sourceTSAP_,
                    Common::TSAP::TSAP_Enum destinationTSAP_) :
            destination(destination_), source(source_), destinationTSAP(destinationTSAP_), sourceTSAP(sourceTSAP_) {
            sentToDevice = false;
        }
};

typedef std::list<SecKey> KeyListT;

class KeyList {
        LOG_DEF("I.S.KeyList");

    public:
        bool AddKey(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP, NE::Model::KeyID keyID, const NE::Model::Policy &sessionKeyPolicy,
                    const NE::Model::SecurityKey& key) {
            LOG_INFO("AddKey source=" << source.toString() << ", destination=" << destination.toString() << ", sourceTSAP="
                        << sourceTSAP << ", destinationTSAP=" << destinationTSAP << ", keyID=" << (int) keyID);

            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            Uint32 offset = GetOffsetInSeconds(sessionKeyPolicy);
            ClockSource cs;

            Key newKey;
            newKey.key = key;
            newKey.sessionKeyPolicy = sessionKeyPolicy;
            newKey.softLifeTime = cs.getTAI(Isa100::Common::SmSettingsLogic::instance()) + offset / 2;
            newKey.hardLifeTime = cs.getTAI(Isa100::Common::SmSettingsLogic::instance()) + offset;

            if (it != keys.end()) {

                if (it->keyMap.find(keyID) != it->keyMap.end()) {
                    it->keyMap.erase(keyID);
                }

                it->keyMap.insert(std::make_pair(keyID, newKey));
                it->lastKeyId = keyID;
            } else {
                keys.push_back(SecKey(source, destination, sourceTSAP, destinationTSAP));
                keys.rbegin()->keyMap.insert(std::make_pair(keyID, newKey));
                keys.rbegin()->lastKeyId = keyID;

            }

            return true;
        }

        bool GetKeyLifetime(const NE::Common::Address128& source, const NE::Common::Address128& destination,
                    Common::TSAP::TSAP_Enum sourceTSAP, Common::TSAP::TSAP_Enum destinationTSAP, Uint32& softLifeTime,
                    Uint32& hardLifeTime) {
            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);
            if (it != keys.end()) {
                softLifeTime = it->keyMap[it->lastKeyId].softLifeTime;
                hardLifeTime = it->keyMap[it->lastKeyId].hardLifeTime;

                return true;
            }

            return false;
        }

        void EraseOldKeys(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP) {
            LOG_INFO("EraseOldKeys source=" << source.toString() << ", destination=" << destination.toString() << ", sourceTSAP="
                        << sourceTSAP << ", destinationTSAP=" << destinationTSAP);

            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                ClockSource cs;

                for (KeyMapT::iterator keyIt = it->keyMap.begin(); keyIt != it->keyMap.end();) {
                    if (keyIt->second.softLifeTime < cs.getTAI(Isa100::Common::SmSettingsLogic::instance())) {
                        it->keyMap.erase(keyIt++);
                    } else {
                        keyIt++;
                    }
                }

            }
        }

        bool FindKey(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP, NE::Model::SecurityKey &securityKey, NE::Model::KeyID & outKey,
                    NE::Model::Policy &sessionKeyPolicy) {
            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                ClockSource cs;
                Uint32 greatestSoft = 0;
                bool foundKey = false;
                for (KeyMapT::iterator keyIt = it->keyMap.begin(); keyIt != it->keyMap.end(); keyIt++) {
                    if ((greatestSoft < keyIt->second.softLifeTime) || ((greatestSoft == keyIt->second.softLifeTime) && (outKey
                                < keyIt->first))) {
                        greatestSoft = keyIt->second.softLifeTime;
                        outKey = keyIt->first;
                        securityKey = keyIt->second.key;

                        sessionKeyPolicy = keyIt->second.sessionKeyPolicy;
                        sessionKeyPolicy.KeyHardLifetime.KeyHardLifetime = (keyIt->second.hardLifeTime - cs.getTAI(
                                    Isa100::Common::SmSettingsLogic::instance())) / GetGranularityInSeconds(sessionKeyPolicy);
                        foundKey = true;
                    }
                }
                return foundKey;
            }

            return false;
        }

        Uint8 GetLastKeyID(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP) {
            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                return it->lastKeyId;
            }

            return 0xFF;
        }

        bool FindKeysForSource(NE::Common::Address128 source, KeyListT& keysList) {
            for (KeyListT::iterator it = keys.begin(); it != keys.end(); ++it) {
                if (it->source == source) {
                    keysList.push_back(*it);
                }
            }
            return true;
        }

        bool IsNewSession(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP) {
            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                if (it->keyMap.size() > 0)
                    return false;
            }

            return true;
        }


        Uint32 GetOffsetInSeconds(const NE::Model::Policy &sessionKeyPolicy) {
            return sessionKeyPolicy.KeyHardLifetime.KeyHardLifetime * GetGranularityInSeconds(sessionKeyPolicy);
        }

        Uint32 GetGranularityInSeconds(const NE::Model::Policy &sessionKeyPolicy) {
            Uint32 offset = 1;
            switch (sessionKeyPolicy.KeyTypeUsage.Granularity) {
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


        void RemoveKey(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP) {
            LOG_INFO("RemoveKey source=" << source.toString() << ", destination=" << destination.toString() << ", sourceTSAP="
                        << sourceTSAP << ", destinationTSAP=" << destinationTSAP);

            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                keys.erase(it);
            }
        }

        void RemoveLastKey(NE::Common::Address128 source, NE::Common::Address128 destination, Common::TSAP::TSAP_Enum sourceTSAP,
                    Common::TSAP::TSAP_Enum destinationTSAP) {

            LOG_INFO("RemoveLastKey source=" << source.toString() << ", destination=" << destination.toString()
                        << ", sourceTSAP=" << sourceTSAP << ", destinationTSAP=" << destinationTSAP);

            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                Uint8 keyID = it->lastKeyId;
                if (it->keyMap.find(keyID) != it->keyMap.end()) {

                    it->keyMap.erase(keyID);
                    if (it->keyMap.empty()) {
                        keys.erase(it);
                    } else {
                        if (it->lastKeyId > 0)
                            --it->lastKeyId;
                    }
                }
            }
        }

        void RemoveAllSessionKeysExceptLast(NE::Common::Address128 source, NE::Common::Address128 destination,
                    Common::TSAP::TSAP_Enum sourceTSAP, Common::TSAP::TSAP_Enum destinationTSAP) {
            LOG_INFO("RemoveAllSessionKeysExceptLast source=" << source.toString() << ", destination=" << destination.toString()
                        << ", sourceTSAP=" << sourceTSAP << ", destinationTSAP=" << destinationTSAP);

            KeyListT::iterator it = FindKey(source, destination, sourceTSAP, destinationTSAP);

            if (it != keys.end()) {
                Uint8 keyID = it->lastKeyId;

                if (it->keyMap.find(keyID) != it->keyMap.end()) {
                    KeyMapT::iterator it2 = it->keyMap.begin();
                    while (it2 != it->keyMap.end()) {
                        if (it2->first != keyID) {
                            it->keyMap.erase(it2++);
                        } else {
                            ++it2;
                        }
                    }

                    if (it->keyMap.empty()) {
                        keys.erase(it);
                    }
                }
            }
        }

    private:
        KeyListT::iterator FindKey(const NE::Common::Address128& source, const NE::Common::Address128& destination,
                    Common::TSAP::TSAP_Enum sourceTSAP, Common::TSAP::TSAP_Enum destinationTSAP) {
            for (KeyListT::iterator it = keys.begin(); it != keys.end(); ++it) {
                if (it->destination == destination && it->source == source && it->destinationTSAP == destinationTSAP
                            && it->sourceTSAP == sourceTSAP) {
                    return it;
                }
            }

            return keys.end();
        }

        KeyListT keys;
};

}
}
#endif /* KEYLIST_H_ */
