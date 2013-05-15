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

#ifndef GDELCONTRACT_H_
#define GDELCONTRACT_H_

#include "AbstractGService.h"
#include "../model/IPv6.h"

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

class GDelContract : public AbstractGService
{

public:
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	boost::uint32_t ContractID;
	boost::uint8_t	ContractType;
	IPv6 IPAddress;

	unsigned char m_ucEndPointsNo;
	unsigned char m_ucTransferMode;
	unsigned char m_ucProtocolType;

	unsigned int m_unResourceID; 
	GDelContract(boost::int32_t contractID):ContractID(contractID)
	{
		m_ucEndPointsNo = 1;
		m_ucTransferMode = 0;
		m_ucProtocolType = 0;
	}
};

} //namespace hostapp
} //namespace nisa100

#endif /*GCONTRACT_H_*/
