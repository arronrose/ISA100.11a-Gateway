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

#ifndef GCLNTSERVERS_H_
#define GCLNTSERVERS_H_

#include "AbstractGService.h"


namespace tunnel {
namespace comm {


class IGServiceVisitor;

class GClientServer_S : public AbstractGService
{
public:
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	boost::int32_t m_tunnelNo;

public:
	boost::uint32_t m_leaseID;			// req
	boost::uint8_t	m_buffer;			// req
	boost::uint8_t	m_transferMode;		// req 

	std::basic_string<boost::uint8_t>	m_reqData;		// req
	TunnelInfo							m_tunnelInfo;	// req

	//NOTE: no response for tunneling

public:
	GClientServer_S(){}
	GClientServer_S(const GClientServer_S& cs_s);
	AbstractGServicePtr Clone() const;
};

} //namespace comm
} //namespace tunnel

#endif
