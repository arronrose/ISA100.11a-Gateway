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

#include "GAlert_Indication.h"
#include "IGServiceVisitor.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {

bool GAlert_Indication::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

const std::string GAlert_Indication::ToString() const
{
	return boost::str(boost::format("GAlert_Indication[CommandID=%1% Status=%2%] ")
		% CommandID % (int)Status );
}

AbstractGServicePtr GAlert_Indication::Clone() const
{
	return AbstractGServicePtr(new GAlert_Indication(*this));
}

} //namespace hostapp
} //namsepace nisa100
