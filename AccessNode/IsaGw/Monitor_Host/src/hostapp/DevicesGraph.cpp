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

#include "DevicesGraph.h"

namespace nisa100 {
namespace hostapp {

void DevicesGraph::AddArc(int from, int to, unsigned char signalQuality, unsigned char clockSource)
{
	m_vertices[from].neighbours.push_back(NeighbourData());
	NeighbourData &neighbour = *m_vertices[from].neighbours.rbegin();
	neighbour.vertexNo = to;
	neighbour.signalQuality = signalQuality;
	neighbour.clockSource = clockSource;
}

bool DevicesGraph::IsArc(int from, int to)
{
	for (std::list<NeighbourData>::const_iterator i = m_vertices[from].neighbours.begin(); 
				i != m_vertices[from].neighbours.end(); ++i)
	{
		if ((*i).vertexNo == to)
			return true;
	}
	
	return false;
}

}// namespace hostapp
}// namespace nisa100
