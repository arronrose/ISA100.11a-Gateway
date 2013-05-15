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



///////////////////////////////////////////////////////////////////////////////
//! @brief Opens a link.
//! @retval 0 failure.
//! @retval !0 success.
//! @param [in] device Can be a hostname or a path to the serial device.
//! @param [in] flags  Can be port number or the baud rate for serial link.
///////////////////////////////////////////////////////////////////////////////
template<class BaseClass>
int EncaseLayer<BaseClass>::OpenLink( const char* p_szDev, uint32_t p_uiDstPort, uint32_t p_uiLocalPort/*=0*/, bool p_bEscMsg/*=false*/ )
{
	m_bEscMsg = p_bEscMsg ;

	int rv = BaseClass::OpenLink( p_szDev, p_uiDstPort, p_uiLocalPort);
	if ( !rv )
	{
		ERR("EncaseLayer.OpenLink: Failed");
		return false ;
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
template<class BaseClass>
void EncaseLayer<BaseClass>::SetOffsets( uint16_t p_uiHeadSpare , uint16_t p_uiTailSpare)
{
	m_uiHeadSpare = p_uiHeadSpare ;
	m_uiTailSpare = p_uiTailSpare ;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
template<class BaseClass>
ProtocolPacket* EncaseLayer<BaseClass>::Read( int tout/*=1000*/ )
{
	int rb =0;
	uint8_t * buf;
	uint16_t  nPkLen;
	bool rv ;

	if( 0 >= (rb=BaseClass::GetMsgLen(tout)) )
		return 0 ;

	buf = new uint8_t[m_uiHeadSpare+rb +m_uiTailSpare ] ;

	if( 0 >= (rb=BaseClass::Read(buf+m_uiHeadSpare, rb)) )
	{
		delete[] buf ;
		return 0;
	}
	nPkLen = rb ;
	ProtocolPacket* pkt = new ProtocolPacket( ) ;
	if ( m_bEscMsg )
	{
		nPkLen = m_oParse.UnescapeMsg(buf+m_uiHeadSpare, rb ) ;
	}
	rv = pkt->SetIn( buf, nPkLen, m_uiHeadSpare, m_uiTailSpare) ;
	if( ! rv )
	{
		delete pkt ;
		delete[] buf ;
		return 0 ;
	}
	return pkt;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
template<class BaseClass>
int EncaseLayer<BaseClass>::Write( ProtocolPacket* pkt )
{
	if ( ! m_bEscMsg )
		return BaseClass::Write( pkt->Data(), pkt->DataLen() );

	uint8_t* emsg = new uint8_t [2*pkt->DataLen()] ;
	uint16_t emsgSz = m_oParse.EscapeMsg( pkt->Data(), pkt->DataLen(), emsg);
	int rv = BaseClass::Write( emsg, emsgSz );
	delete[] emsg ;
	return rv ;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
template<class BaseClass>
ProtocolPacket* EncaseLayer<BaseClass>::Write( const uint8_t* p_rguiMsg, uint16_t p_uiMsgSz )
{
	ProtocolPacket* pkt = new ProtocolPacket( ) ;
	pkt->AllocateCopy( p_rguiMsg, p_uiMsgSz);
	return pkt ;
}

template <class BaseClass>
int EncaseLayer<BaseClass>::SendTo( const char *p_szIP, int p_iRemotePort, const uint8_t *p_rguiMsg, uint16_t p_uiMsgSz)
{
	return BaseClass::m_oSock.SendTo( (const char *)p_szIP, p_iRemotePort, (const char*)p_rguiMsg, p_uiMsgSz );
}
