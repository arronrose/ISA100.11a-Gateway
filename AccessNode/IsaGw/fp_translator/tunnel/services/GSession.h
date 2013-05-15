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

#ifndef GSESSION_H_
#define GSESSION_H_


#include "AbstractGService.h"


namespace tunnel {
namespace comm {

class IGServiceVisitor;

/**
 * @brief Represents a Session Service. Holds requests & responses data.
 */
class GSession : public AbstractGService
{
	
public:
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	boost::int32_t	m_sessionPeriod;	// req/resp
	boost::uint16_t	m_networkID;		// req
};


} //namespace hostapp
} //namsepace nisa100
#endif
