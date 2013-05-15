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

/*
 * Policy.cpp
 *
 *  Created on: Mar 18, 2009
 *      Author: Andy(andrei.petrut)
 */
#include "Policy.h"
#include <sstream>
#include <iomanip>

namespace NE {
namespace Model {

void ToString(Policy policy, std::string &policyString)
{
	std::ostringstream stream;
	stream << "Policy[Granularity=" << std::hex << std::setw(2) << (int)policy.Granularity
	        << ", KeyType=" << std::hex << std::setw(2) << (int)policy.KeyType
			<< ", KeyUsage="  << std::hex << std::setw(2) << (int)policy.KeyUsage
			<< ", NotValidBefore="  << std::hex << std::setw(8) << (int)policy.NotValidBefore
			<< ", KeyHardLifetime=" << std::hex << std::setw(8) << (int)policy.KeyHardLifetime
			<< "]";
	policyString = stream.str();
}

void  ToString(CompressedPolicy policy, std::string &policyString)
{
	std::ostringstream stream;
	stream << "CompressedPolicy[KeyHardLifetime=" << std::hex << std::setw(8) << (int)policy.KeyHardLifetime
			<< "]";
	policyString = stream.str();
}

}
}

