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

#include "MAC.h"

#include <algorithm>
#include <vector>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace nisa100 {
namespace hostapp {

static boost::uint8_t null_address[8] =
{ 0, 0, 0, 0, 0, 0, 0, 0 };

MAC::MAC(const boost::uint8_t * address_)
{
	SetAddress(address_ ? address_ : null_address);
}

//the format should by like: XXXX:XXXX:XXXX:XXXX
MAC::MAC(const std::string& textAddress_)
{
	typedef std::vector<std::string> StringList;
	StringList splitedMac;
	boost::split(splitedMac, textAddress_, boost::is_any_of(":") );
	if (splitedMac.size() != 4)
	{
		LOG_ERROR("Invalid MAC string: " << textAddress_);
		THROW_EXCEPTION1(nlib::ArgumentException, boost::str(boost::format("Invalid MAC string: %1% !") % textAddress_));
	}

	try
	{
		int i = 0;
		for (StringList::const_iterator it = splitedMac.begin(); it != splitedMac.end(); it++)
		{
			boost::uint16_t val;
			std::istringstream hexstring(*it);
			hexstring >> std::hex >> val;

			address[2 * i] = (boost::uint8_t)((val & 0xFF00) >> 8);
			address[2 * i + 1] = (boost::uint8_t)(val & 0x00FF);
			i++;
		}
		textAddress = textAddress_;
	}
	catch(std::exception& ex )
	{
		LOG_ERROR("Invalid MAC string: " << textAddress_);
		THROW_EXCEPTION1(nlib::ArgumentException, boost::str(boost::format("Invalid MAC string: %1% !") % textAddress_));
	}
}

MAC::MAC(const MAC& mac)
{
	(*this) = mac;
}

const MAC& MAC::operator=(const MAC& rhs)
{
	SetAddress(rhs.address);
	return (*this);
}

const std::string MAC::ToString() const
{
	return textAddress;
}

const boost::uint8_t* MAC::Address() const
{
	return address;
}

void MAC::SetAddress(const boost::uint8_t* const newAddress)
{
	textAddress.clear();
	std::copy(newAddress, newAddress + sizeof(address), address);

	try
	{
		for (int i = 0; i < SIZE; i = i + 2)
		{
			boost::uint16_t v = (((boost::uint16_t)address[i]) << 8) | address[i+1];
			textAddress.append( boost::str( boost::format("%1$04X") % v ) );
			if (i < SIZE - 2)
			{
				textAddress.append(":");
			}
		}
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Unhandled exception=" << ex.what());
		throw;
	}
}

bool operator<(const MAC& lhs, const MAC& rhs)
{
	return lhs.textAddress < rhs.textAddress;
}

bool operator==(const MAC& lhs, const MAC& rhs)
{
	return !(lhs < rhs) && !(rhs < lhs);
}

bool operator!=(const MAC& lhs, const MAC& rhs)
{
	return !(lhs == rhs);
}

} //namespace hostapp
} //namespace nisa100
