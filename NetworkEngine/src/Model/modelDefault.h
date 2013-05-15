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

/*
 * modelDefault.h
 *
 * Contains defines with some default IDs and values
 * that are used in default settings of teh model.
 *
 *  Created on: Sep 15, 2009
 *      Author: Catalin Pop
 */

#ifndef MODELDEFAULT_H_
#define MODELDEFAULT_H_

#define DEFAULT_GRAPH_ID 0x1
#define DEFAULT_ROUTE_ID 0x1
#define DEFAULT_SUPERFRAME_ID                 0x1
#define DEFAULT_MANAGEMENT_SUPERFRAME_ID      0x2
#define DEFAULT_ADVERTISE_SUPERFRAME_ID       0x4
#define DEFAULT_NEIGH_DISCOVERY_SUPERFRAME_ID 0x5
/*
 * describes superframe advertise as slotted hopping
 */
#define SUPERFRAME_ADVERTISE_CHANNEL_RATE 1
#define MAX_SUPERFRAME_BIRTH 127
#define DEFAULT_CHANNEL_MAP 0xFFFF
#define DEFAULT_LINK_TX_ID 1
#define DEFAULT_LINK_RX_ID 2
#define DEFAULT_LINK_ADV_RX_ID 3

#define DEFAULT_MAX_NO_CANDIDATES 8

#define PRIORITY_LINK_BBR_RECEIVE 0
#define PRIORITY_LINK_NEIGHBOR_DISCOV 1
#define PRIORITY_LINK_ACCELERATION 2
#define PRIORITY_LINK_GENERAL 3
#define PRIORITY_LINK_NON_ROUTING 4
#define PRIORITY_LINK_ADVERTISE 5




#endif /* MODELDEFAULT_H_ */
