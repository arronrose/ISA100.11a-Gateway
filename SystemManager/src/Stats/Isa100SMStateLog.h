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
 * Isa100SMStateLog.h
 *
 *  Created on: Apr 22, 2009
 *      Author: radu, sorin.bidian
 */

#ifndef ISA100SMSTATELOG_H_
#define ISA100SMSTATELOG_H_

#include "Common/logging.h"
#include <string>

namespace Isa100SMState {

/**
 * Utility logging class.
 * @author Radu Pop, sorin.bidian
 */
class Isa100SMStateLog {

    public:

        Isa100SMStateLog();

        virtual ~Isa100SMStateLog();

        static void logSmStackContracts(std::string msg);

        static void logSmStackRoutes(std::string msg);

        static void logSmStackKeys(std::string msg);

        static void logObjectInstances(std::string reason, std::string msg);

        static void logAlerts(std::ostringstream& stream);
};

}

#endif /* ISA100SMSTATELOG_H_ */
