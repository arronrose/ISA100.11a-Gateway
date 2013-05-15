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

#ifndef IFACTORYDAL_H_
#define IFACTORYDAL_H_

#include <boost/shared_ptr.hpp>
#include "ICommandsDal.h"
#include "IDevicesDal.h"


namespace nisa100 {
namespace hostapp {

class IFactoryDal;
typedef boost::shared_ptr<IFactoryDal> IFactoryDalPtr;


class IFactoryDal
{
public:
	virtual ~IFactoryDal()
	{
	}
	
	virtual void BeginTransaction() = 0;
	virtual void CommitTransaction() = 0;
	virtual void RollbackTransaction() = 0; 
	
	virtual ICommandsDal& Commands() const = 0;
	virtual IDevicesDal& Devices() const = 0;
	
	virtual void VacuumDatabase() = 0;
	virtual void CleanupOldRecords(nlib::DateTime olderThanDate, nlib::DateTime olderThanAlertDate) = 0;

	//for separate flow
	virtual bool IsTransactionOn() = 0;
};


} // namespace hostapp
} // namespace nisa100

#endif /*IFACTORYDAL_H_*/
