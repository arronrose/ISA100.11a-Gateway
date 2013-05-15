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

#ifndef ABSTRACTGSERVICE_H_
#define ABSTRACTGSERVICE_H_

//#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp> //used for inttypes

#include "../flow/Request.h"


namespace tunnel {
namespace comm {


class GClientServer_S;
//typedef boost::shared_ptr<GClientServer_S> ClientServerInwardsPtr;
typedef GClientServer_S* ClientServerInwardsPtr;


class AbstractGService;
//typedef boost::shared_ptr<AbstractGService> AbstractGServicePtr;
typedef AbstractGService* AbstractGServicePtr;

class IGServiceVisitor;

/**
 * @brief The Visited class.
 */
class AbstractGService
{
public:
	enum ResponseStatus m_status;		// resp

public:
	boost::int32_t	m_sessionID;		// req/resp

public:
	AbstractGService()
	{
		m_status	= rsNoStatus;
		m_sessionID = 0;
	}

	AbstractGService(const AbstractGService& rhs)
	{
		m_status = rhs.m_status;
	}

	virtual ~AbstractGService()
	{
	}

	virtual bool Accept(IGServiceVisitor& visitor) = 0;
	virtual const std::string ToString() const = 0;

	virtual AbstractGServicePtr Clone() const
	{
		//return AbstractGServicePtr();
		return NULL;
	}
};

} // namespace comm
} // namespace tunnel

#endif /*ABSTRACTGSERVICE_H_*/
