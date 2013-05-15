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

#ifndef OBJECTSIDS_H_
#define OBJECTSIDS_H_

#include "Common/NETypes.h"
#include "Common/smTypes.h"

namespace Isa100 {
namespace AL {

struct ObjectID {

        typedef Uint16 ObjectIDEnum;

        /**
         * Contains the ids of the objects both DMAP.
         */
        static const ObjectIDEnum NONE = 0;
        // Table 8 Standard management object types in DMAP
        static const ObjectIDEnum ID_DMO = 1; //127; //0x7F Device management object (DMO)
        static const ObjectIDEnum ID_ARMO = 2; //126; //0x7E Alert reporting management object (ARMO)
        static const ObjectIDEnum ID_DSMO = 3; //125; //0x7D Device security management object (DSMO)
        static const ObjectIDEnum ID_DLMO = 4; //124; //0x7C Data link layer management object (DLMO)
        static const ObjectIDEnum ID_NLMO = 5; //122; //0x7A Network layer management object (NLMO)
        static const ObjectIDEnum ID_TLMO = 6; //123; //0x7B Transport layer management object (TLMO)
        static const ObjectIDEnum ID_ASLMO = 7; //121; //0x79 Application sub-layer management object (ASLMO)
        static const ObjectIDEnum ID_UDO = 8; //3; //Upload/download object (UDO)
        static const ObjectIDEnum ID_DPO = 9;
        static const ObjectIDEnum ID_HRCO = 10; //health reports concentrator object

        static const ObjectIDEnum ID_UAPMO = 119; //0x77 User application process management object for UAP-n (UAPMO-n)
        static const ObjectIDEnum ID_DIO = 155; //0x9B (DIO)

        // Table 35 System management object types
        static const ObjectIDEnum ID_STSO = 1; //100; //0x64 System time service object (STSO)
        static const ObjectIDEnum ID_DSO = 2; //101; //0x65 Directory service object (DSO)
        static const ObjectIDEnum ID_SCO = 3; //102; //0x66 System configuration object (SCO)
        static const ObjectIDEnum ID_DMSO = 4; //103; //0x67 Device management service object (DMSO)
        static const ObjectIDEnum ID_SMO = 5; //104; //0x68 System monitoring object (SMO)
        static const ObjectIDEnum ID_PSMO = 6; //105; //0x69 Proxy security management object (PSMO)
        static const ObjectIDEnum ID_UDO1 = 7; //106; //0x6A Upload/download object (UDO)
        static const ObjectIDEnum ID_ARO = 8; //107; //0x6B Alert receiving object (ARO)
        static const ObjectIDEnum ID_DPSO = 9; // Device provisioning service object
        static const ObjectIDEnum ID_DO1 = 11; //dispersion object; custom id
        // custom object
        static const ObjectIDEnum ID_MVO = 222; //dispersion object; custom id
        static const ObjectIDEnum ID_PCO = 223; // custom id
        static const ObjectIDEnum ID_PERO = 224; // custom id
        static const ObjectIDEnum ID_SOO = 225; // custom id
        static const ObjectIDEnum ID_PCSCO = 226; // custom id
        static const ObjectIDEnum ID_BLO = 227; // custom id - blacklisting object


        static void toString(const Isa100::AL::ObjectID::ObjectIDEnum objectID,
                    const Isa100::Common::TSAP::TSAP_Enum tsap, std::string &objectIdString);
};

} //end namespace Common
} // end namespace Isa100


#endif /*OBJECTSIDS_H_*/
