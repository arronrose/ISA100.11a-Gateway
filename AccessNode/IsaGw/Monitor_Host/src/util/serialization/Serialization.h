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

#ifndef BINARYSTREAM_H_
#define BINARYSTREAM_H_

#include <nlib/exception.h>
#include <boost/cstdint.hpp> //used for inttypes

namespace nisa100 {
namespace util {

class NotEnoughBytesException : public nlib::Exception
{
};

/**
 * Serialize an input value into binary format using provided byte order.
 */
template <typename T, typename Stream, typename ByteOrder> inline
void binary_write(Stream& stream, const T& value, const ByteOrder& byteOrder)
{
	T valueByteOrdered = byteOrder.To(value);
	stream.write((boost::uint8_t*)&valueByteOrdered, sizeof(valueByteOrdered));
}



/**
 * Unserialize n input value from a binary format using provided byte order.
 * @throw NotEnoughBytesException
 */
template <typename T, typename Stream, typename ByteOrder> inline
T binary_read(Stream& stream, const ByteOrder& byteOrder)
{
	T valueByteOrdered;
	stream.read((boost::uint8_t*)&valueByteOrdered, sizeof(valueByteOrdered));
	if (!stream)
	{
		THROW_EXCEPTION0(NotEnoughBytesException);
	}

	return byteOrder.From(valueByteOrdered);
}

template <typename Stream, typename Char>
void binary_read_bytes(Stream& stream, Char* buffer, int size)
{
	stream.read((boost::uint8_t*)buffer, size);
	if (!stream)
	{
		THROW_EXCEPTION0(NotEnoughBytesException);
	}
}

/**
 * Unserialize an input value from a binary format using provided byte order.
 * Uses a FrontInsertionSequence<char> as a buffer.
 */
template <typename T, typename FrontInsertionSequence, typename ByteOrder> inline
T binary_read_seq(FrontInsertionSequence& sequence, const ByteOrder& byteOrder)
{
	T valueByteOrdered;

	if (sequence.size() < sizeof(T))
	{
		THROW_EXCEPTION0(NotEnoughBytesException);
	}

	char* address = (char*)&valueByteOrdered;

	for (unsigned int i = 0; i < sizeof(T); i++)
	{
		*address = (char)sequence.front();
		sequence.pop_front();
		address++;
	}

	return byteOrder.From(valueByteOrdered);
}


} // namespace util
} // namespace nisa100

#endif /*BINARYSTREAM_H_*/
