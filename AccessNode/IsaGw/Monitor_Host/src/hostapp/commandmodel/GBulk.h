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

#ifndef GBULK_H_
#define GBULK_H_

#include "AbstractGService.h"
#include "../../../AccessNode/Shared/MicroSec.h"

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a GBulk Service. Holds requests & responses data.
 */
class GBulk : public AbstractGService
{
public:
	GBulk()
	{
	}

public:
	GBulk(int type_):type(type_)
	{
	}


	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:

	/* commented by Cristian.Guef
	boost::uint16_t ContractID;
	*/
	//added by Cristian.Guef
	boost::uint32_t ContractID;
	
	/*commented by Cristian.Guef
	boost::uint8_t TLDE_SAPID;
	*/
	//added by Cristian.Guef
	boost::uint16_t TLDE_SAPID;

	std::basic_string<char> FileName;

	std::basic_string<boost::uint8_t> Data;
	//[Marcel] TODO: replace with char * m_pData and pass only pointer to data
	//[Marcel] TODO: see in ResponseProcessor.cpp line 924 nextBulk->Data = bulk.Data;

	//added by Cristian.Guef
	enum BulkStates
	{
		BulkOpen = 1,
		BulkTransfer,
		BulkEnd
	} m_currentBulkState;

	//added by Cristian.Guef
	unsigned int maxBlockSize;
	unsigned int currentBlockSize;
	int maxBlockCount;
	unsigned int currentBlockCount;
	unsigned int bulkDialogID;

	//bulkType
	enum BulkType
	{
		BULK_WITH_SM = 0,
		BULK_WITH_DEV = 1
	};
	int type; //0 with sm
			  //1 with device
	unsigned short	port;
	unsigned short	objID;
	int				devID;
	int			committedBurst;
	CMicroSec	transferTime;
};

} //namespace hostapp
} //namsepace nisa100

#endif /*GBULK_H_*/
