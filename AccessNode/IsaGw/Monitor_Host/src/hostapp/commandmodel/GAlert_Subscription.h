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

#ifndef GALERTSUBSCRIPTION_H_
#define GALERTSUBSCRIPTION_H_



#include "AbstractGService.h"
#include "../model/IPv6.h"

#include <vector>


namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a GTopology Service. Holds requests & responses data.
 */
class GAlert_Subscription : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	struct CategoryType
	{
		boost::uint8_t	Subscribe;  //0 - unsubscribe
									//1 - subscribe
		boost::uint8_t	Enable;		//0 - disable
									//1 - enable
		boost::uint8_t  Category;
	};

	typedef std::vector<CategoryType> CategoryTypeListT;
	
	struct NetAddrType
	{
		boost::uint8_t	Subscribe;  //0 - unsubscribe
									//1 - subscribe
		boost::uint8_t	Enable;		//0 - disable
									//1 - enable
		IPv6			DevAddr;
		boost::uint16_t	EndPointPort;
		boost::uint16_t	EndObjID;
		boost::uint8_t	AlertType;
	};
	typedef std::vector<NetAddrType> NetAddrTypeListT;

public:
	
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	CategoryTypeListT	CategoryTypeList;
	NetAddrTypeListT	NetAddrTypeList;
	boost::uint32_t		LeaseID;

};

} //namespace hostapp
} //namsepace nisa100


#endif
