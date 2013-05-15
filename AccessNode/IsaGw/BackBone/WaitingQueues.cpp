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

#include "WaitingQueues.h"
#include "../../Shared/ProtocolPacket.h"


CMsgWaitATT::CMsgWaitATT() 
{
	m_nRecvTime = 0;
	m_pPkt = NULL;
	m_nDirection = DIR_UNK;
}

CMsgWaitATT::~CMsgWaitATT() 
{
	Unload();
}

void CMsgWaitATT::Unload()
{
	m_nRecvTime = 0;

	delete m_pPkt;
	m_pPkt = NULL;
	m_nDirection = DIR_UNK;
}
void CMsgWaitATT::Disown()
{
	m_nRecvTime = 0;
	m_pPkt = NULL;
	m_nDirection = DIR_UNK;
}


void CMsgWaitATT::Load(ProtocolPacket* p_pPkt, TMsgDirection p_nDirection)
{
	Unload();

	m_pPkt = p_pPkt;
	
	m_nDirection = p_nDirection;
	m_nRecvTime = time(NULL);	
}

