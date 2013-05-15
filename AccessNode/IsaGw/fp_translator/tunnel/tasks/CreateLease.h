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

#ifndef CREATELEASE_H_
#define CREATELEASE_H_

#include <string>

#include "../interfaces/TaskRun.h"

class CTunnelIO;


namespace tunnel {
namespace comm {

class RequestProcessor;

class CreateLease: public ITaskRun
{
public:
	CreateLease(CTunnelIO *pTunnelIO, RequestProcessor*	reqProcessorPtr);

private:
	CTunnelIO		*m_pTunnelIO;
	RequestProcessor*		m_reqProcessorPtr;

public:
	virtual void operator()(){DoCreateLease();}

private:
	void DoCreateLease();

};


}
}

#endif
