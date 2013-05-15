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
 * IEngine.h
 *
 *  Created on: Mar 3, 2009
 *      Author: mulderul
 */

#ifndef IENGINE_H_
#define IENGINE_H_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "Common/NEAddress.h"
#include "Common/NEException.h"
#include "Common/SettingsLogic.h"
#include "Model/IEngineExceptions.h"
#include "Model/Operations/OperationsContainer.h"
#include "Model/Capabilities.h"
#include "Model/Subnet.h"
#include "Model/Device.h"
#include "Model/ContractRequest.h"
#include "Model/Reports/ReportsStructures.h"
#include "Model/Reports/ReportsEngine.h"
#include "Model/Operations/OperationsProcessor.h"


#include "Model/_signatureHandlerResponse.h"
#include "Common/EvaluationSignal.h"
#include "Model/Operations/IOperationsSender.h"
#include "Model/Operations/IAlertSender.h"

namespace NE {

namespace Model {

class IEngine {
    public:

        virtual ~IEngine() {};

        /**
         * Physical Model interface ********************************************************************************************
         */
        virtual void requestJoinSecurity(const Address64 & deviceAddress64, const Address32 parentAddress32, int requestID,
                    DeviceType::DeviceTypeEnum provisionedType, Uint16 provisionedSubnetId, const PhySessionKey& joinKey,
                    HandlerResponse handlerResponse) = 0;

        virtual void requestJoinSystemManager(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
                    const Capabilities& capabilities, std::string& softwareRevision, const PhyDeviceCapability& deviceCapability,
                    HandlerResponse handlerResponse) = 0;

        virtual void requestJoinContract(const Address64& deviceAddress64, const Address32 parentAddress32, int requestID,
                    HandlerResponse handlerResponse) = 0;

        virtual void requestConfirmJoin(const Address32 deviceAddress32, int requestID, HandlerResponse handlerResponse) = 0;

        virtual void requestSecurityNewSession(const Address32 deviceAddress32, int requestID, const PhySessionKey& keyFrom,
                    const PhySessionKey& keyTo, HandlerResponse handlerResponse) = 0;

        virtual void requestCreateContract(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse) = 0;
        /************************************************************************************************************************/

        virtual PhyContract * createContractForFirmwareUpdate(const Address64& firmwareUpdateDeviceAddress64) = 0;

        virtual void deleteContractForFirmwareUpdate(const Address64& firmwareUpdateDeviceAddress64) = 0;

        virtual void addManagerNeighbor(const Address64& neighborAddress64, const Address128& neighborAddress128) = 0;

        /**
         *
         * @return
         */
        virtual NE::Common::SettingsLogic& getSettingsLogic() = 0;

        virtual void updateAdvPeriod() = 0;

        /**
         *
         * @param settingsLogic
         */
        virtual void setSettingsLogic(NE::Common::SettingsLogic* settingsLogic) = 0;

        /**
         * Find the management contract between the manager of device with given address and the device, between the given TSAP ids.
         * @param deviceAddress32
         * @param tsapSrc
         * @param tsapDest
         * @return
         */
        virtual NE::Model::PhyContract
                    * findManagerContractToDevice(Address32 deviceAddress32, Uint16 tsapSrc, Uint16 tsapDest) = 0;

        /**
         * Returns true if there is a device with the specified address32.
         */
        virtual bool existsDevice(Address32 address) = 0;

        /**
         * Return the device with the given address.
         * @param address
         * @return
         */
        virtual NE::Model::Device * getDevice(const Address32 address) = 0;

        /**
         * Returns true if there is a device with the specified address64.
         */
        virtual bool existsDevice(const Address64& address64) = 0;

        /**
         * Returns true if there is a confirmed device with the specified address.
         */
        virtual bool existsConfirmedDevice(Address32 address) = 0;

        virtual bool existsConfirmedDevice(const Address64& address64) = 0;

        virtual bool existsContract(Address32 clientAddress32, Uint16 contractID) = 0;

        virtual Address32 createAddress32(Uint16 subnetId, Uint16 managerAddress16) = 0;

        virtual Uint16 getSubnetId(Uint32 address32) = 0;

        /**
         * Return the Manager address from the subnet of the device.
         */
        virtual Address32 getManagerAddress32() = 0;

        virtual Address32 getAddress32(const Address64& address64) = 0;

        virtual Address32 getAddress32(const Address128& address128) = 0;

        virtual Address64 getAddress64(Address32 address32) = 0;

        virtual Uint16 getAddress16(Address32 address32) = 0;

        /**
         * Find the DiagLevel setting for a specified device's neighbor.
         */
        virtual Uint8 getDiagLevel(Address32 deviceAddress, Address32 neighborAddress) = 0;

        /**
         * Find a device status by its 32 address. If the device is not found an exception is thrown.
         * @param Address32
         * @return  DeviceStatus::DeviceStatus
         * @throws DeviceNotFoundException if a device with this address is not found.
         */
        virtual NE::Model::DeviceStatus::DeviceStatus getDeviceStatus(Address32 address32) = 0;

        /**
         * Find a device type by its 32 address. If the device is not found an exception is thrown.
         * @param address32
         * @return
         */
        virtual NE::Model::Capabilities& getDeviceCapabilities(Address32 address32) = 0;

        virtual void addCandidate(Device *existingDevice, const PhyCandidate& candidate) = 0;

        virtual void addDiagnostics(Device *device, Device *neighborDevice, Int8 rsl, Uint8 rslQual, Uint16 sent,
                    Uint16 received, Uint16 failed, Uint16 nack) = 0;

        virtual void addDiagnostics(Device *device, const PhyChannelDiag& chDiag) = 0;

        /**
         * Allow engine to perform different period evaluations. The evaluationSignal flags will signal the right time for each evaluation.
         * @param evaluationSignal
         */
        virtual void periodicEvaluation(NE::Common::EvaluationSignal evaluationSignal, Uint32 currentTime) = 0;

        virtual void verifyInactiveDevices(Uint32 period) = 0;

        virtual void removeExpiredKeys() = 0;

        /**
         * Renew an existing contract.
         * @param contractRequest
         */
        virtual void contractRenewal(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse) = 0;

        /**
         * Modify an existing contract.
         * @param contractRequest
         */
        virtual void modifyContract(const ContractRequest& contractReq, int requestID, HandlerResponse handlerResponse) = 0;


        virtual void periodicTerminateExpiredContracts() = 0;

        /**
         * Used for Contract termination, deactivation and reactivation request.
         * @param source
         * @param contractId
         */
        virtual void terminateContract(Address32 source,
                    ContractId contractId) = 0;

        /**
         * Should return the start time of this engine.
         */
        virtual Uint32 getStartTime() = 0;


        /**
         * Returns the subnet topologies.
         */
        virtual NE::Model::SubnetsMap& getSubnetsList() = 0;

        virtual SubnetsContainer& getSubnetsContainer() = 0;

        virtual void removeDeviceOnError(Address32 address32, Operations::OperationsContainerPointer container, int reason) = 0;

        /**
         * Returns the operations processor.
         */
        virtual NE::Model::Operations::OperationsProcessor& getOperationsProcessor() = 0;

        virtual NE::Model::Reports::ReportsEngine& getReportsEngine() = 0;

        virtual void registerSendOperationCallback(NE::Model::Operations::IOperationsSender * sender) = 0;

        /**
         * Sets the pointer to the alert sending object.
         */
        virtual void registerAlertSender(NE::Model::Operations::IAlertSender * alertSender) = 0;


        virtual void processChannelBlackListing(const Model::Tdma::ChannelDiagnostics& channelDiags, const Address32& source) = 0;



};

}// namespace Model

} // namespace Isa100

#endif /* IENGINE_H_ */
