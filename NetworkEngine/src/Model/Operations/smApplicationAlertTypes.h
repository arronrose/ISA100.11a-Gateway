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
 * smApplicationAlertTypes.h
 *
 *  Created on: Apr 13, 2010
 *      Author: Sorin.Bidian
 */

#ifndef SMAPPLICATIONALERTTYPES_H_
#define SMAPPLICATIONALERTTYPES_H_

#include "Common/NEAddress.h"
#include "Model/ContractTypes.h"
#include "Model/model.h"

namespace NE {
namespace Model {
namespace Operations {

namespace SMAlertTypes {
enum SMAlertTypes {
    //device join/leave alerts
    DeviceJoin = 1,
    DeviceJoinFailed = 2,
    DeviceLeave = 3,

    //fw update progress alerts
    TransferStarted = 4,
    TransferProgress = 5,
    TransferEnded = 6,

    //contract alerts
    ContractEstablishment = 7,
    ContractModification = 8,
    ContractRefusal = 9,
    ContractTermination = 10,

    //topology alerts
    ParentChange = 11,
    BackupChange = 12
};
}

namespace JoinFailureReason {
enum JoinFailureReason {
    timeout = 1,
    rejoin = 2,
    parentLeft = 3, //when device is removed due to parent leave
    provisioning_removed = 4, //on provisioning file change + reload, devices may be removed
    not_provisioned = 5,
    invalid_join_key = 6,
    challenge_check_failure = 7, //invalid challenge
    insufficient_parent_resources = 8, //the routing device does not support the join due to resource limitations
    subnet_provisioning_mismatch = 9, //the subnetID provisioned on device is different from the one in the SystemMnager's provisioning file
    other = 10
};
}

namespace LeaveReason {
enum LeaveReason {
    timeout = 1,
    rejoin = 2 //join request while joined
};
}

namespace ContractRefusalReason {
enum ContractRefusalReason {
    insufficient_resources = 1,
    delayed = 2
};
}

namespace ContractTerminationReason {
enum ContractTerminationReason {
    requested = 1,
    expired = 2,
    unjoin = 3
};
}

namespace ParentChangeReason {
enum ParentChangeReason {
    parent_left = 1,
    high_PER = 2,       // high Packet Error Rate with former parent
    reduce_level = 3,   // migrate to a lower level - migrate to BBR if there is free slot or on Level1 candidate router that has free slot (can accept a new child)
    balancing = 4       // migrate from a high-load parent to a parent with more resources available
};
}

struct AlertJoin {
    Address128 networkAddress;
    Uint16 deviceType;
    Address64 deviceEUI64;
    Uint8 powerSupplyStatus;
    Uint8 joinStatus;
    VisibleString manufacturer;
    VisibleString model;
    VisibleString revision;
    VisibleString deviceTag;
    VisibleString serialNo;

    AlertJoin(const Address128& networkAddress_, Uint16 deviceType_, const Address64& deviceEUI64_,
                Uint8 powerSupplyStatus_, Uint8 joinStatus_, const VisibleString& manufacturer_, const VisibleString& model_,
                const VisibleString& revision_, const VisibleString& deviceTag_, const VisibleString& serialNo_) :

                    networkAddress(networkAddress_), deviceType(deviceType_), deviceEUI64(deviceEUI64_),
                    powerSupplyStatus(powerSupplyStatus_), joinStatus(joinStatus_), manufacturer(manufacturer_),
                    model(model_), revision(revision_), deviceTag(deviceTag_), serialNo(serialNo_) {

    }
};

struct AlertJoinFailed {
    Address64 deviceEUI64;
    Uint8 joinPhase;
    Uint8 failureReason;

    AlertJoinFailed(const Address64& deviceEUI64_, Uint8 joinPhase_, Uint8 failureReason_) :
        deviceEUI64(deviceEUI64_), joinPhase(joinPhase_), failureReason(failureReason_) {

    }
};

struct AlertLeave {
    Address64 deviceEUI64;
    Uint8 leaveReason;

    AlertLeave(const Address64& deviceEUI64_, Uint8 leaveReason_) :
        deviceEUI64(deviceEUI64_), leaveReason(leaveReason_) {

    }
};

struct AlertTransferStarted {
    Address64 deviceEUI64;
    Uint16 bytesPerPacket;
    Uint16 totalpackets;
    Uint32 totalBytes;

    AlertTransferStarted(const Address64& deviceEUI64_, Uint16 bytesPerPacket_, Uint32 totalpackets_, Uint32 totalBytes_) :
        deviceEUI64(deviceEUI64_), bytesPerPacket(bytesPerPacket_), totalpackets(totalpackets_), totalBytes(totalBytes_) {

    }
};

struct DeviceTransferProgress {
    Address64 deviceEUI64;
    Uint16 packetsTransferred;

    DeviceTransferProgress(const Address64& deviceEUI64_, Uint16 packetsTransferred_) :
        deviceEUI64(deviceEUI64_), packetsTransferred(packetsTransferred_) {

    }
};

struct AlertTransferProgress {
    std::vector<DeviceTransferProgress> transferProgressList;

    AlertTransferProgress(const std::vector<DeviceTransferProgress>& transferProgressList_) :
        transferProgressList(transferProgressList_) {

    }
};

struct AlertTransferEnded {
    Address64 deviceEUI64;
    Uint8 errorCode;

    AlertTransferEnded(const Address64& deviceEUI64_, Uint8 errorCode_) :
        deviceEUI64(deviceEUI64_), errorCode(errorCode_) {

    }
};

struct AlertContractEstablishment {
    PhyContract contract;

    AlertContractEstablishment(const PhyContract& contract_) :
        contract(contract_) {

    }
};

struct AlertContractModification {
    PhyContract contract;

    AlertContractModification(const PhyContract& contract_) :
        contract(contract_) {

    }
};

struct AlertContractRefusal {
    PhyContract contract;
    Uint8 requestType;  //NE::Model::ContractRequestType
    Uint8 reason;       //ContractRefusalReason

    AlertContractRefusal(const PhyContract& contract_, Uint8 requestType_, Uint8 reason_) :
        contract(contract_), requestType(requestType_), reason(reason_) {

    }
};

struct AlertContractTermination {
    Address128 sourceAddress;
    Uint16 contractID;
    Uint8 reason;   //ContractTerminationReason

    AlertContractTermination(const Address128& sourceAddress_, Uint16 contractID_, Uint8 reason_) :
        sourceAddress(sourceAddress_), contractID(contractID_), reason(reason_) {

    }
};

struct AlertParentChange {
    Address64 deviceEUI64;
    Address64 newParent;
    Address64 oldParent;
    Uint8 reason;   //ParentChangeReason

    // reason-dependent data fields
    Uint16 PER_threshlod; // when reason = high_PER
    Uint16 exceedingComputedValue; // when reason = high_PER; value that exceeded the threshold

    std::vector<PhyCandidate> candidates;

    AlertParentChange(const Address64& deviceEUI64_, const Address64& newParent_, const Address64& oldParent_, Uint8 reason_) :
        deviceEUI64(deviceEUI64_), newParent(newParent_), oldParent(oldParent_), reason(reason_),
        PER_threshlod(0), exceedingComputedValue(0) {

    }

    AlertParentChange(const Address64& deviceEUI64_, const Address64& newParent_, const Address64& oldParent_, Uint8 reason_,
                Uint16 PER_threshlod_, Uint16 exceedingComputedValue_) :
        deviceEUI64(deviceEUI64_), newParent(newParent_), oldParent(oldParent_), reason(reason_),
        PER_threshlod(PER_threshlod_), exceedingComputedValue(exceedingComputedValue_) {

    }

    AlertParentChange(const Address64& deviceEUI64_, const Address64& newParent_, const Address64& oldParent_, Uint8 reason_,
                std::vector<PhyCandidate>& candidates_) :
        deviceEUI64(deviceEUI64_), newParent(newParent_), oldParent(oldParent_), reason(reason_),
        PER_threshlod(0), exceedingComputedValue(0), candidates(candidates_) {

    }

};

struct AlertBackupChange {
    Address64 deviceEUI64;
    Address64 newBackup;
    Address64 oldBackup;

    AlertBackupChange(const Address64& deviceEUI64_, const Address64& newBackup_, const Address64& oldBackup_) :
        deviceEUI64(deviceEUI64_), newBackup(newBackup_), oldBackup(oldBackup_) {

    }
};

}
}
}

#endif /* SMAPPLICATIONALERTTYPES_H_ */
