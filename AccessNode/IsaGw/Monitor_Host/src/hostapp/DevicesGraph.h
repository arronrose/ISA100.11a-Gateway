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

#ifndef DEVICESGRAPH_H_
#define DEVICESGRAPH_H_

#include <list>
#include <vector>
#include "model/Device.h"


namespace nisa100 {
namespace hostapp {

//this class creates a graph to enable algorithms such as BFS (Breadth First Search) and
	//DFS (Depth First Search) to be performed
class DevicesGraph
{
public:
	struct NeighbourData
	{
		short			vertexNo;
		unsigned char	signalQuality;
		unsigned char	clockSource;
	};
private:
	struct VertexData
	{
		DevicePtr					dev;
		std::list<NeighbourData>	neighbours;
	};

private:
	std::vector<VertexData>	m_vertices;

public:
	//0 -ok
	//1 -already set
	int SetVerticesNo(int n)
	{
		if (m_vertices.size() > 0)
			return 1;
		m_vertices.resize(n);
		return 0;
	}
	int GetVerticesNo()
	{
		return m_vertices.size();
	}

	void AddDataToVertex(int i, DevicePtr dev)
	{
		m_vertices[i].dev = dev;
	}

	void AddArc(int from, int to, unsigned char signalQuality, unsigned char clockSource) ;
	bool IsArc(int from, int to);
	std::list<NeighbourData>& GetNeighbours(int i) //the return value should not be this way (break the principle of OOP) but is good for optimization
	{
		return m_vertices[i].neighbours;
	}

	DevicePtr GetVertexData(int i)
	{
		return m_vertices[i].dev;
	}

};

}
}

#endif
