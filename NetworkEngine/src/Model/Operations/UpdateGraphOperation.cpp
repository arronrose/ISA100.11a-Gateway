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

#include "UpdateGraphOperation.h"

namespace NE {

namespace Model {

namespace Operations {

UpdateGraphOperation::UpdateGraphOperation(IPhy* physicalEntity_, EntityIndex entityIndex_) {
    physicalEntity = physicalEntity_;
    entityIndex = entityIndex_;
    opType = EngineOperationType::UPDATE_GRAPH;
}

UpdateGraphOperation::~UpdateGraphOperation() {
}

bool UpdateGraphOperation::accept(IEngineOperationsVisitor& visitor) {
    // This operation will be split and will not go through visitor.
    return true;
}

std::ostream& UpdateGraphOperation::toString(std::ostream& stream) const {
    return stream << *this;
}

std::ostream& operator<<(std::ostream& stream, const UpdateGraphOperation& operation) {
    operation.toStringCommonOperationState(stream);
    stream << "}";

    return stream;
}

}
}
}
