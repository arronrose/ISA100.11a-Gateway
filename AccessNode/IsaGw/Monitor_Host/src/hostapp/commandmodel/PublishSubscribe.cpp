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

#include "PublishSubscribe.h"
#include "IGServiceVisitor.h"

#include <stdio.h> 

namespace nisa100 {
namespace hostapp {

const std::string PublishSubscribe::ToString() const
{
	return boost::str(
	    boost::format("PublishSubscribe[Publisher<MAC=%1%, Channel=%2%>  Subscriber<MAC=%3%, Channel=%4%>]")
	        % PublisherMAC.ToString() % (int) PublisherChannel.channelNumber % SubscriberMAC.ToString() % (int) SubscriberChannel.channelNumber);
}

bool PublishSubscribe::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

const std::string Publish::ToString() const
{
	return std::string("Publish.") + (ps
	  ? ps->ToString()
	  : "");
}

const std::string Subscribe::ToString() const
{
	return std::string("Subscribe.") + (ps
	  ? ps->ToString()
	  : "");
}

//added by Cristian.Guef
PublishIndication::PublishIndication(boost::uint32_t deviceID,
						 boost::uint16_t mappedTLSAP_ID,
						 boost::uint16_t mappedPublisherConcentrator_ID,
						 boost::uint32_t SubscriberLowThreshold,
						 boost::uint32_t SubscriberHighThreshold) :
	PublisherDeviceID(deviceID),
	m_isFirstTime(true),
	m_publishHandle(-1/*invalid data*/),
	m_MappedPublisherTLSAP_ID(mappedTLSAP_ID), 
	m_MappedPublisherConcentrator_ID(mappedPublisherConcentrator_ID),
	m_SubscriberLowThreshold(SubscriberLowThreshold),
	m_SubscriberHighThreshold(SubscriberHighThreshold)
{
}

//added by Cristian.Guef
PublishIndication::PublishIndication(const PublishIndication& rhs) :
	AbstractGService(rhs), PublisherDeviceID(rhs.PublisherDeviceID),
	m_publishHandle(rhs.m_publishHandle),
	m_MappedPublisherTLSAP_ID(rhs.m_MappedPublisherTLSAP_ID),
	m_MappedPublisherConcentrator_ID(rhs.m_MappedPublisherConcentrator_ID),
	m_SubscriberLowThreshold(rhs.m_SubscriberLowThreshold),
	m_SubscriberHighThreshold(rhs.m_SubscriberHighThreshold),
	m_IndicationType(rhs.m_IndicationType)
{
}

bool PublishIndication::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

const std::string PublishIndication::ToString() const
{	char szTmp[256];
	const char *pType;
	switch (m_IndicationType)
	{	case PublishIndication::Publish_Indication:		 pType = "PUBLISH     "; break;
		case PublishIndication::Subscriber_Timer:		 pType = "SUBSCRIB_TMR"; break;
		case PublishIndication::Watchdog_Timer:	default: pType = "WATCHDOG_TMR"; break;
	}

	sprintf (szTmp, "%s IND(DeviceID %2u) ver:%2u, seq:%3u, size:%u", pType, PublisherDeviceID,
		(unsigned) m_ContentVersion,  (unsigned) m_FreshSeqNo, (unsigned) m_PDataLen);
	
	return std::string(szTmp);
}

AbstractGServicePtr PublishIndication::Clone() const
{
	return AbstractGServicePtr(new PublishIndication(*this));
}

} //namspace hostapp
} //namspace nisa100
