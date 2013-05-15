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
 * Main.cpp
 *
 *  Created on: May 14, 2008
 *      Author: catalin.pop, flori.parauan, eduard.budulea,beniamin.tecar, ioan.pocol, george.petrehus, sorin.bidian
 */
#ifndef NETWORKENGINE_H_
#define NETWORKENGINE_H_

#include <vector>
#include <map>

#include "Common/logging.h"
#include "Common/SettingsLogic.h"
#include "Common/ISettingsProvider.h"
#include "Model/IEngine.h"
#include "Model/Operations/OperationsProcessor.h"
#include "Model/SubnetsContainer.h"
#include "Model/TheoreticEngine.h"
#include "Model/IDeviceRemover.h"
#include "Common/NETypes.h"

using namespace NE::Model::Operations;
using namespace NE::Model::Routing;

namespace NE {

namespace Model {

namespace EngineComponents {
enum EngineComponentsEnum {
    ALL = 0, ContractsTable = 1, DeviceTable = 2, Topology = 3, LinkEngine = 4
};
}

typedef std::map<Address64, Address128> AddressMapping64_128;

/**
 * NetworkEngine.
 * @author Catalin Pop
 * @version 1.0
 */
class NetworkEngine: boost::noncopyable, public IEngine, public NE::Common::ISettingsProvider, public NE::Model::IDeviceRemover {

        LOG_DEF("I.M.NetworkEngine");

    private:
        /**
         * The time the NetworkEngine was started.
         */
        Uint32 startTime;

        SubnetsContainer subnetsContainer;

        NE::Model::Operations::OperationsProcessor operationsProcessor;


        TheoreticEngine theoreticEngine;

        NE::Model::Reports::ReportsEngine reportsEngine;

        /**
         * Used to hold temporarily the mapping between 64 and 128 for Backbones and GW before join method.
         * After join this mapping is moved to the Device instance of Backbone or GW.
         */
        AddressMapping64_128 managerNeighborsMapping;

        SettingsLogic* settingsLogic;

        static string REGISTRATION_DEVICE;

    private:


        /**
         * Called by the requestJoinSecurity method from the Physical Model interface.
         */
        void requestJoinSecurityGateway(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
                    const PhySessionKey& joinKey, HandlerResponse handlerResponse);

        /**
         * Called by the requestConfirmJoin method from the Physical Model interface.
         */
        void requestConfirmJoinGW(const Address32 deviceAddress32, int requestID, HandlerResponse handlerResponse);

        void postJoinTask(Subnet::PTR& subnet, Uint32 currentTime);

        /**
         * Called after the device's join confirm request is processed.
         *
         * @param deviceAddress
         * @param requestID
         * @param status
         */
        void handlerVendorAttributes(Address32 deviceAddress, int requestID,
                    ResponseStatus::ResponseStatusEnum status);


        void handlerConfirmationReadMetadata(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status);

        /**
         * Called after the gateway's join confirm request is processed.
         * @param deviceAddress
         * @param requestID
         * @param status
         */
        void readMetadata(Address32 deviceAddress32);



        /**
         * Generates operations for reading device's vendor attributes and adds them to the container.
         *
         * @param device
         * @param confirmContainer
         */
        void readVendorAttributes(Device * device, OperationsContainerPointer& confirmContainer);

//        /**
//         * Called after the readings for device's metadata are completed.
//         *
//         * @param deviceAddress
//         * @param requestID
//         * @param status
//         */
//        void handlerStartPublish(Address32 deviceAddress, int requestID, ResponseStatus::ResponseStatusEnum status);

        void handlerJoinSecurityResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status);

        void handlerSmJoinResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status);

        void handlerSmContractJoinResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status);

        void handlerSecConfirmResponse(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status);

        void handlerJoinedConfigured(Address32 deviceAddress32, int requestID, ResponseStatus::ResponseStatusEnum status);

    public:

        NetworkEngine(SettingsLogic* settingsLogic_);
        // static NetworkEngine& instance();

        virtual ~NetworkEngine();

        Uint32 getStartTime();

        NE::Common::SettingsLogic& getSettingsLogic();

        void updateAdvPeriod();

        void setSettingsLogic(NE::Common::SettingsLogic* _settingsLogic);

        SubnetsContainer& getSubnetsContainer() {
            return subnetsContainer;
        }

        Operations::OperationsProcessor& getOperationsProcessor() {
            return operationsProcessor;
        }

        NE::Model::Reports::ReportsEngine& getReportsEngine() {
            return reportsEngine;
        }

        NE::Model::TheoreticEngine& getTheoreticEngine(){
            return theoreticEngine;
        }

        /**
         * Used to add a mapping between the two addresses.
         * Called by DMO at join of Backbone or Gateway - the two types of devices that have their own 128-bit address.
         */
        void addManagerNeighbor(const Address64& neighborAddress64, const Address128& neighborAddress128);

        bool removePreviousDevice(Subnet::PTR & subnet, const Address32 oldAddress, const Address64 & deviceAddress64,
                    Device * device, const DeviceType::DeviceTypeEnum provisionedType, const Uint16 provisionedSubnetID, OperationsContainerPointer & containerRemDev);

        /**
         * Physical Model interface.
         * Step 1 of join: security join request.
         * - creates a new device and assigns addresses 128-bit, 32-bit and 16-bit address
         * - initializes device's attributes to default values
         * - sets the join key on the device and on the manager
         */
        void requestJoinSecurity(const Address64 & deviceAddress64, const Address32 parentAddress32, int requestID,
                    DeviceType::DeviceTypeEnum provisionedType, Uint16 provisionedSubnetID, const PhySessionKey& joinKey,
                    HandlerResponse handlerResponse);

        /**
         * Physical Model interface.
         * Step 2 of join: system manager join request (join to network).
         * - updates the device type and any other information needed from capabilities in internal model
         * - ensures that all addresses are created in internal model (they will be sent in response).
         * - Call the handlerResponse
         */
        void requestJoinSystemManager(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
                    const Capabilities& capabilities, std::string& softwareRevision, const PhyDeviceCapability& deviceCapability,
                    HandlerResponse handlerResponse);

        /**
         * Physical Model interface.
         * Step 3 of join: contract join request (allocation of network resources for join contract).
         * This function must allocate in field all the management resources needed for the new device.
         * It will send in field all the entities that do not depend on other entities.
         * It will generate a list of Operations (the same as the operations currently implemented Ex: AddLink, AddNeighbor)
         * and this list will be applied on internal model or sent in field (for those entities without dependency; the other operations
         * will be stored in a cache and applied by deviceConfirmation() function).
         * It will return without calling the handlerResponse.
         * The entities that must be created:
         * BBR:
         * - route(SM->device)- combination of Graph of parent and deviceAddress,
         * - more bandwidth Links if needed
         * - ATT table with new device
         * GW:
         * - NetworkRoute (GW-device)
         * - ATT table (device)
         * ProxyRouter: neighbor(deviceAddress) in group 1
         * New device: NetworkRoute - this is not sent directly in the field because its content is placed in the response and
         * the device populates by default a NetworkRoute with this information.
         *
         * @param deviceAddress64
         * @param parentAddress32
         * @param requestID
         * @param handlerResponse
         */
        void requestJoinContract(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
                    HandlerResponse handlerResponse);

        /**
         *
         * @param deviceAddress64
         * @param parentAddress32
         * @param requestID
         * @param handlerResponse
         */
        void requestJoinContractGateway(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
                    HandlerResponse handlerResponse);

        /**
         * Physical Model interface.
         * Last step of join: perform confirmation of the device.
         * For GW:
         * - if BBR is joined send NetworkRoute operation to both GW and BBR.
         * For BBR:
         * - if GW is joined send NetworkRoute operation to both GW and BBR.
         *
         * @param deviceAddress32
         * @param requestID
         * @param handlerResponse
         */
        void requestConfirmJoin(const Address32 deviceAddress32, int requestID, HandlerResponse handlerResponse);

        /**
         * Physical Model interface.
         * Handles the security new session request. Generates the operations for setting keys on the two peer devices.
         *
         * @param requestID
         * @param keyFrom
         * @param keyTo
         * @param handlerResponse
         */
        void requestSecurityNewSession(const Address32 deviceAddress32, int requestID, const PhySessionKey& keyFrom,
                    const PhySessionKey& keyTo, HandlerResponse handlerResponse);

        /**
         * Physical Model interface.
         * Handles the creation of a new contract.
         *
         * @param contractReq
         * @param requestID
         * @param handlerResponse
         */
        void requestCreateContract(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse);

        PhyContract * createContractForFirmwareUpdate(const Address64& firmwareUpdateDeviceAddress64);

        void deleteContractForFirmwareUpdate(const Address64& firmwareUpdateDeviceAddress64);

        /**
         * Finds the contract between Manager and device on the specified TSAPs.
         * @return null pointer if the contract is not found
         */
        NE::Model::PhyContract * findManagerContractToDevice(Address32 deviceAddress32, Uint16 tsapSrc, Uint16 tsapDest);

        NE::Model::Device * getDevice(const Address32 address32) {
            return subnetsContainer.getDevice(address32);
        }

        /**
         * Returns true if there is a device with the specified address32.
         */
        bool existsDevice(Address32 address32) {
            return subnetsContainer.existsDevice(address32);
        }

        /**
         * Returns true if there is a device with the specified address64.
         */
        bool existsDevice(const Address64& address64) {
            return subnetsContainer.existsDevice(address64);
        }

        /**
         * Returns true if there is a confirmed device with the specified address.
         */
        bool existsConfirmedDevice(Address32 address32) {
            return subnetsContainer.existsConfirmedDevice(address32);
        }

        bool existsConfirmedDevice(const Address64& address64) {
            return subnetsContainer.existsConfirmedDevice(address64);
        }


        bool existsContract(Address32 clientAddress32, Uint16 contractID) {
            if( subnetsContainer.existsConfirmedDevice(clientAddress32)) {
                {   Device* dev = subnetsContainer.getDevice(clientAddress32);
                    if(dev) {
                        return dev->existsContract(contractID);
                    }
                }
            }

            return false;
        }
        /**
         * Creates address32 from subnet and address16.
         */
        Address32 createAddress32(Uint16 subnetId, Uint16 address16);

        /**
         * Extract subnetID from address32.
         * For manager and gateway the subnetID is 0.
         */
        Uint16 getSubnetId(Uint32 address32) {
            return subnetsContainer.getSubnetId(address32);
        }

        /**
         * Return the Manager address from the subnet of the device.
         */
        Address32 getManagerAddress32() {
            return subnetsContainer.getManagerAddress32();
        }

        /**
         * It performs a lookup in an internal mapping to find the Address32. If the device is not joined yet it will return 0.
         * @param address64
         * @return
         */
        Address32 getAddress32(const Address64& address64) {
            return subnetsContainer.getAddress32(address64);
        }

        /**
         * Returns the Device.address64 of device with address32.
         * return Address64() if device with given address does not exists.
         * @param address32
         * @return
         * @throws
         */
        Address64 getAddress64(Address32 address32) {
            return subnetsContainer.getAddress64(address32);
        }

        /**
         *
         * @param address128
         * @return 0 if there is no device with address128
         */
        Address32 getAddress32(const Address128& address128) {
            return subnetsContainer.getAddress32(address128);
        }

        Uint16 getAddress16(Address32 address32) {
            return Address::getAddress16(address32);
        }

        /**
         * Find a device status by its 32 address. If the device is not found an exception is thrown.
         * @throws DeviceNotFoundException if a device with this address is not found.
         */
        DeviceStatus::DeviceStatus getDeviceStatus(Address32 address32);

        /**
         * Find a device capabilities by its 32 address. If the device is not found an exception is thrown.
         */
        Capabilities& getDeviceCapabilities(Address32 address32) {
            return getDevice(address32)->capabilities;
        }

        /**
         * Find the DiagLevel setting for a specified device's neighbor.
         */
        Uint8 getDiagLevel(Address32 deviceAddress, Address32 neighborAddress);

        /**
         *
         * @param existingDevice
         * @param candidate - visible neighbor
         */
        void addCandidate(Device *existingDevice, const PhyCandidate& candidate);

        /**
         *
         * @param device - the device that reports diagnostics in relation with its neighbor
         * @param neighborDevice
         * @param rsl - signal strength
         * @param rslQual - signal quality
         * @param sent - transmissions to the neighbor, where an ACK was received in response
         * @param received - valid DPDUs received from neighbor, excluding ACKs, NACKs
         * @param failed - transmissions where no ACK or NACK was received in response
         * @param nack - count of NACKs
         */
        void addDiagnostics(Device *device, Device *neighborDevice, Int8 rsl, Uint8 rslQual, Uint16 sent, Uint16 received,
                    Uint16 failed, Uint16 nack);

        /**
         * Adds channel diagnostics for the device (percentage of time transmissions on each channel aborted due to CCA).
         *
         * @param device
         * @param chDiag
         */
        void addDiagnostics(Device *device, const PhyChannelDiag& chDiag);

        void confirmJoinedDevice(OperationsContainer& operationsList, Address32 joinedDeviceAddress);

        void periodicEvaluation(NE::Common::EvaluationSignal evaluationSignal, Uint32 currentTime);

        /**
         * Determines the devices for which activity hasn't been recorded for a time span greater than the specified period.
         * A command is generated for each of these devices in order to determine their status.
         *
         * @param period
         */
        void verifyInactiveDevices(Uint32 period);

        /**
         * Searches and deletes the expired keys on all devices.
         */
        void removeExpiredKeys();

        void removeSessionKeys(Device *src, Device *dst, int sourceTSAP, int destinationTSAP,
                    Operations::OperationsContainerPointer& operationsContainer);

        void contractRenewal(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse);

        void modifyContract(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse);

        void periodicTerminateExpiredContracts();

        void terminateContract( Address32 source, ContractId contractId);

        void checkExpiredContracts();

        Subnet::PTR getSubnet(Uint16 subnetId);

        Subnet::PTR getSubnet(Address32 address32);

        /**
         * Returns the subnet topologies.
         */
        SubnetsMap& getSubnetsList();

        void registerSendOperationCallback(NE::Model::Operations::IOperationsSender * sender) {
            operationsProcessor.registerSendOperationCallback(sender);
        }

        void registerAlertSender(NE::Model::Operations::IAlertSender * sender) {
            operationsProcessor.registerAlertSender(sender);
        }

        /**
         * Removes device with the given address and returns the generated list of operations.
         */
        void removeDeviceOnError(Address32 address32, Operations::OperationsContainerPointer container, int reason);

       void processChannelBlackListing(const Model::Tdma::ChannelDiagnostics& channelDiags, const Address32& source );


    private:

//        void updateInboundGraphForDeviceOnRemoveParent(Subnet::PTR &  subnet, const Device* backbone ,const Device* removingDevice,
//                    OperationsList & generatedOperations);

        void removeSubscriber(const Device* subscriber, Operations::OperationsContainerPointer & container);

        void removeLocalLoopRoute(const Device* removingDevice, Device* targetDevice, Uint16 outboundGraphID,
                    Operations::OperationsContainerPointer & container);

        void removeATT(const Address32 removingDeviceAddress32, Device* targetDevice, Operations::OperationsContainerPointer & container);

        void removeNetworkRoute(const Device* removingDevice, Device* targetDevice,
                    Operations::OperationsContainerPointer & container);
        void removeNetworkRoute(Uint16 removingSubnet, Device* targetDevice, Operations::OperationsContainerPointer & container);

        void removeLocalLoopRouteForContract(Device* publisher, Uint16 contractID,
                    Operations::OperationsContainerPointer & container);
        void removeContracts(const Device* removingDevice, Device* targetDevice,
                    Operations::OperationsContainerPointer & container);
        void removeNetworkContracts(const Device* removingDevice, Device* targetDevice,
                    Operations::OperationsContainerPointer & container);
        void removeNetworkContracts(Uint16 removingSubnet, Device* targetDevice,
                    Operations::OperationsContainerPointer & container);
        void removeContracts(Uint16 removingSubnet, Device* targetDevice, Operations::OperationsContainerPointer & container);
        void removeKeys(const Device* removingDevice, Device* targetDevice, Operations::OperationsContainerPointer & container);
        void removeKeys(Uint16 removingSubnet, Device* targetDevice, Operations::OperationsContainerPointer & container);
        void initialiseDefaultAttributes(NE::Model::Device * device, NE::Model::Device * parentDevice, Subnet::PTR& subnet);
        void configureNetworkRoute_GW_BBR(SubnetsContainer& subnetsContainer, OperationsContainerPointer& operationsContainer,
                    Device * backbone);
        void configureAddresTranslationTable_GW_BBR(SubnetsContainer& subnetsContainer, OperationsContainer& operationsContainer,
                    Device * backbone);

};

} // namespace Model
}

#endif /* MAIN_H_ */
