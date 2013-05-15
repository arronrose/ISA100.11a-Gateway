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
/// @file GSAP.h
/// @author Marcel Ionescu
/// @brief Gateway Service Access Point interface. Handle a single client
////////////////////////////////////////////////////////////////////////////////

#ifndef GSAP_H
#define GSAP_H

#include "../../Shared/MicroSec.h"
#include "../ISA100/typedef.h"
#include "crc32c.h"
#include <arpa/inet.h> //ntoh/hton
#include <list>
#include "MsgTracker.h"

/// G_Status for services not yet implemented/unknown service type
#define G_STATUS_UNIMPLEMENTED 0xFF

/// TGSAPHdr::m_ucVersion
#define PROTO_VERSION_GSAP  3
#define PROTO_VERSION_YGSAP 4

/// The GSAP service types from the Gateway-Host application protocol (isa100g-ADD.doc)
/// @see GwUtil.cpp getGSAPServiceName()
#define SESSION_MANAGEMENT			0x01
#define LEASE_MANAGEMENT			0x02
#define DEVICE_LIST_REPORT			0x03
#define TOPOLOGY_REPORT				0x04
#define SCHEDULE_REPORT				0x05
#define DEVICE_HEALTH_REPORT		0x06
#define NEIGHBOR_HEALTH_REPORT		0x07
#define NETWORK_HEALTH_REPORT		0x08
#define TIME_SERVICE				0x09
#define CLIENT_SERVER				0x0A
//PUBLISH/SUBSCRIBE category start
#define PUBLISH						0x0B
#define SUBSCRIBE					0x0C
#define PUBLISH_TIMER				0x0D	///< Not used
#define SUBSCRIBE_TIMER				0x0E
#define WATCHDOG_TIMER				0x0F
//PUBLISH/SUBSCRIBE category END
#define BULK_TRANSFER_OPEN			0x10
#define BULK_TRANSFER_TRANSFER		0x11
#define BULK_TRANSFER_CLOSE			0x12
#define ALERT_SUBSCRIBE				0x13
#define ALERT_NOTIFY				0x14
#define GATEWAY_CONFIGURATION_READ	0x15
#define GATEWAY_CONFIGURATION_WRITE	0x16
#define DEVICE_CONFIGURATION_READ	0x17
#define DEVICE_CONFIGURATION_WRITE	0x18
//0x19
//0x1A
//0x1B
//0x1C
//0x1D
//0x1E
//0x1F
//0x20
/// Custom GSAP
#define NETWORK_RESOURCE_REPORT		0x21

#define REQUEST(_svc_type_)		(((_svc_type_) & 0x3F) | 0x00)
#define INDICATION(_svc_type_)	(((_svc_type_) & 0x3F) | 0x40)
#define CONFIRM(_svc_type_)		(((_svc_type_) & 0x3F) | 0x80)
#define RESPONSE(_svc_type_)	(((_svc_type_) & 0x3F) | 0xC0)

#define IS_REQUEST(_svc_type_)		(((_svc_type_) & 0xC0) == 0x00)
#define IS_INDICATION(_svc_type_)	(((_svc_type_) & 0xC0) == 0x40)
#define IS_CONFIRM(_svc_type_)		(((_svc_type_) & 0xC0) == 0x80)
#define IS_RESPONSE(_svc_type_)		(((_svc_type_) & 0xC0) == 0xC0)

#define MAX_SIZE_FAIL_RES	SYS_REP_FAIL_RES_MAX_SIZE
#define STATUS_IDX		0


inline bool IsYGSAP( uint8_t p_ucVersion ) { return (PROTO_VERSION_YGSAP == p_ucVersion); };

////////////////////////////////////////////////////////////////////////////////
/// @class CGSAP
/// @brief Gateway Service Access Point. Base class for all protocol translators.
////////////////////////////////////////////////////////////////////////////////
class CGSAP
{
	////////////////////////////////////////////////////////////////////////////
	/// Data structures - client interface
	////////////////////////////////////////////////////////////////////////////
	/// @brief Session Management REQ data, except data CRC
	typedef struct	{
		long 	m_nPeriod;		///< GS_Session_Period		NETWORK ORDER
		short 	m_shNetworkId;	///< GS_Network_ID			NETWORK ORDER
		void NTOH( void ){ m_nPeriod = ntohl(m_nPeriod); m_shNetworkId = ntohs(m_shNetworkId);};
	}__attribute__ ((packed)) TSessionMgmtReqData;
	
	/// @brief Session Management RSP data, except data CRC
	typedef struct	{
		byte 	m_ucStatus;		///< GS_Status
		long 	m_nPeriod;		///< GS_Session_Period		NETWORK ORDER
		void HTON( void ){ m_nPeriod = htonl(m_nPeriod); };
	}__attribute__ ((packed)) TSessionMgmtRspData;

public:
		/// @brief Data header, same on REQ/RSP
	typedef struct{	/// All data is NETWORK ORDER. First thing: convert to host order
		uint8	m_ucVersion;		///< protocol version number
		uint8	m_ucServiceType;	///< the service type of the packet
		uint32	m_nSessionID;		///< GS_Session_ID				NETWORK ORDER
		uint32	m_unTransactionID;	///< GS_Transaction_ID			NETWORK ORDER
		uint32	m_nDataSize;		///< length of the data block	NETWORK ORDER
		uint32	m_nCrc;				///< CRC-32?					NETWORK ORDER
		void NTOH( void ){ m_nSessionID = ntohl(m_nSessionID); m_unTransactionID = ntohl(m_unTransactionID); m_nDataSize = ntohl(m_nDataSize);};
		void HTON( void ){ m_nSessionID = htonl(m_nSessionID); m_unTransactionID = htonl(m_unTransactionID); m_nDataSize = htonl(m_nDataSize);};
		inline bool IsYGSAP( void ) const { return (PROTO_VERSION_YGSAP == m_ucVersion); };
	}__attribute__ ((packed)) TGSAPHdr;
	/// here: Data block with size m_nDataSize
	
	////////////////////////////////////////////////////////////////////////////
	/// End Data structures - client interface
	////////////////////////////////////////////////////////////////////////////

	CGSAP( void );
	virtual ~CGSAP( void );

	/// Non-blocking read whatever client data is available. When full message
	/// is assembled, unpack parameters, convert to host order and dispatch to proper worker.
	/// Return true is a message was complete and dispatched, false if the message is incomplete
	virtual bool ProcessData( void ) = 0;

	/// (GW -> User) Pack parameters and send the message to the client.
	/// TAKE CARE: This is low-level method, DO NOT USE: use CGSAP::SendConfirm instead
	virtual bool SendMessage(	uint32	p_nSessionID,			///< GS_Session_ID
								uint32	p_nTransactionID,		///< GS_Transaction_ID
								uint8	p_ucServiceType,
								uint8*	p_pMsgData,				///< data starting from GS_Status, type-dependent. Does not include data CRC
								uint32	p_dwMsgDataLen ) = 0;	///< data len, type-dependent. Does not include data CRC

	/// (GW -> User) Send Confirm to user, remove from Tracker
	virtual bool SendConfirm(	uint32	p_nSessionID,		///< GS_Session_ID
								uint32	p_nTransactionID,	///< GS_Transaction_ID
								uint8	p_ucServiceType,
								uint8*	p_pMsgData,			///< data starting with GS_Status, type-dependent. Does not include data CRC
								uint32	p_dwMsgDataLen,		///< data len, type-dependent. Does not include data CRC
								bool	p_bRemoveFromTracker = true);

	/// (GW -> User) Send Indication to user
	inline virtual bool SendIndication( uint32	p_nSessionID,		///< GS_Session_ID
								uint32	p_nTransactionID,	///< GS_Transaction_ID
								uint8	p_ucServiceType,
								uint8*	p_pMsgData,			///< data starting with GS_Status, type-dependent. Does not include data CRC
								uint32	p_dwMsgDataLen  )	///< data len, type-dependent. Does not include data CRC
		{ return SendMessage( p_nSessionID, p_nTransactionID, INDICATION(p_ucServiceType), p_pMsgData, p_dwMsgDataLen ); };


	/// Is current object is valid/usable?
	virtual bool IsValid( void ) { return m_bIsValid; };

		/// Search a session to internal list, NOT from session manager.
	///Return false if not found: session is not associated with this channel
	virtual bool HasSession( uint32 p_unSessionID );

	/// Delete a session from internal list, NOT from session manager. Return false if not found.
	virtual bool DelSession( uint32 p_unSessionID ) ;
	
	/// Log the object content
	virtual void Dump( void );

	/// Return a static string formatted with last Tx/Rx (number of seconds in the past, or never)
	const char * Activity( void );
protected:
	/// Clear the object, reset all buffers, invalidate the connection, return false
	virtual bool clear( void );

	///Unpack parameters, dispatch the message to proper worker. Direction: Client -> GW (request)
	/// Must be called by ProcessData
	void dispatchUserReq( void );

	/// Return a pointer with CGSAP identification
	virtual const char * Identify( void ) = 0;

	/// Compute crc for whole p_dwDataLen
	inline uint32 crc( byte* p_pData, uint32 p_dwDataLen ) { return htonl(GenerateCRC32C(p_pData, p_dwDataLen)); };
		
	CMicroSec	m_uSec;	///< Keep track of GSAP lifetime
	time_t		m_tLastRx;	///< Support for CGSAP expiration based on inactivity - protection against DoS
	time_t		m_tLastTx;	///< Support for CGSAP expiration based on inactivity - protection against DoS
	
	uint32		m_dwRequestCount;		///< Debug info: count number of requests transiting this GSAP
	uint32		m_dwIndicationCount;	///< Debug info: count number of indications transiting this GSAP
	uint32		m_dwConfirmCount;		///< Debug info: count number of confirms transiting this GSAP
	uint32		m_dwResponseCount;		///< Debug info: count number of responses transiting this GSAP
	
	uint32		m_dwTotalRxSize;	///< Debug info: total number of RX bytes transiting this GSAP
	uint32		m_dwTotalTxSize;	///< Debug info: total number of TX bytes transiting this GSAP

	typedef std::list<uint32> TSessionLst;
	TGSAPHdr	m_stReqHeader;	///< Message header (last header received, may be partial)
	byte*		m_aReqData;		///< Message data (last data received, may be partial)
	TSessionLst	m_lstSessions;	///< List with sessions open on this channel
	bool 		m_bIsValid;

private:
	/// Process a request for to session management object, send answer back. Direction: Client -> GW (request)
	void processSessionManagementReq( void );

	/// Validate crc, return true for ok.
	/// Crc invalid: close connection, log error, return false.
	/// CRC is always in the last 4 bytes of data
	/// Expect CRC to be network order. Expect p_dwDataLen > 4. Caller must ensure both.
	bool checkCrc( byte* p_pData, uint32 p_dwDataLen  );

	/// The REQUEST has valid Session ID. Return true if valid/false if invalid, send a message to client if invalid
	bool reqSessionIsValid( void );
		
	/// Add a session to internal list, NOT from session manager. Return false if not found.
	void addSession( uint32 p_unSessionID ) { m_lstSessions.push_back( p_unSessionID ); }
};


#endif //GSAP_H
