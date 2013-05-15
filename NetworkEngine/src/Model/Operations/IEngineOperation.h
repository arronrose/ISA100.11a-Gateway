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
 * ISuperframeOperation.h
 *
 *  Created on: May 15, 2008
 *      Author: catalin.pop
 */

#ifndef IENGINEOPERATION_H_
#define IENGINEOPERATION_H_

#include "Common/NETypes.h"
#include "Common/NEAddress.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <list>
#include "Model/model.h"

#define OPERATION_NAME_WIDTH 2

namespace NE {
namespace Model {
namespace Operations {

namespace EngineOperationType {
enum EngineOperationTypeEnum {
    NONE,//this status may be used by Isa100 operations that do not have attached an EngineOperation
    WRITE_ATTRIBUTE,
    READ_ATTRIBUTE,
    DELETE_ATTRIBUTE,
    UPDATE_GRAPH
};

    inline
    std::string getSign(EngineOperationType::EngineOperationTypeEnum opType) {

        std::string sign = "";

        switch(opType) {
            case NONE :
                sign = "";
                break;
            case WRITE_ATTRIBUTE :
                sign = "+";
                break;
            case READ_ATTRIBUTE :
                sign = "?";
                break;
            case DELETE_ATTRIBUTE :
                sign = "-";
                break;
            case UPDATE_GRAPH :
                sign = "ug";
                break;
            default :
                sign = "?!";
        }
        return sign;
    }

} // namespace EngineOperationType

namespace OperationState {
enum OperationStateEnum {
    GENERATED = 0, SENT = 1, SENT_IGNORED = 2, CONFIRMED = 3, IGNORED = 4
};
inline std::string stateToString(NE::Model::Operations::OperationState::OperationStateEnum state) {
    switch (state) {
        case NE::Model::Operations::OperationState::GENERATED:
            return "G";
        case NE::Model::Operations::OperationState::SENT:
            return "S";
        case NE::Model::Operations::OperationState::SENT_IGNORED:
            return "SI";
        case NE::Model::Operations::OperationState::CONFIRMED:
            return "C";
        case NE::Model::Operations::OperationState::IGNORED:
            return "I";
        default:
            return "?";
    }
}
}

namespace OperationErrorCode {
enum OperationErrorCodeEnum {
    SUCCESS = 0, TIMEOUT = 1, ERROR = 2
};
}

class IEngineOperationsVisitor;
class VisitorOperationToString;
class IEngineOperation;

typedef std::list<EntityIndex> EntityDependency;

typedef boost::shared_ptr<IEngineOperation> IEngineOperationPointer;
typedef std::list<IEngineOperationPointer> OperationDependency;
typedef std::list<NE::Model::Operations::IEngineOperationPointer> OperationsList;

/**
 * @version 1
 * @author Catalin Pop
 */
class IEngineOperation {
    LOG_DEF("N.M.O.IEngineOperation");
    static NE::Model::OperationID lastOperationID;
    protected:

        /**
         * The operation set index.
         */
        int containerId;

        /**
         * Contains the information specific for the physical entity. The type of attribute is contained in entityIndex.
         */
        IPhy* physicalEntity;

        /**
         * The generated operation ID.
         * Uint64: [Address32 deviceAddress(owner)] + [EntityType::EntityTypeEnum entityType] + [Uint16 index].
         */
        EntityIndex entityIndex;

        /**
         * The TAICutOver for the operation.
         */
        Uint32 taiCutOver;

        /**
         * The operation status.
         */
        OperationState::OperationStateEnum state;

        /**
         * The operations are ordered by the dependence, an operation can be sent
         * only if all operations with higher dependence has been applied(confirmed).
         */
        OperationDependency operationDependency;

        /**
         * The entities this operation depends to. All these entities must be be in pending status.
         */
        EntityDependency entityDependency;

        /**
         * The time when the operation has been sent on the network
         */
        time_t sentTimestamp;

        /**
         * The time when the operation has been applied(confirmed)
         */
        time_t confirmTimestamp;

        /**
         * The confirmation error code.
         */
        OperationErrorCode::OperationErrorCodeEnum errorCode;

        EngineOperationType::EngineOperationTypeEnum opType;

        NE::Model::OperationID opID;

    public:
        IEngineOperation();
        virtual ~IEngineOperation();

        /**
         * Returns the structure that contains all the information of attribute that must be written.
         */
        void* getPhysicalEntity() const {
            return this->physicalEntity;
        }

        void setPhysicalEntity(IPhy* physicalEntity_) {
            this->physicalEntity = physicalEntity_;
        }

        void destroyPhysicalEntity() {
            delete physicalEntity;
            physicalEntity = NULL;
        }
        Uint64 getEntityIndex() const { return entityIndex; }
        OperationID getOpID(){ return opID; }

        Address32 getOwner() const  { return getDeviceAddress(entityIndex); }
        void setOwner(Address32 owner) { setDeviceAddress(entityIndex, owner); }


        Uint32 getTaiCutOver() const {return taiCutOver; }
        void setTaiCutOver(Uint32 taiCutOverParam) { taiCutOver = taiCutOverParam; }


        OperationState::OperationStateEnum getState() const { return state; }
        void setState(OperationState::OperationStateEnum operationState) {state = operationState; }

        EntityDependency & getEntityDependency() { return entityDependency; }
        void setEntityDependency(EntityDependency & entityDependencyParam) { entityDependency = entityDependencyParam; }
        void addEntityDependency(EntityIndex entityIndex) { entityDependency.push_back(entityIndex); }
        void addEntityDependencies(EntityDependency & entityDependencyParam) { entityDependency.insert(entityDependency.end(), entityDependencyParam.begin(), entityDependencyParam.end()); }


        OperationDependency & getOperationDependency() {return operationDependency;}
        void setOperationDependency(OperationDependency & operationDependencyParam) { operationDependency = operationDependencyParam; }
        void addOperationDependency(IEngineOperationPointer operation) { operationDependency.push_back(operation); }
        void addOperationDependencies(OperationDependency & operationDependencyParam) { operationDependency.insert(operationDependency.end(), operationDependencyParam.begin(), operationDependencyParam.end()); }


        time_t getSentTimestamp() const { return sentTimestamp; }
        void setSentTimestamp(time_t sentTimestampParam) { sentTimestamp = sentTimestampParam; }


        time_t getConfirmTimestamp() const { return confirmTimestamp; }
        void setConfirmTimestamp(time_t confirmTimestampParam) { confirmTimestamp = confirmTimestampParam; }

        OperationErrorCode::OperationErrorCodeEnum getErrorCode() const { return errorCode; }
        void setErrorCode(OperationErrorCode::OperationErrorCodeEnum errorCodeParam) { errorCode = errorCodeParam; }

        virtual std::string getName() const;

        int getContainerId() const      { return containerId; }
        void setContainerId(int index) { containerId = index; }

        bool operator==(const IEngineOperation& operation) const;


        EngineOperationType::EngineOperationTypeEnum getType() const { return opType; }
        virtual std::ostream& toString(std::ostream& stream) const= 0;
        virtual bool accept(IEngineOperationsVisitor& visitor) = 0;

        void toStringShortState(std::ostream& stream) const;

    protected:

        void toStringCommonOperationState(std::ostream& stream) const;

        void physicalToString(std::ostream& stream, EntityType::EntityTypeEnum entityType, void * physical) const;

};

struct IEngineOperationShortPrinter{
    IEngineOperation& operation;
    IEngineOperationShortPrinter(IEngineOperation& operation_): operation(operation_){}

};

std::ostream& operator<<(std::ostream&, const IEngineOperation& operation);
std::ostream& operator<<(std::ostream&, const IEngineOperationShortPrinter& shortPrinter);

} // namespace Model
}
}

#endif /* IENGINEOPERATION_H_ */
