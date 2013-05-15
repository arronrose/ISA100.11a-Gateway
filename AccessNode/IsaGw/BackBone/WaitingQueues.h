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

#ifndef WaitingQueues_h__
#define WaitingQueues_h__

#include <boost/shared_ptr.hpp> 
#include <list>


class ProtocolPacket;
class CMsgWaitATT
{
public:
	enum TMsgDirection { DIR_UNK = 0, DIR_TR_TO, DIR_TR_FROM};
	typedef boost::shared_ptr<CMsgWaitATT> Ptr; 

public:
	CMsgWaitATT();
	~CMsgWaitATT(); 	

	void Disown(); 
	void Unload();

	void Load(ProtocolPacket* p_pPkt, TMsgDirection p_nDirection);


	
public:
	int					m_nRecvTime;

	TMsgDirection		m_nDirection;
	ProtocolPacket*		m_pPkt;
	
};

typedef std::list<CMsgWaitATT::Ptr>		CMsgWaitATTList;




#endif // WaitingQueues_h__
