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
 * @author beniamin.tecar, radu.pop, sorin.bidian
 */
#ifndef OPERATIONPDUSVISITOR_H_
#define OPERATIONPDUSVISITOR_H_

#include "Model/Contracts/ContractResponse.h"
#include "Model/Contracts/ContractData.h"
#include "Model/Operations/IEngineOperationsVisitor.h"
#include "ASL/PDU/ClientServerPDU.h"
#include "Security/SecurityManager.h"
#include "Common/SubnetSettings.h"

namespace Isa100 {
namespace Model {

typedef std::vector<Isa100::ASL::PDU::ClientServerPDUPointer> ClientServerPDUList;

/**
 * @param address128 = 128-bit address of the device having 32-bit address = contract.source32
 */
ContractResponsePointer createContractResponse(NE::Model::PhyContract& contract);
ContractDataPointer createContractData(NE::Model::PhyContract& contract);


class OperationPDUsVisitor: public NE::Model::Operations::IEngineOperationsVisitor {
        LOG_DEF("I.M.OperationPDUsVisitor");
    private:

        ClientServerPDUList pduList;

        Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID;
        Isa100::Security::SecurityManager& securityManager;

        // <EntityType::EntityTypeEnum, ObjectIDEnum>
        std::map<Uint16, Isa100::AL::ObjectID::ObjectIDEnum> entitiesObjects;

        // <EntityType::EntityTypeEnum, MethodsID>
        std::map<Uint16, Uint16> entitiesMethodsAdd;

        // <EntityType::EntityTypeEnum, MethodsID>
        std::map<Uint16, Uint16> entitiesMethodsDelete;

        // <EntityType::EntityTypeEnum, AttributeID>
        std::map<Uint16, Uint16> entitiesAttributes;

    public:
        OperationPDUsVisitor(Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID, Isa100::Security::SecurityManager& securityManager);

        virtual ~OperationPDUsVisitor();

        ClientServerPDUList& getPDUs() {
            return pduList;
        }


        bool visitWriteAttributeOperation(NE::Model::Operations::WriteAttributeOperation& operation);

        bool visitReadAttributeOperation(NE::Model::Operations::ReadAttributeOperation& operation);

        bool visitDeleteAttributeOperation(NE::Model::Operations::DeleteAttributeOperation& operation);

    private:

        Isa100::AL::ObjectID::ObjectIDEnum getObject(EntityType::EntityTypeEnum entityType);
        Isa100::AL::ObjectID::ObjectIDEnum getObject(NE::Model::Operations::IEngineOperation& operation);

        Uint16 getMethod(NE::Model::Operations::IEngineOperation& operation);

        Uint16 getAttribute(EntityType::EntityTypeEnum entityType);
        Uint16 getAttribute(NE::Model::Operations::IEngineOperation& operation);

        void marshallWrite(NE::Model::Operations::WriteAttributeOperation& operation,
                    NE::Misc::Marshall::NetworkOrderStream& stream);
        void marshallDelete(NE::Model::Operations::DeleteAttributeOperation& operation,
                    NE::Misc::Marshall::NetworkOrderStream& stream);

        /**
         * Generates the commands needed to configure the alert model..
         */
        bool visitAlertOperation(NE::Model::Operations::WriteAttributeOperation& operation);

        /**
         * Generates write commands for attributes 2, 5, 8 and 11 of ARMO.
         * These commands configure the timeout waiting for acknowledgment of an alert that was sent to the alert master.
         * Called by visitAlertOperation method.
         */
        void generateConfirmationTimeoutCommands(SubnetSettings& subnetSettings);
};

}
}

#endif /* OPERATIONPDUSVISITOR_H_ */
