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

#ifndef READCHANNELVALUE_H_
#define READCHANNELVALUE_H_

#include <boost/format.hpp>
#include "../model/Device.h"

#include "PublishSubscribe.h"

namespace nisa100 {
namespace hostapp {

enum ObjectAccessType
{
	/* comented by Cristian.Guef
	oatReadRequest = 0x04,
	oatWriteRequest = 0x05,
	oatExecuteRequest = 0x06
	*/

	//added by Cristian.Guef
	oatReadRequest = 0x03,
	oatWriteRequest = 0x04,
	oatExecuteRequest = 0x05

};

//added by Cristian.Guef
class GetSizeForPubIndication
{
public:
	unsigned short m_TLSAP_ID;
	unsigned short m_Concentrator_id;
	
	unsigned int m_ContentVersion;

	unsigned int m_PublisherDeviceID;

	unsigned int m_SubscriberLowThreshold;
	unsigned int m_SubscriberHighThreshold;

	int m_publishHandle;

	std::vector<Subscribe::ObjAttrSize> m_vecReadObjAttrSize;

	const std::string ToString() const
	{
		return "GetSizeForPubIndication";
	}
};


class WriteObjectAttribute
{
public:
	DeviceChannel channel;

	ChannelValue writeValue;

	const std::string ToString() const
	{
		return "WriteObjectAttribute";
	}
};

class ReadMultipleObjectAttributes
{
public:
	struct ObjectAttribute
	{
		DeviceChannel channel;

		int confirmStatus; //sfcode for each channel readed
		ChannelValue confirmValue;

		//added by Cristian.Guef
		unsigned char ValueStatus; //(ISA standard)
		bool IsISA;
		
		//added by Cristian.Guef - when reading values with c/s
		//nlib::DateTime ReadingTime;
		//short milisec;
		//added
		struct timeval tv;
	};
	typedef std::vector<ObjectAttribute> AttributesList;
public:
	boost::uint8_t TSAPID;
	AttributesList attributes;

	const std::string ToString() const
	{
		return "ReadMultipleObjectAttributes";
	}

	//added by Cristian.Guef
	//needed data for fragmenting
	unsigned int m_unDialogueID;
	unsigned int m_unTotalAttributesNo;
	unsigned int m_unSequenceNo;
};

}  // namespace hostapp
}  // namespace nisa100


#endif /*READCHANNELVALUE_H_*/
