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

#ifndef IPV6_H_
#define IPV6_H_

#include <string>
#include <boost/cstdint.hpp>

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
//#include "../../Log.h"

#include <nlib/exception.h>

namespace nisa100 {
namespace hostapp {

class IPv6
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.IPv6")
	*/

public:
	static const int SIZE = 16;

	static const IPv6& NONE();
	IPv6(const boost::uint8_t * const address = 0);
	IPv6(const std::string& address);
	virtual ~IPv6();

	const boost::uint8_t* Address() const { return ipAddress; }

	const std::string ToString() const;

	friend bool operator<(const IPv6& left, const IPv6& rigth);

	//added by Cristian.Guef
	friend bool operator==(const IPv6& left, const IPv6& rigth);

private:
	void SetAddress(const boost::uint8_t* const newAddress);

	boost::uint8_t ipAddress[SIZE];
	std::string textAddress;
};

bool operator==(const IPv6& left, const IPv6& rigth);
bool operator!=(const IPv6& left, const IPv6& rigth);

} // namespace hostapp
} // namespace nisa100

#endif /*IPV6_H_*/
