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

#ifndef PUBLISHERCONF_H_
#define PUBLISHERCONF_H_

#include "./hostapp/model/Device.h"
#include <vector>
#include <map>



class CIniParser;
class PublisherConf
{

public:
	struct COData
	{
		unsigned short				objID;
		unsigned short				tsapID;
		short						dataPeriod;
		unsigned short				dataPhase;
		unsigned short				dataStaleLimit;
		unsigned char				dataContentVer;
		unsigned char				interfaceType;	// 1 - full_api, 2 - simple api
		bool						HasLease;	//for optimization
		nisa100::hostapp::DevicePtr	devPtr;		//temporary initialized and used when comparing publshers - for optimization process
		COData()
		{
			HasLease = false;
			interfaceType = 1;
		}
	};

	struct COChannel
	{
		unsigned short	tsapID;
		unsigned short	objID;
		unsigned short	attrID;
		unsigned char	index1;
		unsigned char	index2;
		unsigned char	format;
		std::string		name;
		std::string		unitMeasure;
		int				withStatus; 		//!=0 -with Status, 0-no Status
		int				dbChannelNo;			//for optimization

		COChannel()
		{
			withStatus = 0;
		}
	};
	typedef std::vector<COChannel>							COChannelListT;
	
public:
	struct ChannelIndexKey
	{
		unsigned short	tsapID;
		unsigned short	objID;
		unsigned short	attrID;
		unsigned char	index1;
		unsigned char	index2;

		bool operator < (const ChannelIndexKey &obj)const
		{
			if ((tsapID << 16 | objID) < (obj.tsapID << 16 | obj.objID))
				return true;
			if ((tsapID << 16 | objID) == (obj.tsapID << 16 | obj.objID))
				return (attrID << 16 | index1 << 8 | index2) 
							< (obj.attrID << 16 | obj.index1 << 8 | obj.index2);
			return false;
		}

		bool operator == (const ChannelIndexKey &obj)const
		{
			if ((tsapID << 16 | objID) == (obj.tsapID << 16 | obj.objID))
				return (attrID << 16 | index1 << 8 | index2) 
							== (obj.attrID << 16 | obj.index1 << 8 | obj.index2);
			return false;
		}
	};
	typedef std::map<ChannelIndexKey, int/*channelIndexData*/> ChannelIndexT;	//it ensures the intersection of channels 
																				// for 2 publshers has linear complexity;
																				// it also triggers an error when channels are duplicated
																				// when loading from .conf file
public:
	struct PublisherInfo
	{
		COData				coData;
		COChannelListT		coChannelList;
		ChannelIndexT		channelIndex;

		PublisherInfo(const COData &coData_, COChannelListT &coChannelList_, ChannelIndexT	&channelIndex_):
		coData(coData_), coChannelList(coChannelList_), channelIndex(channelIndex_)
		{
		}
	};
	typedef std::map<nisa100::hostapp::MAC, PublisherInfo>	PublisherInfoMAP_T;


private:
	int LoadConfigFile(const char *pFileName/*[in]*/, CIniParser &parser /*[in/out]*/);
	int LoadFirstMAC(CIniParser &parser /*[in]*/, nisa100::hostapp::MAC &mac/*[in/out]*/);
	int LoadNextMAC(CIniParser &parser /*[in]*/, nisa100::hostapp::MAC &mac/*[in/out]*/);
	int LoadConcentrator(CIniParser &parser /*[in]*/, const nisa100::hostapp::MAC &mac, COData &data/*[in/out]*/);
	int LoadChannel(CIniParser &parser /*[in]*/, const nisa100::hostapp::MAC &mac, int channelNo, COChannel &channel/*[in/out]*/);
	int LoadChannels(CIniParser &parser /*[in]*/, const nisa100::hostapp::MAC &mac, COChannelListT &list/*[in/out]*/, ChannelIndexT &index/*[in/out]*/);

public:
	// it loads data from file until an invalid data it is read;
	// ret values:
	//	0 - ok
	// -1 - invalid config file (if no valid data has been read)
	int LoadPublishers(const char *pFileName/*[in]*/, PublisherInfoMAP_T &data /*[in/out]*/);

};

#endif
