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

#ifndef MAC_H_
#define MAC_H_

#include <string>
#include <boost/cstdint.hpp> //used for inttypes

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"

#include <nlib/exception.h>

namespace nisa100 {
namespace hostapp {


/**
 * Represents a serial number for a node.
 */
class MAC
{
	/* commented by Cristian.Guef	
	LOG_DEF("nisa100.hostapp.MAC")
	*/

public:
	static const int SIZE = 8;

	/**
	 *
	 */
	MAC(const boost::uint8_t * address = 0);

	/**
	 * the format should by like: XXXX:XXXX:XXXX:XXXX
	 */
	MAC(const std::string& address);

	MAC(const MAC& mac);

	const std::string ToString() const;
	const boost::uint8_t* Address() const;

	const MAC& operator=(const MAC& rhs);
	friend bool operator<(const MAC& lhs, const MAC& rhs);

private:
	void SetAddress(const boost::uint8_t* const newAddress);

	boost::uint8_t address[SIZE];
	std::string textAddress;
};

bool operator<(const MAC& lhs, const MAC& rhs);
bool operator==(const MAC& lhs, const MAC& rhs);
bool operator!=(const MAC& lhs, const MAC& rhs);


} // namespace hostapp
} // namespace nisa100

#endif /*MAC_H_*/
