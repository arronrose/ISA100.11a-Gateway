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
#include "../model/IPv6.h"
#include "../model/MAC.h"

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a DeviceListReport Service. Holds requests & responses data.
 */
class GSession : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

public:
	GSession()
	{
	}
	GSession(int networkID):m_uwNetworkID(networkID)
	{
	}

	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	int				m_nSessionPeriod;
	unsigned short	m_uwNetworkID;
	unsigned int	m_unSessionID;
};


} //namespace hostapp
} //namsepace nisa100
#endif
