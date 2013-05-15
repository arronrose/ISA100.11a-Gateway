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

#ifndef ENGINEOPERATIONS_H_
#define ENGINEOPERATIONS_H_

#include <map>
#include "Common/logging.h"
#include "Common/NEAddress.h"
#include "Model/Operations/IEngineOperation.h"
#include "Model/_signatureHandlerResponse.h"
#include "Model/Device.h"
#include "Model/Subnet.h"

namespace NE {
namespace Model {

class SubnetsContainer;

namespace Operations {

/**
 * @author Catalin Pop
 * @version 1.0
 */
class OperationsContainer {

        LOG_DEF("I.M.OperationsContainer");

    public:

        static int nextAvailableContainerId;

        int containerId;

        /**
         * The address64 that has generated the event.
         */
        Address64 requesterAddress64;

        /**
         * The assigned address32 of the device that has generated the event.
         */
        Address32 requesterAddress32;

        /**
         * The time when the operation should be applied.
         */
        Uint32 taiCutover;

        /**
         * Error code.
         */
        OperationErrorCode::OperationErrorCodeEnum errorCode;

        /**
         * Flag.
         */
        bool proxyAddress;

        /**
         * The type of engine event this list of operations.
         */
        NetworkEngineEventType::NetworkEngineEventTypeEnum eventEngineType;

        /**
         * This handler should be called when all operations form this container are confirmed (the list is empty).
         * Handler should be called with SUCCESS if all operations confirmed with success.
         * Handler should be called with FAIL if at least one operation failed to be sent or a response with error or timeout has been received..
         */
        NE::Model::HandlerResponseList handlerResponses;

        /**
         * true - if the handlerResponse has been called.
         */
        ResponseStatus::ResponseStatusEnum rspStatus;

        /**
         * Contains the request ID of the request that created this list.
         */
        Uint16 requestId;

        /**
         * The list of operations.
         */
        OperationsList unsentOperations;

        /**
         * The list of operations sent into the field.
         * The operations are moved from the <code>operations</code> as they are sent into the field.
         */
        OperationsList sentOperations;


    public:

        /**
         * The reason why operations were generated.
         */
        std::string reasonOfOperations;

    public:

        OperationsContainer(const char *reason);

        OperationsContainer(Address32 requesterAddress_, Uint16 requestId_, HandlerResponse handlerResponse_, const char *reason);

        OperationsContainer(Address32 requesterAddress_, Uint16 requestId_, HandlerResponseList& handlerResponses_, const char *reason);

        virtual ~OperationsContainer();

        void addHandlerResponse(HandlerResponse handlerResponse_){handlerResponses.push_back(handlerResponse_);}

        void expireContainer(NE::Model::SubnetsContainer * pSubnetsContainers, Address32Set& devicesToBeRemoved);

        void setAsFail( NE::Model::SubnetsContainer * pSubnetsContainers);

        const Address64& getRequesterAddress64() const {
            return requesterAddress64;
        }
        Address32 getRequesterAddress32() const {
            return requesterAddress32;
        }
        bool isProxyAddress() const {
            return proxyAddress;
        }

        void setRequesterAddress64(const Address64& requesterAddress64Param) {
            requesterAddress64 = requesterAddress64Param;
        }

        void setRequesterAddress32(Address32 requesterAddress32Param) {
            requesterAddress32 = requesterAddress32Param;
        }

        /**
         * Returns the absolute TAI cutover that current list of operations must be applied by devices.
         */
        Uint32 getTAICutover() const {
            return taiCutover;
        }

        /**
         * Apply the taiOffset to the current TAI and store it for use by the component that
         * send the operations to be applied on each operation from this list..
         */
        void setTAICutover(Uint32 currentTAI, Uint32 taiOffsetParam) {
            taiCutover = currentTAI + taiOffsetParam;
        }

        bool isExpiredContainer(Uint32 currentTime, Uint16 timeoutValue){
            return  (currentTime - taiCutover) > timeoutValue;//to used value from settings
        }

        void addOperation(const NE::Model::Operations::IEngineOperationPointer & operation, NE::Model::Device * ownerDevice);

        void addToSentOperations(const NE::Model::Operations::IEngineOperationPointer& operation);

        OperationsList& getUnsentOperations() {
            return unsentOperations;
        }
        OperationsList& getSentOperations() {
            return sentOperations;
        }

        OperationErrorCode::OperationErrorCodeEnum getErrorCode() const {
            return errorCode;
        }

        void setErrorCode(OperationErrorCode::OperationErrorCodeEnum errorCodeParam) {
            errorCode = errorCodeParam;
        }

        int getContainerId() const { return containerId; }


        /**
         * In the sentOperations list all operations for given owner are invalidated and removed.
         * Any iterator on unsent list will be invalid after this call.
         * @param owner
         */
        bool removeAllSentOperationsForOwner(Subnet::PTR& subnet, Address32 owner, NE::Model::SubnetsContainer * subnetsContainers);

        /**
         * Remove all operations sent for devices in subnet.
         * @param subnet
         * @return
         */
        bool removeAllSentOperationsForSubnet(Subnet::PTR& subnet, NE::Model::SubnetsContainer * subnetsContainers);

        void setNetworkEngineEventType(NetworkEngineEventType::NetworkEngineEventTypeEnum eventEngineType_) {
            this->eventEngineType = eventEngineType_;
        }

        NetworkEngineEventType::NetworkEngineEventTypeEnum getNetworkEngineEventType() const {
            return eventEngineType;
        }

        std::string getReason(){
            return reasonOfOperations;
        }

        void handleEndContainer(bool skipLogging = false);

        void updateTimer(Uint32 timeValue){
            taiCutover = timeValue;
        }

        bool isContainerEmpty(){return (sentOperations.empty() && unsentOperations.empty());}

        static bool removeOperationInWaitingList(NE::Model::Operations::IEngineOperation & operation, NE::Model::SubnetsContainer * pSubnetsContainers );

        void toShortString(std::ostringstream &stream);

        friend std::ostream& operator<<(std::ostream&, OperationsContainer& container);
};
typedef boost::shared_ptr<OperationsContainer> OperationsContainerPointer;
std::string toString(const OperationsList& operationsList);


}
}
}

#endif /* ENGINEOPERATIONS_H_ */
