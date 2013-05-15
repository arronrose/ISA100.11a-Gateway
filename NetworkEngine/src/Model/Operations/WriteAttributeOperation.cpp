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

#include "WriteAttributeOperation.h"
#include "Model/Operations/IEngineOperationsVisitor.h"

namespace NE {

namespace Model {

namespace Operations {

WriteAttributeOperation::WriteAttributeOperation(IPhy* physicalEntity_, EntityIndex entityIndex_) {
    assert(physicalEntity_);
    physicalEntity = physicalEntity_;
    entityIndex = entityIndex_;
    opType = EngineOperationType::WRITE_ATTRIBUTE;

}

WriteAttributeOperation::~WriteAttributeOperation() {
}

EngineOperationType::EngineOperationTypeEnum WriteAttributeOperation::getType() const {
    return EngineOperationType::WRITE_ATTRIBUTE;
}

bool WriteAttributeOperation::accept(IEngineOperationsVisitor& visitor) {
    return visitor.visitWriteAttributeOperation(*this);
}

std::ostream& WriteAttributeOperation::toString(std::ostream& stream) const {
    return stream << *this;
}

std::ostream& operator<<(std::ostream& stream, const Operations::WriteAttributeOperation& operation) {
    operation.toStringCommonOperationState(stream);
    stream << "}";

    return stream;
}

}
}
}
