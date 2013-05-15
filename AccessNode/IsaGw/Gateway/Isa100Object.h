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

#ifndef Isa100Object_h__
#define Isa100Object_h__

#include <boost/shared_ptr.hpp>
#include <map>

#define UAP_OBJ_INDEX(uap_id,obj_id) (((uap_id)<<16)|(obj_id))	

class CIsa100Object
{
public:
	enum 
	{
		OBJECT_TYPE_ID_TUNNEL = 6
	};
	typedef boost::shared_ptr<CIsa100Object> Ptr;

public:	
	CIsa100Object (int p_nUapId, int p_nObjectId, int p_nObjectTypeId): m_nUapId(p_nUapId), m_nObjectId(p_nObjectId), m_nObjectTypeId(p_nObjectTypeId) {}
	virtual ~CIsa100Object() {}

	int GetObjectId()		{ return m_nObjectId; }
	int GetUapId()			{ return m_nUapId; }
	int GetObjectTypeId()	{ return m_nObjectTypeId; }

private:
	int m_nUapId;
	int m_nObjectId;
	int m_nObjectTypeId;
	
	//attribute map
};

typedef std::map<int,CIsa100Object::Ptr>	CIsa100ObjectsMap;	


#endif // Isa100Object_h__
