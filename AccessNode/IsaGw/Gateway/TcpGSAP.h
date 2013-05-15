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
/// @file TcpGSAP.h
/// @author Marcel Ionescu
/// @brief TCP Gateway Service Access Point interface; GSAP serialiser
////////////////////////////////////////////////////////////////////////////////

#ifndef TCPGSAP_H
#define TCPGSAP_H

#include "../../Shared/TcpSocket.h"
#include "../../Shared/TcpSecureSocket.h"
#include "GSAP.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CGSAP
/// @brief Gateway Service Access Point
/// @remarks Usage of structures on receive: validate CRC, then convert to host order
////////////////////////////////////////////////////////////////////////////////
class CTcpGSAP : public CGSAP
{
public:
	
	CTcpGSAP( CTcpSocket * p_pTcpSocket );
	virtual ~CTcpGSAP( void );

	/// Non-blocking read whatever client data is available. When full message
	/// is assembled, unpack parameters, convert to host order and dispatch to proper worker.
	/// Return true is a message was complete and dispatched, false if the message is incomplete
	virtual bool ProcessData( void );

	/// (GW -> User) Pack parameters and send the message to the client.
	/// TAKE CARE: This is low-level method, DO NOT USE: use CGSAP::SendConfirm instead
	virtual bool SendMessage(
		uint32	p_nSessionID,		///< GS_Session_ID
		uint32	p_nTransactionID,	///< GS_Transaction_ID
		uint8	p_ucServiceType,
		uint8*	p_pMsgData,			///< data starting with GS_Status, type-dependent. Does not include data CRC
		uint32	p_dwMsgDataLen  );	///< data len, type-dependent. Does not include data CRC
	
	/// Is current object is valid/usable?
	virtual bool IsValid( void ) { return CGSAP::IsValid() && m_pTcpSocket ? m_pTcpSocket->IsValid() : false; } ;

	/// Log the object content
	virtual void Dump( void );
	virtual const char * Identify( void );

private:

	/// Clear the object, reset all buffers, close the socket, invalidate the connection
	virtual bool clear( void );

	CTcpSocket*	m_pTcpSocket;	///< TCP server socket
	uint32		m_dwReceived;	///< Number of bytes received
	byte		m_ucStatus;		/// state machine
};
enum GSAP_STATE_MACHINE { GSAP_WAIT_HDR, GSAP_WAIT_DATA, GSAP_MSG_READY };

#endif //TCPGSAP_H
