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

#ifndef TunnelObject_h__
#define TunnelObject_h__

#include <stdint.h>
#include <vector>
#include "../../Shared/EasyBuffer.h"

class CTunnelObject : public CIsa100Object
{
public:
	enum {STATUS_NOT_CONFIG = 0, STATUS_CONFIG = 1, STATUS_CONFIG_FAILED = 2};
	enum {FLOW_TYPE_2PART = 0, FLOW_TYPE_4PART = 1, FLOW_TYPE_PUBLISH = 2, FLOW_TYPE_SUBSCRIBE = 3 };
public:
	CTunnelObject(int p_nUapId, int p_nObjectId): CIsa100Object(p_nUapId, p_nObjectId, CIsa100Object::OBJECT_TYPE_ID_TUNNEL)
		,m_u8MaxPeerTunnels(1)
	{
		m_u8Status = STATUS_NOT_CONFIG;
		memset(m_pu8ForeignSourceAddress, 0, sizeof(m_pu8ForeignSourceAddress));
		memset(m_pu8ForeignDestinationAddress, 0, sizeof(m_pu8ForeignDestinationAddress));
	}
	~CTunnelObject() {}



public:
	uint8_t		m_u8Protocol;
	uint8_t		m_u8Status;
	uint8_t		m_u8FlowType;

	uint8_t		m_u8UpdatePolicy;
	int16_t		m_i16Period;
	uint16_t	m_u16Phase;
	uint8_t		m_u8StaleLimit;
	uint8_t		m_u8MaxPeerTunnels;
	uint8_t		m_u8NumPeerTunnels;

	uint8_t		m_pu8ForeignSourceAddress[16];
	uint8_t		m_pu8ForeignDestinationAddress[16];


	std::vector<uint8_t> m_oConnectionInfo;
	std::vector<uint8_t> m_oTransactionInfo;
};

#endif // TunnelObject_h__
