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

#ifndef DMO_CONSTANTS_H_
#define DMO_CONSTANTS_H_
namespace Isa100 {

namespace AL {
namespace DMAP {
namespace DMO_CONST {

/** 
 * Object attributes ids. 
 */
const int ATTR_ID_EUI_64_ADDRESS = 0;
const int ATTR_ID_16_BIT_DLL_ADDRESS = 1;
const int ATTR_ID_DLL_SUBNET_ID = 2;
const int ATTR_ID_32_BIT_LOGICAL_ADDRESS = 3;
const int ATTR_ID_VENDOR_ID = 4;
const int ATTR_ID_DEVICE_TYPE = 5;
const int ATTR_ID_MODEL_ID = 6;
const int ATTR_ID_TAG_NAME = 7;
const int ATTR_ID_SERIAL_NUMBER = 8;
const int ATTR_ID_TBD = 9;
const int ATTR_ID_POWER_SUPPLY_STATUS = 10;
const int ATTR_ID_DMAP_STATE = 11;
const int ATTR_ID_DMAP_COMMAND = 12;
const int ATTR_ID_STATIC_REVISION_LEVEL = 13;
const int ATTR_ID_RESTART_RESET_DEVICE = 14;
const int ATTR_ID_CONNECT_COMMAND = 15;
const int ATTR_ID_CONNECT_STATUS = 16;
const int ATTR_ID_RESTART_COUNT = 17;
const int ATTR_ID_MEMORY_USAGE_INFORMATION = 18;
const int ATTR_ID_TAI_TIME = 19;
const int ATTR_ID_DLL_SUBNET_CLOCK_MASTER_ROLE = 20;
const int ATTR_ID_DLL_SUBNET_CLOCK_REPEATER_ROLE = 21;
const int ATTR_ID_DLL_SUBNET_CLOCK_MASTER_IDS = 22;
const int ATTR_ID_DLL_SUBNET_CLOCK_PARENT_IDS = 23;
const int ATTR_ID_CLOCK_CORRECTION_THRESHOLD = 24;
const int ATTR_ID_CLOCK_UPDATE_FREQUENCY = 25;
const int ATTR_ID_DEVICE_CLOCK_PRECISION = 26;
const int ATTR_ID_CURRENT_UTC_ADJUSTMENT = 27;
const int ATTR_ID_CLOCK_ACCURACY_THRESHOLD = 28;
const int ATTR_ID_DEVICE_CLOCK_ACCURACY = 29;
const int ATTR_ID_ISA100_11A_COMMUNICATIONS_SOFTWARE_MAJOR_VERSION = 30;
const int ATTR_ID_ISA100_11A_COMMUNICATIONS_SOFTWARE_MINOR_VERSION = 31;
const int ATTR_ID_SOFTWARE_REVISION_NUMBER = 32;

}
}
}
}
#endif /*DMO_CONSTANTS_H_*/
