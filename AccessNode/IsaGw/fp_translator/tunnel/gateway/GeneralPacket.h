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

#ifndef GENERALPACKET_
#define GENERALPACKET_

#include <string>
#include <boost/cstdint.hpp>


namespace tunnel {
namespace gateway {

class GeneralPacket
{
public:
	enum GServiceTypes
	{
		GSessionRequest = 0x01,
		GSessionConfirm = 0x81,

		GLeaseRequest = 0x02,
		GLeaseConfirm = 0x82,
		
		GClientServerRequest = 0x0A,
		GClientServerConfirm = 0x8A,
	};

	GeneralPacket();
	const std::string ToString() const;

	static const boost::uint8_t GeneralPacket_SIZE_VERSION3 = 1 + 4 + 4 + 4 + 4;

	boost::uint8_t version;
	GServiceTypes serviceType;

	boost::uint32_t sessionID;

	boost::int32_t trackingID;
	boost::uint32_t dataSize;

	boost::uint32_t headerCRC;

	std::basic_string<boost::uint8_t> data;
};


} // namespace gateway
} // namespace tunnel

#endif /*GENERALPACKET_*/
