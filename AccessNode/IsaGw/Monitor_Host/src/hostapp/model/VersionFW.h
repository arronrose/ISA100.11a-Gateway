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

#ifndef VERSIONFW_H_
#define VERSIONFW_H_

#include <string>
#include <boost/cstdint.hpp>

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"

#include <nlib/exception.h>

namespace nisa100 {
namespace hostapp {

class VersionFW
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.VersionFW")
	*/

public:
	static const int SIZE = 3;
	VersionFW(const boost::uint8_t * version = 0);
	VersionFW(const std::string& version);

	const boost::uint8_t* Version() const;
	const std::string ToString() const;

private:
	void SetAddress(const boost::uint8_t* const newAddress);
	boost::uint8_t version[SIZE];
	std::string textFW;
	//boost::uint8_t softwareMajorVersion;
	//boost::uint8_t softwareMinorVersion;
	//boost::uint8_t softwareRevisionNumber;
};

} // namespace hostapp
} // namespace nisa100

#endif /*VERSIONFW_H_*/
