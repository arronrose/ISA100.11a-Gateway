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
 * ObjectsIDs.cpp
 *
 *  Created on: Mar 13, 2009
 *      Author: Catalin Pop
 */
#include "ObjectsIDs.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace AL {

const ObjectID::ObjectIDEnum ObjectID::ID_DMO; //0x7F Device management object (DMO)
const ObjectID::ObjectIDEnum ObjectID::ID_ARMO; //0x7E Alert reporting management object (ARMO)
const ObjectID::ObjectIDEnum ObjectID::ID_DSMO; //0x7D Device security management object (DSMO)
const ObjectID::ObjectIDEnum ObjectID::ID_DLMO; //0x7C Data link layer management object (DLMO)
const ObjectID::ObjectIDEnum ObjectID::ID_NLMO; //0x7A Network layer management object (NLMO)
const ObjectID::ObjectIDEnum ObjectID::ID_TLMO; //0x7B Transport layer management object (TLMO)
const ObjectID::ObjectIDEnum ObjectID::ID_ASLMO; //0x79 Application sub-layer management object (ASLMO)
const ObjectID::ObjectIDEnum ObjectID::ID_UDO; //Upload/download object (UDO)
const ObjectID::ObjectIDEnum ObjectID::ID_HRCO; //health reports concentrator object

const ObjectID::ObjectIDEnum ObjectID::ID_UAPMO; //0x77 User application process management object for UAP-n (UAPMO-n)

const ObjectID::ObjectIDEnum ObjectID::ID_DIO; //0x9B (DIO)

// Table 35 System management object types
const ObjectID::ObjectIDEnum ObjectID::ID_STSO; //0x64 System time service object (STSO)
const ObjectID::ObjectIDEnum ObjectID::ID_DSO; //0x65 Directory service object (DSO)
const ObjectID::ObjectIDEnum ObjectID::ID_SCO; //0x66 System configuration object (SCO)
const ObjectID::ObjectIDEnum ObjectID::ID_DMSO; //0x67 Device management service object (DMSO)
const ObjectID::ObjectIDEnum ObjectID::ID_SMO; //0x68 System monitoring object (SMO)
const ObjectID::ObjectIDEnum ObjectID::ID_PSMO; //0x69 Proxy security management object (PSMO)
const ObjectID::ObjectIDEnum ObjectID::ID_UDO1; //0x6A Upload/download object (UDO)
const ObjectID::ObjectIDEnum ObjectID::ID_ARO; //0x6B Alert receiving object (ARO)
const ObjectID::ObjectIDEnum ObjectID::ID_DPSO;
const ObjectID::ObjectIDEnum ObjectID::ID_DO1;
const ObjectID::ObjectIDEnum ObjectID::ID_MVO;
const ObjectID::ObjectIDEnum ObjectID::ID_PCO;
const ObjectID::ObjectIDEnum ObjectID::ID_PERO;
const ObjectID::ObjectIDEnum ObjectID::ID_SOO;
const ObjectID::ObjectIDEnum ObjectID::ID_PCSCO;
const ObjectID::ObjectIDEnum ObjectID::ID_BLO;

void ObjectID::toString(const Isa100::AL::ObjectID::ObjectIDEnum objectID, const TSAP::TSAP_Enum tsap,
            std::string &objectIdString) {
    if (tsap == TSAP::TSAP_DMAP) {
        switch (objectID) {
            case ObjectID::ID_DMO: //127:
                objectIdString = "DMO";
            break;
            case ObjectID::ID_ARMO: //126:
                objectIdString = "ARMO";
            break;
            case ObjectID::ID_DSMO: //125:
                objectIdString = "DSMO";
            break;
            case ObjectID::ID_DLMO: //124:
                objectIdString = "DLMO";
            break;
            case ObjectID::ID_NLMO: //122:
                objectIdString = "NLMO";
            break;
            case ObjectID::ID_TLMO: //123:
                objectIdString = "TLMO";
            break;
            case ObjectID::ID_ASLMO: //121:
                objectIdString = "ASLMO";
            break;
            case ObjectID::ID_UDO: //3:
                objectIdString = "UDO";
            break;
            case ObjectID::ID_HRCO: //10:
                objectIdString = "HRCO";
            break;
            case ObjectID::ID_UAPMO: //custom
                objectIdString = "UAPMO";
            break;
            default: {
                std::ostringstream stream;
                stream << "Unknown object(TSAP_ID=" << (int) tsap << ", ObjID=" << (int) objectID << ")";
                objectIdString = stream.str();
            }
        }
    } else if (tsap == TSAP::TSAP_SMAP) {
        switch (objectID) {
            case ObjectID::ID_DO1: //11:
                objectIdString = "DO1";
            break;
            case ObjectID::ID_STSO: //100:
                objectIdString = "STSO";
            break;
            case ObjectID::ID_DSO: //101:
                objectIdString = "DSO";
            break;
            case ObjectID::ID_SCO: //102:
                objectIdString = "SCO";
            break;
            case ObjectID::ID_DMSO: //103:
                objectIdString = "DMSO";
            break;
            case ObjectID::ID_SMO: //104:
                objectIdString = "SMO";
            break;
            case ObjectID::ID_PSMO: //105:
                objectIdString = "PSMO";
            break;
            case ObjectID::ID_UDO1: //106:
                objectIdString = "UDO";
            break;
            case ObjectID::ID_ARO: //107:
                objectIdString = "ARO";
            break;
            case ObjectID::ID_DPSO:
                objectIdString = "DPSO";
            break;
            case ObjectID::ID_HRCO: //10:
                objectIdString = "HRCO";
            break;
            case ObjectID::ID_UAPMO: //custom
                objectIdString = "UAPMO";
            break;
            case ObjectID::ID_MVO: //custom
                objectIdString = "MVO";
            break;
            case ObjectID::ID_PCO: //custom
                objectIdString = "PCO";
            break;
            case ObjectID::ID_PERO: //custom
                objectIdString = "PERO";
            break;
            case ObjectID::ID_SOO: //custom
                objectIdString = "SOO";
            break;
            case ObjectID::ID_PCSCO: //custom
                objectIdString = "PCSCO";
            break;
            case ObjectID::ID_BLO: //custom
                objectIdString = "BLO";
            break;
            default: {
                std::ostringstream stream;
                stream << "Unknown object(TSAP_ID=" << (int) tsap << ", ObjID=" << (int) objectID << ")";
                objectIdString = stream.str();
            }
        }
    } else {
        std::ostringstream stream;
        stream << "UAP_" << (int) tsap;
        objectIdString = stream.str();
    }
}

}
}
