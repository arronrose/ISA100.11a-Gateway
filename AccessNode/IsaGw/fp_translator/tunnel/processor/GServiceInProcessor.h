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

#ifndef GSERVICEINPROCESSOR_H_
#define GSERVICEINPROCESSOR_H_


#include "../services/IGServiceVisitor.h"

class CTunnelIO;

namespace tunnel {
namespace comm {

class GServiceInProcessor : public IGServiceVisitor
{
	
public:
	void Process(AbstractGService& response, CTunnelIO* pTunnelIO);

private:
	CTunnelIO* m_pTunnelIO;

	//IGServiceVisitor
	void Visit(GSession& session);
	void Visit(GLease& lease);
		
	void Visit(GClientServer_C& client);
	void Visit(GClientServer_S& server);
	
};

} //namespace comm
} //namespace tunnel

#endif
