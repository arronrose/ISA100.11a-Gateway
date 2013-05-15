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

#ifndef PUBLISHEDDATAMNG_H_
#define PUBLISHEDDATAMNG_H_

#include "../commandmodel/PublishSubscribe.h"
#include "../model/DeviceReading.h"

#include <nlib/datetime.h>
#include <vector>
#include <queue>
#include <string>
#include <list>



namespace nisa100 {
namespace hostapp {


class PublishedDataMng
{
public:
	PublishedDataMng(int clockWiseOffset = 20, int noClockWiseOffset = 20):
		upperBoundAfter_0_(0 + clockWiseOffset), lowerBoundTo_O_(255 - noClockWiseOffset + 1),
		m_readingsNo(0), m_saveToDBSigFlag (false)
	{m_handleToData.reserve(50);}
	~PublishedDataMng(){}

public:
	struct PublishedData
	{
		unsigned char						SequenceNo;
		std::basic_string<unsigned char>	DataBuff;
		nlib::DateTime						ReadingTime;
		short 								milisec;

		PublishedData(unsigned char seqNo, std::basic_string<unsigned char> &dataBuff, 
					nlib::DateTime &DT, short msec): SequenceNo(seqNo), DataBuff(dataBuff), 
					ReadingTime(DT), milisec(msec){}
	};
	typedef std::queue<PublishedData> PublishedDataListT;

public:
	typedef int PublishHandle;
	const unsigned char upperBoundAfter_0_;	    //defines a zone after '0' regarding testing next_seq
	const unsigned char lowerBoundTo_O_;		//defines a zone after '0' regarding testing next_seq
	 

//published data
public:
	PublishHandle GetNewPublishHandle();
	bool IsHandleValid(PublishHandle handle);
	void AddPublishedData(PublishHandle handle, const PublishedData &data);
	PublishedDataListT* GetPublishedData(PublishHandle handle);

//parsing info
public:
	void SetParsingInfoErrorFlag(PublishHandle handle, bool flag);
	bool GetParsingInfoErrorFlag(PublishHandle handle);
	bool IsParsingInfo(PublishHandle handle);
	void SaveParsingInfo(PublishHandle handle, std::vector<Subscribe::ObjAttrSize> &parsing);
	std::vector<Subscribe::ObjAttrSize> *GetParsingInfo(PublishHandle handle);

//content version
public:
	bool IsContentVersion(PublishHandle handle);
	void SaveContentVersion(PublishHandle handle, unsigned char version);
	unsigned char GetContentVersion(PublishHandle handle);

//interface type
public:
	bool IsInterfaceType(PublishHandle handle);
	void SaveInterfaceType(PublishHandle handle, unsigned char version);
	unsigned char GetInterfaceType(PublishHandle handle);

//sequence no.
public:
	bool IsNextSequenceValid(PublishHandle handle, unsigned char seq);
	void ChangeLastSequence(PublishHandle handle, unsigned char seq);
	void ClearLastSequence(PublishHandle handle);
	unsigned char GetLastSequence(PublishHandle handle);

//publish error
public:
	bool IsPublishError(PublishHandle handle);
	void SetPublishError(PublishHandle handle, bool val);
	
//publish stale_data status 
public:
	bool IsStaleData(PublishHandle handle);
	void SetStaleDataStatus(PublishHandle handle, bool val);

//publish status 
public:
	bool IsFreshData(PublishHandle handle);
	void SetFreshDataStatus(PublishHandle handle, bool val);

//readings
public:
	void AddReading(PublishHandle handle, DeviceReading &reading);
	DeviceReadingsList* GetReadings(PublishHandle handle);
	void ClearReadings(PublishHandle handle);
	int GetAllReadingsNo();
	std::list<PublishHandle>* GetReadingHandles();

//readings separate flow
public:
	bool GetSaveToDBSigFlag();
	void SetSaveToDBSigFlag(bool val);


public:
	void EraseData(PublishHandle handle);

public:
	struct InterpretInfo
	{
		int channelDBNo;
		int	dataFormat;
	};
	typedef std::vector<InterpretInfo> InterpretInfoListT;
	void SetInterpretInfo(PublishHandle handle, InterpretInfoListT & list);
	InterpretInfoListT* GetInterpretInfo(PublishHandle handle);

private:
	std::queue<PublishHandle>		m_handles;

private:
	struct Readings
	{
		DeviceReadingsList					data;
		std::list<PublishHandle>::iterator	listI;
	};
	struct DataPerDevice
	{
		bool								pubDataError;
		std::vector<unsigned char>			version;		//if it exists then vector has only size = 1
		std::vector<unsigned char>			InterfaceType;	//if it exists then vector has only size = 1
		std::queue<PublishedData>			dataList;
		std::vector<Subscribe::ObjAttrSize> parse;
		InterpretInfoListT					interpret;
		bool								flag;			
		std::vector<unsigned char>			lastSeq;		//if it exists then vector has only size = 1
		bool								isHandled;		//is a handle to it?
		Readings							readings;
		bool								staleData;
		bool								freshData;

		DataPerDevice()
		{
			pubDataError = true;	//consider it as error at first
			staleData = false;
			freshData = false;
		}
	};
	std::vector<DataPerDevice>	m_handleToData;	

private:
	std::list<PublishHandle>		m_readingHandles;		//there is a one-to-one relation between m_handleToData(readings) and m_readingHandles
	int								m_readingsNo;

private:
	bool							m_saveToDBSigFlag;
	bool							m_addToCacheFlag;
};

}
}

#endif
