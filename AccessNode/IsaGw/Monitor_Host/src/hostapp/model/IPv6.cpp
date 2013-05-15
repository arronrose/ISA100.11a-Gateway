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

//added by Cristian.Guef
#ifdef __CYGWIN
#define __USE_W32_SOCKETS
#endif

#ifdef __USE_W32_SOCKETS
#	include <winsock2.h> //hack [nicu.dascalu] - should be included before asio
#endif
#include <boost/asio.hpp>

//
enum LogLevel
{
	LL_ERROR = 1,
	LL_WARN,
	LL_INFO,
	LL_DEBUG,
	LL_MAX_LEVEL
};


bool InitLogEnv(const char *pszIniFile);
bool IsLogEnabled(enum LogLevel);

void mhlog(enum LogLevel debugLevel, const std::ostream& message ) ;
void mhlog(enum LogLevel debugLevel, const char* message) ;

#define LOG_DEBUG(message) \
	mhlog(LL_DEBUG, std::stringstream().flush() <<message)

#define LOG_INFO(message) \
	mhlog(LL_INFO, std::stringstream().flush() <<message)

#define LOG_WARN(message) \
	mhlog(LL_WARN, std::stringstream().flush() <<message)

#define LOG_ERROR(message) \
	mhlog(LL_ERROR, std::stringstream().flush() <<message)

#define LOG_DEF(name) inline void __NOOP(){}
#define LOG_INFO_ENABLED() IsLogEnabled(LL_INFO)
#define LOG_DEBUG_ENABLED() IsLogEnabled(LL_DEBUG)
//
#include "IPv6.h"

#include <algorithm>
#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {

using boost::asio::ip::address_v6;

static boost::uint8_t null_address[16] =
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const IPv6& IPv6::NONE()
{
	static IPv6 instance(null_address);
	return instance;
}

IPv6::IPv6(const boost::uint8_t * const address)
{
	SetAddress(address ? address : null_address);
}

IPv6::IPv6(const std::string& address)
{
	if (address.empty())
	{
		SetAddress(null_address);
		textAddress = "";
	} else
	{
		try
		{
			address_v6 ipv6_address = address_v6::from_string(address);
			address_v6::bytes_type bytes = ipv6_address.to_bytes();

			std::copy(bytes.elems, bytes.elems + SIZE, this->ipAddress);

			textAddress = address;
		}
		catch(std::exception& ex)
		{
			THROW_EXCEPTION1(nlib::ArgumentException, boost::str(boost::format("Invalid IPv6 string: %1% !") % address));
		}
	}
}

IPv6::~IPv6()
{
}

const std::string IPv6::ToString() const
{
	return textAddress;
}
bool operator<(const IPv6& left, const IPv6& rigth)
{
	return left.textAddress < rigth.textAddress;
}

bool operator==(const IPv6& left, const IPv6& rigth)
{
	/* comented by Cristian.Guef
	return !(left < rigth) && !(rigth < left);
	*/
	return left.textAddress == rigth.textAddress;
}

bool operator!=(const IPv6& left, const IPv6& rigth)
{
	return !(left == rigth);
}

void IPv6::SetAddress(const boost::uint8_t* const newAddress)
{
	try
	{
		std::copy(newAddress, newAddress + SIZE, this->ipAddress);

		// compute textAddress
		address_v6::bytes_type bytes;
		std::copy(newAddress, newAddress + SIZE, bytes.elems);
		address_v6 ipv6_address = address_v6(bytes);

		textAddress = ipv6_address.to_string();
	}
	catch(std::exception& ex)
	{
		THROW_EXCEPTION1(nlib::ArgumentException, boost::str(boost::format("Invalid IPv6 string: %1% !") % newAddress));
	}

}

} //namespace hostapp
} //namespace nisa100
