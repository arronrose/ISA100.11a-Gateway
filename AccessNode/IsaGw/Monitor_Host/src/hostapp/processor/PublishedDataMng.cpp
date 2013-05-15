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


#include "PublishedDataMng.h"
#include "../../Log.h"


namespace nisa100 {
namespace hostapp {


//published data
PublishedDataMng::PublishHandle PublishedDataMng::GetNewPublishHandle()
{
	if (m_handles.empty())
	{
		m_handleToData.push_back(DataPerDevice());
		m_handleToData[m_handleToData.size() - 1].isHandled = true;
		m_handleToData[m_handleToData.size() - 1].pubDataError =true;
		m_handleToData[m_handleToData.size() - 1].staleData = false;
		m_handleToData[m_handleToData.size() - 1].freshData = false;
		m_handleToData[m_handleToData.size() - 1].readings.listI = m_readingHandles.end();
		return (PublishHandle)(m_handleToData.size() - 1);
	}
	PublishHandle handle = m_handles.front();
	m_handleToData[handle] = DataPerDevice();
	m_handleToData[handle].readings.listI = m_readingHandles.end();
	m_handleToData[handle].isHandled = true;
	m_handles.pop();
	return handle;
}
bool PublishedDataMng::IsHandleValid(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()))
	{
		LOG_WARN("IsHandleValid() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	return m_handleToData[handle].isHandled;
}
void PublishedDataMng::AddPublishedData(PublishedDataMng::PublishHandle handle, const PublishedData &data)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("AddPublishedData() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}

	m_handleToData[handle].dataList.push(data);
}
PublishedDataMng::PublishedDataListT* PublishedDataMng::GetPublishedData(PublishedDataMng::PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetPublishedData() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return NULL;
	}
	return &m_handleToData[handle].dataList;
}

//parsing info
void PublishedDataMng::SetParsingInfoErrorFlag(PublishedDataMng::PublishHandle handle, bool flag)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SetParsingInfoErrorFlag() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	m_handleToData[handle].flag = flag;
}
bool PublishedDataMng::GetParsingInfoErrorFlag(PublishedDataMng::PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetParsingInfoErrorFlag() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	return m_handleToData[handle].flag;
}
bool PublishedDataMng::IsParsingInfo(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsParsingInfo() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	return m_handleToData[handle].parse.size() > 0 ? true : false;
}
void PublishedDataMng::SaveParsingInfo(PublishedDataMng::PublishHandle handle, std::vector<Subscribe::ObjAttrSize> &parsing)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SaveParsingInfo() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	m_handleToData[handle].parse = parsing;
}
std::vector<Subscribe::ObjAttrSize> *PublishedDataMng::GetParsingInfo(PublishedDataMng::PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetParsingInfo() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return NULL;
	}
	if (m_handleToData[handle].parse.size() == 0)
	{
		LOG_WARN("GetParsingInfo() with inappropiate parameters _2, handle=" << handle);
		return NULL;
	}
	return &m_handleToData[handle].parse;
}

//content version
bool PublishedDataMng::IsContentVersion(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsContentVersion() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	return m_handleToData[handle].version.size() > 0 ? true : false;
}
void PublishedDataMng::SaveContentVersion(PublishedDataMng::PublishHandle handle, unsigned char version)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SaveContentVersion() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	
	//it was for older version
	//assert(m_handleToData[handle].version.size() == 0);
	if (m_handleToData[handle].version.size() > 0)
		m_handleToData[handle].version.clear();
	m_handleToData[handle].version.push_back(version);
}
unsigned char PublishedDataMng::GetContentVersion(PublishedDataMng::PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetContentVersion() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return 0;
	}
	if (m_handleToData[handle].version.size() == 0)
	{
		LOG_WARN("GetContentVersion() with inappropiate parameters _2, handle=" << handle);
		return 0;
	}
	
	//printf("data version = %d", m_handleToData[handle].version[0]);
	//fflush(stdout);
	return m_handleToData[handle].version[0];
}

//interface type
bool PublishedDataMng::IsInterfaceType(PublishHandle handle)
{
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsInterfaceType() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	return m_handleToData[handle].InterfaceType.size() > 0 ? true : false;
}
void PublishedDataMng::SaveInterfaceType(PublishHandle handle, unsigned char version)
{
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SaveInterfaceType() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	
	if (m_handleToData[handle].InterfaceType.size() > 0)
		m_handleToData[handle].InterfaceType.clear();
	m_handleToData[handle].InterfaceType.push_back(version);
}
unsigned char PublishedDataMng::GetInterfaceType(PublishHandle handle)
{
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetInterfaceType() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return 0;
	}
	if (m_handleToData[handle].InterfaceType.size() == 0)
	{
		LOG_WARN("GetInterfaceType() with inappropiate parameters _2, handle=" << handle);
		return 0;
	}
	
	return m_handleToData[handle].InterfaceType[0];
}

//sequence no.
bool PublishedDataMng::IsNextSequenceValid(PublishedDataMng::PublishHandle handle, unsigned char seq)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsNextSequenceValid() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}

	if (m_handleToData[handle].lastSeq.size() > 0)
	{
		if (m_handleToData[handle].lastSeq.size() == 0)
		{
			LOG_WARN("IsNextSequenceValid() with inappropiate parameters _2, handle=" << handle);
			return false;
		}
		if (m_handleToData[handle].lastSeq[0] >= 0 && m_handleToData[handle].lastSeq[0] <= upperBoundAfter_0_)
		{
			if (seq > m_handleToData[handle].lastSeq[0] && seq < lowerBoundTo_O_)
				return true;
			return  false;
		}
		if (m_handleToData[handle].lastSeq[0] >= lowerBoundTo_O_ && m_handleToData[handle].lastSeq[0] <= 255)
		{
			if (seq < m_handleToData[handle].lastSeq[0] && seq <= upperBoundAfter_0_)
				return true;
			if (seq > m_handleToData[handle].lastSeq[0])
				return true;
			return false;
		}
		return seq > m_handleToData[handle].lastSeq[0];
	}
	return true;
}
void PublishedDataMng::ChangeLastSequence(PublishedDataMng::PublishHandle handle, unsigned char seq)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("ChangeLastSequence() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}

	if (m_handleToData[handle].lastSeq.size() > 0)
	{
		if(m_handleToData[handle].lastSeq.size() == 0)
		{
			LOG_WARN("ChangeLastSequence() with inappropiate parameters _2, handle=" << handle);
			return;
		}
		m_handleToData[handle].lastSeq[0] = seq;
		return;
	}

	m_handleToData[handle].lastSeq.push_back(seq);
}

void PublishedDataMng::ClearLastSequence(PublishedDataMng::PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("ClearLastSequence() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	m_handleToData[handle].lastSeq.clear();
}

unsigned char PublishedDataMng::GetLastSequence(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetLastSequence() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return 0;
	}
	
	if (m_handleToData[handle].lastSeq.size() == 0)
	{
		LOG_WARN("GetLastSequence() with inappropiate parameters _2, handle=" << handle);
		return 0;
	}
	return m_handleToData[handle].lastSeq[0];	
}

//publish error
bool PublishedDataMng::IsPublishError(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsPublishError() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	return m_handleToData[handle].pubDataError;
}
void PublishedDataMng::SetPublishError(PublishHandle handle, bool val)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SetPublishError() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}

	m_handleToData[handle].pubDataError = val;
}

//publish stale_data status 
bool PublishedDataMng::IsStaleData(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	
	//LOG_INFO("is stale data status...");
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsStaleData() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	//LOG_INFO("is stale data status = " << m_handleToData[handle].staleData);
	return m_handleToData[handle].staleData;
}
void PublishedDataMng::SetStaleDataStatus(PublishHandle handle, bool val)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	
	//LOG_INFO("set stale data status...");
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SetStaleDataStatus() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	
	//LOG_INFO("set stale data status = " << val);
	m_handleToData[handle].staleData = val;
	
	if (val == true)
		m_handleToData[handle].freshData = false;
}

bool PublishedDataMng::IsFreshData(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	
	//LOG_INFO("is fresh data status...");
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("IsFreshData() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return false;
	}
	
	//LOG_INFO("is fresh data status = " << m_handleToData[handle].freshData);
	return m_handleToData[handle].freshData;
}
void PublishedDataMng::SetFreshDataStatus(PublishHandle handle, bool val)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	
	//LOG_INFO("set fresh data status...");
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
			!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SetFreshDataStatus() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	
	//LOG_INFO("set fresh data status = " << val);
	m_handleToData[handle].freshData = val;

	if (val == true)
		m_handleToData[handle].staleData = false;
}

//readings
void PublishedDataMng::AddReading(PublishHandle handle, DeviceReading &reading)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("AddReading() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}

	if (m_handleToData[handle].readings.data.size() == 0)
	{
		m_readingHandles.push_front(handle);
		m_handleToData[handle].readings.listI = m_readingHandles.begin();
	}
	m_handleToData[handle].readings.data.push_front(reading);	//because the way it is updated the time in devices table when saving data
	m_readingsNo++;
}
std::deque<DeviceReading>* PublishedDataMng::GetReadings(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()))
	{
		LOG_WARN("GetReadings() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return NULL;
	}

	//assert(m_handleToData[handle].isHandled == true); it might happened to delete a sucriber lease for which there is published data in queue
	return &m_handleToData[handle].readings.data;
}
void PublishedDataMng::ClearReadings(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true); old_way
	
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()))
	{
		LOG_WARN("ClearReadings() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}

	if (m_handleToData[handle].readings.data.size() > 0) //it could be deleted
	{
		m_readingsNo-= m_handleToData[handle].readings.data.size();
		m_handleToData[handle].readings.data.clear();
		if (m_handleToData[handle].readings.listI != m_readingHandles.end())
			m_readingHandles.erase(m_handleToData[handle].readings.listI);
		m_handleToData[handle].readings.listI = m_readingHandles.end();
	}
}
int PublishedDataMng::GetAllReadingsNo()
{
	return m_readingsNo;
}
std::list<PublishedDataMng::PublishHandle> *PublishedDataMng::GetReadingHandles()
{
	return &m_readingHandles;
}


//readings separate flow
bool PublishedDataMng::GetSaveToDBSigFlag()
{
	return m_saveToDBSigFlag;
}
void PublishedDataMng::SetSaveToDBSigFlag(bool val)
{
	m_saveToDBSigFlag = val;
}



//erase
void PublishedDataMng::EraseData(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("EraseData() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}
	m_handleToData[handle].isHandled = false;
	m_handles.push(handle);
}

void PublishedDataMng::SetInterpretInfo(PublishHandle handle, InterpretInfoListT & list)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("SetInterpretInfo() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return;
	}

	m_handleToData[handle].interpret = list;
}
PublishedDataMng::InterpretInfoListT* PublishedDataMng::GetInterpretInfo(PublishHandle handle)
{
	//assert(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size());
	//assert(m_handleToData[handle].isHandled == true);
	if (!(handle >= (PublishHandle)0 && handle < (PublishHandle)m_handleToData.size()) || 
		!(m_handleToData[handle].isHandled == true))
	{
		LOG_WARN("GetInterpretInfo() with inappropiate parameters, handle=" << handle << " in vec.size=" << m_handleToData.size());
		return NULL;
	}

	return &m_handleToData[handle].interpret;
}

}
}
