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
 * ManagementLinksUtils.h
 *
 *  Created on: Oct 12, 2009
 *      Author: flori.parauan
 */

#ifndef MANAGEMENTLINKSUTILS_H_
#define MANAGEMENTLINKSUTILS_H_

namespace NE {
namespace Model {
namespace Tdma {

struct ManagementLinksUtils {
    Uint8  setNo;
    Uint8  freqNo;
    Uint16 offset;
    Uint16 period;

    ManagementLinksUtils() : setNo(0), freqNo(0), offset(0), period(0) {}
};
inline
std::ostream& operator<<(std::ostream& stream, const ManagementLinksUtils& linkUtils) {
    stream << "setNo=" << linkUtils.setNo ;
    stream << ", freqNo=" << linkUtils.freqNo;
    stream << ", offset=" << linkUtils.offset;
    stream << ", period=" << linkUtils.period;

    return stream;

}
}
}
}
#endif /* MANAGEMENTLINKSUTILS_H_ */
