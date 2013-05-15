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

#ifndef IGSERVICEVISITOR_H_
#define IGSERVICEVISITOR_H_

#include "AbstractGService.h"


#include "GSession.h"
#include "GLease.h"
#include "GClientServer_C.h"
#include "GClientServer_S.h"


namespace tunnel {
namespace comm {

/**
 * @brief The Visitor class.
 */
class IGServiceVisitor
{
public:
	virtual ~IGServiceVisitor()
	{
	}

	virtual void Visit(GSession& session) = 0;
	virtual void Visit(GLease& lease) = 0;
		
	virtual void Visit(GClientServer_C& client) = 0;
	virtual void Visit(GClientServer_S& server) = 0;
};


}  // namespace comm
}  // namespace tunnel

#endif /* IGSERVICEVISITOR_H_ */
