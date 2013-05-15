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
 * Algorithms.h
 *
 *  Created on: Mar 17, 2009
 *      Author: Andy
 */

#ifndef ALGORITHMS_H_
#define ALGORITHMS_H_
#include <Common/NETypes.h>
namespace Isa100 {
namespace Security {

#define BLOCK_SIZE 16


class Algorithms
{
public:
	typedef Uint8 Hash[BLOCK_SIZE];

	static bool AES128Encrypt(const Uint8 Key[], const Uint8 plain[16], Uint8 encrypted[16]);
	static bool AES128Decrypt(const Uint8 Key[], const Uint8 encrypted[16], Uint8 plain[16]);

	// outputHash must have at least BLOCK_SIZE bytes
	static void HashFunction(Uint8 *input, Uint16 inputLength, Hash& outputHash);
	static void HMACK_JOIN(Uint8 *input, Uint16 inputLength, const Uint8 Key[], Uint8 (&output)[BLOCK_SIZE]);
};
}
}


#endif /* ALGORITHMS_H_ */
