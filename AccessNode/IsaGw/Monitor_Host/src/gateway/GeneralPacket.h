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

#ifndef GENERALPACKET_
#define GENERALPACKET_

#include <string>
#include <boost/cstdint.hpp>


namespace nisa100 {
namespace gateway {

class GeneralPacket
{
public:
	enum GServiceTypes
	{
		//added by Cristian.Guef
		GSessionRequest = 0x01,
		GSessionConfirm = 0x81,

		/* commented by Cristian.Guef
		GContractRequest = 0x01,
		GContractConfirm = 0x81,
		GDeviceListReportRequest = 0x02,
		GDeviceListReportConfirm = 0x82,
		*/
		//added by Cristian.Guef
		GContractRequest = 0x02,
		GContractConfirm = 0x82,
		GDeviceListReportRequest = 0x03,
		GDeviceListReportConfirm = 0x83,

		//added by Cristian.Guef
		GDeviceHealthReportRequest = 0x06,
		GDeviceHealthReportConfirm = 0x86,
		GNeighbourHealthReportRequest = 0x07,
		GNeighbourHealthReportConfirm = 0x87,
		GNetworkHealthReportRequest = 0x08,
		GNetworkHealthReportConfirm = 0x88,
		

		/* commented by Cristian.Guef
		GTopologyReportRequest = 0x03,
		GTopologyReportConfirm = 0x83,
		*/
		//added by Cristian.Guef
		GTopologyReportRequest = 0x04,
		GTopologyReportConfirm = 0x84,

		/* commented by Cristian.Guef
		GScheduleReportRequest = 0x04,
		GScheduleReportConfirm = 0x84,
		*/
		//added by Cristian.Guef
		GScheduleReportRequest = 0x05,
		GScheduleReportConfirm = 0x85,

		/* commented by Cristian.Guef
		GClientServerRequest = 0x08,
		GClientServerConfirm = 0x88,
		*/
		//added by Cristian.Guef
		GClientServerRequest = 0x0A,
		GClientServerConfirm = 0x8A,

		/* commented by Cristian.Guef
		GBulkRequest = 0x10,
		GBulkConfirm = 0x90,
		*/

		//added by cristian.Guef
		GBulkOpenRequest = 0x10,
		GBulkOpenConfirm = 0x90,
		GBulkTransferRequest = 0x11,
		GBulkTransferConfirm = 0x91,
		GBulkCloseRequest = 0x12,
		GBulkCloseConfirm = 0x92,


		/* commented by Cristian.Guef
		GPublishIndication = 0x49
		*/

		//added by Cristian.Guef
		GPublishIndication = 0x4B,
		GSubscribeTimer = 0x4E,
		GWatchdogTimer = 0x4F,

		//added by Cristian.Guef
		GNetworkResourceRequest = 0x21,
		GNetworkResourceConfirm = 0xA1,

		//added by Cristian.Guef
		GAlertSubscriptionRequest = 0x13,
		GAlertSubscriptionConfirm = 0x93,
		GAlertSubscriptionIndication = 0x54
	};

	GeneralPacket();
	const std::string ToString() const;


//	static const boost::uint8_t GeneralPacket_VERSION = 2;
	static const boost::uint8_t GeneralPacket_SIZE_VERSION1 = 1 + 4 + 2;
	static const boost::uint8_t GeneralPacket_SIZE_VERSION2 = 1 + 4 + 4;

	//added by Cristian.Guef
	static const boost::uint8_t GeneralPacket_SIZE_VERSION3 = 1 + 4 + 4 + 4 + 4;

	boost::uint8_t version;
	GServiceTypes serviceType;

	//added by Cristian.Guef - pt versiune 0x3 de packet
	boost::uint32_t sessionID;

	boost::int32_t trackingID;
	boost::uint32_t dataSize;

	//added by Cristian.Guef- pt versiunea 0x3 de packet
	boost::uint32_t headerCRC;

	std::basic_string<boost::uint8_t> data;
};


} // namespace gateway
} // namespace nisa100

#endif /*GENERALPACKET_*/
