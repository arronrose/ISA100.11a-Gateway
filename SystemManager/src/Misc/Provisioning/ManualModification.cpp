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
 * ManualModification.cpp
 *
 *  Created on: Mar 26, 2009
 *      Author: Kedar Kapoor
 */

#include "ManualModification.h"
#include <signal.h>

using namespace Isa100::Misc::Provisioning;

ProvisionManualUpdate::ProvisionManualUpdate() {
    objectInitialized = false;
    configFileName = "";
    ListenSignal = SIGUSR2;
}

void ProvisionManualUpdate::ProcessSignal() {
   // Process only if initialized.
    if (!objectInitialized) return;




}
