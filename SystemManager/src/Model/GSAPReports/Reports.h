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
 * Reports.h
 *
 *  Created on: Jun 11, 2009
 *      Author: flori.parauan, sorin.bidian
 */

#ifndef REPORTS_H_
#define REPORTS_H_

#include "Misc/Marshall/Stream.h"
#include "Model/EngineProvider.h"
#include "Model/Reports/ReportsStructures.h"
#include "Common/Address128.h"

#define BROADCAST_ADDRESS_32 0xFFFFFFFF
#define BROADCAST_ADDRESS_128 "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF"

namespace Isa100 {
namespace Model {
namespace GSAPReports {


struct GenerateReportCmd {

    //Bit 0: request Dev List
    //Bit 1: request Topology
    //Bit 2: request Schedule
    //Bit 3: request Device Health
    //Bit 4: request Neighbor Health -if set, deviceNeighborHealth must be also set
    //Bit 5: request NetworkHealth
    //Bit 6: request NetworkResource
    Uint16 reportsRequested; // see the bitfield definition above

    //device for which Neighbor Health is requested
    //Must be set if reportRequested & 0x10 is true
    NE::Common::Address128 deviceNeighborHealth;

    GenerateReportCmd() :
    	reportsRequested(0) {

    }

    bool isDeviceListRequested() {
    	return (reportsRequested & 0x01);
    }

    bool isTopologyRequested() {
    	return (reportsRequested & 0x02);
    }

    bool isScheduleRequested() {
    	return (reportsRequested & 0x04);
    }

    bool isDeviceHealthRequested() {
    	return (reportsRequested & 0x08);
    }

    bool isNeighborHealthRequested() {
    	return (reportsRequested & 0x10);
    }

    bool isNetworkHealthRequested() {
    	return (reportsRequested & 0x20);
    }

    bool isNetworkResourceRequested() {
         return (reportsRequested & 0x40);
     }

    void unmarshall(NE::Misc::Marshall::InputStream& stream) {
        stream.read(reportsRequested);
        if (isNeighborHealthRequested()) {
        	deviceNeighborHealth.unmarshall(stream);
        }
    }

};

//struct GenerateReportRsp {
//    Uint32  maxBlockSize;           //common to all reports
//
//    Uint32  deviceListSize;         //0 : if DeviceList was not requested
//    Uint32  deviceListHandler;      //0 : if DeviceList was not requested
//
//    Uint32  topologySize;           //0 : if topology was not requested
//    Uint32  topologyHandler;        //0 : if topology was not requested
//
//    Uint32  scheduleSize;           //0 : if schedule was not requested
//    Uint32  scheduleHandler;        //0 : if schedule was not requested
//
//    Uint32  neighborHealthSize;     //0 : if neighborHealth  was not requested
//    Uint32  neighborHealthHandler;  //0 : if neighborHealth was not requested
//
//    Uint32  networkHealthSize;      //0 : if networkHealth was not requested
//    Uint32  networkHealthHandler;   //0 : if networkHealth was not requested
//
//    Uint32 networkResourceSize;     //0 : if networkResource was not requested
//    Uint32 networkResourceHandler;  //0 : if networkResource was not requested
//};

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::DeviceListReport& deviceListReport);

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::TopologyReport& topologyReport);

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::ScheduleReport& scheduleReport);

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::DeviceHealthReport& deviceHealthReport);

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::NeighborHealthReport& neighborHealthReport);

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::NetworkHealthReport& networkHealthReport);

void marshall(NE::Misc::Marshall::OutputStream& stream, NE::Model::Reports::NetworkResourceReport& networkResourceReport);

}
}
}

#endif /* REPORTS_H_ */
