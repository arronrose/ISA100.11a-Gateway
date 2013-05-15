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
 * DO.cpp
 *
 *  Created on: Mar 18, 2009
 *      Author: sorin.bidian
 */

#include "DO.h"
#include "AL/ObjectsIDs.h"
#include "AL/ActiveDevicesTable.h"
#include "Common/AttributesIDs.h"
#include "Common/HandleFactory.h"
#include "Common/MethodsIDs.h"
#include "Common/Utils/DllUtils.h"
#include "Common/NETypes.h"
#include "ASL/PDUUtils.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "Model/EngineProvider.h"
#include "Common/SmSettingsLogic.h"
#include "Model/DllStructures.h"
#include "Model/ModelPrinter.h"

using namespace Isa100::AL;
using namespace Isa100::ASL;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace NE::Common;
using namespace NE::Model;

namespace Isa100 {
namespace AL {
namespace SMAP {


#define WRONG_PUBLISH \
    ++publisherDevice->nrOfwrongPublishReceived;\
    if (publisherDevice->nrOfwrongPublishReceived >= Device::wrongPublishThreshold) {\
        LOG_WARN("Nr of wrong publish received(" << (int)publisherDevice->nrOfwrongPublishReceived << ") elapse the threshold(" << (int)Device::wrongPublishThreshold << "); reconfigure publish...");\
        engine->getSubnetsContainer().getSubnet(publisherDevice->address32)->addNotPublishingDevice(publisherDevice->address32);\
    };


DO::DO(Uint16 objectID_, Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_), objectID(objectID_) {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    //	confirmDuration = 120;
    //	confirmStartTime = 0;
}

DO::~DO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}


void DO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);
    if (executeRequest->methodID == Isa100::Common::MethodsID::DOMethodID::Configure_Object) {
        setConfigurations(executeRequest->parameters);
        sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes()), false);
    } else if (executeRequest->methodID == Isa100::Common::MethodsID::DOMethodID::Get_Device_Statistics) {
        sendDeviceStatistics();
    } else {
        sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), false);
        THROW_EX(NE::Common::NEException, "InvokeMethod: unknown method id: " << (int) executeRequest->methodID << ". Discarding packet.")
        ;
    }
}

NE::Model::Tdma::ChannelDiagnostics getChannelDiagnostics(ChannelDiag& channelDiags)
{
    NE::Model::Tdma::ChannelDiagnostics channelDiagnostics;

    for (int i = 0; i < 16; i++)
    {
        NE::Model::Tdma::ChannelDiagnostics::Diag diag;
        diag.ccaBackoff = channelDiags.channelTransmissionList[i].ccaBackoff;
        diag.noAck = channelDiags.channelTransmissionList[i].noAck;

        channelDiagnostics.Channels.insert(std::make_pair((Uint8)i, diag));
    }

    return channelDiagnostics;
}

void DO::indicate(Isa100::ASL::Services::ASL_Publish_IndicationPointer indication) {
    ASL::PDU::PublishPDUPointer publishPDU = PDUUtils::extractPublishedData(indication->apduPublish);
    NetworkOrderStream publishedDataStream(publishPDU->data);

    Device *publisherDevice = engine->getDevice(engine->getAddress32(indication->publisherAddress));
    RETURN_ON_NULL_MSG(publisherDevice, "Publisher not found: " << Address_toStream(engine->getAddress32(
                indication->publisherAddress)));

    PhyEntityIndexList *expectedAttributes = publisherDevice->phyAttributes.hrcoEntityIndexListAttribute.currentValue;
    if (publisherDevice->phyAttributes.hrcoEntityIndexListAttribute.previousValue
                && publisherDevice->phyAttributes.hrcoEntityIndexListAttribute.isOnPending()) {
        expectedAttributes = publisherDevice->phyAttributes.hrcoEntityIndexListAttribute.previousValue;
    }

    RETURN_ON_NULL_MSG(expectedAttributes, "Expected attributes for publish - not configured. Device " << Address_toStream(
                publisherDevice->address32));

    char publisherStr[64];
    sprintf(publisherStr, "publisher=%x", publisherDevice->address32);

    LOG_INFO(publisherStr << ", wrong publish=" << (int)publisherDevice->nrOfwrongPublishReceived << ", threshold=" << (int)Device::wrongPublishThreshold
                << ", expectedAttributes=" << *expectedAttributes);

//    std::list<NeighborDiag> neighborDiagList;

    for (EntityIndexList::iterator it = expectedAttributes->value.begin(); it != expectedAttributes->value.end(); ++it) {
        EntityType::EntityTypeEnum entityType = getEntityType(*it);
        if (entityType == EntityType::NeighborDiag) {
            try {
                EntityIndex eiNeighbor = createEntityIndex(publisherDevice->address32, EntityType::Neighbour, getIndex(*it));
                NeighborIndexedAttribute::iterator itNeighbors = publisherDevice->phyAttributes.neighborsTable.find(eiNeighbor);
                if (itNeighbors == publisherDevice->phyAttributes.neighborsTable.end()) {

                    LOG_WARN(publisherStr << ", expected entity(" << std::hex << eiNeighbor << ") is missing.");
                    WRONG_PUBLISH;
                    return; //stop parsing when NeighborDiag is expected for a removed neighbor
                }

                Uint16 publisherSubnet = publisherDevice->capabilities.dllSubnetId;
                //check that the parent of the publisher has not changed since the device was configured for publish
                //if the parent has changed, neighbor diagnostics are not collected
                //OBS: this is valid only when the publish was configured with the parent as neighbor!
//                 EntityIndex entityIndex = *it;
//                Address32 configuredParent = engine->createAddress32(publisherSubnet, getIndex(entityIndex));
//                if (configuredParent != publisherDevice->parent32) {
//                    LOG_WARN(publisherStr << ", the parent has changed for device " << publisherDevice->address64.toString() << ". old parent="
//                                << Address_toStream(configuredParent) << " new parent=" << Address_toStream(
//                                publisherDevice->parent32) << ". Skipping NeighborDiag.");
//                    continue;
//                }

                //index is neighbor's address
                Uint16 index = Isa100::Common::Utils::unmarshallExtDLUint(publishedDataStream);
                Address32 neighborAddress32 = engine->createAddress32(publisherSubnet, index);
                Uint8 diagLevel = engine->getDiagLevel(publisherDevice->address32, neighborAddress32);
                NeighborDiag neighborDiag(index);
                neighborDiag.unmarshall(publishedDataStream, diagLevel);
                LOG_INFO(publisherStr << ":" << neighborDiag);

                //store device statistics for the neighbor
                Device *neighborDevice = engine->getDevice(neighborAddress32);
                if (neighborDevice == NULL) {
                    LOG_WARN(publisherStr << ", neighbor no longer exists: " << Address_toStream(neighborAddress32));
                    continue;
                }

                engine->addDiagnostics(publisherDevice, neighborDevice, neighborDiag.summary.rssi, neighborDiag.summary.rsqi,
                            neighborDiag.summary.txSuccessful, neighborDiag.summary.rxDPDU, neighborDiag.summary.txFailed,
                            neighborDiag.summary.txNACK);

            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR(publisherStr << ", NeighborDiag fail, reason=" << ex.what());
                WRONG_PUBLISH;
                return;
            } catch (NE::Common::NEException& ex) {
                LOG_ERROR(publisherStr << ", reason=" << ex.what());
                return;
            } catch (std::exception& ex) {
                LOG_ERROR(publisherStr << ", reason=" << ex.what());
                return;
            }
        } else if (entityType == EntityType::Candidate) {
            //this is the last attribute in pub packet. It is possible to not be present
            if (publishedDataStream.remainingBytes() == 0) {
                LOG_INFO(publisherStr << ", candidates not present in publication packet.");
                continue;
            }
            try {
                Candidates candidates;
                candidates.unmarshall(publishedDataStream);
                LOG_INFO(publisherStr << ", candidates : " << candidates);

                for (int i = 0; i < candidates.candidatesCount; ++i) {
                    PhyCandidate phyCandidate;
                    phyCandidate.neighbor = candidates.neighborRadioList[i].neighbor;
                    phyCandidate.radio = candidates.neighborRadioList[i].radio;

                    engine->addCandidate(publisherDevice, phyCandidate);
                }
            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR(publisherStr << ", Candidate fail, reason=" << ex.what());
                WRONG_PUBLISH;
                return;
            } catch (NE::Common::NEException& ex) {
                LOG_ERROR(publisherStr << ", reason=" << ex.what());
                return;
            } catch (std::exception& ex) {
                LOG_ERROR(publisherStr << ", reason=" << ex.what());
                return;
            }
        } else if (entityType == EntityType::ChannelDiag) {
            try {
                ChannelDiag channelDiag;
                channelDiag.unmarshall(publishedDataStream);

                LOG_INFO(publisherStr << ", channelDiag : " << channelDiag);

                if (SmSettingsLogic::instance().channelBlacklistingEnabled
                            && publisherDevice->capabilities.isBackbone()) {

                    engine->processChannelBlackListing(getChannelDiagnostics(channelDiag), publisherDevice->address32);
                    //TODO - check if PhyChannelDiag can be used

                    LOG_DEBUG(publisherStr << ", forwarded ChannelDiags to NetworkEngine");
                }

                PhyChannelDiag phyChannelDiag;
                phyChannelDiag.count = channelDiag.count;
                for (int i = 0; i < (int) channelDiag.channelTransmissionList.size(); i++) {
                    PhyChannelDiag::ChannelTransmission channelTransmission;
                    channelTransmission.noAck = channelDiag.channelTransmissionList[i].noAck;
                    channelTransmission.ccaBackoff = channelDiag.channelTransmissionList[i].ccaBackoff;
                    phyChannelDiag.channelTransmissionList.push_back(channelTransmission);
                }
                engine->addDiagnostics(publisherDevice, phyChannelDiag);

            } catch (NE::Misc::Marshall::StreamException& ex) {
                LOG_ERROR(publisherStr << ", ChannelDiag fail, reason=" << ex.what());
                WRONG_PUBLISH;
                return;
            } catch (NE::Common::NEException& ex) {
                LOG_ERROR(publisherStr << ", reason=" << ex.what());
                return;
            } catch (std::exception& ex) {
                LOG_ERROR(publisherStr << ", reason=" << ex.what());
                return;
            }
        } else {
            THROW_EX(NE::Common::NEException, publisherStr << ", unsupported publish for: " << entityType << ". Discarding packet.");
        }
    }
    // reset nrOfwrongPublishReceived
    publisherDevice->nrOfwrongPublishReceived = 0;
}

void DO::setConfigurations(BytesPointer configParams) {
    NetworkOrderStream stream(configParams);
    Address64 publisherAddress64;
    publisherAddress64.unmarshall(stream);

    //remove previous settings for the device, if they exist
    std::map<Address64, ObjectAttributeIndexAndSizeList>::iterator it = devicePublishedDataList.find(publisherAddress64);
    if (it != devicePublishedDataList.end()) {
        devicePublishedDataList.erase(it);
    }

    Uint8 numberOfEntries;
    stream.read(numberOfEntries);
    ObjectAttributeIndexAndSize entry;
    for (int i = 0; i < numberOfEntries; ++i) {
        entry.unmarshall(stream);
        devicePublishedDataList[publisherAddress64].push_back(entry);
    }

}

void DO::sendDeviceStatistics() {
    NetworkOrderStream stream;
    stream.write((Uint16) deviceStatisticsMap.size());
    for (DeviceStatisticsMap::iterator it = deviceStatisticsMap.begin(); it != deviceStatisticsMap.end(); ++it) {
        Address64 address64 = it->first;
        address64.marshall(stream); //write addr64
        stream.write(it->second.rssi);
        stream.write(it->second.txSuccessful);
        stream.write(it->second.txFailed);
        stream.write(it->second.txCCA_Backoff);
    }

    sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(stream.ostream.str())), false);
}




}
}
}
