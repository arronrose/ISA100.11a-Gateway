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

#ifndef PUBLISHSUBSCRIBE_H_
#define PUBLISHSUBSCRIBE_H_

#include "AbstractGService.h"
#include "../model/MAC.h"
#include "../model/Device.h"

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include <nlib/datetime.h>

namespace nisa100 {
namespace hostapp {

class PublishSubscribe;
typedef boost::shared_ptr<PublishSubscribe> PublishSubscribePtr;

/**
 * PublishSubscribe command.
 */
class PublishSubscribe : public AbstractGService
{
public:
	boost::uint16_t Period; //in seconds
	boost::uint16_t Phase; // in 10ms

	MAC PublisherMAC;
	DeviceChannel PublisherChannel;

	MAC SubscriberMAC;
	DeviceChannel SubscriberChannel;
	boost::uint32_t SubscriberLowThreshold;
	boost::uint32_t SubscriberHighThreshold;

	//internal data
	bool isLocalLoop;
	int PublisherDeviceID;
	Command::ResponseStatus PublisherStatus;

	int SubscriberDeviceID;
	Command::ResponseStatus SubscriberStatus;

	//added by Cristian.Guef
	unsigned char StaleLimit;

	//added by Cristian.Guef - for new subscribe leases
	PublisherConf::COChannelListT	*pcoChannelList;
	PublisherConf::ChannelIndexT	*pcoChannelIndex;
	unsigned char					dataContentVer;
	unsigned char					interfaceType;


public:
	const std::string ToString() const;

	//AbstractGService
private:
	bool Accept(IGServiceVisitor& visitor);
};

class Publish
{
public:
	PublishSubscribePtr ps;

	const std::string ToString() const;

	//added by Cristian.Guef
	enum PublishStates
	{
		PublishWritePubObjIDAttrID = 1,
		PublishWritePubEndpoint,
		PublishDoLeaseSubscriber
	} m_currentPublishState;

	//added by Cristian.Guef
	IPv6 m_handedSubscriberIP;	//needed for setting PubEndpointIP

	//added by Cristian.Guef
	IPv6 m_handedPublisherIP;  //needed for creation "subscribe lease"
	//client-server contract for publish -> publish->ContractID

	//added by Cristian.Guef
	unsigned int	m_ObtainedLeaseID_S;

	//added by Cristian.Guef - for error checking only
	unsigned int	m_ObtainedLeasePeriod_S;
};

class Subscribe
{
public:
	PublishSubscribePtr ps;

	const std::string ToString() const;

	//added by Cristian.Guef
	enum SubscribeStates
	{
		SubscribeReadPubEndpoint = 1,
		SubscribeReadPubObjAttrSizeArray,
		SubscribeAddPubObjIDAttrID,
		SubscribeIncrementNumItemsSubscribing,
		SubscribeDoLeaseSubscriber,
		SubscribeWriteNumItemsSubscribing_with_one,
		SubscribeWriteSubEndpoint
	} m_currentSubscribeState;


	//added by Cristian.Guef
	IPv6			m_handedPublisherIP;
	unsigned int	m_handedPublisherContractID_CS;

	//added by Cristian.Guef
	IPv6			m_handedSubscriberIP;
	//client-server contract for subscribe -> subscribe->ContractID
	

	//added by Cristian.Guef
	struct Endpoint
	{
		IPv6			EndpointIP;
		unsigned short	TLSAP_ID;
		unsigned short	RemoteObjID;
		unsigned char	StaleDataLimit;
		short			DataPublicationPeriod;
		unsigned short	PublicationPhase;
		unsigned char	PublishAutoRetransmit;
		unsigned char	PS_ConfigStatus;
	} m_ReadEndpoint;
	

	//added by Cristian.Guef
	struct ObjAttrSize
	{
		unsigned short ObjID;
		unsigned short AttrID;
		unsigned short AttrIndex;
		unsigned short Size;
		unsigned char  withStatus;

		ObjAttrSize():ObjID(0), AttrID(0), AttrIndex(0), Size(0)
		{
		}
		
	};
	std::vector<ObjAttrSize> m_vecReadObjAttrSize;

	//added by Cristian.Guef
	unsigned int	m_ObtainedLeaseID_S;
	unsigned int	m_ObtainedLeasePeriod_S; //for error checking only

};


class PublishIndication;
typedef boost::shared_ptr<PublishIndication> PublishIndicationPtr;

class PublishIndication : public AbstractGService
{
public:
	struct Value
	{
		boost::uint8_t PublisherAttributeID;
		boost::uint8_t SubscriberAttributeID;

		boost::uint8_t SequenceNo;
		boost::uint8_t Status;
		ChannelValue Value_;
	};
	typedef std::vector<Value> ValuesList;

public:
	const boost::uint32_t PublisherDeviceID;
	
	//added
	bool m_isFirstTime;

	//added by Cristian.Guef
	int	m_publishHandle;	

	//added by Cristian.Guef
	const boost::uint16_t m_MappedPublisherTLSAP_ID;
	const boost::uint16_t m_MappedPublisherConcentrator_ID;
	
	//added by Cristian.Guef
	const boost::uint32_t m_SubscriberLowThreshold;
	const boost::uint32_t m_SubscriberHighThreshold;

	//added by Cristian.Guef - this is published data for us (from now on)
	unsigned char						m_ContentVersion;
	unsigned char						m_FreshSeqNo;
	unsigned short						m_PDataLen;
	std::basic_string<unsigned char>	m_DataBuff;


	//no need
	//nlib::DateTime ReadingTime;
	//short milisec;
	//added
	struct timeval tv;
	
	
	ValuesList values;

	/* commented by Cristian.Guef
	PublishIndication(boost::uint32_t deviceID);
	*/

	//added by Cristian.Guef
	PublishIndication(boost::uint32_t deviceID, 
		boost::uint16_t mappedTLSAP_ID,
		boost::uint16_t mappedPublisherConcentrator_ID,
		boost::uint32_t SubscriberLowThreshold,
		boost::uint32_t SubscriberHighThreshold);

	PublishIndication(const PublishIndication& rhs);

	const std::string ToString() const;

	//added by Cristian.Guef
	//there could be more indications types: publish_indication, watchdog_timer, subscriber_timer
	enum IndicationTypes
	{
		Publish_Indication = 1,
		Watchdog_Timer,
		Subscriber_Timer
	} m_IndicationType;


	//AbstractGService
private:
	bool Accept(IGServiceVisitor& visitor);
	AbstractGServicePtr Clone() const;
};

} //namspace hostapp
} //namspace nisa100

#endif /*PUBLISHSUBSCRIBE_H_*/
