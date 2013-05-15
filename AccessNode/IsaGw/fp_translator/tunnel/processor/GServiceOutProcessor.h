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

#ifndef GSERVICEOUTPROCESSOR_H_
#define GSERVICEOUTPROCESSOR_H_


#include "../services/IGServiceVisitor.h"
//#include <boost/function.hpp> //for callback

class CTunnelIO;

namespace tunnel {
namespace comm {

class RequestProcessor;

class GServiceOutProcessor : public IGServiceVisitor
{

public:
	void Process(AbstractGServicePtr servicePtr, CTunnelIO* pTunnelIO, 
						RequestProcessor *pReqProcessor);

private:
	AbstractGServicePtr	m_servicePtr;
	CTunnelIO* m_pTunnelIO;
	RequestProcessor *m_pReqProcessor;
	

	//IGServiceVisitor
	void Visit(GSession& session);
	void Visit(GLease& lease);
		
	void Visit(GClientServer_C& client);
	void Visit(GClientServer_S& server);
};

}
}

#endif
