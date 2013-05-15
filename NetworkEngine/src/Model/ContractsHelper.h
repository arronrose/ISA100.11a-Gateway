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
#ifndef CONTRACTSHELPER_H_
#define CONTRACTSHELPER_H_

#include <utility>
#include <map>
#include "Common/NEAddress.h"
#include "Common/NETypes.h"
#include "Common/logging.h"
#include "Model/ContractRequest.h"
#include "Model/Operations/OperationsContainer.h"
#include "Model/model.h"
#include "Model/Operations/OperationsProcessor.h"
#include "Common/SettingsLogic.h"
#include "Model/Device.h"

using namespace NE::Common;

namespace NE {
namespace Model {

/** Maximum number for Contract ID */
#define MAX_CONTRACT_ID 0xFFFF

class ContractsHelper {
        LOG_DEF("I.M.ContractsHelper");

    public:

        ContractsHelper();

        virtual ~ContractsHelper();

        /**
         * Create a contract for a requested details.
         * @param contractRequest
         * @param ownerDevice - device that owns the contract (same as contract.source).
         * @return
         */
        static PhyContract * createContract(const ContractRequest& contractRequest, Device * ownerDevice, Device * destDevice,
                    const SettingsLogic& settingsLogic);

        /**
         * Creates 2 contracts SM->backbone:
         * - contract DMAP->DMAP (used for communication during the join process)
         * - contract SMAP->DMAP (used for communication after the device is joined)
         * @param manager
         * @param backbone/gateway
         */
        static void createContractsSmToNeighbor(Device * manager, Device * neighbor,
                    NE::Model::Operations::OperationsProcessor& operationsProcessor, const SettingsLogic& settingsLogic);

        static PhyNetworkContract * createNetworkContract(PhyContract * contract, const Address128& srcAddress,
                    const Address128& destAddress);

        /**
         * Finds a contract with requested contractId on the source device.
         * @param source
         * @param contractId
         * @return
         */
        static PhyContract * findContract(Device * source, ContractId contractId);

        /**
         * Finds the contract between source and destination on the specified TSAPs.
         * @param source
         * @param destination
         * @return
         */
        static PhyContract * findContractSource2Destination(Device * source, Address32 destination32, Uint16 tsapSrc,
                    Uint16 tsapDest);

        /**
         * Finds the contract between source and destination on the specified TSAPs and having the same contractRequestID.
         *
         * @param source
         * @param destination32
         * @param tsapSrc
         * @param tsapDest
         * @param contractRequestID
         * @return
         */
        static PhyContract * findContractSource2Destination(Device * source, Address32 destination32, Uint16 tsapSrc,
                    Uint16 tsapDest, Uint8 contractRequestID, CommunicationServiceType::CommunicationServiceType type);

        /**
         * Find the contract having the requestID on the source device.
         *
         * @param source
         * @param requestID
         * @return
         */
        static PhyContract * findContractByRequestID(Device * source, Uint8 requestID);


        /**
         * Remove a contract(source, contractId) from model.
         * @param source
         * @param contractId
         */
        void removeContract(Address32 source, ContractId contractId);

        /**
         * Used for Contract termination, deactivation and reactivation request.
         */
        void terminateContract(Operations::OperationsContainer& engineOperations, Address32 source, ContractId contractId);

        /**
         * On gateway rejoin the settings are not released, the already establish settings
         * are sent to the gateway, in this case Contracts
         * @param gatewayAddress32
         * @param managerAddress32
         */
        void renewGatewayContracts(Address32 gatewayAddress32, Address32 managerAddress32);

};

}
}

#endif /*CONTRACTSTABLE_H_*/
