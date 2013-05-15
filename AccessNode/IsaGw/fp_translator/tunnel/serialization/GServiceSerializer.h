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

#ifndef GSERVICESERIALIZER_H_
#define GSERVICESERIALIZER_H_

#include "../services/IGServiceVisitor.h"
#include "../gateway/GeneralPacket.h"
#include "SerializationException.h"

namespace tunnel {
namespace comm {

class GServiceSerializer : public IGServiceVisitor
{
	
public:
	void Serialize(AbstractGService& request, gateway::GeneralPacket& packet);

private:
	//IGServiceVisitor
	void Visit(GSession& session);
	void Visit(GLease& lease);
		
	void Visit(GClientServer_C& client);
	void Visit(GClientServer_S& server);	

private:
	gateway::GeneralPacket *packet;

};

}//namespace comm
}//namespace tunnel

#endif
