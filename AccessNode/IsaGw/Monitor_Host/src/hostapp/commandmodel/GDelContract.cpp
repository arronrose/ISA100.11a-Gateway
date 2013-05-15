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

#include "GDelContract.h"
#include "IGServiceVisitor.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {

bool GDelContract::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

const std::string GDelContract::ToString() const
{
	return boost::str(boost::format("GDelLease[CommandID=%1% IPv6=%2% LID=%3% OBJ_ID=%4% TLSAP_ID=%5% Type=%6% Status=%7%]")
		% CommandID % IPAddress.ToString() % ContractID % (m_unResourceID >> 16) % (m_unResourceID & 0xFFFF) % ContractType % Status);
}

} //namespace hostapp
} //namespace nisa100
