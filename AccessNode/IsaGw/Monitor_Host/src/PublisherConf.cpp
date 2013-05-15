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


#include "PublisherConf.h"
#include "Log.h"
#include "../AccessNode/Shared/Config.h"


int PublisherConf::LoadConfigFile(const char *pFileName/*[in]*/, CIniParser &parser /*[in/out]*/)
{
	return !parser.Load(pFileName, "rb", true) ? -1 : 0;
}

int PublisherConf::LoadFirstMAC(CIniParser &parser /*[in]*/, nisa100::hostapp::MAC &mac/*[in/out]*/)
{
	const char *pgroup = NULL;
	if (!(pgroup = parser.FindGroup(NULL, false, true, true)))
		return -1;

	std::string strMac = pgroup;
	LOG_INFO( "MAC: " << strMac );
	mac = nisa100::hostapp::MAC(strMac);
	return 0;
}
int PublisherConf::LoadNextMAC(CIniParser &parser /*[in]*/, nisa100::hostapp::MAC &mac/*[in/out]*/)
{
	const char *pgroup = NULL;
	if (!(pgroup = parser.FindGroup(NULL, true, true, true)))
		return -1;

	std::string strMac = pgroup;
	LOG_INFO("MAC: " << strMac );

	mac = nisa100::hostapp::MAC(strMac);
	return 0;
}
int PublisherConf::LoadConcentrator(CIniParser &parser /*[in]*/, const nisa100::hostapp::MAC &mac, COData &data/*[in/out]*/)
{
	char szLogVarString[256] = "";
	if (!parser.GetVarRawString(NULL, "CONCENTRATOR", szLogVarString, sizeof(szLogVarString)))
		return -1;

	LOG_INFO("CONCENTRATOR: " << szLogVarString);

	//
	int tsapID, id, dataPeriod, dataPhase, dataStaleLimit, dataContentVer, interfaceType;
	interfaceType = 1/*full_isa*/;
	char exData[2];
	int rv = sscanf( szLogVarString, "%i , %i , %i , %i , %i , %i , %i , %1s "
		, &tsapID
		, &id
		, &dataPeriod
		, &dataPhase
		, &dataStaleLimit
		, &dataContentVer
		, &interfaceType
		, exData
	) ;
	if(rv != 6 && rv != 7)
	{
		//LOG_ERROR("" << boost::str(boost::format("invalid no. of entry fields ='%1%'. It should have been 6 for string = %2%") % rv % szEnryToParseException));
		return -1;
	}
			
	data.tsapID = tsapID;
	data.objID = id;
	data.dataPeriod = dataPeriod;
	data.dataPhase = dataPhase;
	data.dataStaleLimit = dataStaleLimit;
	data.dataContentVer = dataContentVer;
	
	if (interfaceType != 1/*full_api*/ && interfaceType != 2/*simple_api*/)
	{
		LOG_ERROR("invalid interface type");
		return -1;
	}
	
	data.interfaceType = interfaceType;

	data.HasLease = false;
	return 0;
}

/*
* uint8 = 0,
* uint16 = 1,
* uint32 = 2,
* int8 = 3,
* int16 = 4,
* int32 = 5,
* float32 = 6
*/
static const char *formats[] = {"uint8", "uint16", "uint32", "int8", "int16", "int32", "float"};
static int GetCode(std::string szCode, unsigned char &val)
{
	for (std::string::iterator i = szCode.begin(); i != szCode.end(); )
	{
		if (*i == ' ' || *i == '\'')
		{
			i = szCode.erase(i);
			continue;
		}
		if ( *i <= 90  && *i >= 65)
			*i = *i - 65 + 97;
		i++;
	}

	for (unsigned i = 0; i < sizeof(formats)/sizeof(char*); i++)
	{
		if (szCode == formats[i])
		{
			val = i;
			return 0;
		}
	}

	LOG_INFO("WARNING szCode [" << szCode << "] unknown" );

	return -1;
}

static void GetStringVal(std::string raw, std::string &val)
{
	bool erase = true;
	for (std::string::iterator i = raw.begin(); i != raw.end(); )
	{

		if (*i == ' ' && erase == true)
		{
			i = raw.erase(i);
			continue;
		}
		if (*i == '\'')
		{
			i = raw.erase(i);
			erase = !erase;
			continue;
		}
		i++;
	}
	
	val = raw;
}

int PublisherConf::LoadChannel(CIniParser &parser /*[in]*/, const nisa100::hostapp::MAC &mac, int channelNo, COChannel &channel/*[in/out]*/)
{
	char szFormat[100];
	char szName[100];
	char szUnit[100];

	char szLogVarString[256] = "";
	if (!parser.GetVarRawString(NULL, "CHANNEL", szLogVarString, sizeof(szLogVarString), channelNo))
		return -1;
	std::string str = szLogVarString;
	LOG_INFO( "CHANNEL: " << szLogVarString);

	//
	int tsapID, objID, attrID, index1, index2, withStatus;
	withStatus = 1/*with status*/;
	char exData[2];
	int rv = sscanf( szLogVarString, "%i , %i , %i , %i , %i , %[^,] , %[^,] , %[^,] , %i , %1s "
		, &tsapID
		, &objID
		, &attrID
		, &index1
		, &index2
		, szFormat
		, szName
		, szUnit
		, &withStatus
		, exData
	) ;
	
	LOG_DEBUG("withStatus=" <<(int) withStatus);
	if(rv != 8 && rv != 9)
	{
		//LOG_ERROR("" << boost::str(boost::format("invalid no. of entry fields ='%1%'. It should have been 6 for string = %2%") % rv % szEnryToParseException));
		return -1;
	}
	channel.tsapID = tsapID;
	channel.objID = objID;
	channel.attrID = attrID;
	channel.index1 = index1;
	channel.index2 = index2;
	channel.withStatus = withStatus;
	if (GetCode(szFormat, channel.format) < 0)
		return -1;
	GetStringVal(szName, channel.name);
	GetStringVal(szUnit, channel.unitMeasure);
	return 0;
}
int PublisherConf::LoadChannels(CIniParser &parser /*[in]*/, const nisa100::hostapp::MAC &mac, COChannelListT &list/*[in/out]*/, ChannelIndexT &index/*[in/out]*/)
{
	COChannel channel;
	int channelNo = 0;

	list.reserve(5);

	if (LoadChannel(parser, mac, channelNo, channel) < 0)
		return -1;
	list.push_back(channel);

	ChannelIndexKey key;
	key.tsapID = channel.tsapID;
	key.objID = channel.objID;
	key.attrID = channel.attrID;
	key.index1 = channel.index1;
	key.index2 = channel.index2;
	if (index.insert(ChannelIndexT::value_type(key, list.size()-1)).second == false)
		return -1;
	while (LoadChannel(parser, mac, ++channelNo, channel) == 0)
	{
		list.push_back(channel);
		key.tsapID = channel.tsapID;
		key.objID = channel.objID;
		key.attrID = channel.attrID;
		key.index1 = channel.index1;
		key.index2 = channel.index2;
		if (index.insert(ChannelIndexT::value_type(key, list.size()-1)).second == false)
			continue;
	}

	return 0;
}


int PublisherConf::LoadPublishers(const char *pFileName/*[in]*/, PublisherInfoMAP_T &data /*[in/out]*/)
{

	CIniParser parser;
	data.clear();
	if (LoadConfigFile(pFileName, parser) < 0)
	{
		LOG_WARN("Invalid config file");
		data.clear();
		return 0;
	}

	nisa100::hostapp::MAC mac;
	if (LoadFirstMAC(parser, mac) < 0)
	{
		LOG_WARN("Invalid config file - Invalid mac=" << mac.ToString());
	}

	COData co_;
	if (LoadConcentrator(parser, mac, co_) < 0)
	{
		LOG_WARN("Invalid config file - invalid concentrator, skip publisher=" << mac.ToString());
	}

	COChannelListT list_;
	ChannelIndexT  index_;
	if (LoadChannels(parser, mac, list_, index_) < 0)
	{
		LOG_WARN("Invalid config file - invalid channel, skip publisher=" << mac.ToString());
	}

	if (data.insert(PublisherInfoMAP_T::value_type(mac, PublisherInfo (co_, list_, index_))).second == false)
	{
		LOG_WARN("error adding mac, skip publisher=" << mac.ToString());
	}

	while(LoadNextMAC(parser, mac) == 0)
	{
		COData co;
		if (LoadConcentrator(parser, mac, co) < 0)
		{
			LOG_WARN("Invalid config file -invalid concentrator for mac= " << mac.ToString());
			continue;
		}

		COChannelListT list;
		ChannelIndexT  index;
		if (LoadChannels(parser, mac, list, index) < 0)
		{
			LOG_WARN("Invalid config file -invalid channel for mac= " << mac.ToString());
			continue;
		}

		if (data.insert(PublisherInfoMAP_T::value_type(mac, PublisherInfo (co, list, index))).second == false)
		{
			LOG_WARN("Invalid config file -duplicated mac for mac= " << mac.ToString());
			continue;
		}
	}

	return 0;
}


