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

#ifndef GALERTINDICATION_H_
#define GALERTINDICATION_H_


#include "AbstractGService.h"
#include "../model/IPv6.h"

#include <string>

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a GTopology Service. Holds requests & responses data.
 */
class GAlert_Indication : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	
public:
	
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	IPv6				NetworkAddress;
	boost::uint16_t		EndpointPort;
	boost::uint16_t		ObjectID;
	struct TAI
	{
		boost::uint32_t Seconds;
		boost::uint16_t FractionOfSeconds;
	}Time;
	boost::uint8_t		Class;		//0 – Event
									//1 – Alarm 
	boost::uint8_t		Direction;	//0 – Alarm ended
									//1 – Alarm began 
	boost::uint8_t		Category;
	boost::uint16_t		Type;
	boost::uint16_t		Priority;
	std::basic_string<boost::uint8_t> Data;

public:
	GAlert_Indication(){}
	GAlert_Indication(const GAlert_Indication& ind){}
	AbstractGServicePtr Clone() const;
};
 
} //namespace hostapp
} //namsepace nisa100


#endif
