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

#ifndef NISA100OBJECTIDS_H_
#define NISA100OBJECTIDS_H_


#include <boost/cstdint.hpp> //used for inttypes

namespace nisa100 {
namespace hostapp {

/*commented by Cristian.Guef
const boost::uint16_t PUBLISHER_OBJECT = 4;
*/
//added by Cristian.Guef -but only for testing because it must be dinamical_alocated
const boost::uint16_t PUBLISHER_OBJECT = 2;

const boost::uint16_t PUBLISHER_PUBLISH_METHOD = 1;
const boost::uint16_t PUBLISHER_CANCELPUBLISH_METHOD = 2;

const boost::uint16_t SUBSCRIBER_OBJECT = 5;
const boost::uint16_t SUBSCRIBER_SUBSCRIBE_METHOD = 1;
const boost::uint16_t SUBSCRIBER_CANCELSUBSCRIBE_METHOD = 2;

/* commented by Cristian.Guef
const boost::uint16_t PUBLISH_STATISCTICS_OBJECT = 156;
const boost::uint16_t GET_BATTERY_STATISTICS_METHOD = 1;
*/
//added by Cristian.Guef
const boost::uint16_t PUBLISH_STATISCTICS_OBJECT = 11;
const boost::uint16_t GET_BATTERY_STATISTICS_METHOD = 2;

const boost::uint16_t RESET_BATTERY_STATISTICS_METHOD = 2;

/*commented by Cristian.Guef
const boost::uint16_t UPLOAD_DOWNLOAD_OBJECT = 3;
*/
//added by Cristian.Guef
const boost::uint16_t UPLOAD_DOWNLOAD_OBJECT = 7;

/* commented by Cristian.Guef
//TODO:[Ovidiu] - investigate this!!! must remove 0x80
const boost::uint16_t GET_FIRMWAREVERSION_METHOD = 0x8080;
const boost::uint16_t START_FIRMWAREUPDATE_METHOD = 0x8081;
const boost::uint16_t GET_FIRMWAREUPDATESTATUS_METHOD = 0x8082;
const boost::uint16_t CANCEL_FIRMWAREUPDATE_METHOD = 0x8083;
*/
//added by Cristian.Guef
const boost::uint16_t GET_FIRMWAREVERSION_METHOD = 0x0080;
const boost::uint16_t START_FIRMWAREUPDATE_METHOD = 0x0081;
const boost::uint16_t GET_FIRMWAREUPDATESTATUS_METHOD = 0x0082;
const boost::uint16_t CANCEL_FIRMWAREUPDATE_METHOD = 0x0083;


/* commented by Cristian.Guef
const boost::uint16_t SYSTEM_MONITORING_OBJECT = 104;
*/
//added by Cristian.Guef
const boost::uint16_t SYSTEM_MONITORING_OBJECT = 5;
const boost::uint16_t SCO = 3;

const boost::uint16_t GET_DEVICEINFORMATION_METHOD = 5;

//added by Cristian.Guef
const boost::uint16_t GET_CONTRACTSANDROUTES_METHOD = 15;

/* commented by Cristian.Guef
const boost::uint16_t DEVICE_MANAGEMENT_SERVICE_OBJECT = 103;
*/
//added by Cristian.Guef
const boost::uint16_t DEVICE_MANAGEMENT_SERVICE_OBJECT = 3;

/* commented by Cristian.Guef
const boost::uint16_t RESET_DEVICE_METHOD = 3;
*/
//added by Cristian.Guef
const boost::uint16_t RESET_DEVICE_METHOD = 12;

//added
const boost::uint16_t GET_CHANNELS_STATISTICS = 16;


//TSAP ID
enum ApplicationProcesses
{
	/* commented by Cristian.Guef
	apDMAP = 0,
	*/
	//added by Cristian.Guef
#warning [MARCEL]: Cristian, this is wrong. Correct is DMAP == 0, SMAP == 1, UAP == 2
	// probably, most of the code is unchanged, just need to use apSMAP instead of apDMAP
	apDMAP = 1,
	apSMAP = 1,
	apUAP1 = 1,
	apUAP_EvalKits = 2
};

}
}

#endif /*NISA100OBJECTIDS_H_*/
