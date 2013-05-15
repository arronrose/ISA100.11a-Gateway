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

#include "GLease.h"
#include "IGServiceVisitor.h"

#include <boost/format.hpp>

namespace tunnel {
namespace comm {

bool GLease::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

const std::string GLease::ToString() const
{
	return boost::str(boost::format("GLease[LeaseID=%1% TunnelNo=%2% Type=%3% remotePort=%4% remoteObjID=%5% localPort=%6% localObjID=%7% Status=%8%]")
		% m_leaseID % m_tunnelNo % (boost::uint32_t)m_leaseType % m_endPoint.remotePort % m_endPoint.remoteObjID % m_endPoint.localPort % m_endPoint.localObjID % m_status);
}

} //namespace comm
} //namespace tunnel
