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

#ifndef GCONTRACT_H_
#define GCONTRACT_H_

#include "AbstractGService.h"
#include "../model/IPv6.h"

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

class GContract : public AbstractGService
{
public:

	/* commented by Cristian.Guef
	enum GSContractType
	{
		Client = 0,
		Server = 1,
		Publisher = 2,
		Subscriber = 3,
		Source = 4,
		Sink = 5,
		BulkTransforClient = 6,
		BulkTransforServer = 7
	};
	*/

	//added by Cristian.Guef
	enum GSContractType
	{
		Client = 0,
		Server = 1,
		Publisher = 2,
		Subscriber = 3,
		BulkTransforClient = 4,
		BulkTransforServer = 5,
		Alert_Subscription = 6
	};

public:
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	/*commented by Cristian.Guef
	boost::uint16_t ContractID;
	*/
	//added by Cristian.Guef
	boost::uint32_t ContractID;

	boost::uint32_t ContractPeriod;
	IPv6 IPAddress;
	GSContractType ContractType;

	//added by Cristian.Guef
	unsigned char m_ucEndPointsNo;
	unsigned char m_ucTransferMode;
	unsigned char m_ucProtocolType;


	//added by Cristian.Guef -for subscribe
	unsigned char	m_ucUpdatePolicy;
	short			m_wSubscriptionPeriod;
	unsigned char	m_ucPhase;
	unsigned char	m_ucStaleLimit;

	//variable added by Cristian.Guef
	//it is necessary for generating contracts of new type (GSAP 2009.02.04)
	//(m_unResourceID && 0xFFFF) is TSAPID - UDP Port
	//(m_unResourceID && (0xFFFF<<16)) is ObjID 
	unsigned int m_unResourceID; 
	GContract(unsigned int resourceID, unsigned char contractType, boost::int16_t committedBurst,
			int dbCmdID):
			ContractType((GSContractType)contractType),
				m_unResourceID(resourceID), m_committedBurst(committedBurst), 
				m_dbCmdID(dbCmdID)
	{
	}

	//for lease of type C/S and Bulk
	boost::int16_t m_committedBurst;

	//DBcommandID 
	int m_dbCmdID;
};

} //namespace hostapp
} //namespace nisa100

#endif /*GCONTRACT_H_*/
