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

////////////////////////////////////////////////////////////////////////////////
/// @file AlertSvc.h
/// @author Marcel Ionescu
/// @brief Alert service - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef ALERT_SERVICE_H
#define ALERT_SERVICE_H

#include <map>
#include <algorithm>
#include <boost/shared_ptr.hpp>

#include "../ISA100/porting.h"
#include "Service.h"


/// GsapAlertStatus
#define ALERT_SUCCESS			0	// Success
#define ALERT_INVALID_CATEGORY	1	// Failure; Invalid category
#define ALERT_INVALID_ALERT		2	// Failure; Invalid individual alert
#define ALERT_FAIL_OTHER		3	// Failure; other

#define PENDING_ALERT_ACK_TIMEOUT	60

typedef struct
{
	uint8 	m_ucSubscType;
	uint8	m_ucSubscribe;
	uint8	m_ucEnable;
	
	uint8	m_ucCategory;
}__attribute__ ((packed)) TSubscCateg;

typedef struct
{
	uint8 	m_ucSubscType;
	uint8  	m_ucSubscribe;
	uint8  	m_ucEnable;

	uint8 	m_aucNetAddress[16];
 	uint16 	m_ushEndpointPort;
	uint16 	m_ushObjId;
	uint8 	m_u8AlertType;

	void NTOH() { m_ushEndpointPort=ntohs(m_ushEndpointPort);m_ushObjId=ntohs(m_ushObjId);}
}__attribute__ ((packed)) TSubscNetAddr;

typedef struct
{
	uint32	m_unLeaseID;			///< 		NETWORK ORDER
	void NTOH() { m_unLeaseID=ntohl(m_unLeaseID);}
}__attribute__ ((packed)) TAlertSubscriptionReqInfo;

typedef struct
{
	uint8 m_aucNetAddress[16];
	uint16 m_ushEndpointPort;
	uint16 m_ushEndpointObjId;
	struct
	{
		uint32_t m_u32Seconds;
		uint16_t m_u16Fract;
	}__attribute__ ((packed));

	uint8	m_ucClass;
	uint8	m_ucDirection;
	uint8	m_ucCategory;
	uint8	m_u8Type;
	uint8	m_u8Priority;
	uint16	m_ushDataLen;

	
	void HTON( void )	
	{
		m_ushEndpointPort	=	htons(m_ushEndpointPort); 
		m_ushEndpointObjId	=	htons(m_ushEndpointObjId); 
		m_ushDataLen		=	htons(m_ushDataLen);
		m_u32Seconds		=	htonl(m_u32Seconds);
		m_u16Fract			=	htons(m_u16Fract);
	}
}__attribute__ ((packed)) TAlertNotification;

////////////////////////////////////////////////////////////////////////////////
/// @class CAlertService
/// @brief Alert Service
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CAlertService: public CService
{
public:
	enum { SubscriptionTypeCategory = 1, SubscriptionTypeAddress = 2 };
	/// @brief std::pair< Category, LeaseID > TSubscCategLst
	typedef std::list<std::pair< uint8, uint32 > > TSubscCategLst;
	struct TNetAddrId
	{
		uint8_t		m_aucNetAddress[16];
		uint16		m_ushEndpointPort;
		uint16		m_ushEndpointObjId;
		uint16		m_u8Type;

		uint32		m_unLeaseID;
	};
	

	/// @brief std::pair< NetwordAddress, LeaseID > TSubscNetAddrLst
	typedef std::list<TNetAddrId> TSubscNetAddrLst;


	class TPendingAlertAck 
	{
	public:
		typedef boost::shared_ptr<TPendingAlertAck> Ptr;

	public:
		ALERT_ACK_SRVC	m_stAlertAckSrvc;

		uint8_t			m_u8Dest[16];
		uint16_t		m_u16PortDst;
		uint16_t		m_u16PortSrc;
		uint8_t			m_u8Priority;
		int				m_nEndOfLife;
	};

	typedef std::list<TPendingAlertAck::Ptr> CPendingAlertAckList;
	
public:
	CAlertService( const char * p_szName ) :CService(p_szName){};
	virtual void OnLeaseDelete( lease* p_pLease );	
	/// Return true if the service is able to handle the service type received as parameter
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;
	
	bool ProcessAlertAPDU( APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pReq );

	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	
	/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
	virtual bool ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );
	
	/// (ISA -> GW) Process a ISA100 timeout. Call from CInterfaceObjMgr::DispatchISATimeout
	virtual bool ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq );

	void ContractNotification(uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract);
	void DropExpired ();
	void Dump( void );
private:
	bool sendAlertNotification (APDU_IDTF* p_pAPDUIdtf, ALERT_REP_SRVC* p_pAlert, uint32 p_u32LeaseId );
	
	/// Send confirm on connection GSAP, determined from session id, if connection is valid
	bool confirm( CGSAP::TGSAPHdr * p_pHdr, uint8_t p_unYGSAPStatus, uint8_t p_unGSAPStatus, uint8_t * p_pConfirmData, size_t p_unConfirmDataSize );

	TSubscNetAddr * translateYAlertReq( CGSAP::TGSAPHdr * p_pHdr, uint8_t * p_pData  ) const;

	/// (ISA -> GW) Dump alert details
	void dumpSMAlertDetails( ALERT_REP_SRVC* pAlert );
	
	TSubscCategLst		m_lstSubscCateg;
	TSubscNetAddrLst	m_lstSubscNetAddr;
	CPendingAlertAckList	m_oPendingAlertAckList;
};

inline bool operator == (const CAlertService::TNetAddrId& p_rFirst, const CAlertService::TNetAddrId&  p_rSecond )
{
	return		p_rFirst.m_unLeaseID		== p_rSecond.m_unLeaseID 
			&&	p_rFirst.m_ushEndpointPort	== p_rSecond.m_ushEndpointPort
			&&	p_rFirst.m_ushEndpointObjId == p_rSecond.m_ushEndpointObjId
			&&	p_rFirst.m_u8Type			== p_rSecond.m_u8Type
		&& std::equal ( p_rFirst.m_aucNetAddress, p_rFirst.m_aucNetAddress + sizeof(p_rFirst.m_aucNetAddress), p_rSecond.m_aucNetAddress);
}


#endif //ALERT_SERVICE_H
