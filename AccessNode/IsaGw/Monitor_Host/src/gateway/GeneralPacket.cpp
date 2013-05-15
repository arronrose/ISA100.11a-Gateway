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

#include <sstream>
#include <iomanip>
#include <boost/format.hpp>
#include <stdio.h>

#include "GeneralPacket.h"

//added
#include "../Log.h"

namespace nisa100 {
namespace gateway {

GeneralPacket::GeneralPacket()
{
	/* Commented by Cristian.Guef
	version = 0;
	*/

	//added by Cristian.Guef
	version = 0x3;

	dataSize = 0;
}

const std::string GeneralPacket::ToString() const
{
	if (LOG_DEBUG_ENABLED())
	{
		std::ostringstream dataBytes;
		dataBytes << std::hex << std::uppercase << std::setfill('0');

		//HACK:[andrei.petrut] - if GBulk, only display the first 100 bytes
		int finalSize = (int)data.size();
		bool isCutSize = false;

		/*commented by Cristian.Guef
		if ((serviceType == GBulkRequest) && (finalSize > 100))
		*/
		//added Cristian.Guef
		if ((serviceType == GBulkTransferRequest) && (finalSize > 100))
		{
			finalSize = 100;
			isCutSize = true;
		}

		for (int i = 0; i < finalSize; i++)
		{
			dataBytes << std::setw(2) << (int)data[i] << ' ';
		}

		if (isCutSize)
		{
			dataBytes << "...";
		}

		return boost::str(
			boost::format("GeneralPacket[ver=%1$02X, ser=%2$02X, sesid=%3$08X, tid=%4$08X, dataSize=%5$04X(%5%), dataBytes=<%6%>]")
			% (int)version	% (int)serviceType % sessionID % trackingID % dataSize % dataBytes.str());
	}
	else
	{
		char szMsg[200];
		sprintf(szMsg, "GeneralPacket[ver=%02d, ser=%02d, sesid=%02d, tid=%02d, dataSize=%02d]", (int)version, (int)serviceType, sessionID, trackingID, dataSize);
		return szMsg;
	}
}

}  // namespace gateway
}  // namespace nisa100
