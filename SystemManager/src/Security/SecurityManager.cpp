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

/**
 * @author @author Sorin.bidian, catalin.pop, andrei.petrut, flori.parauan, ioan.pocol, beniamin.tecar, florin.muresan
 */
#include "Security/SecurityManager.h"
#include <netinet/in.h>
#include <cstdlib>
#include "Security/DeviceSecurityInfo.h"
#include "Security/MockKeyGenerator.h"
#include "Common/SmSettingsLogic.h"
#include <Common/Address128.h>
#include <Common/Address128.h>
#include <Common/CCM/Ccm2.h>
#include <Common/ClockSource.h>
#include <Model/EngineProvider.h>
#include "Algorithms.h"
#include "Misc/Marshall/NetworkOrderBytesWriter.h"
#include <AL/DMAP/TLMO.h>
#include "KeyUtils.h"
#include "Model/EntitiesHelper.h"
#include "Model/Operations/WriteAttributeOperation.h"
#include <Misc/Marshall/NetworkOrderStream.h>
#include "Model/ModelPrinter.h"

using namespace Isa100::Common::Objects;
using namespace Isa100::Common;
using namespace NE::Model;
using namespace NE::Model::Operations;

namespace Isa100 {
namespace Security {

Uint8 NO_SECURITY_KEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const Uint16 NO_SECURITY_KEYLIFETIME = 0x0000;


SecurityManager::SecurityManager() {
    if (SmSettingsLogic::instance().activateMockKeyGenerator) {
        LOG_WARN("SecurityManager::SecurityManager()...MOCK KEY GENERATOR activated.. should not be used in release.");
        this->keyGenerator = new MockKeyGenerator();
    } else {
        this->keyGenerator = new KeyGenerator();
    }

    masterPolicy.KeyHardLifetime = 0x00; //0x4; //hours

    //48.5 days -- see "Nonce construction for data link layer protocol data units" in standard
    dlKeyPolicy.KeyHardLifetime = SmSettingsLogic::instance().subnetKeysHardLifeTime;

    defaultMasterPolicy.KeyHardLifetime = masterPolicy.KeyHardLifetime; // 0x00 for non-expiring
    defaultMasterPolicy.NotValidBefore = 0x0000;
    defaultMasterPolicy.KeyType = 0x01;
    defaultMasterPolicy.KeyUsage = 0x02;
    defaultMasterPolicy.Granularity = 0x02;

    defaultSubnetPolicy.KeyHardLifetime = dlKeyPolicy.KeyHardLifetime; // 0x00 for non-expiring
    defaultSubnetPolicy.NotValidBefore = 0x0000;
    defaultSubnetPolicy.KeyType = 0x01;
    defaultSubnetPolicy.KeyUsage = 0x00;
    defaultSubnetPolicy.Granularity = 0x02;

    sysMgrSessionKeyPolicy.KeyHardLifetime = SmSettingsLogic::instance().keysHardLifeTime;

    defaultSessionPolicy.KeyHardLifetime = SmSettingsLogic::instance().keysHardLifeTime;
    defaultSessionPolicy.NotValidBefore = 0x0000;
    defaultSessionPolicy.KeyType = 0x01;
    defaultSessionPolicy.KeyUsage = 0x01;
    defaultSessionPolicy.Granularity = 0x02;


    srand(time(NULL));
}

SecurityManager::~SecurityManager() {
    delete this->keyGenerator;
}

//TODO unify these into a log routine that is used everywhere
std::string StreamToString2(Uint8* input, Uint16 len) {
    std::ostringstream stream;
    for (int i = 0; i < len; i++) {
        stream << std::hex << ::std::setw(2) << ::std::setfill('0') << (int) *(input + i) << " ";
    }

    return stream.str();
}

bool SecurityManager::checkChallenge(const SecuritySymJoinRequest& request) {

    std::map<NE::Common::Address64, PreviousChallenges>::iterator it = joinRequestChallenges.find(request.EUI64JoiningDevice);

    LOG_DEBUG("device " << request.EUI64JoiningDevice.toString() //
                << " - new join request challenge= " << StreamToString2((Uint8*) request.bitChallengeFromNewDevice, 16));

    if (it != joinRequestChallenges.end()) {

        //LOG_DEBUG(" old challenge= " << StreamToString2((Uint8*) it->second.uint128, 16));
        LOG_DEBUG(" old challenges: " << StreamToString2((Uint8*) it->second.previousChallenge1.uint128, 16)
                    << ", " << StreamToString2((Uint8*) it->second.previousChallenge2.uint128, 16));

        bool equal = true;
        for (int i = 0; i < 16; i++) {
            if ((it->second.previousChallenge1.uint128[i] != request.bitChallengeFromNewDevice[i])
                        && (it->second.previousChallenge2.uint128[i] != request.bitChallengeFromNewDevice[i])) {
                equal = false;
                break;
            }
        }
        if (equal) {
            // the device last joined with this challenge
            LOG_WARN("challenge compare: found " << NE::Misc::Convert::array2string(request.bitChallengeFromNewDevice, 16) << " == "
                        << NE::Misc::Convert::array2string(it->second.previousChallenge1.uint128, 16)
                        << " or " << NE::Misc::Convert::array2string(it->second.previousChallenge2.uint128, 16));
            return false;
        }
    }

    PreviousChallenges& challenges = joinRequestChallenges[request.EUI64JoiningDevice];
    for (int i = 0; i < 16; i++) {
        challenges.previousChallenge2.uint128[i] = challenges.previousChallenge1.uint128[i];
        challenges.previousChallenge1.uint128[i] = request.bitChallengeFromNewDevice[i];
    }

    return true;
}

void SecurityManager::setKeyGenerator(KeyGenerator* keyGenerator) {
    delete this->keyGenerator;
    this->keyGenerator = keyGenerator;
}

NE::Model::PhySessionKey SecurityManager::createJoinKey(const Address64& joiningDeviceAddress, Uint16 keyID, Uint16 index) {

    const NE::Common::Address128 joiningDeviceAddress128; //not allocated yet, so it cannot be set

    NE::Model::SecurityKey securityKey;

    if (SmSettingsLogic::instance().disableTLEncryption) {
        securityKey.copyKey(NO_SECURITY_KEY);
    } else {
        //securityKey = keyGenerator->generateKey();
        securityKey = getSessionKey(SmSettingsLogic::instance().managerAddress64, joiningDeviceAddress,
                    TSAP::TSAP_SMAP, TSAP::TSAP_DMAP/*, defaultSessionPolicy*/);
    }

    return KeyUtils::getAsPhySessionKey(securityKey, SmSettingsLogic::instance().managerAddress64, joiningDeviceAddress,
                SmSettingsLogic::instance().managerAddress128, joiningDeviceAddress128, //
                TSAP::TSAP_SMAP, TSAP::TSAP_DMAP, defaultSessionPolicy, keyID, index);

}

NE::Model::PhySessionKey SecurityManager::createSecurityKey(const NE::Common::Address64& from64,
            const NE::Common::Address64& to64, const NE::Common::Address128& from128, const NE::Common::Address128& to128,
            TSAP::TSAP_Enum fromTSAP, TSAP::TSAP_Enum toTSAP, Uint16 keyID, Uint16 index) {

    NE::Model::SecurityKey securityKey;
    NE::Model::Policy newPolicy = defaultSessionPolicy;

    //set NotValidBefore to be the beginning of the current hour (substract seconds elapsed since the current hour has started)
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());
    Uint32 granularityInSeconds = KeyUtils::GetGranularityInSeconds(defaultSessionPolicy);
    //newPolicy.NotValidBefore = currentTAI - currentTAI % 3600;
    newPolicy.NotValidBefore = currentTAI - currentTAI % granularityInSeconds;

    if (SmSettingsLogic::instance().disableTLEncryption) {
        securityKey.copyKey(NO_SECURITY_KEY);
    } else {
        securityKey = getSessionKey(from64, to64, fromTSAP, toTSAP/*, newPolicy*/);
    }

    return KeyUtils::getAsPhySessionKey(securityKey, from64, to64, from128, to128, fromTSAP, toTSAP, newPolicy, keyID, index);
}

/**
 * Populate the Security_Sym_Join_Response data structure.
 * The Master_Key_Policy, DL_Key_Policy and Sys_Mgr_Session_Key_Policy shall have the structure of a compressed policy field.
 * Key Type = 0x01 - Symmetric-key keying material, encrypted
 * Key Usage = 0x00 - DLL key
 *             0x01 - session key
 *             0x02 - master key
 * A default granularity of 0x02 (hours) shall be used for the policies in the join response message.
 * The DL_Key_ID shall be the key ID associated with the DL Key sent in the join response.
 * The Key ID of the master key shall be set implicitly (not transmitted but inferred) as 0x00.
 * A hard lifetime special value of 0x00 shall be used for non-expiring keys.
 */
void SecurityManager::SecurityJoinRequest(const SecuritySymJoinRequest& request, const NE::Model::SecurityKey& securityKey,
            Uint16 keyID, NE::Model::PhySpecialKey& phyMasterKey, NE::Model::PhySpecialKey& phySubnetKey,
            SecuritySymJoinResponseCallback responseCallback) {

    NE::Model::SecurityKey joinKey;
    //std::map<Address64, SecurityKey>::iterator it = provisionedKeys.find(request.EUI64JoiningDevice);
    Isa100::Common::ProvisioningItem* provisioningItem = SmSettingsLogic::instance().getProvisioningForDevice(
                request.EUI64JoiningDevice);

    if (!provisioningItem) {
        LOG_ERROR("Join response dropped because of unprovisioned device address: " << request.EUI64JoiningDevice.toString());
        responseCallback(false, SecuritySymJoinResponse(), SecurityJoinFailureReason::unprovisioned_device);
        return;
    }

    joinKey = provisioningItem->key;
    SecuritySymJoinResponse response;
    Uint8 algorithmIdentifier = request.algorithmIdentifier & 0x0F;
    if (algorithmIdentifier == 0x01) { // AES_CCM

        if (ValidateJoinRequestMIC(request, joinKey)) {

            memset(response.BitChallengeFromSystemManager, 0, 16);
            Uint32 challenge = keyGenerator->getNextChallenge();
            memcpy(&response.BitChallengeFromSystemManager[0], (char*) &challenge, sizeof(challenge));

            Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());

            Uint16 subnetID = provisioningItem->subnetId;
            CompressedPolicy dlTempPolicy;

            if (provisioningItem->deviceType == Isa100::Common::ProvisioningItem::GATEWAY) {
                //no need for DL key on gateway but on join response DL related fields must be filled
                phySubnetKey.policy = defaultSubnetPolicy;
                phySubnetKey.policy.KeyHardLifetime = 0; //make it non expiring
            } else if (provisioningItem->deviceType == Isa100::Common::ProvisioningItem::BACKBONE) {

                dlTempPolicy = dlKeyPolicy;
                phySubnetKey.keyID = 0; //at BR join
                phySubnetKey.key = GetSubnetKey(subnetID);
                phySubnetKey.softLifeTime = currentTAI + (dlKeyPolicy.KeyHardLifetime * 3600) / 2;
                phySubnetKey.hardLifeTime = currentTAI + dlKeyPolicy.KeyHardLifetime * 3600;
                phySubnetKey.policy = defaultSubnetPolicy;
            } else {
                //at device join compute the remaining lifetime of DL key
                IEngine* engine = Isa100::Model::EngineProvider::getEngine();
                Subnet::PTR subnet = engine->getSubnetsContainer().getSubnet(subnetID);
                if (!subnet) {
                    LOG_ERROR("Could not find subnet " << (int) subnetID);
                    responseCallback(false, SecuritySymJoinResponse(), SecurityJoinFailureReason::subnet_not_foud);
                    return;
                }
                Device *backbone = subnet->getBackbone();
                if (!backbone) {
                    LOG_ERROR("Backbone not found for subnet " << (int) subnetID);
                    responseCallback(false, SecuritySymJoinResponse(), SecurityJoinFailureReason::backbone_not_found);
                    return;
                }

                NE::Model::PhySpecialKey *newestKey = getNewestActiveSubnetKey(backbone);
                if (!newestKey) { //no key
                    LOG_ERROR("Error occurred while trying to find the DL subnet key");
                    responseCallback(false, SecuritySymJoinResponse(), SecurityJoinFailureReason::dl_key_not_found);
                    return;
                }

                dlTempPolicy.KeyHardLifetime = (newestKey->hardLifeTime - currentTAI) / 3600; //hours

                //[hack] sorin - make sure that the lifetime sent to device is not smaller than the lifetime set for backbone;
                while ((currentTAI + dlTempPolicy.KeyHardLifetime * 3600) <= newestKey->hardLifeTime) {
                    ++dlTempPolicy.KeyHardLifetime;
                }

                phySubnetKey.keyID = newestKey->keyID;
                phySubnetKey.key = newestKey->key;
                phySubnetKey.softLifeTime = currentTAI + (dlTempPolicy.KeyHardLifetime * 3600) / 2;
                phySubnetKey.hardLifeTime = currentTAI + dlTempPolicy.KeyHardLifetime * 3600; //remaining time
                phySubnetKey.policy = defaultSubnetPolicy;
            }

            NE::Model::SecurityKey dlKey = phySubnetKey.key;

            LOG_INFO("DL subnet key for device " << request.EUI64JoiningDevice.toString() << " is <" << dlKey.toString()
                        << "> ; subnet= " << subnetID << "; KeyHardLifetime=" << (int) dlTempPolicy.KeyHardLifetime
                        << "h, expiringTime=" << (currentTAI + dlTempPolicy.KeyHardLifetime * 3600) << "s");

            response.DlKeyPolicy = dlTempPolicy;
            response.DLKeyID = phySubnetKey.keyID;

            if (SmSettingsLogic::instance().disableTLEncryption) {
                response.SysMgrSessionKeyPolicy.KeyHardLifetime = NO_SECURITY_KEYLIFETIME;
            } else {
                response.SysMgrSessionKeyPolicy = sysMgrSessionKeyPolicy;
            }

            NE::Model::SecurityKey masterKey;
            GenerateMasterKey(request, response, joinKey, masterKey);

            phyMasterKey.keyID = 0; //implicit (not transmitted but inferred)
            phyMasterKey.key = masterKey;
            phyMasterKey.softLifeTime = currentTAI + (masterPolicy.KeyHardLifetime * 3600) / 2;
            phyMasterKey.hardLifeTime = currentTAI + masterPolicy.KeyHardLifetime * 3600;
            phyMasterKey.policy = defaultMasterPolicy;

            response.MasterKeyPolicy = masterPolicy;

            //encrypt keys
            {
                Uint8 plainBuffer[dlKey.LENGTH + securityKey.LENGTH];
                memcpy(plainBuffer, dlKey.value, dlKey.LENGTH);
                memcpy(plainBuffer + dlKey.LENGTH, securityKey.value, securityKey.LENGTH);

                Uint8 encBuffer[dlKey.LENGTH + securityKey.LENGTH + 4];
                Isa100::Common::Ccm2 ccm;
                ccm.SetAuthenFldSz(4); // MIC is on 4 bytes
                ccm.SetLenFldSz(2); // (16(totalAESBlockSize) - 13(nonce) -1(firstBlock))
                ccm.AuthEncrypt(masterKey.value, request.bitChallengeFromNewDevice, // nonce - msbs from challenge from device
                            NULL, // auth
                            0, // auth len
                            plainBuffer, dlKey.LENGTH + securityKey.LENGTH, encBuffer);
                memcpy(response.EncryptedDLKey, encBuffer, dlKey.LENGTH);
                memcpy(response.EncryptedSysMgrSessionKey, encBuffer + dlKey.LENGTH, securityKey.LENGTH);
            }

            // generate hash of the challenge
            {
                Uint8 buffer[INPUT_HASH_BUFFERSIZE];
                GetHashInputString(request, response, buffer);

                Algorithms::HMACK_JOIN(buffer, INPUT_HASH_BUFFERSIZE, joinKey.value, response.BitChallengeResponseToNewDevice);
            }

            //store the new challenge for the device (overwrites if already existent)
            Uint128Wrapper& assignedChallenge = assignedChallenges[request.EUI64JoiningDevice] = Uint128Wrapper();
            for (int i = 0; i < 16; i++) {
                assignedChallenge.uint128[i] = response.BitChallengeFromSystemManager[i];
            }

            LOG_DEBUG("SecuritySymJoinResponse response=" << response.toString());

            //respond
            responseCallback(true, response, SecurityJoinFailureReason::success);

        } else {
            LOG_WARN("MIC test failed on Security Join Request. Probably incorrect provisioned key.");
            responseCallback(false, SecuritySymJoinResponse(), SecurityJoinFailureReason::bad_join_key);
        }

    } else {
        LOG_WARN("Join response dropped because of unknown symmetric algorithm= " << (int) algorithmIdentifier);
        responseCallback(false, SecuritySymJoinResponse(), SecurityJoinFailureReason::invalid_aes_alg);
    }

}

bool SecurityManager::SecurityJoinConfirm(const Address64& deviceAddress64, const SecurityConfirmRequest& confirm) {

    Isa100::Common::ProvisioningItem* provisioningItem = SmSettingsLogic::instance().getProvisioningForDevice(deviceAddress64);

    if (!provisioningItem) {
        LOG_ERROR("Join response dropped because of unprovisioned device address =" << deviceAddress64.toString());
        return false;
    }

    // search for the challenge assigned at security join request step
    std::map<NE::Common::Address64, Uint128Wrapper>::iterator itAssignedChallenge = assignedChallenges.find(deviceAddress64);
    if (itAssignedChallenge == assignedChallenges.end()) {
        LOG_ERROR("SecurityJoinConfirm stopped because of missing assigned challenge for device " << deviceAddress64.toString());
        return false;
    }
    // search for the challenge the device joined with
    std::map<NE::Common::Address64, PreviousChallenges>::iterator itReqChallenge = joinRequestChallenges.find(deviceAddress64);
    if (itReqChallenge == joinRequestChallenges.end()) {
        LOG_ERROR("SecurityJoinConfirm stopped because join challenge for device " << deviceAddress64.toString()
                    << "was not found.");
        return false;
    }

    Uint8 expectedResponse[16];

    Uint8 buffer[48];
    memcpy(buffer, itReqChallenge->second.previousChallenge1.uint128, 16);
    memcpy(buffer + 16, itAssignedChallenge->second.uint128, 16);
    memcpy(buffer + 32, deviceAddress64.value, 8);
    memcpy(buffer + 40, SmSettingsLogic::instance().managerAddress64.value, 8);

    Algorithms::HMACK_JOIN(buffer, 48, provisioningItem->key.value, expectedResponse);

    return !memcmp(expectedResponse, confirm.bitChallengeResponseToSecurityManager, 16);

}

void SecurityManager::setStackKeys(const SecuritySymJoinRequest& request, const NE::Model::PhySessionKey& sessionKey,
			 const NE::Common::Address128& joiningDeviceAddress128) {

    AL::DMAP::TLMO tlmo;
	int multiply = KeyUtils::GetGranularityMultiplier(defaultSessionPolicy.Granularity);


    SFC::SFCEnum sfc2 = tlmo.SetKey(joiningDeviceAddress128, Common::TSAP::TSAP_SMAP, Common::TSAP::TSAP_DMAP, sessionKey.keyID, // keyID
                sessionKey.key, //
                request.EUI64JoiningDevice, //
                sessionKey.sessionKeyPolicy.NotValidBefore, // TAI
                sessionKey.sessionKeyPolicy.KeyHardLifetime * multiply / 2, //
                sessionKey.sessionKeyPolicy.KeyHardLifetime * multiply, //
                1, //TODO SLM_KEY_USAGE_SESSION
                0 // policy (not used)
                );

    LOG_INFO("setStackKeys : SM : tlmo.SetKey : notValidBefore=" << std::hex << (long long)sessionKey.sessionKeyPolicy.NotValidBefore
        << ", softLifetime=" << (long long)(sessionKey.sessionKeyPolicy.KeyHardLifetime / 2) * multiply
        << ", hardLifetime=" << (long long)sessionKey.sessionKeyPolicy.KeyHardLifetime * multiply
        << ", TAI=" << (long long)ClockSource::getTAI(SmSettingsLogic::instance()));


    if (sfc2 != SFC::success) {
        LOG_ERROR("CANNOT set key in stack ports (1>0) with peer=" << joiningDeviceAddress128.toString() << ". SFC="
                    << (int) sfc2);
    }
}

SFC::SFCEnum SecurityManager::setStackKeyWithPeer(const Address64& peer, const Address128& peer128, TSAP::TSAP_Enum tsapSrc,
            TSAP::TSAP_Enum tsapDest, const NE::Model::SecurityKey &key, Uint8 newKeyID,
            const NE::Model::Policy &newSessionPolicy) {

    LOG_DEBUG("Setting SM key with peer=" << peer.toString());
    int multiply = KeyUtils::GetGranularityMultiplier(newSessionPolicy.Granularity);
    AL::DMAP::TLMO tlmo;
    SFC::SFCEnum sfc = tlmo.SetKey( //
                peer128, tsapSrc, tsapDest, //
                newKeyID, // keyID
                key, //
                peer, // issuer eui64
                newSessionPolicy.NotValidBefore, // not valid before - TAI
                newSessionPolicy.KeyHardLifetime / 2 * multiply, // softLifetime
                newSessionPolicy.KeyHardLifetime * multiply, // hardLifeTime
                1, //TODO SLM_KEY_USAGE_SESSION
                0 // policy (not used)
                );

    LOG_INFO("setStackKeyWithPeer : SM : tlmo.SetKey : notValidBefore=" << std::hex << (long long)newSessionPolicy.NotValidBefore
        << ", softLifetime=" << (long long)newSessionPolicy.KeyHardLifetime / 2 * multiply
        << ", hardLifetime=" << (long long)newSessionPolicy.KeyHardLifetime * multiply
        << ", TAI=" << (long long)ClockSource::getTAI(SmSettingsLogic::instance()));

    if (sfc != SFC::success) {
        LOG_ERROR("CANNOT set key in stack with peer=" << peer.toString() << "tsap: " << (int) tsapSrc << "->" << (int) tsapDest
                    << ". SFC=" << (int) sfc);
    }
    return sfc;
}

//this is used when SOO is in charge with sending the new session keys
void SecurityManager::createNewSessionKey(NE::Model::PhySessionKey& phyKey, SecurityKeyAndPolicies& responseKey) {

    Address64& from = phyKey.source64;
    Address64& to = phyKey.destination64;
    TSAP::TSAP_Enum tsapFrom = (TSAP::TSAP_Enum) phyKey.sourceTSAP;
    TSAP::TSAP_Enum tsapTo = (TSAP::TSAP_Enum) phyKey.destinationTSAP;

    LOG_INFO("createNewSessionKey - FROM=" << from.toString() << " TO=" << to.toString() << " taspFrom=" << (int) tsapFrom
                << " tsapTo=" << (int) tsapTo << ", notValidBefore=" << std::hex << (long long)phyKey.sessionKeyPolicy.NotValidBefore);


    NE::Model::Policy& newSessionPolicy = phyKey.sessionKeyPolicy;

    //populate responseKey structure with the new key and policy
    prepareKey(from, to, phyKey.source128, phyKey.destination128, tsapFrom, tsapTo, phyKey.key, phyKey.keyID, newSessionPolicy,
                responseKey);
}

//this is used when PSMO is in charge with sending the new session keys
void SecurityManager::createNewSessionKeys(const Address64& from, const Address64& to, Isa100::Common::TSAP::TSAP_Enum tsapFrom,
            Isa100::Common::TSAP::TSAP_Enum tsapTo, SecurityKeyAndPolicies& keyFrom, SecurityKeyAndPolicies& keyTo) {

    LOG_ERROR("Obsolete method called");


}

void SecurityManager::prepareKey(const Address64& from, const Address64& to, const Address128& from128, const Address128& to128,
            Isa100::Common::TSAP::TSAP_Enum tsapFrom, Isa100::Common::TSAP::TSAP_Enum tsapTo,
            const NE::Model::SecurityKey& secKey, Uint16 keyID, const NE::Model::Policy& newSessionPolicy,
            SecurityKeyAndPolicies& sessionKey) {

    Address32 srcAddress32 = Isa100::Model::EngineProvider::getEngine()->getAddress32(from);
    NE::Model::Device *srcDevice = Isa100::Model::EngineProvider::getEngine()->getDevice(srcAddress32);
    if (!srcDevice) {
        LOG_ERROR("Could not find device [" << Address_toStream(srcAddress32) << "] " << from.toString());
        return;
    }

    NE::Model::PhySpecialKey *newestKey = getNewestActiveMasterKey(srcDevice);
    if (!newestKey) {
        LOG_ERROR("Error occurred while trying to find the master key");
        return;
    }

    SecurityKey masterKey = newestKey->key;

    LOG_INFO("Authenticate session key using master key " << (int) newestKey->keyID);


    sessionKey.keyPolicy = newSessionPolicy;
    sessionKey.EUI64Remote = to;
    sessionKey.remoteAddress128 = to128;
    sessionKey.endPortSource = 0xF0B0 + (int) tsapFrom;//Common::TransportPorts::fromTSAP(tsapFrom);
    sessionKey.endPortRemote = 0xF0B0 + (int) tsapTo;//Common::TransportPorts::fromTSAP(tsapTo);
    sessionKey.algorithmIdentifier = 1; // AES_CCM*
    sessionKey.securityControl = 0x05 // security - AES_CCM* enc + 4 bytes MIC
                + (0x01 << 3); // KeyID mode
    sessionKey.keyIdentifier = newestKey->keyID; // Master KeyID
    sessionKey.timeStamp = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());
    sessionKey.newKeyID = keyID; //TODO - obs: conversion from uint16 to uint8 .. may cause keyID bugs
    sessionKey.keyMaterial = secKey;

    LOG_DEBUG("prepareKey() - newKeyID=" << (int) sessionKey.newKeyID << " to " << to.toString() << ", KeyHardLifetime="
                << sessionKey.keyPolicy.KeyHardLifetime);


    EncryptAndAuthSessionKey(sessionKey, masterKey);
}

void SecurityManager::EncryptAndAuthSessionKey(SecurityKeyAndPolicies& sessionKey, NE::Model::SecurityKey& masterKey) {

    Isa100::Common::Ccm2 ccm;
    ccm.SetAuthenFldSz(4); // MIC is on 4 bytes
    ccm.SetLenFldSz(2); // (16(totalAESBlockSize) - 13(nonce) -1(firstBlock))

    NE::Misc::Marshall::NetworkOrderStream nos;
    //sessionKey.marshall(nos);
    Isa100::Model::marshallEntity(sessionKey, nos);

    Bytes authBytes = nos.ostream.str();

    LOG_DEBUG("AddKey serialized:" << StreamToString2((Uint8*) authBytes.c_str(), authBytes.size()));

    int authLen = authBytes.size() - sessionKey.keyMaterial.LENGTH - 4;
    uint8_t auth[authLen];
    uint8_t encKey[20 + authLen + 2 + 8 + 2 + 16];
    uint8_t nonce[13];

    memcpy(auth, authBytes.c_str(), authLen);

    Uint32 networkOrderTime = htonl(sessionKey.timeStamp);

    memcpy(nonce, SmSettingsLogic::instance().managerAddress64.value, 8);
    memcpy(nonce + 8, &networkOrderTime, 4);
    nonce[12] = 0xFF;

    LOG_DEBUG("SECURITY:SESSIONKEY:authLen=" << authLen);
    LOG_DEBUG("SECURITY:SESSIONKEY:masterKey=" << masterKey.toString());
    LOG_DEBUG("SECURITY:SESSIONKEY:nonce=" << StreamToString2(nonce, 13));
    LOG_DEBUG("SECURITY:SESSIONKEY:auth=" << StreamToString2(auth, authLen));
    LOG_DEBUG("SECURITY:SESSIONKEY:plain=" << sessionKey.keyMaterial.toString());

    ccm.AuthEncrypt(masterKey.value, nonce, auth, authLen, sessionKey.keyMaterial.value, sessionKey.keyMaterial.LENGTH, encKey);

    LOG_DEBUG("SECURITY:SESSIONKEY:encKey+MIC=" << StreamToString2(encKey, authLen + sessionKey.keyMaterial.LENGTH + 4));

    sessionKey.keyMaterial.copyKey(encKey + authLen);

    memcpy(sessionKey.MIC, (encKey + authLen + sessionKey.keyMaterial.LENGTH), 4);
}

bool SecurityManager::createNewSpecialKey(Device *device) {

    Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());

    Uint16 subnetID = device->capabilities.dllSubnetId;

    IEngine* engine = Isa100::Model::EngineProvider::getEngine();

    OperationsContainerPointer operationsContainer(new OperationsContainer("createNewSpecialKey"));

    //search for expiring master key
    NE::Model::SpecialKeyIndexedAttribute::iterator itMaster = device->phyAttributes.masterKeysTable.begin();
    if (itMaster == device->phyAttributes.masterKeysTable.end()) {
        LOG_ERROR("Could not find master key for " << device->address64.toString());
        return false;
    }
    for (; itMaster != device->phyAttributes.masterKeysTable.end(); ++itMaster) {

        if (itMaster->second.currentValue
                    && !(itMaster->second.currentValue->policy.KeyHardLifetime == 0) //not a non-expiring key
                    && (currentTAI >= (itMaster->second.currentValue->softLifeTime)) //
                    && !itMaster->second.currentValue->markedAsExpiring) {

            LOG_INFO("expiring master key on device " << device->address64.toString() //
                        << " keyID=" << (int) itMaster->second.currentValue->keyID  //
                        << ", softLifeTime=" << (long long) itMaster->second.currentValue->softLifeTime //
                        << ", hardLifeTime=" << (long long) itMaster->second.currentValue->hardLifeTime //
                        << ", currentTAI=" << (long long) currentTAI);

            itMaster->second.currentValue->markedAsExpiring = true;

            //create new master key and generate the operation
            createNewMasterKeyOperation(device, currentTAI, operationsContainer);

            //after this call, no modification of the operationsContainer should be made
            engine->getOperationsProcessor().addOperationsContainer(operationsContainer);

            return true;
        }
    }

    //search for expiring DL subnet key
    NE::Model::SpecialKeyIndexedAttribute::iterator itDL = device->phyAttributes.subnetKeysTable.begin();
    if (itDL == device->phyAttributes.subnetKeysTable.end()) {
        LOG_ERROR("Could not find DL subnet key for " << device->address64.toString());
        return false;
    }
    for (; itDL != device->phyAttributes.subnetKeysTable.end(); ++itDL) {

        if (itDL->second.currentValue //
                    && !(itDL->second.currentValue->policy.KeyHardLifetime == 0) //not a non-expiring key
                    && (currentTAI >= (itDL->second.currentValue->softLifeTime)) //
                    && !itDL->second.currentValue->markedAsExpiring) {

            LOG_INFO("expiring DL subnet key on device " << device->address64.toString() //
                        << " keyID=" << (int) itDL->second.currentValue->keyID //
                        << ", softLifeTime=" << (long long) itDL->second.currentValue->softLifeTime //
                        << ", hardLifeTime=" << (long long) itDL->second.currentValue->hardLifeTime //
                        << ", currentTAI=" << (long long) currentTAI);

            itDL->second.currentValue->markedAsExpiring = true;

            if (device->capabilities.isBackbone()) {
                NE::Model::PhySpecialKey *newestKey = getNewestSubnetKeyPendingIncluded(device);
                if (!newestKey
                            || (newestKey && !(newestKey == itDL->second.currentValue))) {
                    continue;
                    //there's an updated key on backbone (from a device update) - no need to create a new one
                }

                //create new DL subnet key
                //PhySpecialKey *newSubnetKey =
                createNewSubnetKeyOperation(device, currentTAI, operationsContainer);

            } else {
                PhySpecialKey *subnetKeyForDevice = NULL;

                Subnet::PTR subnet = engine->getSubnetsContainer().getSubnet(subnetID);
                if (!subnet) {
                    LOG_ERROR("Could not find subnet " << (int) subnetID);
                    return false;
                }
                Device *backbone = subnet->getBackbone();
                if (!backbone) {
                    LOG_ERROR("Backbone not found for subnet " << (int) subnet->getSubnetId());
                    return false;
                }

                //NE::Model::PhySpecialKey *newestKey = getNewestActiveSubnetKey(backbone);
                NE::Model::PhySpecialKey *newestKey = getNewestSubnetKeyPendingIncluded(backbone);
                if (!newestKey) { //no key
                    LOG_ERROR("Error occurred while trying to find the DL subnet key");
                    return false;
                }

                //check if backbone's newest key is expiring
                if ((newestKey->policy.KeyHardLifetime > 0)
                            && (newestKey->softLifeTime <= currentTAI)) {

                    //create new DL subnet key
                    PhySpecialKey *newSubnetKey = createNewSubnetKeyOperation(backbone, currentTAI, operationsContainer);

                    subnetKeyForDevice = new PhySpecialKey(*newSubnetKey);
                    newestKey->markedAsExpiring = true;
                } else {

                    subnetKeyForDevice = new PhySpecialKey(*newestKey);
                }

                //send key to device
                Uint16 greatestID = getGreatestSubnetKeyID(device);
                subnetKeyForDevice->keyID = ++greatestID % 0xFF; //usually there should be maximum 2 DL keys

                EntityIndex entityIndexDevice = createEntityIndex(device->address32, EntityType::SubnetKey, subnetKeyForDevice->keyID);
                IEngineOperationPointer newSubnetKeyOperation(new WriteAttributeOperation(subnetKeyForDevice, entityIndexDevice));
                operationsContainer->addOperation(newSubnetKeyOperation, device);

                LOG_INFO("Updated subnet key for device " << device->address64.toString() << " : " << *subnetKeyForDevice);

            }

            //after this call, no modification of the operationsContainer should be made
            engine->getOperationsProcessor().addOperationsContainer(operationsContainer);

            return true;
        }
    }

    LOG_INFO("Did not find an expiring DL or master key. Searched device " << device->address64.toString()
                << " ; currentTAI=" << currentTAI);
    return false;
}

void SecurityManager::prepareMasterKey(NE::Model::PhySpecialKey& phyKey, Address32 deviceAddress32,
            SecurityKeyAndPolicies& responseKey) {

    NE::Model::Device *device = Isa100::Model::EngineProvider::getEngine()->getDevice(deviceAddress32);
    if (!device) {
        LOG_ERROR("prepareMasterKey - Could not find device [" << Address_toStream(deviceAddress32) << "] ");
        return;
    }

    NE::Model::PhySpecialKey *newestKey = getNewestActiveMasterKey(device); //skips the key being sent right now
    if (!newestKey) {
        LOG_ERROR("Error occurred when trying to find the master key.");
        return;
    }

    responseKey.keyPolicy = phyKey.policy;
    responseKey.algorithmIdentifier = 1; // AES_CCM*
    responseKey.securityControl = 0x05 // security - AES_CCM* enc + 4 bytes MIC
                + (0x01 << 3); // KeyID mode
    responseKey.keyIdentifier = newestKey->keyID; //old Master KeyID
    responseKey.timeStamp = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());
    responseKey.newKeyID = phyKey.keyID; //new Master KeyID
    responseKey.keyMaterial = phyKey.key;

    LOG_DEBUG("prepareMasterKey - newKeyID=" << (int) responseKey.newKeyID << ", KeyHardLifetime="
                << responseKey.keyPolicy.KeyHardLifetime);

    EncryptAndAuthSessionKey(responseKey, newestKey->key);
}

void SecurityManager::prepareSubnetKey(NE::Model::PhySpecialKey& phyKey, Address32 deviceAddress32,
            SecurityKeyAndPolicies& responseKey) {

    NE::Model::Device *device = Isa100::Model::EngineProvider::getEngine()->getDevice(deviceAddress32);
    if (!device) {
        LOG_ERROR("prepareSubnetKey - Could not find device [" << Address_toStream(deviceAddress32) << "] ");
        return;
    }

    NE::Model::PhySpecialKey *newestMasterKey = getNewestActiveMasterKey(device);
    if (!newestMasterKey) {
        LOG_ERROR("Error occurred when trying to find the master key.");
        return;
    }

    //responseKey.keyPolicy = subnetKeyPolicy;
    responseKey.keyPolicy = phyKey.policy;
    responseKey.algorithmIdentifier = 1; // AES_CCM*
    responseKey.securityControl = 0x05 // security - AES_CCM* enc + 4 bytes MIC
                + (0x01 << 3); // KeyID mode
    responseKey.keyIdentifier = newestMasterKey->keyID; // Master KeyID
    responseKey.timeStamp = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());
    responseKey.newKeyID = phyKey.keyID;
    responseKey.keyMaterial = phyKey.key;

    LOG_DEBUG("prepareSubnetKey - newKeyID=" << (int) responseKey.newKeyID << ", KeyHardLifetime="
                << responseKey.keyPolicy.KeyHardLifetime);

    EncryptAndAuthSessionKey(responseKey, newestMasterKey->key);
}

void SecurityManager::getKeyDeletionParams(NE::Model::PhySessionKey& phyKey, SecurityDeleteKeyReq& deleteKeyParams) {

    Address32 srcAddress32 = Isa100::Model::EngineProvider::getEngine()->getAddress32(phyKey.source64);
    NE::Model::Device *srcDevice = Isa100::Model::EngineProvider::getEngine()->getDevice(srcAddress32);
    if (!srcDevice) {
        LOG_ERROR("getKeyDeletionParams - Could not find device [" << Address_toStream(srcAddress32) << "] ");
        return;
    }
    NE::Model::PhySpecialKey *newestMasterKey = getNewestActiveMasterKey(srcDevice);
    if (!newestMasterKey) {
        LOG_ERROR("Error occurred when trying to find the master key.");
        return;
    }

    SecurityKey masterKey = newestMasterKey->key;

    LOG_INFO("Authenticate using master key " << (int) newestMasterKey->keyID);

    //deleteKeyParams.keyType = 0x02; //b1b0 10 = TL
    deleteKeyParams.keyType = defaultSessionPolicy.KeyUsage; // = 0x01;
    deleteKeyParams.masterKeyID = newestMasterKey->keyID;
    deleteKeyParams.keyID = phyKey.keyID;
    deleteKeyParams.sourcePort = 0xF0B0 + phyKey.sourceTSAP;
    deleteKeyParams.destinationAddress = phyKey.destination128;
    deleteKeyParams.destinationPort = 0xF0B0 + phyKey.destinationTSAP;

    Uint32 networkOrderTime = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance()); //TODO
    deleteKeyParams.nonce = networkOrderTime; //this must be added before applying htonl

    networkOrderTime = htonl(networkOrderTime);
    uint8_t nonce[13];
    memcpy(nonce, SmSettingsLogic::instance().managerAddress64.value, 8);
    memcpy(nonce + 8, &networkOrderTime, 4);
    nonce[12] = 0xFF;

    Isa100::Common::Ccm2 ccm;
    ccm.SetAuthenFldSz(4); // MIC is on 4 bytes
    ccm.SetLenFldSz(2); // (16(totalAESBlockSize) - 13(nonce) -1(firstBlock))

    NE::Misc::Marshall::NetworkOrderStream nos;
    Isa100::Model::marshallEntity(deleteKeyParams, nos);

    Bytes authBytes = nos.ostream.str();

    LOG_DEBUG("Delete_key params serialized:" << StreamToString2((Uint8*) authBytes.c_str(), authBytes.size()));

    int authLen = authBytes.size() - 4; // 4 = mic size
    uint8_t auth[authLen];
    uint8_t encKey[authLen + 4];

    memcpy(auth, authBytes.c_str(), authLen);

    LOG_DEBUG("SECURITY:DELETE_SESSIONKEY:authLen=" << authLen);
    LOG_DEBUG("SECURITY:DELETE_SESSIONKEY:masterKey=" << masterKey.toString());
    LOG_DEBUG("SECURITY:DELETE_SESSIONKEY:nonce=" << StreamToString2(nonce, 13));
    LOG_DEBUG("SECURITY:DELETE_SESSIONKEY:auth=" << StreamToString2(auth, authLen));

    ccm.AuthEncrypt(masterKey.value, nonce, auth, authLen, NULL, 0, encKey);

    LOG_DEBUG("SECURITY:DELETE_SESSIONKEY:encKey+MIC=" << StreamToString2(encKey, authLen + 4));

    memcpy(deleteKeyParams.mic, (encKey + authLen), 4);
    //uint32_t netMIC = ntohl(*((uint32_t*)deleteKeyParams.mic));
    //memcpy(deleteKeyParams.mic, &netMIC, 4);
}

void SecurityManager::getNewSessionResponse(Device *requester, const Uint8 status, SecurityNewSessionResponse& newSessionResponse) {
    NE::Model::PhySpecialKey *newestMasterKey = getNewestActiveMasterKey(requester);
    if (!newestMasterKey) {
        LOG_ERROR("Error occurred when trying to find the master key.");
        return;
    }

    SecurityKey masterKey = newestMasterKey->key;

    LOG_INFO("Authenticate using master key " << (int) newestMasterKey->keyID);

    Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());

    newSessionResponse.status = status;
    newSessionResponse.securityControl = 0x09; //(32-bitAuthentication only)
    newSessionResponse.keyIdentifier = newestMasterKey->keyID;
    newSessionResponse.timeStamp = currentTAI;

    Uint32 networkOrderTime = currentTAI;

    networkOrderTime = htonl(networkOrderTime);
    uint8_t nonce[13];
    memcpy(nonce, SmSettingsLogic::instance().managerAddress64.value, 8);
    memcpy(nonce + 8, &networkOrderTime, 4);
    nonce[12] = 0xFF;

    Isa100::Common::Ccm2 ccm;
    ccm.SetAuthenFldSz(4); // MIC is on 4 bytes
    ccm.SetLenFldSz(2); // (16(totalAESBlockSize) - 13(nonce) -1(firstBlock))

    NE::Misc::Marshall::NetworkOrderStream nos;
    newSessionResponse.marshall(nos);

    Bytes authBytes = nos.ostream.str();

    LOG_DEBUG("SecurityNewSessionResponse params serialized:" << StreamToString2((Uint8*) authBytes.c_str(), authBytes.size()));

    int authLen = authBytes.size() - 4; // 4 = mic size
    uint8_t auth[authLen];
    uint8_t encKey[authLen + 4]; //authenticated and encrypted message

    memcpy(auth, authBytes.c_str(), authLen);

    LOG_DEBUG("SECURITY:NEW_SESSION_RESPONSE:authLen=" << authLen);
    LOG_DEBUG("SECURITY:NEW_SESSION_RESPONSE:masterKey=" << masterKey.toString());
    LOG_DEBUG("SECURITY:NEW_SESSION_RESPONSE:nonce=" << StreamToString2(nonce, 13));
    LOG_DEBUG("SECURITY:NEW_SESSION_RESPONSE:auth=" << StreamToString2(auth, authLen));

    ccm.AuthEncrypt(masterKey.value, nonce, auth, authLen, NULL, 0, encKey);

    LOG_DEBUG("SECURITY:NEW_SESSION_RESPONSE:encKey+MIC=" << StreamToString2(encKey, authLen + 4));

    memcpy(newSessionResponse.mic, (encKey + authLen), 4);

}

void SecurityManager::createNewSessionKeysPair(Device *device, Device *destDevice, PhySessionKey& key, PhySessionKey& peerKey,
            Uint16 newKeyID, Uint32 currentTAI, OperationsContainerPointer& operationsContainer) {
    SecurityKey newSecurityKey = getSessionKey(device->address64, destDevice->address64,
                (TSAP::TSAP_Enum) key.sourceTSAP,
                (TSAP::TSAP_Enum) key.destinationTSAP);

    PhySessionKey* newKeyTo = new PhySessionKey(peerKey);
    newKeyTo->keyID = newKeyID;
    newKeyTo->key = newSecurityKey;
    newKeyTo->index = destDevice->getNextKeysTableIndex();

    SmSettingsLogic &smSettingsLogic = SmSettingsLogic::instance();

    KeyUtils::updateKey(currentTAI, smSettingsLogic, newKeyTo); //update lifetime and NotValidBefore

    EntityIndex entityIndexTo = createEntityIndex(destDevice->address32, EntityType::SessionKey, newKeyTo->index);
    IEngineOperationPointer newKeyToOperation(new WriteAttributeOperation(newKeyTo, entityIndexTo));
    operationsContainer->addOperation(newKeyToOperation, destDevice);

    PhySessionKey* newKeyFrom = new PhySessionKey(key);
    newKeyFrom->keyID = newKeyID;
    newKeyFrom->key = newSecurityKey;
    newKeyFrom->index = device->getNextKeysTableIndex();

    KeyUtils::updateKey(currentTAI, smSettingsLogic, newKeyFrom); //update lifetime and NotValidBefore

    EntityIndex entityIndexFrom = createEntityIndex(device->address32, EntityType::SessionKey, newKeyFrom->index);
    IEngineOperationPointer newKeyFromOperation(new WriteAttributeOperation(newKeyFrom, entityIndexFrom));
    newKeyFromOperation->addOperationDependency(newKeyToOperation);
    operationsContainer->addOperation(newKeyFromOperation, device);

    LOG_INFO("New keyID=" << (int) newKeyID << "; on [" << Address_toStream(device->address32) << "]: index="
                << (int) newKeyFrom->index << ", hardLifeTime=" << (long long) newKeyFrom->hardLifeTime //
                << ", NotValidBefore=" << (long long)newKeyFrom->sessionKeyPolicy.NotValidBefore
                << "; on [" << Address_toStream(destDevice->address32) << "]: index=" << (int) newKeyTo->index //
                << ", hardLifeTime=" << (long long) newKeyTo->hardLifeTime
                << ", NotValidBefore=" << (long long)newKeyTo->sessionKeyPolicy.NotValidBefore
                << ", currentTAI=" << currentTAI);
}

//update session keys with manager + master key
void SecurityManager::updateExpiringKeysOnGateway(Uint32 deviation, Uint32 currentTAI) {
    OperationsContainerPointer operationsContainerGW(new OperationsContainer("updateGWExpiringKeys"));

    IEngine* engine = Isa100::Model::EngineProvider::getEngine();
    Device *gw = engine->getDevice(ADDRESS16_GATEWAY);
    if (!gw) {
        LOG_ERROR("Could not find gateway device.");
        return;
    }
    Device *manager = engine->getDevice(engine->getManagerAddress32());

    //session keys
    for (SessionKeyIndexedAttribute::iterator itKeys = gw->phyAttributes.sessionKeysTable.begin(); itKeys
                != gw->phyAttributes.sessionKeysTable.end(); ++itKeys) {

        if (itKeys->second.currentValue //
                    && itKeys->second.currentValue->destination64 == manager->address64 //
                    && !itKeys->second.currentValue->markedAsExpiring) {

            //search for keys with expired softLifeTime, skip others
            //use deviation to find keys BEFORE actual expiring time
            if ((itKeys->second.currentValue->sessionKeyPolicy.KeyHardLifetime == 0) //non-expiring key
                        || (itKeys->second.currentValue->softLifeTime - deviation) >= currentTAI) {
                continue;
            }

            Uint16 keyID = gw->getGreatestKeyIDwithPeer(manager, itKeys->second.currentValue->sourceTSAP,
                        itKeys->second.currentValue->destinationTSAP);
            if (keyID == 0) {
                LOG_ERROR("updateExpiringKeys - keyIDwithPeer=0 - skipping key " << (int) itKeys->second.currentValue->keyID
                            << " of gateway");
                continue;
            }

            //find the peer key on the manager
            PhySessionKey* peerKey = NULL;
            for (SessionKeyIndexedAttribute::iterator itKeysDest = manager->phyAttributes.sessionKeysTable.begin(); itKeysDest
                        != manager->phyAttributes.sessionKeysTable.end(); ++itKeysDest) {
                if (itKeysDest->second.currentValue
                            && itKeysDest->second.currentValue->destination64 == gw->address64 //
                            && itKeysDest->second.currentValue->keyID == itKeys->second.currentValue->keyID
                            && itKeysDest->second.currentValue->sourceTSAP == itKeys->second.currentValue->destinationTSAP
                            && itKeysDest->second.currentValue->destinationTSAP == itKeys->second.currentValue->sourceTSAP
                            && !itKeysDest->second.currentValue->markedAsExpiring) {
                    peerKey = itKeysDest->second.currentValue;
                    break;
                }
            }
            if (!peerKey) {
                LOG_ERROR("updateExpiringKeys - the peer key on manager was not found; keyID="
                            << (int) itKeys->second.currentValue->keyID);
                continue;
            }

            LOG_INFO("Update key pair between manager and gateway; keyID=" << (int) peerKey->keyID);

            createNewSessionKeysPair(gw, manager, *itKeys->second.currentValue, *peerKey,
                        keyID, currentTAI, operationsContainerGW);

            //mark keys as expiring
            itKeys->second.currentValue->markedAsExpiring = true;
            peerKey->markedAsExpiring = true;
        }
    }

    //master key
    for (SpecialKeyIndexedAttribute::iterator itMaster = gw->phyAttributes.masterKeysTable.begin(); itMaster
                != gw->phyAttributes.masterKeysTable.end(); ++itMaster) {

        if (!itMaster->second.currentValue || itMaster->second.currentValue->markedAsExpiring) {
            continue;
        }

        //search for keys with expired softLifeTime, skip others
        //use deviation to find keys BEFORE actual expiring time
        if ((itMaster->second.currentValue->policy.KeyHardLifetime == 0) //non-expiring key
                    || (itMaster->second.currentValue->softLifeTime - deviation) >= currentTAI) {
            continue;
        }

        LOG_INFO("expiring master key on device " << gw->address64.toString() //
                    << " keyID=" << (int) itMaster->second.currentValue->keyID  //
                    << ", softLifeTime=" << (long long) itMaster->second.currentValue->softLifeTime //
                    << ", hardLifeTime=" << (long long) itMaster->second.currentValue->hardLifeTime //
                    << ", currentTAI=" << (long long) currentTAI);

        itMaster->second.currentValue->markedAsExpiring = true;

        //create new master key and generate the operation
        createNewMasterKeyOperation(gw, currentTAI, operationsContainerGW);

    }


    //after this call, no modification of the operationsContainer should be made
    engine->getOperationsProcessor().addOperationsContainer(operationsContainerGW);

}

void SecurityManager::updateExpiringKeys(Uint32 deviation) {
    SmSettingsLogic &smSettingsLogic = SmSettingsLogic::instance();
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(smSettingsLogic);
    IEngine* engine = Isa100::Model::EngineProvider::getEngine();

	LOG_INFO("updateExpiringKeys - time : " << (long long)currentTAI << ", deviation=" << (long long)deviation);

	//need to handle Gateway separately because it has session keys with all the devices in subnet
	//the keys with the Manager will be checked here
	//the keys with the other devices will be updated when checking the devices
	updateExpiringKeysOnGateway(deviation, currentTAI);

    int maxNumberOfUpdates = 4; //update keys on at most 4 devices in one call
    //this function is called every 30 seconds;
    //if there are 200 devices in a subnet this means 30*200=6000 seconds if update is done separately for every device
    //if the update is done for 4 devices at once this means 6000/4=1500 seconds to update all
    //so the deviation (parameter of this function) should be at least 1500 in this case
    //OBS: new computation must be made when the network is larger than 200 devices!

    OperationsContainerPointer operationsContainer(new OperationsContainer("updateExpiringKeys"));

    //check session keys for the rest of devices
    SubnetsMap& subnets = engine->getSubnetsList();
    for (SubnetsMap::iterator itSubnets = subnets.begin(); itSubnets != subnets.end(); ++itSubnets) {
        if (!itSubnets->second) {
            continue;
        }

        PhySpecialKey *backboneSubnetKey = NULL; //holds a new created subnet key (valuable for the whole subnet)

        for (Address16 i = 3; i < MAX_NUMBER_OF_DEVICES; ++i) {
            Device * device = itSubnets->second->getDevice(i);
            if (device == NULL) {
                continue;
            }

            bool foundExpiredKeys = false; //used in counting the devices that need updates

            //session keys
            for (SessionKeyIndexedAttribute::iterator itKeys = device->phyAttributes.sessionKeysTable.begin(); itKeys
                        != device->phyAttributes.sessionKeysTable.end(); ++itKeys) {

                if (!itKeys->second.currentValue || itKeys->second.currentValue->markedAsExpiring) {
                    continue;
                }

                //search for keys with expired softLifeTime, skip others
                //use deviation to find keys BEFORE actual expiring time
                if ((itKeys->second.currentValue->sessionKeyPolicy.KeyHardLifetime == 0) //non-expiring key
                            || (itKeys->second.currentValue->softLifeTime - deviation) >= currentTAI) {
                    continue;
                }

                Address32 destAddress32 = engine->getAddress32(itKeys->second.currentValue->destination64);
                Device * destDevice = engine->getDevice(destAddress32);
                if (!destDevice) {
                    LOG_INFO("updateExpiringKeys - for key " << (int) itKeys->second.currentValue->keyID
                                << ", could not find destination device [" << Address_toStream(destAddress32) << "] "
                                << itKeys->second.currentValue->destination64.toString());
                    continue;
                }

                Uint16 keyID = device->getGreatestKeyIDwithPeer(destDevice, itKeys->second.currentValue->sourceTSAP,
                            itKeys->second.currentValue->destinationTSAP);
                if (keyID == 0) {
                    LOG_ERROR("updateExpiringKeys - keyIDwithPeer=0 - skipping key " << (int) itKeys->second.currentValue->keyID
                                << " of device " << device->address64.toString());
                    continue;
                }

                //find the peer key on the peer device
                PhySessionKey* peerKey = NULL;
                for (SessionKeyIndexedAttribute::iterator itKeysDest = destDevice->phyAttributes.sessionKeysTable.begin(); itKeysDest
                            != destDevice->phyAttributes.sessionKeysTable.end(); ++itKeysDest) {
                    if (itKeysDest->second.currentValue
                                && itKeysDest->second.currentValue->destination64 == device->address64 //
                                && itKeysDest->second.currentValue->keyID == itKeys->second.currentValue->keyID
                                && itKeysDest->second.currentValue->sourceTSAP == itKeys->second.currentValue->destinationTSAP
                                && itKeysDest->second.currentValue->destinationTSAP == itKeys->second.currentValue->sourceTSAP
                                && !itKeysDest->second.currentValue->markedAsExpiring) {
                        peerKey = itKeysDest->second.currentValue;
                        break;
                    }
                }
                if (!peerKey) {
                    LOG_ERROR("updateExpiringKeys - the peer key on the peer device was not found.. device="
                                << destDevice->address64.toString() << " key=" << (int) itKeys->second.currentValue->keyID);
                    continue;
                }

                LOG_INFO("Update key " << (int) peerKey->keyID << " between [" << Address_toStream(device->address32) << "] "
                            << device->address64.toString() << " and [" << Address_toStream(destDevice->address32) << "] "
                            << destDevice->address64.toString());

                //create new session keys and generate the operations
                createNewSessionKeysPair(device, destDevice, *itKeys->second.currentValue, *peerKey,
                            keyID, currentTAI, operationsContainer);

                //mark keys as expiring
                itKeys->second.currentValue->markedAsExpiring = true;
                peerKey->markedAsExpiring = true;

                foundExpiredKeys = true;
            }

            //master keys
            for (SpecialKeyIndexedAttribute::iterator itMaster = device->phyAttributes.masterKeysTable.begin(); itMaster
                        != device->phyAttributes.masterKeysTable.end(); ++itMaster) {

                if (!itMaster->second.currentValue || itMaster->second.currentValue->markedAsExpiring) {
                    continue;
                }

                //search for keys with expired softLifeTime, skip others
                //use deviation to find keys BEFORE actual expiring time
                if ((itMaster->second.currentValue->policy.KeyHardLifetime == 0) //non-expiring key
                            || (itMaster->second.currentValue->softLifeTime - deviation) >= currentTAI) {
                    continue;
                }

                LOG_INFO("expiring master key on device " << device->address64.toString() //
                            << " keyID=" << (int) itMaster->second.currentValue->keyID  //
                            << ", softLifeTime=" << (long long) itMaster->second.currentValue->softLifeTime //
                            << ", hardLifeTime=" << (long long) itMaster->second.currentValue->hardLifeTime //
                            << ", currentTAI=" << (long long) currentTAI);

                itMaster->second.currentValue->markedAsExpiring = true;

                //create new master key and generate the operation
                createNewMasterKeyOperation(device, currentTAI, operationsContainer);

                foundExpiredKeys = true;
            }

            //subnet keys
            for (SpecialKeyIndexedAttribute::iterator itDL = device->phyAttributes.subnetKeysTable.begin(); itDL
                        != device->phyAttributes.subnetKeysTable.end(); ++itDL) {

                if (!itDL->second.currentValue || itDL->second.currentValue->markedAsExpiring) {
                    continue;
                }

                //search for keys with expired softLifeTime, skip others
                //use deviation to find keys BEFORE actual expiring time
                if ((itDL->second.currentValue->policy.KeyHardLifetime == 0) //non-expiring key
                            || (itDL->second.currentValue->softLifeTime - deviation) >= currentTAI) {
                    continue;
                }

                LOG_INFO("expiring DL subnet key on device " << device->address64.toString() //
                            << " keyID=" << (int) itDL->second.currentValue->keyID //
                            << ", softLifeTime=" << (long long) itDL->second.currentValue->softLifeTime //
                            << ", hardLifeTime=" << (long long) itDL->second.currentValue->hardLifeTime //
                            << ", currentTAI=" << (long long) currentTAI);

                itDL->second.currentValue->markedAsExpiring = true;

                if (device->capabilities.isBackbone()) {
                    if (!backboneSubnetKey) {
                        NE::Model::PhySpecialKey *newestKey = getNewestSubnetKeyPendingIncluded(device);
                        if (!newestKey
                                    || (newestKey && !(newestKey == itDL->second.currentValue))) {
                            continue;
                            //there's an updated key on backbone (from a device update) - no need to create a new one
                        }

                        //create new DL subnet key and generate the operation
                        PhySpecialKey *newSubnetKey = createNewSubnetKeyOperation(device, currentTAI, operationsContainer);

                        backboneSubnetKey = newSubnetKey;
                    }
                //device
                } else {
                    PhySpecialKey *subnetKeyForDevice = NULL;
                    if (backboneSubnetKey) {

                        subnetKeyForDevice = new PhySpecialKey(*backboneSubnetKey);

                    } else {

                        Device *backbone = itSubnets->second->getBackbone();
                        if (!backbone) {
                            LOG_ERROR("Backbone not found for subnet " << (int) itSubnets->second->getSubnetId());
                            return;
                        }

                        //NE::Model::PhySpecialKey *newestKey = getNewestActiveSubnetKey(backbone);
                        NE::Model::PhySpecialKey *newestKey = getNewestSubnetKeyPendingIncluded(backbone);
                        if (!newestKey) { //no key
                            LOG_ERROR("Error occurred while trying to find the DL subnet key");
                            return;
                        }

                        //check if backbone's newest key is expiring
                        if ((newestKey->policy.KeyHardLifetime > 0)
                                    && ((newestKey->softLifeTime - deviation) < currentTAI)) {

                            //create new DL subnet key and generate the operation
                            PhySpecialKey *newSubnetKey = createNewSubnetKeyOperation(backbone, currentTAI, operationsContainer);

                            backboneSubnetKey = newSubnetKey;
                            subnetKeyForDevice = new PhySpecialKey(*backboneSubnetKey);

                            newestKey->markedAsExpiring = true;
                        } else {

                            backboneSubnetKey = newestKey;
                            subnetKeyForDevice = new PhySpecialKey(*newestKey);
                        }
                    }

                    //send key to device
                    Uint16 greatestID = getGreatestSubnetKeyID(device);
                    subnetKeyForDevice->keyID = ++greatestID % 0xFF; //usually there should be maximum 2 DL keys

                    EntityIndex entityIndexDevice = createEntityIndex(device->address32, EntityType::SubnetKey, subnetKeyForDevice->keyID);
                    IEngineOperationPointer newSubnetKeyOperation(new WriteAttributeOperation(subnetKeyForDevice, entityIndexDevice));
                    operationsContainer->addOperation(newSubnetKeyOperation, device);

                    LOG_INFO("Updated subnet key for device " << device->address64.toString() << " : " << *subnetKeyForDevice);

                }

                foundExpiredKeys = true;
            }

            //send updates only to a group of devices, in order to avoid generating a great number of operations
            if (foundExpiredKeys) {
                --maxNumberOfUpdates;
            }
            if ((maxNumberOfUpdates == 0) ) {
                //after this call, no modification of the operationsContainer should be made
                engine->getOperationsProcessor().addOperationsContainer(operationsContainer);
                return;
            }
        }
        //the whole subnet was traversed but there are less than maxNumberOfUpdates

        //after this call, no modification of the operationsContainer should be made
        engine->getOperationsProcessor().addOperationsContainer(operationsContainer);
        return;

    }
}

void SecurityManager::GetHashInputString(const SecuritySymJoinRequest& request, const SecuritySymJoinResponse& response,
            Uint8(&buffer)[INPUT_HASH_BUFFERSIZE]) {
    memcpy(buffer, response.BitChallengeFromSystemManager, 16);
    memcpy(buffer + 16, request.bitChallengeFromNewDevice, 16);
    memcpy(buffer + 32, request.EUI64JoiningDevice.value, 8);
    memcpy(buffer + 40, SmSettingsLogic::instance().managerAddress64.value, 8);
    Uint16 noBuffer = htons(response.MasterKeyPolicy.KeyHardLifetime);
    memcpy(buffer + 48, &noBuffer, 2);
    noBuffer = htons(response.DlKeyPolicy.KeyHardLifetime);
    memcpy(buffer + 50, &noBuffer, 2);
    noBuffer = htons(response.SysMgrSessionKeyPolicy.KeyHardLifetime);
    memcpy(buffer + 52, &noBuffer, 2);
    //memcpy(buffer + 54, &response.DLKeyID, 1); //NOT in standard
    memcpy(buffer + 54, response.EncryptedDLKey, 16);
    memcpy(buffer + 70, response.EncryptedSysMgrSessionKey, 16);

}

bool SecurityManager::ValidateJoinRequestMIC(const SecuritySymJoinRequest& request, const NE::Model::SecurityKey& joinKey) {
    const Uint16 bufSize = 16 + 8 + 1 + 1;
    Uint8 authBuffer[bufSize];

    Uint32 hostOrderMic = htonl(request.mic);

    memcpy(authBuffer, request.EUI64JoiningDevice.value, 8);
    memcpy(authBuffer + 8, request.bitChallengeFromNewDevice, 16);
    memcpy(authBuffer + 24, &request.keyInfoFromNewDevice, 1);
    memcpy(authBuffer + 25, &request.algorithmIdentifier, 1);

    Uint8 outbuffer[bufSize];
    Uint8 *tempBuf = outbuffer;

    Isa100::Common::Ccm2 ccm;
    ccm.SetAuthenFldSz(4); // MIC is on 4 bytes
    ccm.SetLenFldSz(2); // (16(totalAESBlockSize) - 13(nonce) -1(firstBlock))

    if (!ccm.CheckAuthDecrypt((const Uint8*) joinKey.value, (const Uint8*) request.bitChallengeFromNewDevice,
                (Uint8*) authBuffer, (Uint16) bufSize, (Uint8*) &hostOrderMic, (Uint16) 4, tempBuf)) {
        return false;
    }
    return true;
}

bool SecurityManager::validateNewSessionRequest(SecurityNewSessionRequest newSessionRequest, Device *requester) {
    //search for the master key
    NE::Model::SpecialKeyIndexedAttribute::iterator itMaster = requester->phyAttributes.masterKeysTable.begin();
    if (itMaster == requester->phyAttributes.masterKeysTable.end()) {
        LOG_ERROR("No master key found for device " << requester->address64.toString());
        return false;
    }

    SecurityKey masterKey;
    bool found = false;
    for (; itMaster != requester->phyAttributes.masterKeysTable.end(); ++itMaster) {
        if (itMaster->second.currentValue //
                    && (itMaster->second.currentValue->keyID == newSessionRequest.keyIdentifier)) {

            found = true;
            masterKey = itMaster->second.currentValue->key;
            break;
        }
    }
    if (!found) {
        LOG_ERROR("validateNewSessionRequest - could not find master key with ID=" << (int) newSessionRequest.keyIdentifier
                    << "for device " << requester->address64.toString());
        return false;
    }

    Uint8 nonce[13];
    const Uint16 authSize = 16 + 2 + 16 + 2 + 1 + 1 + 1 + 1 + 4; //SecurityNewSessionRequest auth bytes (MIC excluded)
    Uint8 authBuffer[authSize];

    Uint32 timestamp = htonl(newSessionRequest.timeStamp);

    memcpy(nonce, requester->address64.value, 8);
    memcpy(nonce + 8, &timestamp, 4);
    nonce[12] = 0xFF;

    Uint16 portA = htons(newSessionRequest.endPortA + 0xF0B0);
    Uint16 portB = htons(newSessionRequest.endPortB + 0xF0B0);

    memcpy(authBuffer, newSessionRequest.address128A.value, 16);
    memcpy(authBuffer + 16, &portA, 2);
    memcpy(authBuffer + 18, newSessionRequest.address128B.value, 16);
    memcpy(authBuffer + 34, &portB, 2);
    memcpy(authBuffer + 36, &newSessionRequest.algorithmIdentifier, 1);
    memcpy(authBuffer + 37, &newSessionRequest.protocolVersion, 1);
    memcpy(authBuffer + 38, &newSessionRequest.securityControl, 1);
    memcpy(authBuffer + 39, &newSessionRequest.keyIdentifier, 1);
    //memcpy(authBuffer + 40, &newSessionRequest.timeStamp, 4);
    memcpy(authBuffer + 40, &timestamp, 4);

    LOG_DEBUG("SECURITY:SESSION_REQ:authLen=" << authSize);
    LOG_DEBUG("SECURITY:SESSION_REQ:masterKey=" << masterKey.toString());
    LOG_DEBUG("SECURITY:SESSION_REQ:nonce=" << StreamToString2(nonce, 13));
    LOG_DEBUG("SECURITY:SESSION_REQ:auth=" << StreamToString2(authBuffer, authSize));

    Uint32 hostOrderMic = htonl(newSessionRequest.MIC);

    LOG_DEBUG("SECURITY:SESSION_REQ:mic=" << std::hex << (int) hostOrderMic);

    Uint8 outbuffer[authSize];
    Uint8 *tempBuf = outbuffer;

    Isa100::Common::Ccm2 ccm;
    ccm.SetAuthenFldSz(4); // MIC is on 4 bytes
    ccm.SetLenFldSz(2); // (16(totalAESBlockSize) - 13(nonce) -1(firstBlock))

    if (!ccm.CheckAuthDecrypt((const Uint8*) masterKey.value, (const Uint8*) nonce,
                (Uint8*) authBuffer, (Uint16) authSize, (Uint8*) &hostOrderMic, (Uint16) 4, tempBuf)) {
        return false;
    }
    return true;
}

void SecurityManager::GenerateMasterKey(const SecuritySymJoinRequest& request, const SecuritySymJoinResponse& response,
            const NE::Model::SecurityKey& joinKey, NE::Model::SecurityKey& masterKey) {
    Uint8 keyBuffer[16 + 16 + 8 + 8];

    memcpy(keyBuffer, request.bitChallengeFromNewDevice, 16);
    memcpy(keyBuffer + 16, response.BitChallengeFromSystemManager, 16);
    memcpy(keyBuffer + 32, request.EUI64JoiningDevice.value, 8);
    memcpy(keyBuffer + 40, SmSettingsLogic::instance().managerAddress64.value, 8);

    Algorithms::HMACK_JOIN(keyBuffer, 16 + 16 + 8 + 8, joinKey.value, masterKey.value);

    //masterKeyPolicy.KeyHardLifetime.KeyHardLifetime = 0xFFFF * 3600;
}

NE::Model::PhySpecialKey * SecurityManager::getNewestActiveMasterKey(Device *srcDevice) {
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());

    NE::Model::SpecialKeyIndexedAttribute::iterator itMaster = srcDevice->phyAttributes.masterKeysTable.begin();
    if (itMaster == srcDevice->phyAttributes.masterKeysTable.end()) {
        LOG_ERROR("No master key found for device " << srcDevice->address64.toString() << ". Aborting...");
        return NULL;
    }
    //search for the newest master key which is active too
    NE::Model::PhySpecialKey *newestKey = itMaster->second.currentValue;
    ++itMaster;
    while (itMaster != srcDevice->phyAttributes.masterKeysTable.end()) {
        if (!newestKey && itMaster->second.currentValue) {
            newestKey = itMaster->second.currentValue;
        }

        if (itMaster->second.currentValue //
                    && (newestKey->hardLifeTime < itMaster->second.currentValue->hardLifeTime) //
                    && (newestKey->policy.NotValidBefore <= currentTAI) //
                    && !itMaster->second.isOnPending()) {
            newestKey = itMaster->second.currentValue;
        }

        ++itMaster;
    }

    return newestKey;
}

NE::Model::PhySpecialKey * SecurityManager::getNewestMasterKey(Device *srcDevice) {

    NE::Model::SpecialKeyIndexedAttribute::iterator itMaster = srcDevice->phyAttributes.masterKeysTable.begin();
    if (itMaster == srcDevice->phyAttributes.masterKeysTable.end()) {
        LOG_ERROR("No master key found for device " << srcDevice->address64.toString() << ". Aborting...");
        return NULL;
    }
    //search for the newest master key
    NE::Model::PhySpecialKey *newestKey = itMaster->second.currentValue;
    ++itMaster;
    while (itMaster != srcDevice->phyAttributes.masterKeysTable.end()) {
        if (!newestKey && itMaster->second.currentValue) {
            newestKey = itMaster->second.currentValue;
        }

        if (itMaster->second.currentValue //
                    && (newestKey->hardLifeTime < itMaster->second.currentValue->hardLifeTime) //
                    && !itMaster->second.isOnPending()) {
            newestKey = itMaster->second.currentValue;
        }

        ++itMaster;
    }

    return newestKey;
}

Uint16 SecurityManager::getGreatestMasterKeyID(Device *device) {
    Uint16 greatest = 0;

    for (NE::Model::SpecialKeyIndexedAttribute::iterator itMaster = device->phyAttributes.masterKeysTable.begin(); itMaster
                != device->phyAttributes.masterKeysTable.end(); ++itMaster) {

        if (itMaster->second.currentValue && (greatest < itMaster->second.currentValue->keyID)) {
            greatest = itMaster->second.currentValue->keyID;
        }
    }

    return greatest;
}

NE::Model::PhySpecialKey * SecurityManager::getNewestActiveSubnetKey(Device *srcDevice) {
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());

    NE::Model::SpecialKeyIndexedAttribute::iterator itSubnet = srcDevice->phyAttributes.subnetKeysTable.begin();
    if (itSubnet == srcDevice->phyAttributes.subnetKeysTable.end()) {
        LOG_ERROR("No subnet key found for device " << srcDevice->address64.toString() << ". Aborting...");
        return NULL;
    }
    //search for the newest subnet key
    NE::Model::PhySpecialKey *newestKey = itSubnet->second.currentValue;
    ++itSubnet;
    while (itSubnet != srcDevice->phyAttributes.subnetKeysTable.end()) {
        if (!newestKey && itSubnet->second.currentValue) {
            newestKey = itSubnet->second.currentValue;
        }

        if (itSubnet->second.currentValue //
                    && (newestKey->hardLifeTime < itSubnet->second.currentValue->hardLifeTime) //
                    && (newestKey->policy.NotValidBefore <= currentTAI) //
                    && (!itSubnet->second.isOnPending())) {
            newestKey = itSubnet->second.currentValue;
        }

        ++itSubnet;
    }

    return newestKey;
}

NE::Model::PhySpecialKey * SecurityManager::getNewestSubnetKeyPendingIncluded(Device *srcDevice) {
    //Uint32 currentTAI = NE::Common::ClockSource::getTAI(SmSettingsLogic::instance());

    NE::Model::SpecialKeyIndexedAttribute::iterator itSubnet = srcDevice->phyAttributes.subnetKeysTable.begin();
    if (itSubnet == srcDevice->phyAttributes.subnetKeysTable.end()) {
        LOG_ERROR("No subnet key found for device " << srcDevice->address64.toString() << ". Aborting...");
        return NULL;
    }
    //search for the newest subnet key
    NE::Model::PhySpecialKey *newestKey = itSubnet->second.currentValue;
    ++itSubnet;
    while (itSubnet != srcDevice->phyAttributes.subnetKeysTable.end()) {
        if (!newestKey && itSubnet->second.currentValue) {
            newestKey = itSubnet->second.currentValue;
        }

        if (itSubnet->second.currentValue //
                    && (newestKey->hardLifeTime < itSubnet->second.currentValue->hardLifeTime)) {
            newestKey = itSubnet->second.currentValue;
        }

        ++itSubnet;
    }

    return newestKey;
}

Uint16 SecurityManager::getGreatestSubnetKeyID(Device *device) {
    Uint16 greatest = 0;

    for (NE::Model::SpecialKeyIndexedAttribute::iterator itSubnet = device->phyAttributes.subnetKeysTable.begin(); itSubnet
                != device->phyAttributes.subnetKeysTable.end(); ++itSubnet) {

        if (itSubnet->second.currentValue && (greatest < itSubnet->second.currentValue->keyID)) {
            greatest = itSubnet->second.currentValue->keyID;
        }
    }

    return greatest;
}

void SecurityManager::createNewMasterKeyOperation(Device *device, Uint32 currentTAI, OperationsContainerPointer& operationsContainer) {
    Uint16 greatestID = getGreatestMasterKeyID(device);
    PhySpecialKey *newMasterKey = new PhySpecialKey();
    newMasterKey->keyID = ++greatestID % 0xFF; //usually there should be maximum 2 master keys so there's plenty of free IDs
    newMasterKey->key = keyGenerator->generateKey();
    newMasterKey->policy = defaultMasterPolicy;

    KeyUtils::updateKey(currentTAI, SmSettingsLogic::instance(), newMasterKey); //update lifetime and NotValidBefore

    EntityIndex entityIndexMaster = createEntityIndex(device->address32, EntityType::MasterKey, newMasterKey->keyID);
    IEngineOperationPointer newMasterKeyOperation(new WriteAttributeOperation(newMasterKey, entityIndexMaster));
    operationsContainer->addOperation(newMasterKeyOperation, device);

    LOG_INFO("New master key: " << *newMasterKey);
}

PhySpecialKey * SecurityManager::createNewSubnetKeyOperation(Device *device, Uint32 currentTAI, OperationsContainerPointer& operationsContainer) {
    Uint16 greatestID = getGreatestSubnetKeyID(device);
    PhySpecialKey *newSubnetKey = new PhySpecialKey();
    newSubnetKey->keyID = ++greatestID % 0xFF; //usually there should be maximum 2 DL keys
    newSubnetKey->key = GetSubnetKey(device->capabilities.dllSubnetId);
    newSubnetKey->policy = defaultSubnetPolicy;

    KeyUtils::updateKey(currentTAI, SmSettingsLogic::instance(), newSubnetKey); //update lifetime and NotValidBefore

    EntityIndex entityIndexSubnet = createEntityIndex(device->address32, EntityType::SubnetKey, newSubnetKey->keyID);
    IEngineOperationPointer newSubnetKeyOperation(new WriteAttributeOperation(newSubnetKey, entityIndexSubnet));
    operationsContainer->addOperation(newSubnetKeyOperation, device);

    LOG_INFO("New subnet key: " << *newSubnetKey);

    return newSubnetKey;
}

SecurityKey SecurityManager::GetSubnetKey(Uint16 subnetID) {
    if (SmSettingsLogic::instance().debugMode) { // subnet key= {subnet b1 subnet b0 c3 c4 c5 c6 c7 ... cf rnd
        NE::Model::SecurityKey newSubnetKey;
        Uint8 key[16] = { (Uint8) ((subnetID & 0xFF00) >> 8), (Uint8) subnetID & 0xFF, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
                0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0x00 };
        key[15] = (Uint8) (rand() % 16);
        newSubnetKey.copyKey(key);

        return newSubnetKey;
    }

    SecurityKey dlKey = keyGenerator->generateKey();
    return dlKey;

}

NE::Model::SecurityKey SecurityManager::getSessionKey(const NE::Common::Address64& from, const NE::Common::Address64& to,
            TSAP::TSAP_Enum fromTSAP, TSAP::TSAP_Enum toTSAP/*, NE::Model::Policy& sessionPolicy, Uint16& keyID*/) {

    LOG_INFO("GetSessionKey from=" << from.toString() << ", to=" << to.toString() << ", fromTSAP=" << fromTSAP << ", toTSAP="
                << toTSAP);

    NE::Model::SecurityKey secKey;

    Address32 fromAddr32 = Isa100::Model::EngineProvider::getEngine()->getAddress32(from);
    Address32 toAddr32 = Isa100::Model::EngineProvider::getEngine()->getAddress32(to);

    if (SmSettingsLogic::instance().debugMode) {
        // compose key: { from16 b1, from16 b0, 0, 0, 0, 0, 0, 0, to16 b1, to16 b0, 0, 0, 0, 0, 0, 0}
        Uint8 key[16];
        memset(key, 0, 16);
        Address16 from16 = Isa100::Model::EngineProvider::getEngine()->getAddress16(fromAddr32);
        Address16 to16 = Isa100::Model::EngineProvider::getEngine()->getAddress16(toAddr32);
        key[0] = (from16 & 0xFF00) >> 8;
        key[1] = from16 & 0xFF;
        key[8] = (to16 & 0xFF00) >> 8;
        key[9] = to16 & 0xFF;

        secKey.copyKey(key);

    } else {
        secKey = keyGenerator->generateKey();

    }

    return secKey;
}

SFC::SFCEnum SecurityManager::deleteStackKey(const NE::Common::Address128& destination, TSAP::TSAP_Enum tsapFrom,
            TSAP::TSAP_Enum tsapTo, Uint16 keyID) {

    AL::DMAP::TLMO tlmo;

    Uint8 keyType = 1; // TODO SLM_KEY_USAGE_SESSION

    SFC::SFCEnum sfc = tlmo.DeleteKey(destination, tsapFrom, tsapTo, keyID, keyType);

    std::ostringstream stream;
    stream << "delete stack key: keyID=" << (int) keyID << ", destination=" << destination.toString() << ", sourceTSAP=" << tsapFrom
                << ", destinationTSAP=" << tsapTo << ", sfc=" << sfc;
    if (sfc != SFC::success) {
        LOG_ERROR(stream.str());
    } else {
        LOG_INFO(stream.str());
    }
    return sfc;
}

Uint32 SecurityManager::getNextChallenge() {
    return keyGenerator->getNextChallenge();
}

void SecurityManager::setNextChallenge(Uint32 nextChallenge) {
    keyGenerator->setNextChallenge(nextChallenge);
}


}
}
