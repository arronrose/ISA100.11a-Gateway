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
 * FirmwareUpdateManager.cpp
 *
 *  Created on: 30.07.2008
 *      Author: radu.pop, ioan.pocol, beniamin.tecar, sorin.bidian
 */

#include "Common/SmSettingsLogic.h"
#include "Misc/Convert/Convert.h"
#include "FirmwareUpdateManager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Misc::FirmwareUpdate;
using namespace NE::Common;

namespace Isa100 {
namespace Misc {
namespace FirmwareUpdate {

FirmwareUpdateManager::MMappedFile::MMappedFile():fileRegion(NULL), fileDescriptor(-1), mapSize(0) {
}

FirmwareUpdateManager::MMappedFile::~MMappedFile() {
    if (fileRegion != NULL && mapSize > 0) {
        munmap(fileRegion, mapSize);
        fileRegion = NULL;
        mapSize = 0;
    }
    if (fileDescriptor >= 0) {
        while (close(fileDescriptor))
            if (errno != EINTR) {
                LOG_ERROR("UDO::File descriptor << " << fileDescriptor << ". Cannot close file errno = " << errno << " !");
                break;
            }
    fileDescriptor = -1;
    }
}

bool FirmwareUpdateManager::MMappedFile::init(char const *fileName, Uint32 &fileSize) {
    fileDescriptor = open(fileName, O_RDONLY, 0);
    if (fileDescriptor < 0) {
        LOG_ERROR("UDO::File << " << fileName << " does not exist!, errno = " << errno);
        return false;
    }
    if (0 > (mapSize = fileSize = lseek(fileDescriptor, 0, SEEK_END))) {
        LOG_ERROR("UDO::File << " << fileName << " cannot reveal the size of it!, errno = "<< errno);
        return false;
    }
    fileRegion = (char *)mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fileDescriptor, 0);
    if (fileRegion == MAP_FAILED) {
        LOG_ERROR("UDO::File << " << fileName << " cannot be mmap, errno = " << errno << " !");
        fileRegion = NULL;
        return false;
    }
    return true;
}

char *FirmwareUpdateManager::MMappedFile::getRegion()const {
    return fileRegion;
}

FirmwareUpdateManager::FirmwareUpdateManager()
    : fwManager2DeviceContractID(0) {

    initSession();
}

void FirmwareUpdateManager::initSession() {
    sessionType = NOT_INITIALIZED;
    fileName = "";
    fileSize = 0;
    blockSize = 0;
    blocksNo = 0;
    currentBlockId = 0;
    fileBlocks.clear();
    startTime = 0;
    lastActivityTime = 0;
    phase = 0;
    status = 0;
    retriesNumbers = 0;
    taiCutoverTime = 0;
    fractionalTaiCutoverTime = 0;
    lastPhaseIdleTime = 0;
	m_bAlertEndSent = false;
}

Uint8 FirmwareUpdateManager::getSessionType() {
    return sessionType;
}

bool FirmwareUpdateManager::initClientDownload(Address64 deviceAddress64, std::string fileName) {
    initSession();
    sessionType = CLIENT_DOWNLOAD;
    peerDeviceAddress = deviceAddress64;
    this->fileName = fileName;
    startTime = NE::Common::ClockSource::getCurrentTime();
    lastActivityTime = startTime;


    blockSize = SmSettingsLogic::instance().fileUploadChunkSize;
    std::string pathToFile = SmSettingsLogic::instance().firmwareFilesDirectory;

    if (pathToFile[pathToFile.length() - 1] != '/') {
        pathToFile += "/";
    }
    pathToFile += fileName;

    if (fileRegion.get() == NULL)
    {
    	fileRegion = boost::shared_ptr<MMappedFile>(new MMappedFile());
    }

    if (!fileRegion->init(pathToFile.c_str(), fileSize)) {
        return false;
    }

    blocksNo = (fileSize + blockSize - 1) / blockSize;

    return true;
}

bool FirmwareUpdateManager::fileSetup(Address64 deviceAddress64, std::string fileName) {
    initSession();
    sessionType = SERVER_DOWNLOAD;
    startTime = NE::Common::ClockSource::getCurrentTime();
    lastActivityTime = startTime;
    currentBlockId = 0;
    fileBlocks.clear();
    peerDeviceAddress = deviceAddress64;
    this->fileName = fileName;

    std::string pathToFile = SmSettingsLogic::instance().firmwareFilesDirectory;

    if (pathToFile[pathToFile.length() - 1] != '/') {
        pathToFile += "/";
    }

    pathToFile += "tst.tst";

    ofstream firmFile(pathToFile.c_str(), ios::trunc | ios::out | ios::binary);
    if (firmFile.is_open()) {
        firmFile.close();
        return true;
    }

    LOG_ERROR("UDO::File << " << pathToFile << " can't be created!");
    return false;
}

bool FirmwareUpdateManager::apply() {

    std::string pathToFile = SmSettingsLogic::instance().firmwareFilesDirectory;

    if (pathToFile[pathToFile.length() - 1] != '/') {
        pathToFile += "/";
    }
    pathToFile += fileName;

    ofstream firmFile(pathToFile.c_str(), ios::trunc | ios::out | ios::binary);
    if (!firmFile.is_open()) {
        LOG_ERROR("UDO::File << " << pathToFile << " does not exist!");
        return false;
    }


    for (std::vector<BytesPointer>::iterator it = fileBlocks.begin(); it != fileBlocks.end(); ++it) {
        firmFile.write((const char*)(*it)->c_str(), (*it)->size());
    }

    firmFile.close();

    fileBlocks.clear();

    updateTime(ClockSource::getCurrentTime());

    return true;
}

void FirmwareUpdateManager::initServerDownload(Uint16 blockSize, Uint32 fileSize, Uint8 operationMode) {
    sessionType = SERVER_DOWNLOAD;
    this->blockSize = blockSize;
    this->fileSize = fileSize;
    this->blocksNo = (fileSize + blockSize - 1) / blockSize;
    updateTime(ClockSource::getCurrentTime());
}

SFC::SFCEnum FirmwareUpdateManager::confirmBlock(Uint16 blockId, BytesPointer bytesPointer) {
    updateTime(ClockSource::getCurrentTime());

    // specification
    if (currentBlockId == blockId) {
        return SFC::dataSequenceError;
    } else
    if (currentBlockId + 1 != blockId) {
        return SFC::invalidBlockNumber;
    } else if ((blockId != blocksNo && bytesPointer->size() != blockSize) ||
               (blockId == blocksNo && (Uint16)bytesPointer->size() != (fileSize % blockSize == 0 ? blockSize : fileSize % blockSize))) {
        return SFC::blockDataError;
    }

    currentBlockId++;
    LOG_DEBUG("UDO::File  received block=" << (int) blockId << "  size=" << bytesPointer->size());
    fileBlocks.push_back(bytesPointer);

    return SFC::success;
}

SFC::SFCEnum FirmwareUpdateManager::isDownloadComplete(Uint8 rationale) {
    updateTime(ClockSource::getCurrentTime());

    if (rationale == CANCELED) {
        setPhase(IDLE);
        return SFC::success;
    } else if (currentBlockId != blocksNo) {
        phase = DL_ERROR;
        return SFC::unexpectedMethodSequence;
    } else {
        setPhase(DL_COMPLETE);
        return SFC::success;
    }
}

void FirmwareUpdateManager::checkInitStatus() {
    if (sessionType == NOT_INITIALIZED) {
        throw NE::Common::NEException(
                    "FirmwareUpdateManager has not been initialized! First call the initXXX() method.");
    }
}

bool FirmwareUpdateManager::hasBeenInitialized() const {
    return sessionType != NOT_INITIALIZED;
}

bool FirmwareUpdateManager::hasMoreBlocks() {
    checkInitStatus();

    return (currentBlockId < blocksNo);
}

Uint16 FirmwareUpdateManager::getBlocksNo() {
    checkInitStatus();

    return blocksNo;
}

BytesPointer FirmwareUpdateManager::nextBlock() {
    checkInitStatus();
    int bs = currentBlockId + 1 < blocksNo ? blockSize : fileSize - (blocksNo - 1) * blockSize;
    char * o = fileRegion->getRegion() + (blockSize * currentBlockId);
    BytesPointer bp(new Bytes(o, o + bs));
    currentBlockId++;
    return bp;
}

void FirmwareUpdateManager::setRetryBlock(Uint32 lastActivityTime_) {

    if(currentBlockId > 0) {
        currentBlockId--;
    }

    updateTime(lastActivityTime_);
}

void FirmwareUpdateManager::updateTime(Uint32 lastActivityTime_) {
    //lastActivityTime =  ClockSource::getCurrentTime();
    lastActivityTime =  lastActivityTime_;
}

bool FirmwareUpdateManager::confirmBlock() {
    checkInitStatus();

    updateTime(ClockSource::getCurrentTime());

    return true;
}

Uint32 FirmwareUpdateManager::getStartTime() {
    checkInitStatus();

    return startTime;
}

Uint32 FirmwareUpdateManager::getLastActivityTime() {
    checkInitStatus();

    return lastActivityTime;
}

Uint16 FirmwareUpdateManager::getCurrentBlockId() {
    checkInitStatus();

    return currentBlockId;
}

void FirmwareUpdateManager::setCurrentBlockId(Uint16 blockId) {
    checkInitStatus();

    currentBlockId = blockId;
}


Address64 FirmwareUpdateManager::getPeerDeviceAddress64() {
    checkInitStatus();

    return peerDeviceAddress;
}

std::string FirmwareUpdateManager::getFileName() {
    checkInitStatus();

    return fileName;
}

Uint32 FirmwareUpdateManager::getFileSize() {
    return fileSize;
}

Uint16 FirmwareUpdateManager::getBlockSize() {
    return blockSize;
}

Uint16 FirmwareUpdateManager::getNextBlockSize() {
    if (currentBlockId + 1 == blocksNo) {
       return  (fileSize % blockSize == 0 ? blockSize : fileSize % blockSize);
    } else {
        return blockSize;
    }
}

void FirmwareUpdateManager::setPhase(Uint8 phase_) {

    LOG_DEBUG("Device " << peerDeviceAddress.toString() << " - setPhase " << (int)phase_);

    phase = phase_;

    Uint32 currentTime = ClockSource::getCurrentTime();
    updateTime(currentTime);

    if (phase_ == IDLE) {
        lastPhaseIdleTime = currentTime;
    }
}

Uint8 FirmwareUpdateManager::getPhase() {
    return phase;
}

void FirmwareUpdateManager::setStatus(Uint8 status_) {
    status = status_;
}

Uint8 FirmwareUpdateManager::getStatus() {
    checkInitStatus();

    return status;
}
Uint8 FirmwareUpdateManager::getPercentDone() {
    checkInitStatus();

    return (currentBlockId * 100) / blocksNo;
}

bool FirmwareUpdateManager::isClientTimeOut() {
    checkInitStatus();

    // check if the difference between last sent chunk and current time is greater
    // than a given period (set in config file)
    Uint32 udoRetryTimeout = SmSettingsLogic::instance().fileUploadChunkTimeout;
    Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();

    if ((currentTime - lastActivityTime) > udoRetryTimeout) {
        return true;
    }

    return false;
}

bool FirmwareUpdateManager::isServerTimeOut() {
    checkInitStatus();

    // check if the difference between last sent chunk and current time is greater
    // than a given period (set in config file)
    Uint32 udoTimeout = SmSettingsLogic::instance().fileUploadChunkTimeout;
    Uint32 currentTime = NE::Common::ClockSource::getCurrentTime();

    if ((currentTime - lastActivityTime) > udoTimeout * 100) {
        return true;
    }

    return false;
}

Uint16 FirmwareUpdateManager::getExceedMaxRetries() {

    return retriesNumbers;
}

bool FirmwareUpdateManager::exceedMaxRetries() {

    return (++retriesNumbers > MAX_RETRIES);
}

void FirmwareUpdateManager::resetRetries() {
    retriesNumbers = 0;
}


AppHandle FirmwareUpdateManager::getAppHandle() {
    return appHandle;
}

void FirmwareUpdateManager::setAppHandle(AppHandle appHandle) {
    this->appHandle = appHandle;
}

std::ostream& operator<<(std::ostream & stream, const FirmwareUpdateManager & firmwareUpdateManager) {

    if (!firmwareUpdateManager.hasBeenInitialized()) {
        stream << "Object has not been initialized!";
    } else {
        stream << "peerDeviceAddress = " << firmwareUpdateManager.peerDeviceAddress.toString();
        stream << ", fileName = " << firmwareUpdateManager.fileName;
        stream << ", blocksNo = " << (int)firmwareUpdateManager.blocksNo;
        stream << ", currentBlockId = " << (int)firmwareUpdateManager.currentBlockId;
        stream << ", startTime = " << (long long)firmwareUpdateManager.startTime;
        stream << ", fileSize = " << (long long)firmwareUpdateManager.fileSize;
        stream << ", phase = " << (int)firmwareUpdateManager.phase;
        stream << ", status = " << (int)firmwareUpdateManager.status;

        std::string timeStr;
        Time::toString(firmwareUpdateManager.lastPhaseIdleTime, timeStr);
        stream << ", lastPhaseIdleTime = " << timeStr;
    }

    return stream;
}

}
}
}
