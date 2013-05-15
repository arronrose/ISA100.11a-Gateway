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

#include "GClientServer_S.h"
#include "IGServiceVisitor.h"

#include <boost/format.hpp>

namespace tunnel {
namespace comm {

bool GClientServer_S::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

const std::string GClientServer_S::ToString() const
{
	return boost::str(boost::format("GClientServer_S[LeaseID=%1% TunnelNo=%2% Status=%3%]")
		% m_leaseID % m_tunnelNo % m_status);
}

GClientServer_S::GClientServer_S(const GClientServer_S& cs_s)
{
	m_tunnelNo = cs_s.m_tunnelNo;
}

AbstractGServicePtr GClientServer_S::Clone() const
{
	return AbstractGServicePtr(new GClientServer_S(*this));
}

} //namespace comm
} //namespace tunnel
