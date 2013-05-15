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
 * Algorithms.cpp
 *
 *  Created on: Mar 17, 2009
 *      Author: Andy
 */

#include "Algorithms.h"
#include <Common/CCM/rijndael-alg-fst.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <netinet/in.h>		//for htons

#include <Common/logging.h>

namespace Isa100 {
namespace Security {

LOG_DEF("I.S.Algorithms");


std::string StreamToString(Uint8* input, Uint16 len)
{
	std::ostringstream stream;
	for (int i = 0; i < len; i++)
	{
		stream << std::hex << ::std::setw(2) << ::std::setfill('0') << (int)*(input + i) << " ";
	}

	return stream.str();
}

bool Algorithms::AES128Encrypt(const Uint8 Key[], const Uint8 plain[16], Uint8 encrypted[16])
{
	// max 60 u32's for 256 keybits required, so allocate 60 always
	Isa100::Common::u32 bufferSpace[60];

	int rounds = Isa100::Common::rijndaelKeySetupEnc(bufferSpace, Key, 8* 16);
	Isa100::Common::rijndaelEncrypt(bufferSpace, rounds, plain, encrypted);

	return true;
}

bool Algorithms::AES128Decrypt(const Uint8 Key[], const Uint8 encrypted[16], Uint8 plain[16])
{
	// max 60 u32's for 256 keybits required, so allocate 60 always
	Isa100::Common::u32 bufferSpace[60];

	int rounds = Isa100::Common::rijndaelKeySetupDec(bufferSpace, Key, 8 * sizeof(Key));
	Isa100::Common::rijndaelDecrypt(bufferSpace, rounds, encrypted, plain);

	return true;
}

void Algorithms::HashFunction(Uint8 *input, Uint16 inputLength, Hash& outputHash)
{
	Uint16 l = inputLength * 8;
	unsigned int n = BLOCK_SIZE;
	int k = 7*n - l - 1;

	while (k < 0)
		k += 8 * n;	// if k is negative, make it positive modulo 8n
	k = k % (8 * n);	// modulo 8n

	int t = (l + (k + 1 + n * 8)) / BLOCK_SIZE / 8;	// the number of blocks of BLOCK_SIZE bits



	Uint8* Mptr = new Uint8[inputLength + (k + 1 + n * 8) / 8];	// the temporary bit string used for hash
	Uint8* baseMptr = Mptr;

	memcpy(Mptr, input, inputLength);		// copy input buffer to temporary buffer
	Mptr += inputLength;

	k = k / 8;		// convert to number of bytes

	if (k >= 0)		// append first bit as 1 and rest as 0
	{
		*Mptr++ = 0x80;
	}
	while (k-- > 0)	// append 0x00
	{
		*Mptr++ = 0x00;
	}

	l = htons(l);
	for (int j = 0; j < BLOCK_SIZE; j++)
		*Mptr++ = *((Uint8*)&l + j);				// append binary representation of L as a N-bit string

	Uint8 H[BLOCK_SIZE];		// first hash, initialize with 0
	for (int j = 0; j < BLOCK_SIZE; j++)
		H[j] = 0;

	Uint8 Mplain[BLOCK_SIZE];
	Uint8 Menc[BLOCK_SIZE];
	for (int j = 0; j < t; j++)
	{
		memcpy(&Mplain[0], baseMptr + j * BLOCK_SIZE, BLOCK_SIZE);
		AES128Encrypt(H, Mplain, Menc);

		for (int cursor = 0; cursor < BLOCK_SIZE; cursor++)
			H[cursor] = Menc[cursor] ^ Mplain[cursor];			// Hj = E(Hj-1, Mj) ^ Mj


}

	delete [] baseMptr;

	memcpy(&outputHash[0], &H[0], BLOCK_SIZE);
}

const Uint8 ipad = 0x36;
const Uint8 opad = 0x5c;

void Algorithms::HMACK_JOIN(Uint8 *input, Uint16 inputLength, const Uint8 Key[], Uint8 (&output)[BLOCK_SIZE])
{
	LOG_DEBUG("HMACK_JOIN buffer size= " << (int)inputLength << " value={" << StreamToString(input, inputLength) << "}.");

	Uint8 K0[BLOCK_SIZE + inputLength];
	memcpy(K0, Key, BLOCK_SIZE);
	memcpy(K0 + BLOCK_SIZE, input, inputLength);

	for (int i = 0; i < BLOCK_SIZE; i++)
		K0[i] ^= ipad;

	Hash Hash1;
	HashFunction(K0, sizeof(K0), Hash1);

	Uint8 K1[BLOCK_SIZE + sizeof(Hash)];

	memcpy(K1, Key, BLOCK_SIZE);
	memcpy(K1 + BLOCK_SIZE, Hash1, sizeof(Hash1));
	for (int i = 0; i < BLOCK_SIZE; i++)
		K1[i] ^= opad;

	HashFunction(K1, sizeof(K1), output);

	LOG_DEBUG("HMACK_JOIN buffer result={" << StreamToString(output, 16) << "}.");
}


}
}
