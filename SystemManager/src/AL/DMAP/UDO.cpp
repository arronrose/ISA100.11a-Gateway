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
 * @author Ioan V. Pocol, radu.pop, beniamin.tecar, sorin.bidian
 */
#include "UDO.h"

#include "Model/Device.h"

#include "Common/HandleFactory.h"

#include "Misc/Convert/Convert.h"
#include "AL/ObjectsIDs.h"
#include "Model/EngineProvider.h"
#include "Model/Operations/AlertOperation.h"
#include "Common/SmSettingsLogic.h"
#include "Common/Utils/ContractUtils.h"
#include "Common/AttributesIDs.h"
#include "ASL/PDUUtils.h"
#include "ASL/Services/ASL_Service_PrimitiveTypes.h"

using namespace Isa100::ASL;
using namespace Isa100::ASL::Services;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace NE::Model::Operations;
using namespace NE::Common;

namespace Isa100 {
namespace AL {
namespace DMAP {

namespace UDO_Predicates {

class FindByStatusPredicate {
    private:
        Uint8 status;

    public:
        FindByStatusPredicate(const Uint8 status_) :
            status(status_) {
        }

        bool operator()(const DevicesActivationStatusEntry& entry) {
            return (entry.second.second == status);
        }
};

class FindByAddressPredicate {
    private:
        Address128 address128;

    public:
        FindByAddressPredicate(const Address128& address128_) :
            address128(address128_) {
        }

        bool operator()(const DevicesActivationStatusEntry& entry) {
            return (entry.second.first == address128);
        }
};

}

UDO::UDO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    lastActivity = NE::Common::ClockSource::getCurrentTime();
    lastUdoAlertTimestamp = lastActivity;

    isActivationProcessing = false;

    registerDeleteDevice();
}

UDO::~UDO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

void UDO::deviceDeletedCallback(Address32 deletedDevAddr32, Uint16 deviceType) {

    LOG_INFO("deviceDeletedCallback - device=" << Address::toString(deletedDevAddr32) << ", deviceType=" << (int)deviceType);

    // set removing device session phase as IDLE
    Device * removingDevice = engine->getDevice(deletedDevAddr32);
    if (!removingDevice) {
        LOG_WARN("Removing device " << Address_toStream(deletedDevAddr32) << " not found");
        return;
    }

    UploadDownloadSessions::iterator itSession = uploadDownloadSessions.find(removingDevice->address64);
    if (itSession != uploadDownloadSessions.end()) {
        itSession->second.setPhase(IDLE);
        itSession->second.setFwManager2DeviceContractID(0);
        engine->deleteContractForFirmwareUpdate(removingDevice->address64);
        if(!itSession->second.m_bAlertEndSent) {
            sendAlertTransferEnd( removingDevice->address64, UDOALERT_FAIL);
            itSession->second.m_bAlertEndSent = true;
            LOG_INFO("sendAlertTransferEnd: " << removingDevice->address64.toString() << " " << __FUNCTION__ << " " << __LINE__);
        }
    }
}

void UDO::registerDeleteDevice() {
    // TODO: BENI check if is called before load the subnets
    SubnetsMap& subnetsMap = engine->getSubnetsList();
    for (SubnetsMap::iterator itSubnet = subnetsMap.begin(); itSubnet != subnetsMap.end(); ++itSubnet) {
        itSubnet->second->registerDeleteDeviceCallback(this);
    }
}

void UDO::execute(Uint32 currentTime) {

    resetLifeTime(currentTime);//resets the timeout alarm. This object has forever life time.

    if (lastActivity != currentTime) {
        lastActivity = currentTime;
        checkTimeouts(currentTime);
    }

    // send TransferProgress alert to GW
    if (lastUdoAlertTimestamp + SmSettingsLogic::instance().udoAlertPeriod <= currentTime) {

        std::vector<DeviceTransferProgress> transferProgressList;
        ///(50 devices + 1 BBR) ensure the memory is never re-allocated. TODO CHANGE THIS when number of devices + BBRs exceeds 51
        transferProgressList.reserve( 51 );

        for (UploadDownloadSessions::iterator it = uploadDownloadSessions.begin(); it != uploadDownloadSessions.end(); ++it) {
            if (it->second.getPhase() != DOWNLOADING) {
                continue;
            }
            DeviceTransferProgress deviceTransferProgress(it->first, it->second.getCurrentBlockId() );
            transferProgressList.push_back(deviceTransferProgress);
        }

        //no need to send alert when there's no firmware downloading
        if (!transferProgressList.empty()) {	/// TODO: MAKE SURE 51 events type UDO fit inside a single alert
            AlertTransferProgress * alertTransferProgress = new AlertTransferProgress(transferProgressList);
            AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::TransferProgress,
                                                                    alertTransferProgress,
                                                                    ClockSource::getTAI(engine->getSettingsLogic())));
            engine->getOperationsProcessor().sendAlert(alertOperation);
        }

        lastUdoAlertTimestamp = currentTime;
    }
}

void UDO::checkTimeouts(Uint32 currentTime) {

    map<Address64, FirmwareUpdateManager>::iterator it;
    for (it = uploadDownloadSessions.begin(); it != uploadDownloadSessions.end();) {
        Address64 sessionAddress64 = it->first;

        Uint8 sessionPhase = it->second.getPhase();
        AppHandle appHandle = it->second.getAppHandle();

        if (sessionPhase == IDLE) {
            if (it->second.getLastPhaseIdleTime() + 5 * 60 < currentTime) { // after 5 minutes
                LOG_INFO("Delete uploadDownloadSession: device=" << it->first.toString() << ", session=" << it->second);
                if(!it->second.m_bAlertEndSent) {
                    sendAlertTransferEnd( it->first, udoAlertTransferStatus( it->second.getStatus() ) );
                    it->second.m_bAlertEndSent = true;
                    LOG_INFO("sendAlertTransferEnd: " << sessionAddress64.toString() << " " << __FUNCTION__ << " " << __LINE__ << " just BEFORE DELETE");
                }
                uploadDownloadSessions.erase(it++);
            } else {
                ++it;
            }
            continue;
        }

        if (it->second.getSessionType() != FirmwareUpdateManager::CLIENT_DOWNLOAD) {
            if (!engine->existsConfirmedDevice(sessionAddress64)) {
            	it->second.setPhase(IDLE);
                it->second.setFwManager2DeviceContractID(0);
                engine->deleteContractForFirmwareUpdate(sessionAddress64);
                ++it;
                continue;
            }

            if (it->second.isServerTimeOut()) {
            	it->second.setPhase(DL_ERROR);
                LOG_DEBUG("setPhase(DL_ERROR) 1");
            }

            ++it;
            continue;
        }

        if (!it->second.isClientTimeOut()) {
            ++it;
            continue;
        }

        if (!engine->existsConfirmedDevice(sessionAddress64)) {

            if (sessionPhase != IDLE) {
                if (sessionPhase == DL_ERROR) {
                	it->second.setStatus(CANCELED_BY_CLINET);
                    //CANCELED_NOT_CONFIRMED);
                } else if (sessionPhase == APPLYING) {
                	it->second.setStatus(DONE_WITH_SUCCESS);
                    //APPLY_NOT_CONFIRMED);
                } else {
                    LOG_DEBUG("UDO::checkTimeouts() address=" << sessionAddress64.toString() << ", setStatus(FAILED)");
                    it->second.setStatus(FAILED);
                }

                it->second.setPhase(IDLE);
                it->second.setFwManager2DeviceContractID(0);
                LOG_DEBUG("UDO::checkTimeouts() the device not exists: address=" << sessionAddress64.toString());
                engine->deleteContractForFirmwareUpdate(sessionAddress64);
                if(!it->second.m_bAlertEndSent) {
                    sendAlertTransferEnd( sessionAddress64, udoAlertTransferStatus( it->second.getStatus() ) );
                    it->second.m_bAlertEndSent = true;
                    LOG_INFO("sendAlertTransferEnd: " << sessionAddress64.toString() << " " << __FUNCTION__ << " " << __LINE__);
                }
            }

            ++it;
            continue;
        }

        LOG_DEBUG("checkTimeouts - UDO:: - " << (int)it->second.getExceedMaxRetries());

        if (it->second.exceedMaxRetries()) {
            if (sessionPhase == DL_ERROR) {
            	it->second.setPhase(IDLE);
                it->second.setFwManager2DeviceContractID(0);
                engine->deleteContractForFirmwareUpdate(sessionAddress64);
                ++it;
                continue;
            }

            if (sessionPhase == DOWNLOADING && it->second.getCurrentBlockId() == 0) {
            	it->second.setPhase(IDLE);
                it->second.setFwManager2DeviceContractID(0);
                engine->deleteContractForFirmwareUpdate(sessionAddress64);
                ++it;
                continue;
            }

            it->second.setPhase(DL_ERROR);
            LOG_DEBUG("setPhase(DL_ERROR) 1");

            it->second.setAppHandle(HandleFactory().CreateHandle());
            clientEndDownload(appHandle, sessionAddress64, CANCELED);
            if(!it->second.m_bAlertEndSent) {
                sendAlertTransferEnd(sessionAddress64, UDOALERT_FAIL);
                it->second.m_bAlertEndSent = true;
                LOG_INFO("sendAlertTransferEnd: " << sessionAddress64.toString() << " "<< __FUNCTION__ << " " << __LINE__);
            }

            ++it;
            continue;
        }

        switch (sessionPhase) {
            case DOWNLOADING: {
                if (it->second.getCurrentBlockId() == 0) {

                    LOG_DEBUG("UDO::checkTimeouts() RETRY clientStartDownload() for address=" << sessionAddress64.toString());
                    it->second.updateTime(currentTime);

                    clientStartDownload(appHandle, sessionAddress64, it->second.getBlocksNo(), it->second.getFileSize());

                } else {

                    it->second.setRetryBlock(currentTime);
                    Uint16 nextBlockId = it->second.getCurrentBlockId() + 1;

                    if (it->second.hasMoreBlocks()) {
                        LOG_DEBUG("UDO::checkTimeouts() on address=" << sessionAddress64.toString()
                                    << " retry clientDataDownload for packet " << (int) nextBlockId << " #############################");
                        clientDataDownload(appHandle, sessionAddress64, nextBlockId, it->second.nextBlock());
                    }
                }
                break;
            }
            case DL_COMPLETE: {
                LOG_DEBUG("UDO::checkTimeouts() RETRY clientEndDownload for address=" << sessionAddress64.toString());
                it->second.updateTime(currentTime);
                clientEndDownload(appHandle, sessionAddress64, FINISHED);
                if(!it->second.m_bAlertEndSent) {
                    sendAlertTransferEnd(sessionAddress64, UDOALERT_OK);
                    it->second.m_bAlertEndSent = true;
                    LOG_INFO("sendAlertTransferEnd: " << sessionAddress64.toString() << " "<< __FUNCTION__ << " " << __LINE__ );
                }

                break;
            }
            case DL_ERROR: {
                LOG_DEBUG("UDO::checkTimeouts() RETRY clientEndDownload for address=" << sessionAddress64.toString());
                it->second.updateTime(currentTime);
                clientEndDownload(appHandle, sessionAddress64, CANCELED);
                if(!it->second.m_bAlertEndSent) {
                    sendAlertTransferEnd(sessionAddress64, UDOALERT_FAIL);
                    it->second.m_bAlertEndSent = true;
                    LOG_INFO("sendAlertTransferEnd: " << sessionAddress64.toString() << " "<< __FUNCTION__ << " " << __LINE__ );
                }

                break;
            }
            case APPLYING: {
                LOG_DEBUG("UDO::checkTimeouts() RETRY clientWriteCommand(APPLYING_COMMAND) for address=" << sessionAddress64.toString());
                it->second.updateTime(currentTime);
                clientWriteCommand(appHandle, sessionAddress64, APPLYING_COMMAND);
                break;
            }
            case RESETTING: {
                LOG_DEBUG("UDO::checkTimeouts() RETRY clientWriteCommand(RESET_COMMAND) for address=" << sessionAddress64.toString());
                it->second.updateTime(currentTime);
                clientWriteCommand(appHandle, sessionAddress64, RESET_COMMAND);
                break;
            }
            case READ_STATE: {
                LOG_DEBUG("UDO::checkTimeouts() RETRY clientReadState() for address=" << sessionAddress64.toString());
                it->second.updateTime(currentTime);
                clientReadState(appHandle, sessionAddress64);
                break;
            }
            case READ_LAST_BLOCK: {
                LOG_DEBUG("UDO::checkTimeouts() RETRY clientReadLastBlockDownloaded() for address=" << sessionAddress64.toString());
                it->second.updateTime(currentTime);
                clientReadLastBlockDownloaded(appHandle, sessionAddress64);
                break;
            }
        default:
            LOG_ERROR("UDO::checkTimeouts() - unknow sessionPhase");
        }
        ++it;
    }
}

void UDO::indicateRead(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {

    ASL::PDU::ReadRequestPDUPointer readRequest = PDUUtils::extractReadRequest(indication->apduRequest);

    LOG_DEBUG("UDO::indicateRead. attribute ID: " << (int) readRequest->targetAttribute.attributeID);

    if (readRequest->targetAttribute.attributeID == OPERATION_SUPPORTED) {
        readOperationSupported(indication);
    } else if (readRequest->targetAttribute.attributeID == DESCRIPTION) {
        readDescription(indication);
    } else if (readRequest->targetAttribute.attributeID == STATE) {
        readState(indication);
    } else if (readRequest->targetAttribute.attributeID == MAX_BLOCK_SIZE) {
        readMaxBlockSize(indication);
    } else if (readRequest->targetAttribute.attributeID == MAX_DOWNLOAD_SIZE) {
        readMaxDownloadSize(indication);
    } else if (readRequest->targetAttribute.attributeID == MAX_UPLOAD_SIZE) {
        readMaxUploadSize(indication);
    } else if (readRequest->targetAttribute.attributeID == DOWNLOAD_PREP_TIME) {
        readDownloadPrepTime(indication);
    } else if (readRequest->targetAttribute.attributeID == DOWNLOAD_ACTIVATION_TIME) {
        readDownloadActivationTime(indication);
    } else if (readRequest->targetAttribute.attributeID == UPLOAD_PREP_TIME) {
        readUploadPrepTime(indication);
    } else if (readRequest->targetAttribute.attributeID == UPLOAD_PROCESSING_TIME) {
        readUploadProcessingTime(indication);
    } else if (readRequest->targetAttribute.attributeID == DOWNLOAD_PROCESSING_TIME) {
        readDownloadProcessingTime(indication);
    } else if (readRequest->targetAttribute.attributeID == CUTOVER_TIME) {
        readCutoverTime(indication);
    } else if (readRequest->targetAttribute.attributeID == LAST_BLOCK_DOWNLOADED) {
        readLastBlockDownloaded(indication);
    } else if (readRequest->targetAttribute.attributeID == LAST_BLOCK_UPLOADED) {
        readLastBlockUploaded(indication);
    } else {
        std::ostringstream stream;
        stream << "UDO::read: unknown attribute id: " << (int) readRequest->targetAttribute.attributeID;
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }
}

void UDO::indicateWrite(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    ASL::PDU::WriteRequestPDUPointer writeRequest = PDUUtils::extractWriteRequest(indication->apduRequest);

    LOG_DEBUG("UDO::indicateWrite. attribute ID: " << (int) writeRequest->targetAttribute.attributeID);

    if (writeRequest->targetAttribute.attributeID == COMMAND) {
        writeCommand(indication, writeRequest);
    } else if (writeRequest->targetAttribute.attributeID == CUTOVER_TIME) {
        writeCutoverTime(indication, writeRequest);
    } else {
        std::ostringstream stream;
        stream << "UDO::write: unknown attribute id: " << (int) writeRequest->targetAttribute.attributeID;
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }
}

void UDO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);

    LOG_DEBUG("UDO::indicateExecute. method ID: " << (int) executeRequest->methodID);

    if (executeRequest->methodID == START_DOWNLOAD) {
        serverStartDownload(indication, executeRequest);
    } else if (executeRequest->methodID == DATA_DOWNLOAD) {
        serverDataDownload(indication, executeRequest);
    } else if (executeRequest->methodID == END_DOWNLOAD) {
        serverEndDownload(indication, executeRequest);
    } else if (executeRequest->methodID == START_UPLOAD) {
        serverStartUpload(indication);
    } else if (executeRequest->methodID == DATA_UPLOAD) {
        serverDataUpload(indication);
    } else if (executeRequest->methodID == END_UPLOAD) {
        serverEndUpload(indication);
    } else if (executeRequest->methodID == START_FIRMWARE_UPDATE) {
        startFirmwareUpdate(indication, executeRequest);
    } else if (executeRequest->methodID == GET_FIRMWARE_UPDATE_STATUS) {
        getFirmwareUpdateStatus(indication, executeRequest);
    } else if (executeRequest->methodID == CANCEL_FIRMWARE_UPDATE) {
        cancelFirmwareUpdate(indication, executeRequest);
    } else if (executeRequest->methodID == SET_FILE_NAME) {
        setFileName(indication, executeRequest);
    } else if (executeRequest->methodID == GROUP_FW_ACTIVATION_TAI) {
        configureGroupActivation(indication, executeRequest);
    } else {
        LOG_ERROR("UDO::invokeMethod: unknown method id: " << (int) executeRequest->methodID);
        sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), false);

    }
}

void UDO::confirmRead(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {

    LOG_DEBUG("Received confirmRead response=" << confirm->toString());

    Address32 deviceAddress32 = engine->getAddress32(confirm->serverNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);
    if (itUDSession == uploadDownloadSessions.end()) {
        std::ostringstream stream;
        stream << "confirmRead - upload/download session doesn't exist for address: " << device->address64.toString();
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }


    SFC::SFCEnum feedbackCode = ASL::PDUUtils::extractReadResponse(confirm->apduResponse)->feedbackCode;
    ReadResponsePDUPointer readResponse = PDUUtils::extractReadResponse(confirm->apduResponse);
    NetworkOrderStream stream(readResponse->value);

    if (itUDSession->second.getAppHandle() != confirm->apduResponse->appHandle) {
        LOG_WARN("UDO::confirmRead unexpected response, drop it " << confirm->toString());
        return;
    } else if (feedbackCode != SFC::success) {
        LOG_ERROR("UDO::confirmRead error with SFC: " << (int) feedbackCode);
        return;
    } else if (itUDSession->second.getPhase() == READ_STATE) {
        Uint8 state;
        stream.read(state);

        LOG_DEBUG("UDO::confirmRead READ_STATE: " << (int) state);
        if (state == IDLE) {
            itUDSession->second.setPhase(IDLE);
            LOG_DEBUG("UDO::confirmRead IDLE device state: " << device->address64.toString() << "  status: " << (int) state);
        } else if (state == DOWNLOADING) {
            LOG_DEBUG("UDO::confirmRead -> READ_LAST_BLOCK for: " << device->address64.toString());
            itUDSession->second.setPhase(READ_LAST_BLOCK);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientReadLastBlockDownloaded(itUDSession->second.getAppHandle(), device->address64);
        } else if (state == APPLYING) {
            LOG_DEBUG("UDO::confirmRead : the device is APPLYING");
        } else if (state == DL_COMPLETE) {
            itUDSession->second.setPhase(APPLYING);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientWriteCommand(itUDSession->second.getAppHandle(), device->address64, APPLYING_COMMAND);
            LOG_DEBUG("UDO::confirmRead - : APPLYING_COMMAND");
        } else if (state == DL_ERROR) {
            itUDSession->second.setPhase(RESETTING);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientWriteCommand(itUDSession->second.getAppHandle(), device->address64, RESET_COMMAND);
            LOG_DEBUG("UDO::confirmRead - : RESET_COMMAND");
        } else {
            itUDSession->second.setPhase(IDLE);
            LOG_ERROR("UDO::confirmRead wrong device state: " << device->address64.toString() << ", status: " << (int) state);
        }
    } else if (itUDSession->second.getPhase() == READ_LAST_BLOCK) {
        Uint16 lastBlock;
        stream.read(lastBlock);

        LOG_DEBUG("UDO::confirmRead READ_LAST_BLOCK: " << (int)lastBlock);

        if (lastBlock > itUDSession->second.getBlocksNo()) {
            LOG_ERROR("UDO::confirmRead -> READ_LAST_BLOCK for is: " << (int)lastBlock << " but max blocks are: " << (int)itUDSession->second.getBlocksNo());
        } else if (lastBlock == itUDSession->second.getBlocksNo()) {
            LOG_DEBUG("UDO::confirmRead -> READ_LAST_BLOCK for is last one: " << (int)lastBlock << " go to DL_COMPLETE");
            itUDSession->second.setPhase(DL_COMPLETE);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientEndDownload(itUDSession->second.getAppHandle(), device->address64, FINISHED);
            if(!itUDSession->second.m_bAlertEndSent) {
                sendAlertTransferEnd(device->address64, UDOALERT_OK);
                itUDSession->second.m_bAlertEndSent = true;
                LOG_INFO("sendAlertTransferEnd: " << device->address64.toString() << " "<< __FUNCTION__ << " " << __LINE__ );
            }

        } else {
            itUDSession->second.setPhase(DOWNLOADING);
            itUDSession->second.setCurrentBlockId(lastBlock);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientDataDownload(itUDSession->second.getAppHandle(), device->address64, lastBlock + 1, itUDSession->second.nextBlock());
        }

    } else {
        LOG_ERROR("UDO:: confirmRead unexpected read confirm:" << confirm->toString());
        return;
    }

    itUDSession->second.resetRetries();

    if (itUDSession->second.getPhase() == IDLE) {
        itUDSession->second.setFwManager2DeviceContractID(0);
        engine->deleteContractForFirmwareUpdate(device->address64);
        ///no need to send alert here
    }
}

void UDO::confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {
    LOG_DEBUG("Received confirmWrite response=" << confirm->toString());

    //FW activation confirmations
    if (confirm->serverObject == ObjectID::ID_DPO) {

        SFC::SFCEnum feedbackCode = ASL::PDUUtils::extractWriteResponse(confirm->apduResponse)->feedbackCode;

        DevicesActivationStatus::iterator it = std::find_if(devicesActivationStatus.begin(), devicesActivationStatus.end(),
                    UDO_Predicates::FindByAddressPredicate(confirm->serverNetworkAddress));
        if (it != devicesActivationStatus.end()) {
            if (feedbackCode == SFC::success) {
                it->second.second = 1; //configured
            } else {
                it->second.second = 0;
            }

        }

        //send the response: 1.when there is a timeout, 2.when all the configurations are done
        //at timeout set status=0 for the devices with status=2
        DevicesActivationStatus::iterator itExpecting = std::find_if(devicesActivationStatus.begin(),
                    devicesActivationStatus.end(), UDO_Predicates::FindByStatusPredicate(2));
        if ((feedbackCode == SFC::timeout) || itExpecting == devicesActivationStatus.end()) {

            NetworkOrderStream responseStream;
            responseStream.write((Uint16) devicesActivationStatus.size());
            for (DevicesActivationStatus::iterator itDevices = devicesActivationStatus.begin(); itDevices
                        != devicesActivationStatus.end(); ++itDevices) {

                if (itDevices->second.second == 2) { //at timeout
                    itDevices->second.second = 0;
                }

                itDevices->first.marshall(responseStream); //addr64
                responseStream.write(itDevices->second.second); //status
            }

            sendExecuteResponseToRequester(groupActivationRequest, SFC::success, BytesPointer(
                        new Bytes(responseStream.ostream.str())), false);

            isActivationProcessing = false; //now a new FW activation request can be processed
            devicesActivationStatus.clear();
        }
        return;
    }

    Address32 deviceAddress32 = engine->getAddress32(confirm->serverNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);
    if (itUDSession == uploadDownloadSessions.end()) {
        std::ostringstream stream;
        stream << "confirmWrite - upload/download session doesn't exist for address: " << device->address64.toString();
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }

    SFC::SFCEnum feedbackCode = ASL::PDUUtils::extractWriteResponse(confirm->apduResponse)->feedbackCode;

    if (itUDSession->second.getAppHandle() != confirm->apduResponse->appHandle) {
        LOG_WARN("UDO::confirmWrite unexpected response, drop it " << confirm->toString());
        return;
    } else if (feedbackCode != SFC::success && feedbackCode != SFC::operationAccepted) {
        LOG_DEBUG("UDO::confirmWrite -> READ_STATE for: " << device->address64.toString());
        itUDSession->second.setPhase(READ_STATE);
        itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
        clientReadState(itUDSession->second.getAppHandle(), device->address64);
    } else if (itUDSession->second.getPhase() == APPLYING) {
        itUDSession->second.setPhase(IDLE);
        itUDSession->second.setStatus(DONE_WITH_SUCCESS);
        LOG_DEBUG("UDO::confirmWrite - APPLY CONFIRM for address :" << device->address64.toString());
    } else if (itUDSession->second.getPhase() == RESETTING) {
        itUDSession->second.setPhase(IDLE);
        itUDSession->second.setStatus(CANCELED_BY_CLINET);
        LOG_DEBUG("UDO::confirmWrite - RESET CONFIRM for address :" << device->address64.toString());
    } else {
        LOG_WARN("UDO:: confirmWrite ignored for address :" << device->address64.toString() << "  confirm=" << confirm->toString());
        return;
    }

    itUDSession->second.resetRetries();

    if (itUDSession->second.getPhase() == IDLE) {
        itUDSession->second.setFwManager2DeviceContractID(0);
        engine->deleteContractForFirmwareUpdate(device->address64);
    }
}

void UDO::confirmExecute(Isa100::ASL::Services::PrimitiveConfirmationPointer confirm) {

    Address32 deviceAddress32 = engine->getAddress32(confirm->serverNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);
    if (itUDSession == uploadDownloadSessions.end()) {
        std::ostringstream stream;
        stream << "confirmExecute - upload/download session doesn't exist for address: " << device->address64.toString();
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }

    if (itUDSession->second.getPhase() == IDLE) {
        std::ostringstream stream;
        stream << "UDO::confirmExecute - upload/download session is IDLE for address :" << device->address64.toString();
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }

    SFC::SFCEnum feedbackCode = ASL::PDUUtils::extractExecuteResponse(confirm->apduResponse)->feedbackCode;

    if (itUDSession->second.getAppHandle() != confirm->apduResponse->appHandle) {
        LOG_INFO("UDO::confirmExecute unexpected response, drop it " << confirm->toString());
        return;
    } else if (feedbackCode != SFC::success) {

        //        if (itUDSession->second.getPhase() == DOWNLOADING
        //                    && itUDSession->second.getCurrentBlockId() == 0) {
        //            //do nothing, retry 3 times and then stop
        //            return;
        //        }

        // SM receive invalidBlockNumber - when SM send a packet number X to device and device expect a packet number Y
        // SM receive unexpectedMethodSequence - when SM has phase X for device and device is in phase Y
        if (feedbackCode == SFC::invalidBlockNumber || feedbackCode == SFC::unexpectedMethodSequence) {
            LOG_INFO("UDO::confirmExecute -> READ_STATE for: " << device->address64.toString() << " reported SFC " << feedbackCode);
            itUDSession->second.setPhase(READ_STATE);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientReadState(itUDSession->second.getAppHandle(), device->address64);
        } else {
//            itUDSession->second.setPhase(IDLE);
//            itUDSession->second.setStatus(FAILED);
//            LOG_INFO("UDO::confirmExecute - : setPhase(IDLE).");

            itUDSession->second.setPhase(DL_ERROR);
            LOG_DEBUG("setPhase(DL_ERROR) 1");
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientEndDownload(itUDSession->second.getAppHandle(), device->address64, CANCELED);
            if(!itUDSession->second.m_bAlertEndSent) {	// if a wrong fw binary is sent, the device return SFC 40 on first packet
                Uint8 u8Reason = (feedbackCode == SFC::invalidData) ? UDOALERT_INVAL_FW : UDOALERT_FAIL;
                sendAlertTransferEnd(device->address64, u8Reason);
                itUDSession->second.m_bAlertEndSent = true;
                LOG_INFO("sendAlertTransferEnd: " << device->address64.toString() << " "<< __FUNCTION__ << " " << __LINE__ << " reported SFC " << feedbackCode );
            }
        }
    } else if (itUDSession->second.getPhase() == DL_COMPLETE || itUDSession->second.getPhase() == DL_ERROR) {

        if (itUDSession->second.getPhase() == DL_COMPLETE) {
            itUDSession->second.setPhase(APPLYING);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientWriteCommand(itUDSession->second.getAppHandle(), device->address64, APPLYING_COMMAND);
            LOG_DEBUG("UDO::confirmExecute - : APPLYING_COMMAND");
        } else {
            itUDSession->second.setPhase(RESETTING);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientWriteCommand(itUDSession->second.getAppHandle(), device->address64, RESET_COMMAND);
            LOG_DEBUG("UDO::confirmExecute - : RESET_COMMAND");
        }

    } else if (itUDSession->second.getPhase() == DOWNLOADING) {
        //download data
        itUDSession->second.confirmBlock();

        if (itUDSession->second.hasMoreBlocks()) {
            Uint16 nextChunkId = itUDSession->second.getCurrentBlockId() + 1;
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());

            //            //TODO: IVP test
            //            if ((nextChunkId % 10) == ((lastActivity / 1113) % 10)) {
            //                LOG_WARN("UDO:: testing - force duplicate for: " << address64.toString());
            //                clientDataDownload(uploadDownloadSessions[address64].getRequestId(), address64, nextChunkId, uploadDownloadSessions[address64].nextBlock());
            //                clientDataDownload(uploadDownloadSessions[address64].getRequestId(), address64, nextChunkId, uploadDownloadSessions[address64].nextBlock());
            //            } else if ((nextChunkId % 10) == ((lastActivity / 1111) % 10)) {
            //                LOG_WARN("UDO:: testing - force retry for: " << address64.toString());
            //            } else {

            clientDataDownload(itUDSession->second.getAppHandle(), device->address64, nextChunkId, itUDSession->second.nextBlock());
            //            }
        } else {
            itUDSession->second.setPhase(DL_COMPLETE);
            itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
            clientEndDownload(itUDSession->second.getAppHandle(), device->address64, FINISHED);
            if(!itUDSession->second.m_bAlertEndSent) {
                sendAlertTransferEnd(device->address64, UDOALERT_OK);
                itUDSession->second.m_bAlertEndSent = true;
                LOG_INFO("sendAlertTransferEnd: " << device->address64.toString() << " "<< __FUNCTION__ << " " << __LINE__ );
            }

            LOG_DEBUG("UDO::confirmExecute - upload/download session done for address: " << device->address64.toString());
        }
    } else if (itUDSession->second.getPhase() == READ_STATE || itUDSession->second.getPhase() == READ_LAST_BLOCK) {
        LOG_WARN("UDO::confirmExecute - READ_STATE / READ_LAST_BLOCK -> drop confirm: " << confirm->toString());
    } else {
        std::ostringstream stream;
        stream << "UDO::confirmExecute - upload/download session with errors for address :" << device->address64.toString()
                    << " requestId=" << (int) (confirm->apduResponse->appHandle);
        LOG_ERROR(stream.str());

        return;
    }

    itUDSession->second.resetRetries();
}

///TODO This method is apparently unused. REMOVE
#if 0
void UDO::notifyJoinDevice(Address128& deviceAddress) {
    LOG_DEBUG("notifyJoinDevice device=" << deviceAddress.toString() << ", UploadDownloadSessions: "
                << getUploadDownloadSessionsAsString());

    Address32 deviceAddress32 = engine->getAddress32(deviceAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    try {
        std::map<Address64, FirmwareUpdateManager>::iterator it = uploadDownloadSessions.find(device->address64);
        if (it != uploadDownloadSessions.end() && it->second.getPhase() != IDLE) {
            it->second.setPhase(IDLE);
            it->second.setStatus(FAILED);

            it->second.setFwManager2DeviceContractID(0);
            engine->deleteContractForFirmwareUpdate(it->first);
            if(!it->second.m_bAlertEndSent) { sendAlertTransferEnd( it->first, udoAlertTransferStatus( it->second.getStatus() ) ); it->second.m_bAlertEndSent = true; 
			LOG_INFO("sendAlertTransferEnd: 1 " << it->first.toString() << " " << __FUNCTION__ << " " << __LINE__ );}

            LOG_DEBUG("uploadDownloadSessions[" << device->address64.toString() << "].setStatus=FAILED");
        }
        //        else {
        //            // LOG_DEBUG("uploadDownloadSessions[" << mapping.address64.toString() << "] not found!");
        //        }

        for (UploadDownloadSessions::iterator itSession = uploadDownloadSessions.begin(); itSession
                    != uploadDownloadSessions.end(); itSession++) {
            if (!engine->existsDevice(itSession->first) && it->second.getPhase() != IDLE) {
                itSession->second.setPhase(IDLE);
                itSession->second.setStatus(FAILED);

                itSession->second.setFwManager2DeviceContractID(0);
                engine->deleteContractForFirmwareUpdate(itSession->first);
                if(!itSession->second.m_bAlertEndSent) { sendAlertTransferEnd( itSession->first, udoAlertTransferStatus( itSession->second.getStatus() ) ); itSession->second.m_bAlertEndSent = true; 
				LOG_INFO("sendAlertTransferEnd: 2 " << itSession->first.toString() << " "<< __FUNCTION__ << " " << __LINE__ );}

                LOG_DEBUG("2. uploadDownloadSessions[" << device->address64.toString() << "].setStatus=FAILED");
            }
        }
    } catch (NE::Common::NEException& ex) {
        LOG_ERROR(ex.what());
    } catch (...) {
        LOG_ERROR("notifyJoinDevice unknown exception. Object=" << getObjectID());
    }
}
#endif
void UDO::writeCommand(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::WriteRequestPDUPointer writeRequest) {
    LOG_DEBUG("writeCommand - client: " << indication->clientNetworkAddress.toString());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        sendWriteResponse(indication, SFC::failure);
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    NetworkOrderStream stream(writeRequest->value);
    Uint8 command = 0;
    stream.read(command);
    LOG_DEBUG("writeCommand: " << (int) command);

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);
    if (itUDSession == uploadDownloadSessions.end()) {
        std::ostringstream stream;
        stream << "writeCommand - upload/download session doesn't exist for address: " << device->address64.toString();
        LOG_ERROR(stream.str());
        throw NE::Common::NEException(stream.str());
    }

    if (itUDSession->second.getPhase() == DL_COMPLETE
                || itUDSession->second.getPhase() == DL_ERROR
                || itUDSession->second.getPhase() == APPLICATION_SETUP) {

        if (command == APPLYING_COMMAND) {
            if (itUDSession->second.getTaiCutoverTime() == 0) {
                itUDSession->second.apply();
                itUDSession->second.setPhase(IDLE);
            } else {
                itUDSession->second.setPhase(APPLYING);
            }
            sendWriteResponse(indication, SFC::operationAccepted);
            // UDO server state machine does not send UDO transfer alerts. Avoid sending one on checkTimeouts
            itUDSession->second.m_bAlertEndSent = true;
        } else {
            itUDSession->second.setPhase(IDLE);
            sendWriteResponse(indication, SFC::success);
        }
    } else {
        //the session is reset
        itUDSession->second.setPhase(IDLE);
        sendWriteResponse(indication, SFC::unexpectedMethodSequence);

    }

    UploadDownloadSessions::iterator itSession = uploadDownloadSessions.find(device->address64);
    if (itSession != uploadDownloadSessions.end() && itSession->second.getPhase() == IDLE) {
        itSession->second.setFwManager2DeviceContractID(0);
        engine->deleteContractForFirmwareUpdate(device->address64);
    }
}

void UDO::writeCutoverTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::WriteRequestPDUPointer writeRequest) {
    LOG_DEBUG("writeCutoverTime - client:" << indication->clientNetworkAddress.toString());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        sendWriteResponse(indication, SFC::failure);
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    if (!device->capabilities.isGateway()) {
        sendWriteResponse(indication, SFC::failure);
        LOG_ERROR("CutoverTime attribute can only be written by gateway. Device that attempted to write: " << Address::toString(
                    deviceAddress32));
        return;
    }

    NetworkOrderStream stream(writeRequest->value);
    cutoverTime.unmarshall(stream);

    //    Uint32 taiCutoverTime;
    //    Uint16 fractionalTaiCutoverTime;
    //    stream.read(taiCutoverTime);
    //    stream.read(fractionalTaiCutoverTime);+
    //    itUDSession->second.setTaiCutoverTime(taiCutoverTime);
    //    itUDSession->second.setFractionalTaiCutoverTime(fractionalTaiCutoverTime);

    sendWriteResponse(indication, SFC::success);
}

void UDO::readOperationSupported(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    //1 = Defined size unicast download only
    Uint8 operationSupported = 3;
    stream.write(operationSupported);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readDescription(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;

    //	Bytes description;
    //	description.append("test");
    //	stream.write(description);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readState(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_DEBUG("readState - client:" << indication->clientNetworkAddress.toString());

    NetworkOrderStream stream;

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_WARN(errStream.str());

        Uint8 state = IDLE;
        stream.write(state);
        sendReadResponse(indication, SFC::success, stream);
        return;
    }

    stream.write(uploadDownloadSessions[device->address64].getPhase());

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readMaxBlockSize(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint16 maxBlockSize = SmSettingsLogic::instance().gw_max_NSDU_Size - 64;
    stream.write(maxBlockSize);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readMaxDownloadSize(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint32 maxDownloadSize = 0x7FFFFFFF; //0xFFFFFFFF max payload size is on 15 bits
    stream.write(maxDownloadSize);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readMaxUploadSize(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint32 maxUploadSize = 0x7FFFFFFF; //0xFFFFFFFF max payload size is on 15 bits
    stream.write(maxUploadSize);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readDownloadPrepTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint16 downloadPrepTime = 1;
    stream.write(downloadPrepTime);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readDownloadActivationTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint16 downloadActivationTime = 1;
    stream.write(downloadActivationTime);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readUploadPrepTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint16 uploadPrepTime = 1;
    stream.write(uploadPrepTime);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readUploadProcessingTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint16 uploadProcessingTime = 1;
    stream.write(uploadProcessingTime);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readDownloadProcessingTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    NetworkOrderStream stream;
    Uint16 downloadProcessingTime = 1;
    stream.write(downloadProcessingTime);

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readCutoverTime(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_DEBUG("readCutoverTime - client:" << indication->clientNetworkAddress.toString());

    NetworkOrderStream stream;
    cutoverTime.marshall(stream);

    //    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    //    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    //    if (!device) {
    //        std::ostringstream errStream;
    //        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
    //        LOG_ERROR(errStream.str());
    //
    //        stream.write((Uint32) 0);
    //        stream.write((Uint16) 0);
    //        sendReadResponse(indication, SFC::failure, stream);
    //        return;
    //    }
    //    stream.write(itUDSession->second.getTaiCutoverTime());
    //    stream.write(itUDSession->second.getFractionalTaiCutoverTime());

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readLastBlockDownloaded(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_DEBUG("readLastBlockDownloaded - client:" << indication->clientNetworkAddress.toString());

    NetworkOrderStream stream;

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(errStream.str());

        stream.write((Uint16) 0);
        sendReadResponse(indication, SFC::failure, stream);
        return;
    }

    stream.write(uploadDownloadSessions[device->address64].getCurrentBlockId());

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::readLastBlockUploaded(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_DEBUG("readLastBlockUploaded - client:" << indication->clientNetworkAddress.toString());

    NetworkOrderStream stream;

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(errStream.str());

        stream.write((Uint16) 0);
        sendReadResponse(indication, SFC::failure, stream);
        return;
    }

    stream.write(uploadDownloadSessions[device->address64].getCurrentBlockId());

    sendReadResponse(indication, SFC::success, stream);
}

void UDO::serverStartDownload(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {
    LOG_DEBUG("serverStartDownload - client:" << indication->clientNetworkAddress.toString());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(errStream.str());
        return;
    }

    Uint16 blockSize;
    Uint32 downloadSize;
    Uint8 operationMode;

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);

    //if (!existsSession(device->address64) || itUDSession->second.getPhase() != APPLICATION_SETUP) {
    if ((itUDSession == uploadDownloadSessions.end()) || (itUDSession->second.getPhase() != APPLICATION_SETUP)) {

        NetworkOrderStream responseStream;
        sendResponseToRequester(indication, responseStream, SFC::unexpectedMethodSequence);

    } else {

        LOG_DEBUG("serverStartDownload()  parameters=" << bytes2string(*executeRequest->parameters));

        NetworkOrderStream stream(executeRequest->parameters);

        stream.read(blockSize);
        stream.read(downloadSize);
        stream.read(operationMode);

        LOG_DEBUG("serverStartDownload(): " << " blockSize=" << (int) blockSize << " downloadSize=" << (int) downloadSize
                    << " operationMode=" << (int) operationMode);

        if (blockSize == 0) {
            LOG_ERROR("Invalid block size= " << (int) blockSize);
            sendExecuteResponseToRequester(indication, SFC::invalidArgument, BytesPointer(new Bytes()), false);
            return;
        }

        itUDSession->second.initServerDownload(blockSize, downloadSize, operationMode);
        itUDSession->second.setPhase(DOWNLOADING); // TO BE moved to serverDataDownload(first packet)

        NetworkOrderStream responseStream;
        sendResponseToRequester(indication, responseStream, SFC::success);
    }
}

void UDO::serverDataDownload(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {
    LOG_DEBUG("serverDataDownload - client:" << indication->clientNetworkAddress.toString());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(errStream.str());
        return;
    }

    Uint16 blockId;
    Bytes bytes;
    NetworkOrderStream responseStream;

    LOG_DEBUG("serverDataDownload - BEGIN");

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);

    //if (!existsSession(device->address64) || itUDSession->second.getPhase() != DOWNLOADING)
    if ((itUDSession == uploadDownloadSessions.end()) || (itUDSession->second.getPhase() != DOWNLOADING)) {

        NetworkOrderStream responseStream;
        sendResponseToRequester(indication, responseStream, SFC::unexpectedMethodSequence);

    } else {

        NetworkOrderStream stream(executeRequest->parameters);

        LOG_DEBUG("UDO::serverDataDownload(): next block size=" << itUDSession->second.getNextBlockSize());

        stream.read(blockId);
        stream.read(bytes, itUDSession->second.getNextBlockSize());

        LOG_DEBUG("serverDataDownload(): " << indication->clientNetworkAddress.toString() << "; AppHandle="
                    << (int) indication->apduRequest->appHandle << " blockId: " << blockId << " block: " << bytes2string(bytes));

        BytesPointer bytesPointer(new Bytes(bytes));

        SFC::SFCEnum serviceFeedbackCode = itUDSession->second.confirmBlock(blockId, bytesPointer);

        LOG_DEBUG("serverDataDownload - SFC=" << (int) serviceFeedbackCode);

        if (serviceFeedbackCode == SFC::unexpectedMethodSequence || serviceFeedbackCode == SFC::invalidBlockNumber
                    || serviceFeedbackCode == SFC::dataSequenceError) {

            responseStream.write(itUDSession->second.getCurrentBlockId());
        }

        sendResponseToRequester(indication, responseStream, serviceFeedbackCode);
    }
}

void UDO::serverEndDownload(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {
    LOG_DEBUG("serverDataDownload - client:" << indication->clientNetworkAddress.toString());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(errStream.str());
        return;
    }

    NetworkOrderStream responseStream;

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(device->address64);

    //if (!existsSession(device->address64) || itUDSession->second.getPhase() != DOWNLOADING) {
    if ((itUDSession == uploadDownloadSessions.end()) || (itUDSession->second.getPhase() != DOWNLOADING)) {
        sendResponseToRequester(indication, responseStream, SFC::unexpectedMethodSequence);
    } else {

        NetworkOrderStream stream(executeRequest->parameters);

        LOG_DEBUG("serverEndDownload()  parameters=" << bytes2string(*executeRequest->parameters));

        Uint8 rationale;
        stream.read(rationale);

        sendResponseToRequester(indication, responseStream, itUDSession->second.isDownloadComplete(rationale));
    }
}

void UDO::serverStartUpload(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_ERROR("Not implemented StartUpload method.");
    throw NE::Common::NEException("execute...Not implemented StartUpload method.");
}

void UDO::serverDataUpload(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_ERROR("Not implemented DataUpload method.");
    throw NE::Common::NEException("execute...Not implemented DataUpload method.");
}

void UDO::serverEndUpload(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    LOG_ERROR("Not implemented EndUpload method.");
    throw NE::Common::NEException("execute...Not implemented EndUpload method.");
}

void UDO::clientStartDownload(Uint16 requestId, const Address64& address64, Uint16 blocksCount, Uint32 downloadSize) {

    Uint16 blockSize = SmSettingsLogic::instance().fileUploadChunkSize;
    Uint8 operationMode = 0;

    NetworkOrderStream stream;
    stream.write(blockSize);
    stream.write(downloadSize);
    stream.write(operationMode);

    ASL::PDU::ExecuteRequestPDUPointer forwardedRequest(new ASL::PDU::ExecuteRequestPDU(START_DOWNLOAD, BytesPointer(
                new Bytes(stream.ostream.str()))));

    Isa100::ASL::PDU::ClientServerPDUPointer
                pdu(
                            new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::execute, getObjectID(), ObjectID::ID_UDO, requestId));

    ASL::PDUUtils::appendExecuteRequest(pdu, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(address64);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap,
                (Uint16) TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream errStream;
        errStream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                    << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
        LOG_ERROR(LOG_OI << errStream.str());
        throw NEException(errStream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_UDO, //
                tsap, //
                getObjectID(), //
                pdu));

    LOG_DEBUG("clientStartDownload() - device: " << deviceAddress32 << " params: " << bytes2string(stream.ostream.str()));

    messageDispatcher->Request(primitiveRequest);

    // send AlertTransferStarted to GW
    AlertTransferStarted * alertTransferStarted = new AlertTransferStarted(address64, blockSize, blocksCount, downloadSize);
    AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::TransferStarted, //
                                                            alertTransferStarted, //
                                                            ClockSource::getTAI(engine->getSettingsLogic())));
    engine->getOperationsProcessor().sendAlert(alertOperation);
}

void UDO::clientDataDownload(Uint16 requestId, const Address64& address64, Uint16 crtPacket, BytesPointer bytesPointer) {

    NetworkOrderStream stream;
    stream.write(crtPacket);
    stream.write(*bytesPointer);

    ASL::PDU::ExecuteRequestPDUPointer forwardedRequest(new ASL::PDU::ExecuteRequestPDU(DATA_DOWNLOAD, BytesPointer(
                new Bytes(stream.ostream.str()))));

    Isa100::ASL::PDU::ClientServerPDUPointer pdu(new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request,
                Isa100::Common::ServiceType::execute, getObjectID(), ObjectID::ID_UDO, requestId));

    ASL::PDUUtils::appendExecuteRequest(pdu, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(address64);

//    // Udo-Contract
//    Device *manager = engine->getDevice(engine->getManagerAddress32());
//    ContractIndexedAttribute::iterator itContract = manager->phyAttributes.contractsTable.begin();
//    for(; itContract != manager->phyAttributes.contractsTable.end(); ++itContract) {
//        PhyContract * contract = itContract->second.getValue();
//        if (contract && contract->usedForFirmwareUpdate
//                    && contract->destination32 == deviceAddress32
//                    && contract->sourceSAP == tsap
//                    && contract->destinationSAP == TSAP::TSAP_DMAP) {
//            break;
//        }
//    }
//
//    Uint16 contractID;
//    if (itContract == manager->phyAttributes.contractsTable.end()) { // Udo-Contract
//        NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap, (Uint16) TSAP::TSAP_DMAP);
//        if (!contract_SM_Dev) {
//            std::ostringstream errStream;
//            errStream << "Couldn't find contract(usedForFirmwareUpdate) between manager and " << Address_toStream(deviceAddress32) << ", tsap "
//                        << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
//            LOG_ERROR(LOG_OI << errStream.str());
//            throw NEException(errStream.str());
//        } else {
//            contractID = contract_SM_Dev->contractID;
//        }
//    } else {
//        contractID = itContract->second.getValue()->contractID;
//    }

    Uint16 contractID;
    UploadDownloadSessions::iterator itSession = uploadDownloadSessions.find(address64);
    if (itSession != uploadDownloadSessions.end()) {
        ContractId fwManager2DeviceContractID = itSession->second.getFwManager2DeviceContractID();
        if (fwManager2DeviceContractID != 0) {
            contractID = fwManager2DeviceContractID;
        } else {
            NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap, (Uint16) TSAP::TSAP_DMAP);
            if (!contract_SM_Dev) {
                std::ostringstream errStream;
                errStream << "Couldn't find contract(usedForFirmwareUpdate) between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                            << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
                LOG_ERROR(LOG_OI << errStream.str());
                throw NEException(errStream.str());
            } else {
                contractID = contract_SM_Dev->contractID;
            }
        }
    } else {
        std::ostringstream stream;
        stream << "No session found for device " << address64.toString();
        LOG_ERROR(LOG_OI << stream.str());
        throw NEException(stream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                // contract_SM_Dev->contractID, //
                contractID, // Udo-Contract
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_UDO, //
                tsap, //
                getObjectID(), //
                pdu));

    LOG_DEBUG("clientDataDownload() - device: " << Address_toStream(deviceAddress32) << " params: " << bytes2string(stream.ostream.str()));

    messageDispatcher->Request(primitiveRequest);

}

void UDO::clientEndDownload(Uint16 requestId, const Address64& address64, Uint8 rationale) {

    NetworkOrderStream stream;
    stream.write(rationale);

    ASL::PDU::ExecuteRequestPDUPointer forwardedRequest(new ASL::PDU::ExecuteRequestPDU(END_DOWNLOAD, BytesPointer(
                new Bytes(stream.ostream.str()))));

    Isa100::ASL::PDU::ClientServerPDUPointer pdu(new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request,
                Isa100::Common::ServiceType::execute, getObjectID(), ObjectID::ID_UDO, requestId));

    ASL::PDUUtils::appendExecuteRequest(pdu, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(address64);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap,
                (Uint16) TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream errStream;
        errStream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                    << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
        LOG_ERROR(LOG_OI << errStream.str());
        throw NEException(errStream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_UDO, //
                tsap, //
                getObjectID(), //
                pdu));

    LOG_DEBUG("clientEndDownload() - device: " << deviceAddress32 << " params: " << bytes2string(stream.ostream.str()));

    messageDispatcher->Request(primitiveRequest);
}

void UDO::clientWriteCommand(Uint16 requestId, const Address64& address64, Uint8 command) {

    NetworkOrderStream stream;
    stream.write(command);

    Isa100::Common::Objects::ExtensibleAttributeIdentifier extensibleAttributeIdentifier((Uint16) COMMAND);

    ASL::PDU::WriteRequestPDUPointer forwardedRequest(new ASL::PDU::WriteRequestPDU(extensibleAttributeIdentifier, BytesPointer(
                new Bytes(stream.ostream.str()))));

    Isa100::ASL::PDU::ClientServerPDUPointer
                pdu(
                            new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::write, getObjectID(), ObjectID::ID_UDO, requestId));

    ASL::PDUUtils::appendWriteRequest(pdu, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(address64);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap,
                (Uint16) TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream errStream;
        errStream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                    << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
        LOG_ERROR(LOG_OI << errStream.str());
        throw NEException(errStream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_UDO, //
                tsap, //
                getObjectID(), //
                pdu));

    LOG_DEBUG("clientWriteCommand() - device: " << deviceAddress32 << " params: " << bytes2string(stream.ostream.str()));

    messageDispatcher->Request(primitiveRequest);
}

void UDO::clientReadState(Uint16 requestId, const Address64& address64) {

    NetworkOrderStream stream;

    Isa100::Common::Objects::ExtensibleAttributeIdentifier extensibleAttributeIdentifier((Uint16) STATE);

    ASL::PDU::ReadRequestPDUPointer forwardedRequest(new ASL::PDU::ReadRequestPDU(extensibleAttributeIdentifier));

    Isa100::ASL::PDU::ClientServerPDUPointer
                pdu(
                            new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::read, getObjectID(), ObjectID::ID_UDO, requestId));

    ASL::PDUUtils::appendReadRequest(pdu, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(address64);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap,
                (Uint16) TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream errStream;
        errStream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                    << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
        LOG_ERROR(LOG_OI << errStream.str());
        throw NEException(errStream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_UDO, //
                tsap, //
                getObjectID(), //
                pdu));

    messageDispatcher->Request(primitiveRequest);

    LOG_INFO("clientReadState() - device: " << Address_toStream(deviceAddress32) << " params: " << bytes2string(stream.ostream.str()));
}

void UDO::clientReadLastBlockDownloaded(Uint16 requestId, const Address64& address64) {
    NetworkOrderStream stream;

    Isa100::Common::Objects::ExtensibleAttributeIdentifier extensibleAttributeIdentifier((Uint16) LAST_BLOCK_DOWNLOADED);

    ASL::PDU::ReadRequestPDUPointer forwardedRequest(new ASL::PDU::ReadRequestPDU(extensibleAttributeIdentifier));

    Isa100::ASL::PDU::ClientServerPDUPointer
                pdu(
                            new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::read, getObjectID(), ObjectID::ID_UDO, requestId));

    ASL::PDUUtils::appendReadRequest(pdu, forwardedRequest);

    Address32 deviceAddress32 = engine->getAddress32(address64);
    NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32, (Uint16) tsap,
                (Uint16) TSAP::TSAP_DMAP);

    if (!contract_SM_Dev) {
        std::ostringstream errStream;
        errStream << "Couldn't find contract between manager and " << Address_toStream(deviceAddress32) << ", tsap "
                    << (int) tsap << "->" << (int) TSAP::TSAP_DMAP;
        LOG_ERROR(LOG_OI << errStream.str());
        throw NEException(errStream.str());
    }

    PrimitiveRequestPointer primitiveRequest(new PrimitiveRequest( //
                contract_SM_Dev->contractID, //
                deviceAddress32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_UDO, //
                tsap, //
                getObjectID(), //
                pdu));

    LOG_DEBUG("clientReadLastBlockDownloaded() - device: " << Address_toStream(deviceAddress32) << " params: " << bytes2string(
                stream.ostream.str()));

    messageDispatcher->Request(primitiveRequest);
}

void UDO::startFirmwareUpdate(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {

    Address64 address64;
    Uint16 length;
    std::string fileName;
    Bytes tmpFileName;

    NetworkOrderStream stream(executeRequest->parameters);

    address64.unmarshall(stream);
    stream.read(length);
    stream.read(tmpFileName, length);
    fileName.assign(tmpFileName.begin(), tmpFileName.end());

    LOG_INFO("UDO::startFirmwareUpdate() address=" << address64.toString() << "  fileName=" << fileName);

    Device * device = engine->getDevice(engine->getAddress32(address64));
    if (!device) {
        LOG_ERROR("Device " << address64.toString() << " not found!");
        return;
    }

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(address64);

    if (!engine->existsConfirmedDevice(address64)) {
        //1– device not exists
        NetworkOrderStream responseStream;
        Uint8 status = DEVICE_NOT_EXISTS;
        responseStream.write(status);

        sendResponseToGateway(indication, responseStream);
    //} else if (existsSession(address64) && uploadDownloadSessions[address64].getPhase() != IDLE) {
    } else if ((itUDSession != uploadDownloadSessions.end()) && itUDSession->second.getPhase() != IDLE) {
        //2 - already exists an active firmware update for this device

        NetworkOrderStream responseStream;
        Uint8 status = ALREADY_EXISTS_SESSION;
        responseStream.write(status);

        sendResponseToGateway(indication, responseStream);
    } else {

        FirmwareUpdateManager session;

        if (!device->capabilities.isBackbone()) {
            PhyContract * manager2DevicePhyContract = engine->createContractForFirmwareUpdate(address64);
            session.setFwManager2DeviceContractID(manager2DevicePhyContract->contractID);
        }

        if (session.initClientDownload(address64, fileName)) {
            session.setPhase(DOWNLOADING);
            session.setStatus(NOT_SET);

            LOG_DEBUG("UDO::startFirmwareUpdate() reading file: " << fileName << " Size=" << (int) session.getFileSize()
                        << " BlocksNo=" << (int) session.getBlocksNo());
        } else {
            NetworkOrderStream responseStream;
            Uint8 status = FILE_ERROR;
            responseStream.write(status);

            sendResponseToGateway(indication, responseStream);
            LOG_ERROR("UDO::startFirmwareUpdate() error reading file: " << fileName);
            return;
        }

        uploadDownloadSessions[address64] = session;

        uploadDownloadSessions[address64].setAppHandle(HandleFactory().CreateHandle());
        clientStartDownload(uploadDownloadSessions[address64].getAppHandle(), address64, session.getBlocksNo(), session.getFileSize());

        NetworkOrderStream responseStream;
        Uint8 status = SUCCESS;
        responseStream.write(status);
        sendResponseToGateway(indication, responseStream);
    }
}

void UDO::getFirmwareUpdateStatus(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {
    Address64 address64;

    NetworkOrderStream stream(executeRequest->parameters);
    address64.unmarshall(stream);

    LOG_DEBUG("UDO::getFirmwareUpdateStatus() address=" << address64.toString());

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(address64);

    //if (!existsSession(address64)) {
    if (itUDSession == uploadDownloadSessions.end()) {
        Uint8 percentDone = 0;
        Uint8 phase = 0;

        NetworkOrderStream responseStream;
        Uint8 status = NO_SESSION;
        responseStream.write(status);
        responseStream.write(phase);
        responseStream.write(percentDone);

        sendResponseToGateway(indication, responseStream);

    } else {

        FirmwareUpdateManager& session = itUDSession->second;

        Uint8 percentDone = session.getPercentDone();
        Uint8 phase = session.getPhase();
        Uint8 status = phase != IDLE ? SUCCESS : session.getStatus();

        NetworkOrderStream responseStream;

        responseStream.write(status);
        responseStream.write(phase);
        responseStream.write(percentDone);

        sendResponseToGateway(indication, responseStream);

    }
}

void UDO::cancelFirmwareUpdate(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {
    Address64 address64;

    NetworkOrderStream stream(executeRequest->parameters);
    address64.unmarshall(stream);

    LOG_DEBUG("UDO::cancelFirmwareUpdate() address=" << address64.toString());

    UploadDownloadSessions::iterator itUDSession = uploadDownloadSessions.find(address64);

    if (!engine->existsConfirmedDevice(address64)) {
        //1– device not exists

        NetworkOrderStream responseStream;
        Uint8 status = DEVICE_NOT_EXISTS;
        responseStream.write(status);

        sendResponseToGateway(indication, responseStream);
    //} else if (!existsSession(address64) || uploadDownloadSessions[address64].getPhase() == IDLE) {
    } else if ((itUDSession == uploadDownloadSessions.end()) || itUDSession->second.getPhase() == IDLE) {

        NetworkOrderStream responseStream;
        Uint8 status = NO_SESSION;
        responseStream.write(status);

        sendResponseToGateway(indication, responseStream);
    } else {
        itUDSession->second.setPhase(DL_ERROR);
        LOG_DEBUG("setPhase(DL_ERROR) 1");
        itUDSession->second.setAppHandle(HandleFactory().CreateHandle());
        clientEndDownload(itUDSession->second.getAppHandle(), address64, CANCELED);
        if(!itUDSession->second.m_bAlertEndSent) {
            sendAlertTransferEnd(address64, UDOALERT_CANCEL);
            itUDSession->second.m_bAlertEndSent = true;
            LOG_INFO("sendAlertTransferEnd: " << address64.toString() << " "<< __FUNCTION__ << " " << __LINE__ );
        }

        NetworkOrderStream responseStream;
        Uint8 status = SUCCESS;
        responseStream.write(status);

        sendResponseToGateway(indication, responseStream);
    }
}

void UDO::setFileName(Isa100::ASL::Services::PrimitiveIndicationPointer indication,
            ASL::PDU::ExecuteRequestPDUPointer executeRequest) {
    Uint16 length;
    std::string fileName;
    Bytes tmpFileName;

    LOG_DEBUG("setFileName()- parameters=" << bytes2string(*executeRequest->parameters));

    NetworkOrderStream stream(executeRequest->parameters);

    stream.read(length);
    stream.read(tmpFileName, length);
    fileName.assign(tmpFileName.begin(), tmpFileName.end());

    Address32 deviceAddress32 = engine->getAddress32(indication->clientNetworkAddress);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream stream;
        stream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(stream.str());
        throw NEException(stream.str());
    }

    LOG_DEBUG("startFirmwareDownload()  address=" << device->address64.toString() << "  fileName=" << fileName);

    if (!engine->existsConfirmedDevice(device->address64)) {
        //1– device not exists
        NetworkOrderStream responseStream;
        Uint8 status = DEVICE_NOT_EXISTS;
        responseStream.write(status);

        sendResponseToGateway(indication, responseStream);
        THROW_EX(NE::Common::NEException, "setFileName() - device not joined: " << device->address64.toString())
        ;
        //ivp: if the file is set the current session is discarded
        //    } else if (existsSession(address64) && uploadDownloadSessions[address64].getPhase() != IDLE) {
        //        //2 - already exists an active firmware update for this device
        //
        //        NetworkOrderStream responseStream;
        //        Uint8 status = SFC::objectStateConflict;
        //        responseStream.write(status);
        //
        //        sendResponseToGateway(indication, responseStream);
    } else {
        FirmwareUpdateManager session;

        if (session.fileSetup(device->address64, fileName)) {
            session.setPhase(APPLICATION_SETUP);
            session.setStatus(NOT_SET);

            LOG_DEBUG("setFileName() write file: " << fileName);

        } else {
            NetworkOrderStream responseStream;
            Uint8 status = FILE_ERROR;
            responseStream.write(status);

            sendResponseToGateway(indication, responseStream);
            LOG_ERROR("setFileName() error writing file: " << fileName);
            return;
        }

        uploadDownloadSessions[device->address64] = session;

        NetworkOrderStream responseStream;
        Uint8 status = SUCCESS;
        responseStream.write(status);
        sendResponseToGateway(indication, responseStream);
    }
}

void UDO::sendResponseToGateway(Isa100::ASL::Services::PrimitiveIndicationPointer indication, const NetworkOrderStream& stream) {

    LOG_DEBUG("UDO::sendResponseToGateway(): " << bytes2string(stream.ostream.str()));

    sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(stream.ostream.str())), false);
}

void UDO::sendResponseToRequester(Isa100::ASL::Services::PrimitiveIndicationPointer indication, const NetworkOrderStream& stream,
            SFC::SFCEnum serviceFeedbackCode) {

    LOG_DEBUG("UDO::sendResponseToRequester(): feedbackCode= " << (int) serviceFeedbackCode << "  parameters=" << bytes2string(
                stream.ostream.str()));

    sendExecuteResponseToRequester(indication, serviceFeedbackCode, BytesPointer(new Bytes(stream.ostream.str())), false);
}

void UDO::sendWriteResponse(Isa100::ASL::Services::PrimitiveIndicationPointer indication, SFC::SFCEnum serviceFeedbackCode) {

    LOG_DEBUG("UDO::sendWriteResponse(): " << (int) serviceFeedbackCode);

    sendWriteResponseToRequester(indication, serviceFeedbackCode, false);
}

void UDO::sendReadResponse(Isa100::ASL::Services::PrimitiveIndicationPointer indication, SFC::SFCEnum serviceFeedbackCode,
            const NetworkOrderStream& stream) {

    LOG_DEBUG("UDO::sendReadResponse(): " << bytes2string(stream.ostream.str()));

    sendReadResponseToRequester(indication, serviceFeedbackCode, BytesPointer(new Bytes(stream.ostream.str())), false);

}

std::string UDO::getUploadDownloadSessionsAsString() {
    std::ostringstream stream;
    for (UploadDownloadSessions::iterator itSession = uploadDownloadSessions.begin(); itSession != uploadDownloadSessions.end(); itSession++) {

        stream << "Address: " << itSession->first.toString() << ", FirmwareUpdateManager: " << itSession->second << std::endl;
    }
    return stream.str();

}

void UDO::configureGroupActivation(PrimitiveIndicationPointer indication, PDU::ExecuteRequestPDUPointer executeRequest) {
    if (isActivationProcessing) {
        LOG_WARN("A group FW activation is already in progress. Discarding request...");
        sendExecuteResponseToRequester(indication, SFC::objectStateConflict, BytesPointer(new Bytes()), false);
        return;
    }

    groupActivationRequest = indication;
    isActivationProcessing = true;

    Uint16 numberOfDevices;
    //Uint16 firmwareVersion;
    Bytes firmwareVersion;
    TAINetworkTimeValue activationTai;

    NetworkOrderStream reqStream(executeRequest->parameters);
    reqStream.read(numberOfDevices);

    Address64 deviceAddress;
    Address128 deviceAddress128;
    for (int i = 0; i < numberOfDevices; ++i) {
        deviceAddress.unmarshall(reqStream);
        devicesActivationStatus[deviceAddress] = std::pair<Address128, Uint8>(deviceAddress128, 0);
    }

    Uint8 length = 0;
    reqStream.read(length);
    reqStream.read(firmwareVersion, length);

    activationTai.unmarshall(reqStream);

    if (length > 16) {
        LOG_ERROR("Firmware version should be maximum 16 bytes long. Actual length is " << (int) length << ". Truncating to 16 bytes..");
        firmwareVersion = firmwareVersion.substr(0, 16);
    }

    //send DPO write commands to all the devices in list
    for (DevicesActivationStatus::iterator itDevices = devicesActivationStatus.begin(); itDevices
                != devicesActivationStatus.end(); ++itDevices) {

        Address32 deviceAddress32 = engine->getAddress32(itDevices->first);
        NE::Model::Device * device = engine->getDevice(deviceAddress32);
        if (!device) {
            std::ostringstream stream;
            stream << "Device [" << Address_toStream(deviceAddress32) << "]" << deviceAddress.toString() << " not found."
                        << " Configurations for FW activation will not be sent to this device.";
            LOG_INFO(stream.str());
            continue; //configurations are not sent to this device; status remains 0
        }

        NE::Model::PhyContract * contract_SM_Dev = engine->findManagerContractToDevice(deviceAddress32,
                    Isa100::Common::TSAP::TSAP_SMAP, Isa100::Common::TSAP::TSAP_DMAP);
        if (!contract_SM_Dev) {
            std::ostringstream stream;
            stream << "Contract not found between manager and [" << Address_toStream(deviceAddress32) << "]"
                        << deviceAddress.toString() << ". Configurations for FW activation will not be sent to this device.";
            LOG_INFO(LOG_OI << stream.str());
            continue; //configurations are not sent to this device; status remains 0
        }

        // 1- command for DPO.27
        Uint16 reqID = HandleFactory().CreateHandle();

        Isa100::ASL::PDU::ClientServerPDUPointer apdu(new Isa100::ASL::PDU::ClientServerPDU( //
                    Isa100::Common::PrimitiveType::request, //
                    Isa100::Common::ServiceType::write, //
                    ObjectID::ID_UDO, //
                    ObjectID::ID_DPO, //
                    reqID));

        ExtensibleAttributeIdentifier targetAttribute(DPO_Attributes::FW_TAI);

        NetworkOrderStream taiStream;
        activationTai.marshall(taiStream);
        Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest(
                    new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute, BytesPointer(new Bytes(taiStream.ostream.str()))));

        apdu = Isa100::ASL::PDUUtils::appendWriteRequest(apdu, writeRequest);

        Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequest(new Isa100::ASL::Services::PrimitiveRequest( //
                    contract_SM_Dev->contractID, //
                    deviceAddress32, //
                    ServicePriority::medium, //
                    false, //
                    Isa100::Common::TSAP::TSAP_DMAP, ObjectID::ID_DPO, //,
                    Isa100::Common::TSAP::TSAP_SMAP, //
                    ObjectID::ID_UDO, //
                    apdu));
        messageDispatcher->Request(primitiveRequest);

        // 2- command for DPO.28
        reqID = HandleFactory().CreateHandle();

        Isa100::ASL::PDU::ClientServerPDUPointer apduV(new Isa100::ASL::PDU::ClientServerPDU( //
                    Isa100::Common::PrimitiveType::request, //
                    Isa100::Common::ServiceType::write, //
                    ObjectID::ID_UDO, //
                    ObjectID::ID_DPO, //
                    reqID));

        ExtensibleAttributeIdentifier targetAttributeV(DPO_Attributes::FW_Version);

        NetworkOrderStream versionStream;
        versionStream.write(firmwareVersion);
        Isa100::ASL::PDU::WriteRequestPDUPointer
                    writeRequestV(new Isa100::ASL::PDU::WriteRequestPDU(targetAttributeV, BytesPointer(
                                new Bytes(versionStream.ostream.str()))));

        apdu = Isa100::ASL::PDUUtils::appendWriteRequest(apdu, writeRequestV);

        Isa100::ASL::Services::PrimitiveRequestPointer primitiveRequestV(new Isa100::ASL::Services::PrimitiveRequest( //
                    contract_SM_Dev->contractID, //
                    deviceAddress32, //
                    ServicePriority::medium, //
                    false, //
                    Isa100::Common::TSAP::TSAP_DMAP, ObjectID::ID_DPO, //,
                    Isa100::Common::TSAP::TSAP_SMAP, //
                    ObjectID::ID_UDO, //
                    apduV));
        messageDispatcher->Request(primitiveRequestV);

        //change devicesActivationStatus.status
        itDevices->second.first = device->address128;
        itDevices->second.second = 2; //expecting confirm
    }

}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send UDO transfer alert
/// @param p_rAddress64
/// @param p_u8ErrCode
/// @remarks the alert is passed to ARMO which in turn will send it to GW
////////////////////////////////////////////////////////////////////////////////
void UDO::sendAlertTransferEnd( const Address64& p_rAddress64, Uint8 p_u8ErrCode)
{
    AlertTransferEnded * alertTransferEnded = new AlertTransferEnded(p_rAddress64, p_u8ErrCode);
    AlertOperationPointer alertOperation(new AlertOperation(SMAlertTypes::TransferEnded, alertTransferEnded, ClockSource::getTAI(engine->getSettingsLogic())));
    engine->getOperationsProcessor().sendAlert(alertOperation);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief return udo alert status depending on the transfer status
/// @param p_u8UDOStatus udo transfer status (FirmwareUpdateManager::status)
/// @remarks TAKE CARE: statuses FINISHED / CANCELED are also sent to device. Double meaning is not ok.
////////////////////////////////////////////////////////////////////////////////
Uint8 UDO::udoAlertTransferStatus( Uint8 p_u8UDOStatus )
{
    switch( p_u8UDOStatus )
    {
        case DONE_WITH_SUCCESS:  return UDOALERT_OK;
        case CANCELED_BY_CLINET: return UDOALERT_CANCEL;
        case FAILED:             return UDOALERT_FAIL;
        default:                 return UDOALERT_OTHER;
    }
}

}
}
}

