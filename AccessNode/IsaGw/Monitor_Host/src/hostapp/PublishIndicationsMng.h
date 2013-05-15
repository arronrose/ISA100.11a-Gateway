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

#ifndef _PUBLISHINDICATIONMNG_H_
#define _PUBLISHINDICATIONMNG_H_


#include "commandmodel/AbstractGService.h"
#include <vector>

namespace nisa100 {
namespace hostapp {


class PublishIndicationsMng
{
private:
	struct elem
	{
		AbstractGServicePtr	indicationPtr;
		bool				isEmpty;
		elem():isEmpty(true){}
	};
	std::vector<elem/*indexed by leaseID*/> m_pendingIndications;

public:
	PublishIndicationsMng(unsigned int maxLeaseID=250){m_pendingIndications.resize(maxLeaseID+1);}

public:
	/*
	 *  0 - not ok
	 *  1 - ok 
	 */
	int AddIndication(unsigned int leaseID/*[in]*/, AbstractGServicePtr indicationPtr/*[in]*/)
	{
		if (leaseID >= m_pendingIndications.size() || leaseID == 0) 
			return 0;

		m_pendingIndications[leaseID].indicationPtr = indicationPtr;
		m_pendingIndications[leaseID].isEmpty = false;
		return 1;
	}
	/*
	 * 
	 *
	 */ 
	AbstractGServicePtr GetIndication(unsigned int leaseID/*[in]*/)
	{
		if (leaseID >= m_pendingIndications.size() || leaseID == 0) 
			return AbstractGServicePtr();
		if (m_pendingIndications[leaseID].isEmpty == true)
			return AbstractGServicePtr();

		return m_pendingIndications[leaseID].indicationPtr;
	}
	/*
	 *  0 - not ok
	 *  1 - ok 
	 */
	int DeleteIndication(unsigned int leaseID)
	{
		if (leaseID >= m_pendingIndications.size() || leaseID == 0) 
			return 0;

		m_pendingIndications[leaseID].indicationPtr.reset();
		m_pendingIndications[leaseID].isEmpty = true;
		return 1;
	}
};

}
}

#endif
