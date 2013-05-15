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

#ifndef CONVERT_H_
#define CONVERT_H_

#include <cstdio>
#include "Common/NETypes.h"

using namespace boost;
using namespace std;
using namespace NE::Common;

namespace NE {
namespace Misc {
namespace Convert {

std::string bytes2string(const std::basic_string<unsigned char>& bytes);

/**
 * Transform only the first <code>length</code> bytes into a string.
 */
std::string bytes2string(const std::basic_string<unsigned char>& bytes, size_t length);

Bytes string2bytes(std::string text);

BytesPointer text2bytes(const char* text, int length);

std::string array2string(const Uint8* value, Uint16 length);

void binary2text(Uint8* text, int length);

void printToText(BytesPointer text);

bool compare(BytesPointer sir1, BytesPointer sir2, int length);

bool compareBytes(Bytes& sir1, Bytes& sir2);

} //namespace Convert
} //namespace Misc
} //namespace NE

#endif /*CONVERT_H_*/
