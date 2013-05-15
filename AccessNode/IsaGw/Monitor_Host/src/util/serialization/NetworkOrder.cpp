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

#include "NetworkOrder.h"

#ifndef _WINDOWS
#	include <netinet/in.h>
#else
#	include <windows.h>
#endif

namespace nisa100 {
namespace util {

const NetworkOrder& NetworkOrder::Instance()
{
	static NetworkOrder instance;
	return instance;
}

boost::uint8_t NetworkOrder::To(const boost::uint8_t& value) const
{
	return value;
}

boost::uint8_t NetworkOrder::From(const boost::uint8_t& value) const
{
	return value;
}

boost::int8_t NetworkOrder::To(const boost::int8_t& value) const
{
	return value;
}

boost::int8_t NetworkOrder::From(const boost::int8_t& value) const
{
	return value;
}

boost::uint16_t NetworkOrder::To(const boost::uint16_t& value) const
{
	return ntohs(value);
}

boost::uint16_t NetworkOrder::From(const boost::uint16_t& value) const
{
	return htons(value);
}

boost::int16_t NetworkOrder::To(const boost::int16_t& value) const
{
	return ntohs(value);
}

boost::int16_t NetworkOrder::From(const boost::int16_t& value) const
{
	return htons(value);
}

boost::uint32_t NetworkOrder::To(const boost::uint32_t& value) const
{
	return ntohl(value);
}

boost::uint32_t NetworkOrder::From(const boost::uint32_t& value) const
{
	return htonl(value);
}

boost::int32_t NetworkOrder::To(const boost::int32_t& value) const
{
	return ntohl(value);
}

boost::int32_t NetworkOrder::From(const boost::int32_t& value) const
{
	return htonl(value);
}


} //namespace util
} //namespace nisa100
