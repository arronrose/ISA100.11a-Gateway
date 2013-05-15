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

#ifndef SECURITYMANAGER_H_
#define SECURITYMANAGER_H_

#include <map>
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <Common/NETypes.h>
#include <Common/NEAddress.h>
#include <Common/logging.h>
#include <Common/Objects/SFC.h>
#include <Common/NEException.h>
#include <Model/SecurityKey.h>
#include <Security/KeyGenerator.h>
#include <Security/SecurityException.h>
#include <Model/Policy.h>
#include <Model/model.h>
#include <Model/Device.h>
#include <Security/SecuritySymJoinRequest.h>
#include <Security/SecuritySymJoinResponse.h>
#include <Security/SecurityConfirmRequest.h>
#include <Security/SecurityKeyAndPolicies.h>
#include "Security/SecurityDeleteKeyReq.h"
#include "Security/SecurityNewSessionRequest.h"
#include "Security/SecurityNewSessionResponse.h"
#include <Common/smTypes.h>
#include <Model/Operations/OperationsContainer.h>

using namespace Isa100::Common;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Security {

/**
 * Security Manager is the only entity that generates security keys in system
 * It deals with device provisioning, device join and device contract
 * @author Sorin.bidian, catalin.pop, andrei.petrut, flori.parauan, ioan.pocol, beniamin.tecar, florin.muresan
 * @version 1.0
 */

/**
 * Callback for the security join request.
 * Parameters: bool - status
 * 			   SecurityJoinResponse - the response
 */
typedef boost::function3<void, bool, Security::SecuritySymJoinResponse, int> SecuritySymJoinResponseCallback;

namespace SecurityJoinFailureReason {
enum SecurityJoinFailureReason {
    success = 0,
    bad_join_key = 1,
    subnet_not_foud = 2,
    backbone_not_found = 3,
    dl_key_not_found = 4,
    unprovisioned_device = 5,
    invalid_aes_alg = 6
};
}

#define INPUT_HASH_BUFFERSIZE 86 //87
/*
 16 + 	// challenge from security manager
 16 + 	// challenge from new device
 8 +		// EUI64 new device
 8 +		// EUI64 security manager
 2 +		// Master Key Policy
 3 +		// DL Key Policy
 3 +		// SysMan Session Key Policy
 16 +	// Encrypted DL key
 16;		// Encrypted SysMan session key
 */

/**
 * Used to store security keys along with their offset expressed in TAI.
 */
struct SecurityKeyAndTAIOffset {
        NE::Model::SecurityKey securityKey;
        Uint32 keyTAIOffset;

        SecurityKeyAndTAIOffset() :
            keyTAIOffset(0) {

        }
};

class SecurityManager: boost::noncopyable {
        LOG_DEF("I.S.SecurityManager");

    public:

        /** Join operation type */
        static const int TYPE_SIZE = 1;

        /** The lenght of the nonce used by Ccm alghoritm */
        static const int NONCE_SIZE = 13;

        /** The lenght of the challange exchanged between new device and Security Manager */
        static const int CHALLENGE_SIZE = 4;

        /** The lenght of authentication data used by Ccm alghoritm */
        static const int M = 4;

        /** Join request command */
        static const Uint8 JOIN_REQUEST = 1;

        /** Join reply command */
        static const Uint8 JOIN_REPLAY = 2;

        /** Join confirmation command */
        static const Uint8 JOIN_CONFIRMATION = 3;

        /** Fill characters used by Ccm alghoritm */
        static const Uint8 FILL_VALUE = 0;

        /** The device in not provisioned */
        static const Uint8 CAUSE_NOT_PROVISIONED = 1;

        /** The device has no join request */
        static const Uint8 CAUSE_NO_JOIN_REQUEST = 2;

        /** The the challange is not valid */
        static const Uint8 CAUSE_INVALID_CHALENGE = 3;

        virtual ~SecurityManager();

        //static SecurityManager& instance();
        SecurityManager();

    private:
        // SecurityManager();

    public:

        /**
         * Performs a check on challenge contained by request. It makes sure that the challenge was not used before
         * by the joining device. A join request containing a challenge that has already been used by the device in the previous
         * join process is not valid.
         *
         * @return false if the challenge is not valid
         */
        bool checkChallenge(const SecuritySymJoinRequest& request);

        /**
         * Calls createSecurityKey method to create a new SecurityKey between manager and the specified device; returns a PhySessionKey structure containing
         * this SecurityKey, the keyID, and the index.
         */
        NE::Model::PhySessionKey createJoinKey(const Address64& joiningDeviceAddress, Uint16 keyID, Uint16 index);

        /**
         * Creates a new SecurityKey between the specified devices; returns a PhySessionKey structure containing
         * this SecurityKey, the keyID, and the index.
         */
        NE::Model::PhySessionKey createSecurityKey(const NE::Common::Address64& from64, const NE::Common::Address64& to64,
                    const NE::Common::Address128& from128, const NE::Common::Address128& to128,
                    Isa100::Common::TSAP::TSAP_Enum fromTSAP, Isa100::Common::TSAP::TSAP_Enum toTSAP, Uint16 keyID, Uint16 index);

        /**
         * Creates the security join response and sets the security key in TLMO.
         * The response is sent along with a boolean status, through the callback function.
         */
        void SecurityJoinRequest(const SecuritySymJoinRequest& request, const NE::Model::SecurityKey& securityKey, Uint16 keyID,
                    NE::Model::PhySpecialKey& masterKey, NE::Model::PhySpecialKey& subnetKey,
                    SecuritySymJoinResponseCallback responseCallback);

        bool SecurityJoinConfirm(const Address64& address64, const SecurityConfirmRequest& confirm);

        /**
         * Set the Key Generator used to generate new keys
         * @param KeyGenerator
         */
        void setKeyGenerator(KeyGenerator* keyGenerator);

        /**
         * Reset the value of the next chellenge
         */
        void setNextChallenge(Uint32 nextChallenge);

        /**
         * Used to authenticate the request for a new session.
         */
        bool validateNewSessionRequest(SecurityNewSessionRequest newSessionRequest, Device *requester);

        /**
         * Creates new security keys and populates keyFrom and keyTo structures with the new keys and policies.
         */
        void createNewSessionKeys(const Address64& from, const Address64& to, Isa100::Common::TSAP::TSAP_Enum tsapFrom,
                    Isa100::Common::TSAP::TSAP_Enum tsapTo, SecurityKeyAndPolicies& keyFrom, SecurityKeyAndPolicies& keyTo);

        /**
         * Populates the responseKey structure with the key and policy based on phyKey.
         */
        void createNewSessionKey(NE::Model::PhySessionKey& phyKey, SecurityKeyAndPolicies& responseKey);

        /**
         * Method for setting keys in TLMO.
         */
        void setStackKeys(const SecuritySymJoinRequest& request, const NE::Model::PhySessionKey& sessionKey,
                    const NE::Common::Address128& joiningDeviceAddress128);

        /**
         * Method for setting in stack a key between SM and peer.
         */
        Isa100::Common::Objects::SFC::SFCEnum setStackKeyWithPeer(const Address64& peer, const Address128& peer128,
                    Isa100::Common::TSAP::TSAP_Enum tsapFrom, Isa100::Common::TSAP::TSAP_Enum tsapTo,
                    const NE::Model::SecurityKey &key, Uint8 newKeyID, const NE::Model::Policy &newSessionPolicy);

        /**
         * Delete the keys from stack for the given key list.
         */
        Isa100::Common::Objects::SFC::SFCEnum deleteStackKey(const NE::Common::Address128& destination,
                    Isa100::Common::TSAP::TSAP_Enum tsapFrom, Isa100::Common::TSAP::TSAP_Enum tsapTo, Uint16 keyID);

        void getKeyDeletionParams(NE::Model::PhySessionKey& phyKey, SecurityDeleteKeyReq& deleteKeyParams);

        void getNewSessionResponse(Device *requester, const Uint8 status, SecurityNewSessionResponse& newSessionResponse);

        /*
         * Searches the model for expiring keys (in theory softLifetime but here a bit earlier - deviation defines how much earlier).
         * When such a key is found on a device it is marked as expiring and the peer key on the peer device is searched and marked as expiring also.
         * New keys are created to replace these two.
         * Expiring keys already marked are skipped.
         */
        void updateExpiringKeys(Uint32 deviation);

        /**
         * Method called when the creation of a new DL subnet key or master key is requested by a device.
         * The expiring time of the two keys is checked and a new key is created to replace the expiring key.
         * If the both keys are expiring the DL key will be created and a new request will be expected to create the master key also.
         */
        bool createNewSpecialKey(Device *device);

        /**
         * Method called to populate the SecurityKeyAndPolicies structure needed to send the master key to the device.
         */
        void prepareMasterKey(NE::Model::PhySpecialKey& phyKey, Address32 deviceAddress32, SecurityKeyAndPolicies& responseKey);

        /**
         * Method called to populate the SecurityKeyAndPolicies structure needed to send the DL subnet key to the device.
         */
        void prepareSubnetKey(NE::Model::PhySpecialKey& phyKey, Address32 deviceAddress32, SecurityKeyAndPolicies& responseKey);

    private:

        /**
         * Populates the provisionedKeys member. Keys are obtained from SmSettingsLogic.
         */
        //        void initProvisionedKeys();

        void GetHashInputString(const SecuritySymJoinRequest& request, const SecuritySymJoinResponse& response,
                    Uint8(&buffer)[INPUT_HASH_BUFFERSIZE]);

        //        void SetKeys(const SecuritySymJoinRequest& request, NE::Model::SecurityKey& dlKey,
        //                    const NE::Model::SecurityKey& sessionKey, NE::Model::SecurityKey& masterKey, Uint16 keyID, bool NotEncrypted);

        bool ValidateJoinRequestMIC(const SecuritySymJoinRequest& request, const NE::Model::SecurityKey& joinKey);

        void GenerateMasterKey(const Security::SecuritySymJoinRequest& request, const SecuritySymJoinResponse& response,
                    const NE::Model::SecurityKey& joinKey, NE::Model::SecurityKey& masterKey);

        /**
         * Searches for the newest master key on device.
         */
        NE::Model::PhySpecialKey * getNewestMasterKey(Device *srcDevice);

        /**
         * Searches for the newest master key that is active too (i.e. NotValidBefore is not in the future).
         */
        NE::Model::PhySpecialKey * getNewestActiveMasterKey(Device *srcDevice);

        /**
         * Searches for the newest DL key that is active too (i.e. NotValidBefore is not in the future).
         */
        NE::Model::PhySpecialKey * getNewestActiveSubnetKey(Device *srcDevice);

        /**
         * Searches for the newest subnet key on device, including pending keys.
         */
        NE::Model::PhySpecialKey * getNewestSubnetKeyPendingIncluded(Device *srcDevice);

        /**
         * Returns the greatest master key ID out of the existent keys.
         */
        Uint16 getGreatestMasterKeyID(Device *device);

        /**
         * Returns the greatest DL key ID out of the existent keys.
         */
        Uint16 getGreatestSubnetKeyID(Device *device);

        SecurityKey GetSubnetKey(Uint16 subnetID);

        /**
         * Creates a new master key and generates an operation to be sent.
         */
        void createNewMasterKeyOperation(Device *device, Uint32 currentTAI,
                    NE::Model::Operations::OperationsContainerPointer& operationsContainer);

        /**
         * Creates a new subnet key and generates an operation to be sent.
         */
        PhySpecialKey * createNewSubnetKeyOperation(Device *device, Uint32 currentTAI,
                    NE::Model::Operations::OperationsContainerPointer& operationsContainer);

        /**
         * Returns an existing key 'from' -> 'to' or a new key if there isn't an existing one.
         * If an existing key is found, the keyID parameter takes the value of the keyID associated to that key.
         */
        NE::Model::SecurityKey getSessionKey(const NE::Common::Address64& from, const NE::Common::Address64& to,
                    Isa100::Common::TSAP::TSAP_Enum fromTSAP, Isa100::Common::TSAP::TSAP_Enum toTSAP);


        void EncryptAndAuthSessionKey(SecurityKeyAndPolicies& sessionKey, NE::Model::SecurityKey& masterKey);


        /**
         * Populates the SecurityKeyAndPolicies structure, encrypts and authenticates the key.
         * Called by createNewSessionKey method.
         */
        void prepareKey(const Address64& from, const Address64& to, const Address128& from128, const Address128& to128,
                    Isa100::Common::TSAP::TSAP_Enum tsapFrom, Isa100::Common::TSAP::TSAP_Enum tsapTo,
                    const NE::Model::SecurityKey& secKey, Uint16 keyID, const NE::Model::Policy& newSessionPolicy,
                    SecurityKeyAndPolicies& sessionKey);

        /**
         * Creates new session keys to replace the old ones sent as parameters.
         * The generated operations are placed in the operationsContainer.
         */
        void createNewSessionKeysPair(Device *device, Device *destDevice, PhySessionKey& key, PhySessionKey& peerKey,
                    Uint16 newKeyID, Uint32 currentTAI, NE::Model::Operations::OperationsContainerPointer& operationsContainer);

        /**
         * Searches and sends updates for expiring keys on gateway: session keys with manager and master key.
         */
        void updateExpiringKeysOnGateway(Uint32 deviation, Uint32 currentTAI);

    private:

        //std::map<NE::Common::Address64, SecurityKeyAndTAIOffset> masterKeys;

        typedef std::map<Uint16, SecurityKeyAndTAIOffset> DlKeysMap;
        DlKeysMap dlKeys;

        //the key generator
        KeyGenerator* keyGenerator;

        Uint32 getNextChallenge();

        NE::Model::CompressedPolicy masterPolicy;
        NE::Model::CompressedPolicy dlKeyPolicy;
        NE::Model::CompressedPolicy sysMgrSessionKeyPolicy;

        /**
         * Default policy for session keys.
         */
        NE::Model::Policy defaultSessionPolicy;

        /**
         * Default policy for master keys.
         */
        NE::Model::Policy defaultMasterPolicy;

        /**
         * Default policy for DL keys.
         */
        NE::Model::Policy defaultSubnetPolicy;

        //HACK: [andy] for map, can't put array as value type
        struct Uint128Wrapper {
                Isa100::Common::Uint128 uint128;

                Uint128Wrapper() {
                    for (int i = 0; i < 16; i++) {
                        uint128[i] = 0;
                    }
                }
        };

        struct PreviousChallenges {
            Uint128Wrapper previousChallenge1; //most recent
            Uint128Wrapper previousChallenge2;
        };

        // holds challenges contained in join requests
        std::map<NE::Common::Address64, PreviousChallenges> joinRequestChallenges;

        // holds challenges assigned to devices at creation of SecuritySymJoinResponse
        // used by SecurityJoinConfirm method
        std::map<NE::Common::Address64, Uint128Wrapper> assignedChallenges;

};

}
}

#endif /*SECURITYMANAGER_H_*/
