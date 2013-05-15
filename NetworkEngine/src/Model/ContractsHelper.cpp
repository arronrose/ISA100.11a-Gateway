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
 * @author catalin.pop, radu.pop, beniamin.tecar,sorin.bidian
 */
#include "ContractsHelper.h"
#include "Model/Operations/WriteAttributeOperation.h"
#include "Misc/Marshall/NetworkOrderBytesWriter.h"
#include "SMState/SMStateLog.h"
#include "Model/Operations/OperationsProcessor.h"
#include "Common/ClockSource.h"

using namespace NE::Common;
using namespace NE::Model::Operations;

namespace NE {
namespace Model {

ContractsHelper::ContractsHelper() {
}

ContractsHelper::~ContractsHelper() {
}

PhyContract * ContractsHelper::createContract(const ContractRequest& contractRequest, Device * ownerDevice, Device * destDevice,
            const SettingsLogic& settingsLogic) {
    LOG_DEBUG("createContract(), request: " << contractRequest);

    Uint16 currentID = ownerDevice->getNextContractID();

    PhyContract * contract = new PhyContract();
    contract->contractID = currentID;
    contract->requestID = contractRequest.contractRequestId;
    contract->source32 = contractRequest.sourceAddress;
    contract->destination32 = contractRequest.destinationAddress;
    contract->destination128 = destDevice->address128;
    contract->isManagement = contractRequest.isManagement;


    assert(contractRequest.sourceSAP < 0xF0B0 && "invalid tsap >= 0xF0B0");
    contract->sourceSAP = contractRequest.sourceSAP;

    assert(contractRequest.destinationSAP < 0xF0B0 && "invalid tsap >= 0xF0B0");
    contract->destinationSAP = contractRequest.destinationSAP;

    contract->responseCode = ContractResponseCode::SuccessWithImmediateEffect;
    contract->communicationServiceType = contractRequest.communicationServiceType;

    //check ContractRequest.contractLife - we interpret and store it as an offset;
    //make adjustments if needed
    //contract_activation_time will be the current TAI
    //contract will expire at (contract_activation_time + contractLife)
    Uint32 currentTAI = NE::Common::ClockSource::getTAI(settingsLogic);
    contract->contract_Activation_Time = currentTAI;

    if (contractRequest.contractLife == MAX_32BITS_VALUE) { //non-expiring
        contract->assigned_Contract_Life = MAX_32BITS_VALUE; // = 0;
    } else if (contractRequest.contractLife > (MAX_32BITS_VALUE - currentTAI)) {
        contract->assigned_Contract_Life = MAX_32BITS_VALUE - currentTAI;
    } else {
        contract->assigned_Contract_Life = contractRequest.contractLife;
    }

    //for the publish diagnostics contract (device->SM) allocate a lower priority
    //for client server contract with manager allocate highest priority
    if (ownerDevice->capabilities.isManager() || destDevice->capabilities.isManager()){
        if (contractRequest.communicationServiceType == CommunicationServiceType::Periodic){
           contract->assigned_Contract_Priority = ContractPriority::BestEffortQueued;
        } else {
           contract->assigned_Contract_Priority = ContractPriority::NetworkControl;
        }
    } else {
        contract->assigned_Contract_Priority = contractRequest.contractPriority;
    }

    if ((ownerDevice->capabilities.isManager() && destDevice->capabilities.isGateway()) //
                || (ownerDevice->capabilities.isGateway() && destDevice->capabilities.isManager())) {
        //for contracts between GW and SM the NSDUsize is bigger
        contract->assigned_Max_NSDU_Size = settingsLogic.gw_max_NSDU_Size;

    } else if((ownerDevice->capabilities.isManager() && destDevice->capabilities.isBackbone()) //
            || (ownerDevice->capabilities.isBackbone() && destDevice->capabilities.isManager())) {

    	contract->assigned_Max_NSDU_Size = settingsLogic.bbr_max_NSDU_Size;

    } else {
        contract->assigned_Max_NSDU_Size = settingsLogic.max_NSDU_Size;
    }

    contract->assigned_Reliability_And_PublishAutoRetransmit = contractRequest.reliability_And_PublishAutoRetransmit;
    contract->assignedPeriod = contractRequest.requestedPeriod;
    contract->assignedPhase = contractRequest.requestedPhase;
    contract->assignedDeadline = contractRequest.requestedDeadline;
    contract->assignedCommittedBurst = contractRequest.committedBurst;
    contract->assignedExcessBurst = contractRequest.excessBurst;
    contract->assigned_Max_Send_Window_Size = settingsLogic.maxSendWindowSize;

    return contract;
}

void ContractsHelper::createContractsSmToNeighbor(Device * manager, Device * neighbor,
            NE::Model::Operations::OperationsProcessor& operationsProcessor, const SettingsLogic& settingsLogic) {

    // add contract Manager2Backbone - temporary during JOIN: sm(DMAP)->bbr(DMAP) needed because join from BBR is handled between DMAP-DMAP
    ContractRequest contractRequestJoin;
    contractRequestJoin.contractRequestId = 0xFFFF; //ContractRequestID=0xFFFF for management contracts
    contractRequestJoin.sourceAddress = manager->address32;
    contractRequestJoin.sourceSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;
    contractRequestJoin.destinationSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;
    contractRequestJoin.destinationAddress = neighbor->address32;
    contractRequestJoin.communicationServiceType = CommunicationServiceType::NonPeriodic;
    contractRequestJoin.committedBurst = settingsLogic.manager2NeighborCommittedBurst;//contractele cu BBR sau GW suporta trafic mare 20 pkg/sec
    contractRequestJoin.excessBurst = settingsLogic.manager2NeighborCommittedBurst;
    contractRequestJoin.isManagement = true;

    if (neighbor->capabilities.isGateway()
                && findContractSource2Destination(manager, neighbor->address32, contractRequestJoin.sourceSAP, contractRequestJoin.destinationSAP)) {
        LOG_INFO("Skip create manager to neighbor contract; contract already exist; recover contractRequest=" << contractRequestJoin);
    } else {
        // create join contract
        PhyContract * joinContract = ContractsHelper::createContract(contractRequestJoin, manager, neighbor, settingsLogic);
        EntityIndex joinContractEntityIndex = createEntityIndex(manager->address32, EntityType::Contract, joinContract->contractID);
        IEngineOperationPointer joinContractOperation(new WriteAttributeOperation(joinContract, joinContractEntityIndex));
        operationsProcessor.addManagerOperation(joinContractOperation);

        // create the associated network contract
        PhyNetworkContract * networkJoinContract = ContractsHelper::createNetworkContract(joinContract, manager->address128,
                    neighbor->address128);
        EntityIndex networkJoinContractEntityIndex = createEntityIndex(manager->address32, EntityType::NetworkContract,
                    networkJoinContract->contractID);
        //manager->phyAttributes.createNetworkContract(networkJoinContractEntityIndex, networkJoinContract);
        IEngineOperationPointer networkJoinContractOperation(new WriteAttributeOperation(networkJoinContract, networkJoinContractEntityIndex));
        operationsProcessor.addManagerOperation(networkJoinContractOperation);
    }


    // add contract Manager2Backbone - contract used after JOIN: sm(SMAP)->bbr(DMAP), needed because all communication(after join) will be SMAP-DMAP
    ContractRequest contractRequestAfterJoin(contractRequestJoin);
    contractRequestAfterJoin.contractRequestId = 0xFFFF; //ContractRequestID=0xFFFF for management contracts
    contractRequestAfterJoin.sourceSAP = ContractTDSAP::TDSAP_SMAP - 0xF0B0;//port of SMAP
    contractRequestAfterJoin.destinationSAP = ContractTDSAP::TDSAP_DMAP - 0xF0B0;//port of DMAP

    if (neighbor->capabilities.isGateway()
                && findContractSource2Destination(manager, neighbor->address32, contractRequestAfterJoin.sourceSAP, contractRequestAfterJoin.destinationSAP)) {
        LOG_INFO("Skip create manager to neighbor contract; contract already exist; recover contractRequest=" << contractRequestAfterJoin);
    } else {
        // add contract
        PhyContract * contractAfterJoin = ContractsHelper::createContract(contractRequestAfterJoin, manager, neighbor, settingsLogic);
        EntityIndex contractAfterJoinEntityIndex = createEntityIndex(manager->address32, EntityType::Contract,
                    contractAfterJoin->contractID);
        IEngineOperationPointer contractAfterJoinOperation(
                    new WriteAttributeOperation(contractAfterJoin, contractAfterJoinEntityIndex));
        operationsProcessor.addManagerOperation(contractAfterJoinOperation);

        // create the associated network contract
        PhyNetworkContract * networkContractAfterJoin = ContractsHelper::createNetworkContract(contractAfterJoin,
                    manager->address128, neighbor->address128);
        EntityIndex networkContractAfterJoinEntityIndex = createEntityIndex(manager->address32, EntityType::NetworkContract,
                    networkContractAfterJoin->contractID);
        //manager->phyAttributes.createNetworkContract(networkContractAfterJoinEntityIndex, networkContractAfterJoin);
        IEngineOperationPointer networkContractAfterJoinOperation(
                    new WriteAttributeOperation(networkContractAfterJoin, networkContractAfterJoinEntityIndex));
        operationsProcessor.addManagerOperation(networkContractAfterJoinOperation);
    }

    //add contract SM->GW : DMAP->UAP2 - for alert reports
    if (neighbor->capabilities.isGateway()
                && findContractSource2Destination(manager, neighbor->address32, 0, 2)) {
        LOG_INFO("Skip create manager to gw contract, tsap: 0->2; contract already exists");

    } else {

        ContractRequest contractRequestAfterJoin(contractRequestJoin);
        contractRequestAfterJoin.sourceSAP = 0;//port of DMAP
        contractRequestAfterJoin.destinationSAP = 2;//port of UAP2
        // add contract
        PhyContract * contractAfterJoin = ContractsHelper::createContract(contractRequestAfterJoin, manager, neighbor, settingsLogic);
        EntityIndex contractAfterJoinEntityIndex = createEntityIndex(manager->address32, EntityType::Contract,
                    contractAfterJoin->contractID);
        IEngineOperationPointer contractAfterJoinOperation(
                    new WriteAttributeOperation(contractAfterJoin, contractAfterJoinEntityIndex));
        operationsProcessor.addManagerOperation(contractAfterJoinOperation);

        // create the associated network contract
        PhyNetworkContract * networkContractAfterJoin = ContractsHelper::createNetworkContract(contractAfterJoin,
                    manager->address128, neighbor->address128);
        EntityIndex networkContractAfterJoinEntityIndex = createEntityIndex(manager->address32, EntityType::NetworkContract,
                    networkContractAfterJoin->contractID);
        //manager->phyAttributes.createNetworkContract(networkContractAfterJoinEntityIndex, networkContractAfterJoin);
        IEngineOperationPointer networkContractAfterJoinOperation(
                    new WriteAttributeOperation(networkContractAfterJoin, networkContractAfterJoinEntityIndex));
        operationsProcessor.addManagerOperation(networkContractAfterJoinOperation);
    }
}

PhyNetworkContract * ContractsHelper::createNetworkContract(PhyContract * contract, const Address128& srcAddress,
            const Address128& destAddress) {
    LOG_DEBUG("creating network contract associated to contract " << (int) contract->contractID);

    PhyNetworkContract * networkContract = new PhyNetworkContract();

    networkContract->contractID = contract->contractID;
    networkContract->sourceAddress = srcAddress;
    networkContract->destinationAddress = destAddress;
    networkContract->contract_Priority = contract->assigned_Contract_Priority;
    networkContract->include_Contract_Flag = 0;
    //TODO: include_Contract_Flag needs to be updated when route selection by contractID is available

    networkContract->assigned_Max_NSDU_Size = contract->assigned_Max_NSDU_Size;
    networkContract->assigned_Max_Send_Window_Size = contract->assigned_Max_Send_Window_Size;
    networkContract->assignedCommittedBurst = contract->assignedCommittedBurst;
    networkContract->assignedExcessBurst = contract->assignedExcessBurst;

    return networkContract;
}

PhyContract * ContractsHelper::findContract(Device * source, ContractId contractId) {
    EntityIndex index = createEntityIndex(source->address32, EntityType::Contract, contractId);
    ContractIndexedAttribute::iterator it = source->phyAttributes.contractsTable.find(index);
    if (it != source->phyAttributes.contractsTable.end()) {
        return it->second.getValue();
    } else {
        return NULL;
    }
}

PhyContract * ContractsHelper::findContractSource2Destination(Device * source, Address32 destination32, Uint16 tsapSrc,
            Uint16 tsapDest) {
    ContractIndexedAttribute::iterator itContract = source->phyAttributes.contractsTable.begin();
    ContractIndexedAttribute::iterator itEnd = source->phyAttributes.contractsTable.end();
    for (; itContract != itEnd; ++itContract) {
        PhyContract * contract = itContract->second.getValue();
        if (contract && contract->destination32 == destination32 && contract->destinationSAP == tsapDest && contract->sourceSAP
                    == tsapSrc) {
            return contract;
        }
    }

    return NULL;
}

PhyContract * ContractsHelper::findContractSource2Destination(Device * source, Address32 destination32, Uint16 tsapSrc,
            Uint16 tsapDest, Uint8 contractRequestID, CommunicationServiceType::CommunicationServiceType type) {
    ContractIndexedAttribute::iterator itContract = source->phyAttributes.contractsTable.begin();
    for (; itContract != source->phyAttributes.contractsTable.end(); ++itContract) {
        PhyContract * contract = itContract->second.getValue();
        if (contract && contract->destination32 == destination32 //
                    && contract->destinationSAP == tsapDest //
                    && contract->sourceSAP == tsapSrc //
                    && contract->requestID == contractRequestID //
                    && contract->communicationServiceType == type) {
            return contract;
        }
    }

    return NULL;
}

PhyContract * ContractsHelper::findContractByRequestID(Device * source, Uint8 requestID) {
    for (ContractIndexedAttribute::iterator it = source->phyAttributes.contractsTable.begin(); it
                != source->phyAttributes.contractsTable.end(); ++it) {
        PhyContract * currentContract = it->second.getValue();
        if (currentContract == NULL) {
            continue;
        }
        if (currentContract->requestID == requestID) {
            return currentContract;
        }
    }
    return NULL;
}

void ContractsHelper::removeContract(Address32 source, ContractId contractId) {
#warning review ContractsHelper.cpp
}

void ContractsHelper::terminateContract(Operations::OperationsContainer& engineOperations, Address32 source,
            ContractId contractId) {
#warning review ContractsHelper.cpp
}


void ContractsHelper::renewGatewayContracts(Address32 gatewayAddress32, Address32 managerAddress32) {

}


}
}
