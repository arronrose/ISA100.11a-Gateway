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

#ifndef IENGINEOPERATIONSVISITOR_H_
#define IENGINEOPERATIONSVISITOR_H_

#include "Model/Operations/WriteAttributeOperation.h"
#include "Model/Operations/ReadAttributeOperation.h"
#include "Model/Operations/DeleteAttributeOperation.h"

namespace NE {
namespace Model {
namespace Operations {


class IEngineOperationsVisitor {

    public:
        virtual ~IEngineOperationsVisitor() {
        }

        virtual bool visitWriteAttributeOperation(NE::Model::Operations::WriteAttributeOperation& operation) = 0;

        virtual bool visitReadAttributeOperation(NE::Model::Operations::ReadAttributeOperation& operation) = 0;

        virtual bool visitDeleteAttributeOperation(NE::Model::Operations::DeleteAttributeOperation& operation) = 0;
};

}
}
}
#endif /* IENGINEOPERATIONSVISITOR_H_ */
