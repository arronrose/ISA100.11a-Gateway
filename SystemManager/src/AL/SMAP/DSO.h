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

#ifndef DSO_H_
#define DSO_H_

#include "Common/NEAddress.h"
#include "Common/Address128.h"
#include "Common/NETypes.h"
#include "Common/logging.h"
#include "AL/Isa100Object.h"

namespace Isa100 {
namespace AL {
namespace SMAP {

struct AddressTranslationRow {
    /**
     * Globally unique IEEE EUI-64 address of device.
     */
    NE::Common::Address64 eui64;

    /**
     * 128-bit network address of device assigned by system manager.
     */
    NE::Common::Address128 networkAddress128;

    /**
     * DL subnet in which or from which this device is reachable;
     * a device may be reachable from multiple subnets in which case this element corresponds to one such subnet.
     */
    Uint16 dlSubnetID;

    /**
     * 16-bit alias of device in the subnet indicated by the dlSubnetID element given above.
     */
    Uint16 dlAlias16Bit;

    AddressTranslationRow();

    void marshall(NE::Misc::Marshall::OutputStream& stream);
};

typedef std::vector<AddressTranslationRow> AddressTranslationRowList;

/**
 * Structure that will be populated with the response arguments for DSO's Read_Address_Row method.
 */
struct LookupResponse {
    /**
     * Indicates the type of information being provided;
        Enumeration:
            0 - single row if DL subnet ID value provided as input argument
            1 - all rows if DL subnet ID value not provided as input argument
                (i.e., all rows for given EUI-64 or 128-bit address are returned);
     */
    Uint8 valueType;

    /**
     * Number of rows being returned.
     */
    Uint8 valueSize;

    AddressTranslationRowList rows;

    LookupResponse(Uint8 type, AddressTranslationRowList returnedRows);

    void marshall(NE::Misc::Marshall::OutputStream& stream);
};




}
}
}

#endif /*DSO_H_*/
