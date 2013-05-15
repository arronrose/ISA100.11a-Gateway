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
 * Compute.h
 *
 *  Created on: Oct 8, 2009
 *      Author: flori.parauan
 */

#ifndef COMPUTE_H_
#define COMPUTE_H_

namespace NE {
namespace Misc {

namespace Math {

class MathUtils {
    public:
        static int power(int base, int exp) {
            if (exp == 0) return 1;
            if (exp == 1) return base;
            if (exp % 2 == 0) return power(base * base, exp / 2);
            if (exp % 2 == 1) return base * power(base * base, exp / 2);
        }
};
}
}

}

#endif /* COMPUTE_H_ */
