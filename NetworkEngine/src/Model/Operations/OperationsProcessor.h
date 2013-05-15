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

#ifndef OPERATIONSPROCESSOR_H_
#define OPERATIONSPROCESSOR_H_

#include <list>
#include <boost/noncopyable.hpp>
#include "Common/logging.h"
#include "Model/Operations/IEngineOperation.h"
#include "Model/Operations/OperationsContainer.h"
#include "Model/IDeletedDeviceListener.h"
#include "Model/IDeviceRemover.h"
#include "Model/Operations/IOperationsSender.h"
#include "Model/Operations/IAlertSender.h"
#include "Model/Device.h"

namespace NE {
namespace Model {

class SubnetsContainer;

namespace Operations {


namespace ProcessedOperationStatus {
enum ProcessedOperationStatusEnum {
    CAN_BE_SENT = 1, EXISTS_DEPENDENCIES = 2, DEVICE_DELETED = 3
};
}

namespace SentOperationStatus {
enum SentOperationStatusEnum {
    OPERATION_SENT = 1, //DEVICE_DELETED = 12,
    OPERATION_SENT_AND_CONFIRMED = 2, //NOT_SENT_DEPENDENCIES = 14
// sent to SM and confirmed right away
};
}


typedef std::list<NE::Model::Operations::OperationsContainerPointer> ListOfOperationsContainer;

/**
 * Responsible with sending the operations into the field.
 * There are several operation types : WRITE_ATTRIBUTE, READ_ATTRIBUTE, DELETE_ATTRIBUTE, UPDATE_GRAPH.
 * Update graph operation has to be split into several write operations (neighbors, graphs).
 * Operations can be added to OperationsProcessor either one by one using addOperation(), or several operations
 * at a time using addOperationsContainer() method. First method should be used for operations that are directed
 * to the SystemManager or operations whose confirmation result it's not expected. In case you have only one operation
 * to send but you need a confirmation from a field device, you have to add it to a container,
 * register a call back into the container so that it will be called later on the operation confirmation.
 * If the operation added with addOperation() can not be send into the field then there will be no more tries made by
 * the OperationsProcessor to send the operation and an exception is thrown. On the other hand the operation containers
 * are cached into the OperationsProcessor and next time when a confirm is received the all cached operations from
 * all the containers are checked again to see if they can be sent into the field.
 * When the last confirm for a container is received the handler from the container is called. If an error confirmation
 * is received for an operation the container' handler is called with error code then the operation owner device is
 * removed and container is removed from the list of cached containers.
 *
 * @author Radu Pop, Catalin Pop
 */
class OperationsProcessor: public boost::noncopyable, public NE::Model::IDeletedDeviceListener {

        LOG_DEF("N.M.OperationsProcessor")
        ;

    private:

        ListOfOperationsContainer listOfContainers;

        IDeviceRemover * deviceRemover;
        IOperationsSender * sender;

        IAlertSender * alertSender;


        NE::Model::SubnetsContainer& subnetsContainer;

    public:

        OperationsProcessor(NE::Model::SubnetsContainer& subnetsContainer);

        virtual ~OperationsProcessor();

        /**
         * Processes multiples operations. If some of the operations could not be processed because of the dependencies,
         * caches the containers and on the next confirm received all the cached operations will be processed again.
         */
        void addOperationsContainer(NE::Model::Operations::OperationsContainerPointer container);

        /**
         * Used to process one Manager operation. If it fails to process the operation then throws an exception.
         * UPDATE_GRAPH operations can not be processed with this method (throws an exception if you try to).
         */
        void addManagerOperation(NE::Model::Operations::IEngineOperationPointer operation);

        /**
         * Call back implementation for delete device event. All the containers that contain an operation for
         * the deleted device are removed from the list of containers. The container's handler is called with
         * error code prior to deleting it from the list.
         */
        void deviceDeletedCallback(Address32 deletedDeviceAddress, Uint16 deviceType);

        /**
         * Registers a call back that will be used to send the operations into the field when there
         * are no active dependencies for them.
         */
        void registerSendOperationCallback(IOperationsSender * sender_){
            sender = sender_;
        }

        /**
         * Sets the pointer to the alert sending object.
         */
        void registerAlertSender(NE::Model::Operations::IAlertSender * sender) {
            alertSender = sender;
        }

        /**
         * Registers a call back that will be used to delete a device when an error is received.
         */
        void registerRemoveDeviceOnErrorCallback(NE::Model::IDeviceRemover * deviceRemover_){
            deviceRemover = deviceRemover_;
        }

        /**
         * Called when a response from the field is received. Can be a success, timeout, error code,
         * time out etc. The operation's container must be deleted from the list of operations containers,
         * so that the remaining operations will not be sent into the field. Also, the device corresponding
         * to the owner of the operation will be mark as deleted.
         * If a confirm is received for an operation that has no associated container in the list of containers
         * then a message is logged and returns.
         */
        void confirm(NE::Model::Operations::IEngineOperationPointer operation,bool escapeSend = false);

        /**
         * For each operation in container.engineOperations { setPhysicalEntity(NULL) } && clear engineOperations.
         * For operation set PhysicalEntity as NULL.
         * Container.handleResponse if container.sentOperations && container.engineOperations is empty.
         * @param operation
         * @param container
         */

        void cleanEmptyContainers(Uint32 currentTime, Uint16 timeoutValue);

        /**
         * Check if the listOfContainers contains the operations for the given device address.
         * @param deviceAddress32
         * @return
         */
        bool existsOperationsForDevice(Address32 deviceAddress32);

        int getNumberOfContainers() {
            return listOfContainers.size();
        }

        void getNumberOfOperations(int& sent, int& unsent) {
            for (ListOfOperationsContainer::iterator it = listOfContainers.begin(); it != listOfContainers.end(); ++it){
                sent += (*it)->sentOperations.size();
                unsent += (*it)->unsentOperations.size();
            }
        }

        void sendAlert(AlertOperationPointer& alertOperation) {
            if (alertSender) {
                alertSender->sendAlert(alertOperation);
            } else {
                LOG_WARN("SM application alert generated but there's no alert sender registered");
            }
        }

    private:

        /**
         * Splits a high level operation into several simple operations.
         * (An ADD_GRAPH_EDGE operation is split into ADD_NEIGHBOR, ADD_GRAPH, etc).
         */
        void addDirectDependencies(NE::Model::Operations::IEngineOperation & operation );

        /**
         * Checks to see if the operation can be sent into the field.
         * Checks all its status and its dependencies.
         * Returns <code>true</code> if the operation can be sent.
         */
        ProcessedOperationStatus::ProcessedOperationStatusEnum canSendOperation(
                    NE::Model::Operations::IEngineOperationPointer operation,
                    OperationsContainer & container
                    );

        /**
         * Used to process one operation and send the operation into the field using the call back method.
         * It is a convenience method called by the addOperation and addOperations method.
         * If the throwEXceptionOnError flag is set to true then this method throw an error.
         * Returns true if the operation have to be deleted (have been sent into the field or the
         * owner of the operation is deleted).
         * @param operation
         */
        SentOperationStatus::SentOperationStatusEnum sendSingleOperation(
                    NE::Model::Operations::IEngineOperationPointer operation);

        void addOperationToWaitingList( NE::Model::Operations::IEngineOperation & operation );
        bool isFirstInWaitingList(NE::Model::Operations::IEngineOperation & operation );

        /**
         * Update the device metadata for the given device and given metadata type(operation).
         * @param device
         * @param operation
         */
        void updateMetadata(Device * device, NE::Model::Operations::IEngineOperationPointer& operation);

        /**
         * Called when an operation was confirmed with success and tries to send other operations that
         * are dependent on the last confirmed operation.
         */
        void sendOperations();
        bool sendAllContainerOperations( NE::Model::Operations::OperationsContainer & continer, OperationsList & confirmedOperations);
        /**
         * Create a new entity for the specified operation.
         */
        bool updatePhyEntity(IEngineOperationPointer operation, Device * device);

        /**
         * Commit the changes from this operation into the corresponding attribute from the physical model:
         * If it is a WRITE operation :
         *      - deletes previousValue
         * If it is a read operation :
         *      - deletes currentValue
         *      - copies operation.getPhysicalEntity() into the currentValue
         * If it is a DELETE operation:
         *      - deletes currentValue
         *      - deletes previousValue
         *      - deletes the attribute
         * If the owner's device or the attribute does not exist anymore returns.
         */
        bool commitChanges(IEngineOperationPointer operation, Device * device );

        /**
         * Returns a string representation of this OperationsProcessor.
         */
        friend std::ostream& operator<<(std::ostream&, const OperationsProcessor& operationsProcessor);

};

}
}
}

#endif /* OPERATIONSPROCESSOR_H_ */
