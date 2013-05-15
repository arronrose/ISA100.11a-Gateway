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
 * ManualModification.h
 *
 *  Created on: Mar 26, 2009
 *      Author: Kedar Kapoor
 */

#ifndef MANUALMODIFICATION_H_
#define MANUALMODIFICATION_H_
/**
 * This class implements a quick "re-read" of the SysMgr configuration file that contains detailed
 *  provision information, so that a device may be added to the "allowed" devices for the network.
 *  This module registers with the system to listen to a signal, which then calls functions to refresh
 *  the configuration information.
 *  This signal is given AFTER a user manually modifies the configuration file.
 */
#include "Common/NETypes.h"
#include "Common/logging.h"

namespace Isa100 {
namespace Misc {
namespace Provisioning {

class ProvisionManualUpdate {
        LOG_DEF("I.M.F.ProvisionManualUpdate")

    private:
        /**
         * Set to <code>true</code> if the <code>init()</code> method has been invoked on the current instance.
         */
        bool objectInitialized;

        /**
         * The name of the file that contains the configuration used to make the update.
         */
        std::string configFileName;

        //TODO Change this to TCP socket listener, rather than a signal listener.
        /**
         * The signal that this module will listen to
         */
        Uint16 ListenSignal;

    private:
        /**
         * Verifies that the <code>init()</code> method has been invoked on the current
         * instance. If it was not invoked then throws an Exception.
         */
        void checkInitStatus();

    public:
        /**
         * Creates a new instance of this class.
         * In order to be use the class, the <code>init()</code> must be first invoked.
         */
        ProvisionManualUpdate();


        /**
         * Function that processes the signal received by re-reading the config.
         */
        void ProcessSignal();

        /**
         * Read the config file from the disk and process name/values for devices.
         */
        bool init(std::string provisionManualUpdate);

        /**
         * Get config file finish status.
         */
        Uint8 getStatus();

        /**
         * Returns a string representation of the status of the current object.
         * @return a string representation of the status of the current object
         */
        std::string toString();

};


} //namespace Provisioning
} //namespace Misc
} //namespace Isa100

#endif /* MANUALMODIFICATION_H_ */
