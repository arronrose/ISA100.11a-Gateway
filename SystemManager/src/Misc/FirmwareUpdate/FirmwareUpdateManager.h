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

/**
 * FirmwareUpdateManager.h
 *
 *  Created on: 30.07.2008
 *      Author: Radu Pop, ioan.pocol, beniamin.tecar, sorin.bidian
 */

#ifndef FIRMWAREUPDATEMANAGER_H_
#define FIRMWAREUPDATEMANAGER_H_

#include "Common/NEAddress.h"
#include "Common/logging.h"
#include "Common/ClockSource.h"
#include "Common/Objects/SFC.h"
#include "Model/model.h"

namespace Isa100 {
namespace Misc {
namespace FirmwareUpdate {

/**
 * An instance of this class manages a firmware update for a device.
 * The class must be initialized with an <code>Address64</code>, representing the
 * address of the device that makes the update, and a file name,
 * representing the name of the file containing the firmware used to make the update.
 * <code>init</code> is the first method that must be called. Calling any other method
 * before the <code>init</code> method will get an <code>Isa100Exception</code>.
 * The content of the firmware update is divided into chunks of equal size (the size of the chunk
 * is read from configuration files). The calling class will invoke the <code>nextChunk()</code>
 * method until there are no more chunks available (there are no more chunks available when
 * the <code>hasMoreChunks()</code> method returns <code>false</code>.
 * If a chunk has been requested there cannot be any other chunk requests
 * until a confirmation that the last chunk has got to the device is received.
 * @author Radu Pop
 * @version 1.0
 */
class FirmwareUpdateManager {

        LOG_DEF("Isa100.Misc.FirmwareUpdate.FirmwareUpdateManager")

        class MMappedFile {
            LOG_DEF("Isa100.Misc.FirmwareUpdate.FirmwareUpdateManager.MMappedFile")
            char *fileRegion;
            int fileDescriptor;
            Uint32 mapSize;
        public:
            MMappedFile();
            ~MMappedFile();
            bool init(char const *fileName, Uint32 &fileSize);
            char *getRegion()const;
        };
        public:
        static const Uint8 NOT_INITIALIZED = 0;
        static const Uint8 CLIENT_UPLOAD = 1;
        static const Uint8 CLIENT_DOWNLOAD = 2;
        static const Uint8 SERVER_UPLOAD = 3;
        static const Uint8 SERVER_DOWNLOAD = 4;

        static const Uint8 IDLE = 0;
        static const Uint8 DOWNLOADING = 1;
        static const Uint8 UPLOADING = 2;
        static const Uint8 APPLYING = 3;
        static const Uint8 DL_COMPLETE = 4;
        static const Uint8 UL_COMPLETE = 5;
        static const Uint8 DL_ERROR = 6;
        static const Uint8 UL_ERROR = 7;

        private:
        /*
         * The type of session, client upload, client download, server upload, server download
         */
        Uint8 sessionType;

        /**
         * The address of the device which is peer on UDO session.
         */
        Address64 peerDeviceAddress;

        /**
         * Firmware Manager to Device ContractID.
         */
        ContractId fwManager2DeviceContractID;

        /**
         * The name of the file where the data is read/write in UDO session.
         */
        std::string fileName;

        /**
         * This is the size of the file in bytes.
         */
        Uint32 fileSize;

        /**
         * Block Size
         */
        Uint16 blockSize;

        /**
         * The total number of blocks for the given firmware file.
         */
        Uint16 blocksNo;

        /**
         * The id of the current block. Current is block is the block that has been sent/received
         * In the case of sent it waits for confirmation.
         */
        Uint16 currentBlockId;

        /**
         * Contains the blocks for the selected file.
         */
        std::vector<BytesPointer> fileBlocks;

        /** The keeper of the mmapped region of the selected file.
          */
        boost::shared_ptr<MMappedFile> fileRegion;

        /**
         * This is the time of the initialization of the object.
         */
        Uint32 startTime;

        /**
         * This is the time of the last confirmation received.
         */
        Uint32 lastActivityTime;

        /**
         * Phase of the upload/download.
         */
        Uint8 phase;

        /**
         * Finish status of upload/download session
         */
        Uint8 status;

        /* the numbers of retries for the last packet */
        Uint8 retriesNumbers;

        /**
         * TAI cutover time
         */
        Uint32 taiCutoverTime;

        /**
         * Fractional TAI cutover time
         */
        Uint16 fractionalTaiCutoverTime;

        /**
         * The current request Id
         */
        AppHandle appHandle;

        /**
         * The last phase idle time.
         */
        Uint32 lastPhaseIdleTime;

        private:
        /**
         * Returns <code>true</code> if the <code>init()</code> has been invoked on the current instance.
         * @return <code>true</code> if the <code>init()</code> has been invoked
         */
        bool hasBeenInitialized() const;

        /**
         * Verifies that the <code>init()</code> method has been invoked on the current
         * instance. If it was not invoked then throws an Isa100Exception.
         */
        void checkInitStatus();

        /**
         * Clear all data related to the previous UDO session
         */
        void initSession();

        public:


        static const Uint8 MAX_RETRIES = 4;

        /* stop download rationale */
        static const Uint8 FINISHED = 0; ///TODO: duplicate in UDO.h. FIX IT
        static const Uint8 CANCELED = 1; ///TODO: duplicate in UDO.h. FIX IT

        /**
         * Creates a new instance of this class.
         * In order to be use the class, the <code>init()</code> must be first invoked.
         */
		bool m_bAlertEndSent;

        /**
         * Creates a new instance of this class.
         * In order to be use the class, the <code>init()</code> must be first invoked.
         */
        FirmwareUpdateManager();

        /**
         * returns the type of the UploadDownload session
         */
        Uint8 getSessionType();

        /**
         * Read the firmware file from the disk and prepares the chunks.
         * @param deviceAddress represents the address of the device for which the update is made
         * @param firmwareFileUpdate represents the name of the file that contains the firmware
         */
        bool initClientDownload(Address64 deviceAddress64, std::string fileName);

        ContractId getFwManager2DeviceContractID() {
            return fwManager2DeviceContractID;
        }

        void setFwManager2DeviceContractID(ContractId fwManager2DeviceContractID_){
            fwManager2DeviceContractID = fwManager2DeviceContractID_;
        }

        /**
         * Set the name of the file and test if the file can be written to the disk.
         * @param deviceAddress represents the address of the device for which the upload is made
         * @param firmwareFileDownload represents the name of the file that contains the firmware
         */
        bool fileSetup(Address64 deviceAddress64, std::string fileName);

        /**
         * Set the specific UDO data for the download session
         */
        void initServerDownload(Uint16 blockSize, Uint32 downloadSize, Uint8 operationMode);

        /**
         * Apply the upload/download operation
         */
        bool apply();
        /**
         * Return the set block size
         */
        Uint16 getBlockSize();

        /**
         * Return the size of the next expected block
         */
        Uint16 getNextBlockSize();

        /**
         * Confirm the received block
         */
        Isa100::Common::Objects::SFC::SFCEnum confirmBlock(Uint16 crtPacket, BytesPointer bytes);

        /**
         * Check if the received download blocks are valid
         */
        Isa100::Common::Objects::SFC::SFCEnum isDownloadComplete(Uint8 rationale);

        /**
         * Checks if there is at least one more chunk to be delivered.
         * If there has been no initialization throws an Isa100Exception.
         * @return <code>true</code> if the there are more chunks to delivere.
         */
        bool hasMoreBlocks();

        /**
         * Returns the next chunk and block until the confirm is received.
         * If there has been no initialization throws an Isa100Exception.
         * @return BytesPointer to the chunk to be sent to the device
         */
        BytesPointer nextBlock();

        /**
         * Set the last chunk for retry
         */
        void setRetryBlock(Uint32 lastActivityTime_);

        /**
         * Update last operation time
         */
        void updateTime(Uint32 lastActivityTime_);

        /**
         * Confirms that the last send chunk has reached the destination device.
         * If the received <code>chunkId</code> does not match the current chunk id
         * <code>false</code> is returned. Otherwise returns <code>true</code>.
         * @return <code>true</code> if the confirmed chunk id is the same with the current chunk id.
         */
        bool confirmBlock();

        /**
         * Returns the id of the last confirmed chunk.
         * If there has been no initialization throws an Isa100Exception.
         * @return id of the last confirmed chunk
         */
        Uint16 getCurrentBlockId();

        /**
         * Set the current confirmed block id
         */
        void setCurrentBlockId(Uint16 id);

        /**
         * Returns the time the <code>init</code> has been invoked.
         * If there has been no initialization throws an Isa100Exception.
         * @return the time the <code>init</code> has been invoked
         */
        Uint32 getStartTime();

        /**
         * Returns the time of the last confirmation received
         * If there has been no initialization throws an Isa100Exception.
         * @return the time of the last confirmation received
         */
        Uint32 getLastActivityTime();

        /**
         * Returns the address of the device on which the update is made.
         * If there has been no initialization throws an Isa100Exception.
         * @return the address of the device on which the update is made
         */
        Address64 getPeerDeviceAddress64();

        /**
         * Returns the name of the file that contains the firmware used to make the update.
         * If there has been no initialization throws an Isa100Exception.
         * @return the name of the file that contains the firmware used to make the update
         */
        std::string getFileName();

        /**
         * Returns the size of the initialized file in bytes.
         * @return the size of the initialized file in bytes
         */
        Uint32 getFileSize();

        /**
         * Returns the number of the chunks for the current initialized firmware file.
         * If there has been no initialization throws an Isa100Exception.
         * @return the number of chunks for the current initialized firmware file.
         */
        Uint16 getBlocksNo();

        /**
         * Get upload/download phase.
         */
        Uint8 getPhase();

        /**
         * Set upload/download finish status
         */
        void setStatus(Uint8 status);

        /**
         * Get upload/download finish status.
         */
        Uint8 getStatus();

        /**
         * Set upload/download phase
         */
        void setPhase(Uint8 phase);

        Uint32 getTaiCutoverTime() {
            return taiCutoverTime;
        }

        void setTaiCutoverTime(Uint32 taiCutoverTime) {
            this->taiCutoverTime = taiCutoverTime;
        }

        Uint16 getFractionalTaiCutoverTime() {
            return fractionalTaiCutoverTime;
        }

        void setFractionalTaiCutoverTime(Uint16 fractionalTaiCutoverTime) {
            this->fractionalTaiCutoverTime = fractionalTaiCutoverTime;
        }

        Uint32 getLastPhaseIdleTime() {
            return lastPhaseIdleTime;
        }

        /**
         * Returns the procent of chunks already sent to the device.
         * @return the procent of chunks already sent to the device
         */
        Uint8 getPercentDone();

        /**
         * Check if the difference between the last sent chunk and current time is greater.
         * than a given period(set in configuration file)
         */
        bool isClientTimeOut();

        /**
         * Check if the difference between the last sent confirmation and current time is greater.
         * than a given period(set in configuration file)
         */
        bool isServerTimeOut();

        Uint8 getDataDownloadRequest();

        void setDataDownloadRequest(Uint8 dataDownloadRequest_, bool retry);

        Uint8 getStartDownloadRequest();

        void setStartDownloadRequest(Uint8 startDownloadRequest_, bool retry);

        Uint8 getEndDownloadRequest();

        void setEndDownloadRequest(Uint8 endDownloadRequest_, bool retry);

        Uint8 getApplyDownloadRequest();

        void setApplyDownloadRequest(Uint8 applyDownloadRequest_, bool retry);

        Uint8 getResetDownloadRequest();

        void setResetDownloadRequest(Uint8 resetDownloadRequest_, bool retry);

        bool exceedMaxRetries();

        Uint16 getExceedMaxRetries();

        void resetRetries();

        /**
         * Current request Id
         */
        AppHandle getAppHandle ();

        /**
         * Set the current request id
         */
        void setAppHandle(AppHandle appHandle);

        /**
         * Returns a string representation of this FirmwareUpdateManager.
         */
        friend std::ostream& operator<<(std::ostream & stream, const FirmwareUpdateManager & firmwareUpdateManager);

    };

}
}
}

#endif /* FIRMWAREUPDATEMANAGER_H_ */
