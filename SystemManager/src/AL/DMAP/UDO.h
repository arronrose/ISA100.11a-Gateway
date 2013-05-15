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

#ifndef UDO_H_
#define UDO_H_

#include <map>
#include "AL/Isa100Object.h"
#include "Misc/FirmwareUpdate/FirmwareUpdateManager.h"
#include "ASL/Services/ASL_AlertReport_PrimitiveTypes.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Common/Objects/SFC.h"
#include "Common/Objects/TAINetworkTimeValue.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "ASL/PDU/WriteRequestPDU.h"

using namespace Isa100::Common;
using namespace Isa100::Misc::FirmwareUpdate;

namespace Isa100 {
namespace AL {
namespace DMAP {

class UDO;
typedef boost::shared_ptr<UDO> UDOPointer;

typedef std::pair<Address64, pair<Address128, Uint8> > DevicesActivationStatusEntry;

/**
 * Upload/Download Object (UDO).
 * This object manages upload/download large data chunks.
 * Also a custom interface(considered in protocol to be out of scope of SP100)
 * is offered to the Monitoring Application in order to initiate the device firmware update
 * @author Ioan V. Pocol, radu.pop, beniamin.tecar, sorin.bidian
 */
class UDO : public Isa100Object,
            public NE::Model::IDeletedDeviceListener {

        LOG_DEF("Isa100.AL.UAP.UDO");

    public:

        /* UDO methods ids*/
        static const Uint8 START_DOWNLOAD = 1;
        static const Uint8 DATA_DOWNLOAD = 2;
        static const Uint8 END_DOWNLOAD = 3;
        static const Uint8 START_UPLOAD = 4;
        static const Uint8 DATA_UPLOAD = 5;
        static const Uint8 END_UPLOAD = 6;

        /* UDO attributes ids */
        static const Uint8 OPERATION_SUPPORTED = 1;
        static const Uint8 DESCRIPTION = 2;
        static const Uint8 STATE = 3;
        static const Uint8 COMMAND = 4;
        static const Uint8 MAX_BLOCK_SIZE = 5;
        static const Uint8 MAX_DOWNLOAD_SIZE = 6;
        static const Uint8 MAX_UPLOAD_SIZE = 7;
        static const Uint8 DOWNLOAD_PREP_TIME = 8;
        static const Uint8 DOWNLOAD_ACTIVATION_TIME = 9;
        static const Uint8 UPLOAD_PREP_TIME = 10;
        static const Uint8 UPLOAD_PROCESSING_TIME = 11;
        static const Uint8 DOWNLOAD_PROCESSING_TIME = 12;
        static const Uint8 CUTOVER_TIME = 13;
        static const Uint8 LAST_BLOCK_DOWNLOADED = 14;
        static const Uint8 LAST_BLOCK_UPLOADED = 15;

        /* UDO COMMAND VALUES */
        static const Uint8 RESET_COMMAND = 0;
        static const Uint8 APPLYING_COMMAND = 1;

        /* stop download rationale */
        static const Uint8 FINISHED          = 0; /// transfer done OK 	///TODO: duplicate in FirmwareUpdateManager.h. FIX IT
        static const Uint8 CANCELED          = 1; /// cancel by user	///TODO: duplicate in FirmwareUpdateManager.h. FIX IT

        /* udo transfer end reason*/
        enum{
			UDOALERT_OK         = 0, /// transfer done OK
			UDOALERT_CANCEL     , //= 1, /// cancel by user
			UDOALERT_FAIL       , //= 2, /// transfer failed reported by UDO state machine
			UDOALERT_INVAL_FW   , //= 3, /// invalid firmware
			UDOALERT_OTHER      , //= 4, /// other reason
		};

        /* session upload/download phase */
        static const Uint8 IDLE = 0;
        static const Uint8 DOWNLOADING = 1;
        static const Uint8 UPLOADING = 2;
        static const Uint8 APPLYING = 3;
        static const Uint8 DL_COMPLETE = 4;
        static const Uint8 UL_COMPLETE = 5;
        static const Uint8 DL_ERROR = 6;
        static const Uint8 UL_ERROR = 7;
        /*custom phases */
        static const Uint8 RESETTING = 8;
        static const Uint8 APPLICATION_SETUP = 9;
        static const Uint8 READ_STATE = 10;
        static const Uint8 READ_LAST_BLOCK = 11;

        /* custom methods ids */
        static const Uint8 START_FIRMWARE_UPDATE = 129;
        static const Uint8 GET_FIRMWARE_UPDATE_STATUS = 130;
        static const Uint8 CANCEL_FIRMWARE_UPDATE = 131;
        static const Uint8 SET_FILE_NAME = 132;
        static const Uint8 GROUP_FW_ACTIVATION_TAI = 133;

        /* status code for custom methods */
        static const Uint8 SUCCESS = 0;
        static const Uint8 DEVICE_NOT_EXISTS = 1;
        static const Uint8 ALREADY_EXISTS_SESSION = 2;
        static const Uint8 TIMEOUT = 2;
        static const Uint8 NO_SESSION = 3;
        static const Uint8 FILE_ERROR = 3;

        /* session upload/download finish status */
        static const Uint8 DONE_WITH_SUCCESS = 1;
        static const Uint8 CANCELED_BY_CLINET = 2;
        static const Uint8 APPLY_NOT_CONFIRMED = 4;
        static const Uint8 CANCELED_NOT_CONFIRMED = 5;
        static const Uint8 FAILED = 6;
        static const Uint8 NOT_SET = 6;

    private:
        /** Last activity timestamp */
        Uint32 lastActivity;

        /**
         * Last udo alert timpestamp.
         */
        Uint32 lastUdoAlertTimestamp;

        /** The list of Upload/Download Sessions active on the system */
        typedef map<Address64, FirmwareUpdateManager> UploadDownloadSessions;
        UploadDownloadSessions uploadDownloadSessions;

        int tsapId;

        /**
         * Time (in seconds) specified to activate the download content.
         */
        Isa100::Common::Objects::TAINetworkTimeValue cutoverTime;

        /**
         * The list of devices that need to be configured for FW activation and their activation status.
         */
        typedef std::map<Address64, pair<Address128, Uint8> > DevicesActivationStatus;
        // pair <address128,status>
        //status: 0 - initial, 1 - configured, 2 - expecting confirm
        //address128 - needed for the scenarios when a response is received from a device removed from the internal model
        DevicesActivationStatus devicesActivationStatus;

        //flags if there is an FW activation going on
        bool isActivationProcessing;

        /**
         * Holds the request for FW activation. Needed for sending back the response when all the configurations are done.
         */
        Isa100::ASL::Services::PrimitiveIndicationPointer groupActivationRequest;

    public:
        UDO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine);

        virtual ~UDO();

        void deviceDeletedCallback(Address32 deletedDevAddr32, Uint16 deviceType);

        void execute(Uint32 currentTime);

        Isa100::AL::ObjectID::ObjectIDEnum getObjectID() const {
            return (tsap == TSAP::TSAP_SMAP ? ObjectID::ID_UDO1 : ObjectID::ID_UDO);
        }

        /**
         * Always return false; there is only one instance of this object.
         */
        bool isJobFinished() {
            return false;
        }

        void registerDeleteDevice();

        /**
         * Always return true.
         */
        bool expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
            return true;
        }

        bool expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
            return true;
        }

        void confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);
        void confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);
        void confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm);

        void indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        void indicateWrite(Isa100::ASL::Services::PrimitiveIndicationPointer indication);
        void indicateRead(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

//        void notifyJoinDevice(Address128& deviceAddress);

    private:

        void writeCommand(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest);

        void writeCutoverTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest);

        void readOperationSupported(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readDescription(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readState(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readMaxBlockSize(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readMaxDownloadSize(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readMaxUploadSize(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readDownloadPrepTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readDownloadActivationTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readUploadPrepTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readUploadProcessingTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readDownloadProcessingTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readCutoverTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readLastBlockDownloaded(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        void readLastBlockUploaded(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        /*
         * A client uses the StartDownload method to indicate to an UploadDownload object
         * instance that it desires to download the object.
         */
        void serverStartDownload(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * A client uses the DownloadData method to provide data to an UploadDownload object that
         * has agreed to be downloaded.
         */
        void serverDataDownload(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /*
         * A client uses the EndDownload method to indicate that the download is
         * terminating.
         */
        void serverEndDownload(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /*
         * A client uses the StartUpload method to indicate to an UploadDownload object
         * instance that it desires to upload the object.
         */
        void serverStartUpload(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        /**
         * A client uses the DataUpload method to provide data to an UploadDownload object that
         * has agreed to be uploaded.
         */
        void serverDataUpload(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        /*
         * A client uses the EndUpload method to indicate that the upload is
         * terminating.
         */
        void serverEndUpload(Isa100::ASL::Services::PrimitiveIndicationPointer indication);

        /**
         * Send StartDownload command to device
         */
        void clientStartDownload(Uint16 requestId, const Address64& address64, Uint16 blocksCount, Uint32 downloadSize);

        /**
         * Send DataDownload command to device
         */
        void clientDataDownload(Uint16 requestId, const Address64& address64, Uint16 crtPacket, BytesPointer bytesPointer);

        /**
         * Send EndDownload command to given device with given rationale
         */
        void clientEndDownload(Uint16 requestId, const Address64& address64, Uint8 rationale);

        /**
         * Send Apply command
         */
        void clientWriteCommand(Uint16 requestId, const Address64& address64, Uint8 command);

        /**
         * Send a read command for UDO state
         */
        void clientReadState(Uint16 requestId, const Address64& address64);

        /**
         * Send a read command for last block received with success
         */
        void clientReadLastBlockDownloaded(Uint16 requestId, const Address64& address64);

        /**
         * Process the Start Firmware Update command received from Monitoring Application
         */
        void startFirmwareUpdate(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Process the GET Firmware Update Status command received from Monitoring Application
         */
        void getFirmwareUpdateStatus(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Process the Cancel Firmware Update command received from Monitoring Application
         */
        void cancelFirmwareUpdate(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Process the Start Firmware Download command received from Monitoring Application
         */
        void setFileName(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Sends configurations to the devices in the list received as parameter.
         */
        void configureGroupActivation(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest);

        /**
         * Check if exist a download session hanged
         */
        void checkTimeouts(Uint32 currentTime);

        /**
         * Respond back to the Gateway with the parameters received serializated in stream
         */
        void sendResponseToGateway(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    const NE::Misc::Marshall::NetworkOrderStream& stream);

        /**
         * Respond back to requester
         */
        void sendResponseToRequester(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    const NetworkOrderStream& stream, Objects::SFC::SFCEnum serviceFeedbackCode);

        void sendWriteResponse(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Objects::SFC::SFCEnum serviceFeedbackCode);

        void sendReadResponse(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
                    Objects::SFC::SFCEnum serviceFeedbackCode, const NetworkOrderStream& stream);

        /**
         * Send UDO transfer end alert
         */
        void sendAlertTransferEnd( const Address64& address64, Uint8 rationale);
        Uint8 udoAlertTransferStatus( Uint8 p_u8UDOStatus );

        std::string getUploadDownloadSessionsAsString();
};

}
}
}
#endif /* UDO_H_ */
