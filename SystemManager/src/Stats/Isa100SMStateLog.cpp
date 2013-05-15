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
 * Isa100SMStateLog.cpp
 *
 *  Created on: Apr 22, 2009
 *      Author: radu
 */

#include "Isa100SMStateLog.h"
#include "Version.h"
#include "ASL/StackWrapper.h"

namespace Isa100SMState {

struct Isa100StateContracts {
        LOG_DEF("Isa100SMState.SmStackContracts");
        static void logSmContracts(std::string msg) {
            LOG_DEBUG("SYSTEM MANAGER VERSION: " << SYSTEM_MANAGER_VERSION);
            LOG_DEBUG("The reason : " << msg);
            std::ostringstream stream;
            stream << std::endl;
            Stack_PrintContracts(stream);
            LOG_DEBUG(stream.str());
        }
};

struct Isa100StateRoutes {
        LOG_DEF("Isa100SMState.SmStackRoutes");
        static void logSmRoutes(std::string msg) {
            LOG_DEBUG("SYSTEM MANAGER VERSION: " << SYSTEM_MANAGER_VERSION);
            LOG_DEBUG("The reason : " << msg);
            std::ostringstream stream;
            stream << std::endl;
            Stack_PrintRoutes(stream);
            LOG_DEBUG(stream.str());
        }
};

struct Isa100StateKeys {
        LOG_DEF("Isa100SMState.SmStackKeys");
        static void logSmKeys(std::string msg) {
            LOG_DEBUG("SYSTEM MANAGER VERSION: " << SYSTEM_MANAGER_VERSION);
            LOG_DEBUG("The reason : " << msg);
            std::ostringstream stream;
            stream << std::endl;
            Stack_PrintKeys(stream);
            LOG_DEBUG(stream.str());
        }
};

struct Isa100StateAlerts {
        LOG_DEF("Isa100SMState.Alerts");
        static void logAlert(std::ostringstream& stream) {
            //LOG_DEBUG("SYSTEM MANAGER VERSION: " << SYSTEM_MANAGER_VERSION);
            LOG_DEBUG(stream.str());
        }
};

Isa100SMStateLog::Isa100SMStateLog() {
}

Isa100SMStateLog::~Isa100SMStateLog() {
}

void Isa100SMStateLog::logSmStackContracts(std::string msg) {
    Isa100StateContracts::logSmContracts(msg);
}

void Isa100SMStateLog::logSmStackRoutes(std::string msg) {
    Isa100StateRoutes::logSmRoutes(msg);
}

void Isa100SMStateLog::logSmStackKeys(std::string msg) {
    Isa100StateKeys::logSmKeys(msg);
}

void Isa100SMStateLog::logAlerts(std::ostringstream& stream) {
    Isa100StateAlerts::logAlert(stream);
}

}
