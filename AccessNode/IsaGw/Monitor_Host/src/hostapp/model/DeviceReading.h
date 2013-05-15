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

#ifndef DEVICEREADING_H_
#define DEVICEREADING_H_

#include <string>
#include "Device.h"

#include <vector>
#include <boost/format.hpp>

//added by Cristian.Guef
#include <deque>

namespace nisa100 {
namespace hostapp {

class DeviceReading;

/* commented by Cristian.Guef
typedef std::vector<DeviceReading> DeviceReadingsList;
*/
//added by Cristian.Guef
typedef std::deque<DeviceReading> DeviceReadingsList;

class DeviceReading
{
public:
	enum ReadingType
	{
		ClientServer = 0,
		PublishSubcribe = 1,
	};
public:
	int deviceID;
	/* commented
	DeviceChannel channel;
	*/
	//added
	int channelNo;

	//nlib::DateTime readingTime;
	//short milisec;
	//added
	struct timeval tv;

	//added by Cristian.Guef
	unsigned char ValueStatus; //(ISA standard)
	bool IsISA;
	
	//holds the computed value (raw value translated into interval [channel.minValue..channel.maxValue])
	/* commented
	mutable std::string value;
	*/
	mutable ChannelValue rawValue;
	ReadingType readingType;
};

} //namespace hostapp
} //namespace nisa100


#endif /*DEVICEREADING_H_*/
