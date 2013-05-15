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
 * @author sorin.bidian, catalin.pop, flori.parauan, Beniamin Tecar
 */
#include "SMO.h"
#include "Common/CCM/AuthenticationFailException.h"
#include "Common/HandleFactory.h"
#include "Common/SmSettingsLogic.h"
#include "Common/AttributesIDs.h"
#include "Common/MethodsIDs.h"
#include "Misc/Marshall/NetworkOrderBytesWriter.h"
#include "Misc/Marshall/NetworkOrderStream.h"
#include "Misc/Convert/Convert.h"
#include "Model/Device.h"
#include "Model/EngineProvider.h"
#include "ASL/PDUUtils.h"
#include "ASL/PDU/ExecuteRequestPDU.h"
#include "Stats/Cmds.h"
#include "Common/Objects/ObjectAttributeIndexAndSize.h"
#include "Model/GSAPReports/Reports.h"
#include "Model/AddressAllocator.h"
#include "DurationWatcher.h"

using namespace Isa100::ASL;
using namespace Isa100::ASL::Services;
using namespace Isa100::Common;
using namespace Isa100::Common::Objects;
using namespace Isa100::Model;
using namespace NE::Model;
using namespace NE::Model::Routing;
using namespace NE::Misc::Convert;
using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace AL {
namespace SMAP {

Uint32 HandlerGenerator::newHandler = 0;

Uint32 HandlerGenerator::createHandler() {
    newHandler = (newHandler + 1) % 0xFFFFFFFF;
    return newHandler;
}

SMO::SMO(Isa100::Common::TSAP::TSAP_Enum tsap, NE::Model::IEngine* engine_) :
    Isa100Object(tsap, engine_) {

    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: created object:" << objectIdString);

    lastCheckRejoinsTime = time(NULL);
    jobFinished = true; //will be set to false, IF REQUIRED, when a request is received

}

SMO::~SMO() {
    std::string objectIdString;
    getObjectIDString(objectIdString);
    LOG_DEBUG(LOG_OI << "LIFE: destroyed object:" << objectIdString);
}

bool SMO::expectIndicate(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {
    if ((indication->apduRequest->serviceInfo == Isa100::Common::ServiceType::execute)) {

        ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);

        //after generateReports is called, getBlock requests are expected
        if ((executeRequest->methodID == MethodsID::SMOMethodId::GET_BLOCK)) {
            //check that the handler in the request is among this object's handlers
            NetworkOrderStream stream(executeRequest->parameters);
            Uint32 reqHandler;
            stream.read(reqHandler);
            ReportsMap::iterator itReport = reportsMap.find(reqHandler);
            if (itReport != reportsMap.end()) {
                return true;
            }

        }
    }
    return false;
}

bool SMO::expectConfirm(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation) {
    //expect reset device confirmation
    //it's enough to wait for a write response from DMO as only resetDevice method generates a write request for DMO;
    //any existing instance of SMO may expect the response as it doesn't influence the flow
    if ((confirmation->apduResponse->serviceInfo == Isa100::Common::ServiceType::write) && confirmation->serverObject
                == ObjectID::ID_DMO) {
        return true;
    }

    return false;
}

void SMO::indicateExecute(Isa100::ASL::Services::PrimitiveIndicationPointer indication) {

	ASL::PDU::ExecuteRequestPDUPointer executeRequest = PDUUtils::extractExecuteRequest(indication->apduRequest);

	switch (executeRequest->methodID) {
//		case MethodsID::SMOMethodId::GET_DEVICE_INFORMATION: {
//			jobFinished = false; //job will be finished after DeviceInformation is sent
//			sendDeviceInformation(executeRequest->parameters);
//			break;
//		}
		case MethodsID::SMOMethodId::RESTART_DEVICE: {
			jobFinished = false; //job will be finished after receiving the response for reset
			resetDevice(executeRequest->parameters);
			break;
		}
		case MethodsID::SMOMethodId::GENERATE_REPORT: {
			jobFinished = false; //job will be finished after all all blocks are sent
			BytesPointer reportResponse = generateReport(executeRequest->parameters);
			sendExecuteResponseToRequester(indication, SFC::success, reportResponse, false);
			break;
		}
		case MethodsID::SMOMethodId::GET_BLOCK: {
			getBlock(executeRequest->parameters); //when the last block is sent, jobFinished will be set to 'true'
			break;
		}
		case MethodsID::SMOMethodId::GET_CONTRACTS_AND_ROUTES: {
			jobFinished = false; //job will be finished after the response is sent
			getContractsAndRoutes();
			break;
		}
		case MethodsID::SMOMethodId::GET_CCA_BACKOFF: {
			jobFinished = false; //job will be finished after the response is sent
			getCCABackoff(executeRequest->parameters);
			break;
		}
		default: {
			LOG_ERROR(LOG_OI << "InvokeMethod: unknown method id: " << (int) executeRequest->methodID << "Discarding packet");
			sendExecuteResponseToRequester(indication, SFC::invalidMethod, BytesPointer(new Bytes()), true);
		}
	}

}

void SMO::resetDevice(BytesPointer parameters) {

    NetworkOrderStream stream(parameters);

    Address128 address128;
    Uint8 restartType;

    address128.unmarshall(stream);
    stream.read(restartType);

    LOG_DEBUG(LOG_OI << "resetDevice address128=" << address128.toString() << ", restartType=" << (int) restartType);

    Address32 deviceAddress32 = engine->getAddress32(address128);
    NE::Model::Device * device = engine->getDevice(deviceAddress32);
    if (!device) {
        std::ostringstream errStream;
        errStream << "Device " << Address::toString(deviceAddress32) << " not found.";
        LOG_WARN(errStream.str());
        sendExecuteResponseToRequester(indication, SFC::deviceNotFound, BytesPointer(new Bytes()), true);
        return;
    }

    NetworkOrderStream streamOut;
    streamOut.write(restartType);

    ExtensibleAttributeIdentifier extensibleAttributeIdentifier((Uint16) RESTART_TYPE);

    ASL::PDU::WriteRequestPDUPointer forwardedRequest(new ASL::PDU::WriteRequestPDU(extensibleAttributeIdentifier, BytesPointer(
                new Bytes(streamOut.ostream.str()))));

    Isa100::ASL::PDU::ClientServerPDUPointer
                pdu(
                            new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::write, ObjectID::ID_SMO, ObjectID::ID_DMO, HandleFactory().CreateHandle()));

    ASL::PDUUtils::appendWriteRequest(pdu, forwardedRequest);

    PhyContract* contractManagerDevice = engine->findManagerContractToDevice(deviceAddress32, (Uint16) TSAP::TSAP_SMAP,
                (Uint16) TSAP::TSAP_DMAP);
    if (!contractManagerDevice) {
        std::ostringstream errStream;
        errStream << "Contract SMAP-DMAP between manager and " << Address::toString(deviceAddress32) << " not found.";
        LOG_ERROR(LOG_OI << errStream.str());
        throw NEException(errStream.str());
    }

    PrimitiveRequestPointer primitiveRequest( //
                new PrimitiveRequest(contractManagerDevice->contractID, //
                contractManagerDevice->destination32, //
                ServicePriority::medium, //
                false, //
                TSAP::TSAP_DMAP, //
                ObjectID::ID_DMO, //
                TSAP::TSAP_SMAP, //
                ObjectID::ID_SMO, //
                pdu));

    messageDispatcher->Request(primitiveRequest);

    // set job finished; this will generate an unexpected confirm if the device
    //responds to the write request and there is no other instance of SMO (the device may not respond)
    sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes()), true); // job finished
}


void SMO::confirmWrite(Isa100::ASL::Services::PrimitiveConfirmationPointer confirmation) {
    LOG_DEBUG(LOG_OI << "Received write confirmation=" << confirmation->toString());

    if (confirmation->serverObject == ObjectID::ID_DMO) { // reset device response
        //TODO refine condition if other DMO responses should be awaited
        LOG_DEBUG(LOG_OI << "confirmation received for device reset");
        //do nothing
    }
}


#define SMO_DurationWatcherReal     (20)
#define SMO_DurationWatcherProc     (20)

#define SMO_WATCH_DURATION_DEF(obj) ((obj).WatchDuration( __FILE__, __FUNCTION__, __LINE__,SMO_DurationWatcherReal,SMO_DurationWatcherProc))
//#define SMO_WATCH_DURATION(obj,real_dur,proc_dur) ((obj).WatchDuration( __FILE__, __FUNCTION__, __LINE__,real_dur,proc_dur))

BytesPointer SMO::generateReport(BytesPointer parameters) {
    NetworkOrderStream reqStream(parameters);
    GSAPReports::GenerateReportCmd generateReportCmd;
    generateReportCmd.unmarshall(reqStream);

    LOG_INFO(LOG_OI << "requested: devList=" << (int) generateReportCmd.isDeviceListRequested() << " Topology="
                << (int) generateReportCmd.isTopologyRequested() << " Schedule=" << (int) generateReportCmd.isScheduleRequested()
                << " DeviceHealth=" << (int) generateReportCmd.isDeviceHealthRequested() << " NeighborHealth="
                << (int) generateReportCmd.isNeighborHealthRequested() << " NetworkHealth="
                << (int) generateReportCmd.isNetworkHealthRequested() << " NetworkResource="
                << (int) generateReportCmd.isNetworkResourceRequested());

    Uint32 deviceListHandler = 0;
    Uint32 topologyHandler = 0;
    Uint32 scheduleHandler = 0;
    Uint32 deviceHealthHandler = 0;
    Uint32 neighborHealthHandler = 0;
    Uint32 networkHealthHandler = 0;
    Uint32 networkResourceHandler = 0;

    try {
        if (generateReportCmd.isDeviceListRequested()) {

//            NetworkOrderStream streamOld;
//            NE::Model::Reports::DeviceListReport deviceListReport;
//            engine->getReportsEngine().createDeviceListReport(deviceListReport);
//            GSAPReports::marshall(streamOld, deviceListReport);
//            LOG_DEBUG(LOG_OI << "requested report: device list: OLD: " << bytes2string(streamOld.ostream.str()));

            NetworkOrderStream stream;
            engine->getReportsEngine().createDeviceListReport(stream);

            deviceListHandler = HandlerGenerator::createHandler();
            reportsMap[deviceListHandler] = BytesPointer(new Bytes(stream.ostream.str()));
            reportsReadFlags[deviceListHandler] = false;

            LOG_DEBUG(LOG_OI << "requested report: device list: " << bytes2string(*reportsMap[deviceListHandler]));
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "DeviceList - Unknown exception!" << ex.what());
    }

    try {
        if (generateReportCmd.isTopologyRequested()) {

            CDurationWatcher dw;
            SMO_WATCH_DURATION_DEF(dw);

//            NetworkOrderStream streamOld;
//            NE::Model::Reports::TopologyReport topologyReport;
//            engine->getReportsEngine().createTopologyReport(topologyReport);
//            GSAPReports::marshall(streamOld, topologyReport);
//            LOG_DEBUG(LOG_OI << "requested report: topology: OLD: " << bytes2string(streamOld.ostream.str()));

            NetworkOrderStream stream;
            engine->getReportsEngine().createTopologyReport(stream);

            topologyHandler = HandlerGenerator::createHandler();
            reportsMap[topologyHandler] = BytesPointer(new Bytes(stream.ostream.str()));
            reportsReadFlags[topologyHandler] = false;

            SMO_WATCH_DURATION_DEF(dw);

            LOG_DEBUG(LOG_OI << "requested report: topology: " << bytes2string(*reportsMap[topologyHandler]));
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "Topology - Unknown exception!" << ex.what());
    }

    try {
        if (generateReportCmd.isScheduleRequested()) {
            scheduleHandler = HandlerGenerator::createHandler();

//            NetworkOrderStream streamOld;
//            NE::Model::Reports::ScheduleReport scheduleReport;
//            engine->getReportsEngine().createScheduleReport(scheduleReport);
//            GSAPReports::marshall(streamOld, scheduleReport);
//            LOG_DEBUG(LOG_OI << "requested report: schedule: OLD: " << bytes2string(streamOld.ostream.str()));

            NetworkOrderStream stream;
            engine->getReportsEngine().createScheduleReport(stream);

            reportsMap[scheduleHandler] = BytesPointer(new Bytes(stream.ostream.str()));
            reportsReadFlags[scheduleHandler] = false;

            LOG_DEBUG(LOG_OI << "requested report: schedule: " << bytes2string(*reportsMap[scheduleHandler]));
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "Schedule - Unknown exception!" << ex.what());
    }

    try {
        if (generateReportCmd.isDeviceHealthRequested()) {
            deviceHealthHandler = HandlerGenerator::createHandler();

//            NetworkOrderStream streamOld;
//            NE::Model::Reports::DeviceHealthReport deviceHealthReport;
//            engine->getReportsEngine().createDeviceHealthReport(deviceHealthReport);
//            GSAPReports::marshall(streamOld, deviceHealthReport);
//            LOG_DEBUG(LOG_OI << "requested report: device health: OLD: " << bytes2string(streamOld.ostream.str()));

            NetworkOrderStream stream;
            engine->getReportsEngine().createDeviceHealthReport(stream);

            reportsMap[deviceHealthHandler] = BytesPointer(new Bytes(stream.ostream.str()));
            reportsReadFlags[deviceHealthHandler] = false;

            LOG_DEBUG(LOG_OI << "requested report: device health:" << bytes2string(*reportsMap[deviceHealthHandler]));
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "DeviceHealth - Unknown exception!" << ex.what());
    }

    try {
        if (generateReportCmd.isNeighborHealthRequested()) {
            try {
                //NetworkOrderStream streamOld;
                NetworkOrderStream stream;
                Address32 address32 = engine->getAddress32(generateReportCmd.deviceNeighborHealth);
                //NE::Model::Reports::NeighborHealthReport neighborHealthReport;
                if (address32 != 0) {
                    //engine->getReportsEngine().createNeighborHealthReport(neighborHealthReport, address32);
                    bool generated = engine->getReportsEngine().createNeighborHealthReport(address32, stream);
                    if (generated) {
                        neighborHealthHandler = HandlerGenerator::createHandler();
                        reportsMap[neighborHealthHandler] = BytesPointer(new Bytes(stream.ostream.str()));
                        reportsReadFlags[neighborHealthHandler] = false;

                        LOG_DEBUG(LOG_OI << "requested report: neighbor health: " << bytes2string(*reportsMap[neighborHealthHandler]));
                    } else {
                        LOG_INFO("Report not generated for device " << generateReportCmd.deviceNeighborHealth.toString()
                                    << "; device is not fully joined and configured.");
                    }

                } else {
                    LOG_ERROR("Report req for removed device: " << generateReportCmd.deviceNeighborHealth.toString());
                }
                //GSAPReports::marshall(streamOld, neighborHealthReport);
                //LOG_DEBUG(LOG_OI << "requested report: neighbor health: OLD: " << bytes2string(streamOld.ostream.str()));

            } catch (NE::Common::AddressNotFoundException ex) {
                LOG_ERROR(LOG_OI << "generateReport - neighborHealthReport: " << ex.what());
            }
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "NeighborHealth - Unknown exception!" << ex.what());
    }

    try {
        if (generateReportCmd.isNetworkHealthRequested()) {

//            NetworkOrderStream streamOld;
//            NE::Model::Reports::NetworkHealthReport networkHealthReport;
//            engine->getReportsEngine().createNetworkHealthReport(networkHealthReport);
//            GSAPReports::marshall(streamOld, networkHealthReport);
//            LOG_DEBUG(LOG_OI << "requested report: network health: OLD: " << bytes2string(streamOld.ostream.str()));

            NetworkOrderStream stream;
            engine->getReportsEngine().createNetworkHealthReport(stream);

            networkHealthHandler = HandlerGenerator::createHandler();
            reportsMap[networkHealthHandler] = BytesPointer(new Bytes(stream.ostream.str()));
            reportsReadFlags[networkHealthHandler] = false;

            LOG_DEBUG(LOG_OI << "requested report: network health: " << bytes2string(*reportsMap[networkHealthHandler]));
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "NetworkHealth - Unknown exception!" << ex.what());
    }

    try {
        if (generateReportCmd.isNetworkResourceRequested()) {

//            NetworkOrderStream streamOld;
//            NE::Model::Reports::NetworkResourceReport networkResourceReport;
//            engine->getReportsEngine().createNetworkResourceReport(networkResourceReport);
//            GSAPReports::marshall(streamOld, networkResourceReport);
//            LOG_DEBUG(LOG_OI << "requested report: network resource: " << bytes2string(streamOld.ostream.str()));

            NetworkOrderStream stream;
            engine->getReportsEngine().createNetworkResourceReport(stream);

            networkResourceHandler = HandlerGenerator::createHandler();
            reportsMap[networkResourceHandler] = BytesPointer(new Bytes(stream.ostream.str()));
            reportsReadFlags[networkResourceHandler] = false;
            LOG_INFO(LOG_OI << "requested report: network resource: " << bytes2string(*reportsMap[networkResourceHandler]));
        }
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "NetworkResource - Unknown exception!" << ex.what());
    }

    //create the response
    Uint32 maxBlockSize = (Uint32) engine->getSettingsLogic().gw_max_NSDU_Size - MAX_HEADER_SIZE;
    NetworkOrderStream stream;
    stream.write(maxBlockSize);

    Uint32 empty = 0;

    if (generateReportCmd.isDeviceListRequested() && reportsMap[deviceListHandler]) {
        //assert(reportsMap[deviceListHandler]);
        stream.write((Uint32) reportsMap[deviceListHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(deviceListHandler);

    if (generateReportCmd.isTopologyRequested() && reportsMap[topologyHandler]) {
        //assert(reportsMap[topologyHandler]);
        stream.write((Uint32) reportsMap[topologyHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(topologyHandler);

    if (generateReportCmd.isScheduleRequested() && reportsMap[scheduleHandler]) {
        //assert(reportsMap[scheduleHandler]);
        stream.write((Uint32) reportsMap[scheduleHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(scheduleHandler);

    if (generateReportCmd.isDeviceHealthRequested() && reportsMap[deviceHealthHandler]) {
        //assert(reportsMap[deviceHealthHandler]);
        stream.write((Uint32) reportsMap[deviceHealthHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(deviceHealthHandler);

    if (generateReportCmd.isNeighborHealthRequested() && (neighborHealthHandler != 0) && reportsMap[neighborHealthHandler]) {
        //assert(reportsMap[neighborHealthHandler]);
        stream.write((Uint32) reportsMap[neighborHealthHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(neighborHealthHandler);

    if (generateReportCmd.isNetworkHealthRequested() && reportsMap[networkHealthHandler]) {
        //assert(reportsMap[networkHealthHandler]);
        stream.write((Uint32) reportsMap[networkHealthHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(networkHealthHandler);

    if (generateReportCmd.isNetworkResourceRequested() && reportsMap[networkResourceHandler]) {
        //assert(reportsMap[networkResourceHandler]);
        stream.write((Uint32) reportsMap[networkResourceHandler]->size());
    } else {
        stream.write(empty);
    }
    stream.write(networkResourceHandler);

    LOG_INFO(LOG_OI << "generatedReport:" << bytes2string(Bytes(stream.ostream.str())));
    return BytesPointer(new Bytes(stream.ostream.str()));
}

void SMO::getBlock(BytesPointer parameters) {
    NetworkOrderStream stream(parameters);
    Uint32 reqHandler;
    stream.read(reqHandler);
    Uint32 offset;
    stream.read(offset);
    Uint16 size;
    stream.read(size);

    LOG_DEBUG(LOG_OI << "getBlock - handler=" << (int) reqHandler << ", offset=" << (int) offset << ", size=" << (int) size);

    if (!reportsMap[reqHandler]) {
        //this means that the report hasn't been created or it's not available anymore
        //(may be caused by a duplicate getBlock request on the last block)
        //report error and terminate object
        //LOG_ERROR(LOG_OI << "getDeviceListBlock - unexpected request (may be a duplicate).");
        jobFinished = true;
        THROW_EX(NE::Common::NEException, "getBlock - unexpected request (may be a duplicate). handler: " << reqHandler
                    << ". Discarding packet.")
        ;
    }

    try {
        Bytes block = reportsMap[reqHandler]->substr(offset, size);
        sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(block)), false);
    } catch (std::exception& ex) {
        LOG_ERROR(LOG_OI << "getTopologyBlock - invalid arguments." << ex.what());
        sendExecuteResponseToRequester(indication, SFC::invalidArgument, BytesPointer(new Bytes()), false);
    }

    if (reportsMap[reqHandler]->size() == offset + size) { //last block
        reportsReadFlags[reqHandler] = true;
    }

    //set job finished if last block were read from all the reports
    if (!existUnreadBlocks()) {
        jobFinished = true;
    }
}

bool SMO::existUnreadBlocks() {
    ReportsReadFlags::iterator itFlags = reportsReadFlags.begin();
    //search for a flag set to false
    while (itFlags != reportsReadFlags.end() && itFlags->second) {
        ++itFlags;
    }

    return itFlags != reportsReadFlags.end();
}

void SMO::marshallContract(const PhyContract& contract, NE::Misc::Marshall::OutputStream& outStream, int& contractsCount) {

    NE::Model::Device * device = engine->getDevice(contract.destination32);
    if (device == NULL) {
        LOG_ERROR(LOG_OI << "getContractsAndRoutes - on marshallContract destination device not found: "
                    << Address_toStream(contract.destination32) << ", owner: " << Address_toStream(contract.source32));
        return;
    }

    ++contractsCount;

    outStream.write(contract.contractID);
    outStream.write((Uint8) contract.communicationServiceType);
    outStream.write(contract.contract_Activation_Time);
    outStream.write(contract.sourceSAP);

    device->address128.marshall(outStream);

    outStream.write(contract.destinationSAP);
    outStream.write(contract.assigned_Contract_Life);
    outStream.write((Uint8) contract.assigned_Contract_Priority);
    outStream.write(contract.assigned_Max_NSDU_Size);
    outStream.write(contract.assigned_Reliability_And_PublishAutoRetransmit);

    int empty = 0;

    if (contract.communicationServiceType == CommunicationServiceType::Periodic) {
        outStream.write(contract.assignedPeriod);
        outStream.write(contract.assignedPhase);
        outStream.write(contract.assignedDeadline);
        outStream.write((Int16) empty); //assignedCommittedBurst
        outStream.write((Int16) empty); //assignedExcessBurst
        outStream.write((Uint8) empty); //assigned_Max_Send_Window_Size

    } else { //CommunicationServiceType::NonPeriodic
        outStream.write((Int16) empty); //assignedPeriod
        outStream.write((Uint8) empty); //assignedPhase
        outStream.write((Uint16) empty); //assignedDeadline
        outStream.write(contract.assignedCommittedBurst);
        outStream.write(contract.assignedExcessBurst);
        outStream.write(contract.assigned_Max_Send_Window_Size);
    }
}

//Address64 ownerDevice - used when there's an error; to know which device is the owner of the route
void SMO::marshallRoute(const PhyRoute& route, Uint16 subnetID, Address64& ownerDevice, NE::Misc::Marshall::OutputStream& outStream) {

    outStream.write(route.index);
    outStream.write((Uint8) route.route.size());
    outStream.write(route.alternative);
    outStream.write(route.forwardLimit);

    for (Uint8 i = 0; i < route.route.size(); ++i) {
        if (isRouteGraphElement(route.route[i])) {
            outStream.write((Uint8) 1);
            outStream.write(route.route[i]);
        } else {
            Address128 address128;
            Address32 nodeAddress32 = engine->createAddress32(subnetID, route.route[i]);
            NE::Model::Device * device = engine->getDevice(nodeAddress32);
            if (device == NULL) {
                LOG_ERROR(LOG_OI << "getContractsAndRoutes - on marshallRoute " << (int) route.index << " could not find node "
                            << Address_toStream(nodeAddress32) << ", owner:" << ownerDevice.toString());
                Address64 address64; //not used
                address128 = AddressAllocator::getNextAddress128(address64, nodeAddress32, engine->getSettingsLogic().ipv6AddrPrefix);
                //continue;
            } else {
                address128 = device->address128;
            }

            outStream.write((Uint8) 0);
            address128.marshall(outStream);
        }
    }

    switch (route.alternative) {
        case 0: {
            Address128 srcAddress;
            Address32 srcAddress32 = engine->createAddress32(subnetID, route.sourceAddress);
            NE::Model::Device * device = engine->getDevice(srcAddress32);
            if (device == NULL) {
                LOG_ERROR(LOG_OI
                            << "getContractsAndRoutes - on marshallRoute" << (int) route.index
                            << " could not find the device for route.sourceAddress=" << std::hex << route.sourceAddress);
                Address64 address64; //not used
                srcAddress = AddressAllocator::getNextAddress128(address64, srcAddress32, engine->getSettingsLogic().ipv6AddrPrefix);
            } else {
                srcAddress = device->address128;
            }

            outStream.write(route.selector); //contractID
            srcAddress.marshall(outStream);

            break;
        }
        case 1: {
            outStream.write(route.selector); //contractID
            break;
        }
        case 2: {
            //route.selector = nodeAddress
            Address128 nodeAddress;
            Address32 nodeAddress32 = engine->createAddress32(subnetID, route.selector);
            NE::Model::Device * device = engine->getDevice(nodeAddress32);
            if (device == NULL) {
                LOG_ERROR(LOG_OI << "getContractsAndRoutes - on marshallRoute" << (int) route.index
                            << " could not find the device for route.selector=" << std::hex << route.selector);
                Address64 address64; //not used
                nodeAddress = AddressAllocator::getNextAddress128(address64, nodeAddress32, engine->getSettingsLogic().ipv6AddrPrefix);
            } else {
                nodeAddress = device->address128;
            }

            nodeAddress.marshall(outStream);
            break;
        }
        case 3: {
            //default route; selector is null
            break;
        }
        default: {
            std::ostringstream streamEx;
            streamEx << "Unknown route.alternative=" << (int) route.alternative << "routeID=" << (int) route.index;
            throw NEException(streamEx.str());
        }
    }
}

void SMO::marshallDevice(Device* device, NetworkOrderStream& devicesStream, int& devicesCount) {
    ++devicesCount;

    NetworkOrderStream contractsStream;
    NetworkOrderStream routesStream;
    int contractsCount = 0;

    for (ContractIndexedAttribute::iterator itContracts = device->phyAttributes.contractsTable.begin(); itContracts
                != device->phyAttributes.contractsTable.end(); ++itContracts) {

        if (itContracts->second.getValue() == NULL) {
            continue;
        }
        //skip SM->SM contract
        if (device->capabilities.isManager()
                    && (itContracts->second.getValue()->destination32 == ADDRESS16_MANAGER)) {
            continue;
        }

        marshallContract(*(itContracts->second.getValue()), contractsStream, contractsCount);
    }

    for (RouteIndexedAttribute::iterator itRoutes = device->phyAttributes.routesTable.begin(); itRoutes
                != device->phyAttributes.routesTable.end(); ++itRoutes) {

        if (itRoutes->second.getValue() == NULL) {
            continue;
        }
        marshallRoute(*(itRoutes->second.getValue()), device->capabilities.dllSubnetId, device->address64, routesStream);
    }

    device->address128.marshall(devicesStream);
    devicesStream.write((Uint8) contractsCount);
    devicesStream.write((Uint8) device->phyAttributes.routesTable.size());
    devicesStream.write(contractsStream.ostream.str());
    devicesStream.write(routesStream.ostream.str());
}

void SMO::getContractsAndRoutes() {
    NetworkOrderStream responseStream;
    NetworkOrderStream devicesStream;
    int devicesCount = 0;

    //add manager and gateway
    SubnetsContainer& subnetsContainer = engine->getSubnetsContainer();

    //Address32 managerAddress32 = subnetsContainer->getManagerAddress32();
    Device* manager = subnetsContainer.getDevice(subnetsContainer.getManagerAddress32());
    if (!manager) {
        std::ostringstream errStream;
        errStream << "Manager not found.";
        LOG_ERROR(errStream.str());
        throw NEException(errStream.str());
    }

    marshallDevice(manager, devicesStream, devicesCount);

    Device* gateway = subnetsContainer.getGateway();
    if (gateway && gateway->isJoinConfirmed()) {
        marshallDevice(gateway, devicesStream, devicesCount);
    }

    SubnetsMap& subnets = engine->getSubnetsList();
    for (SubnetsMap::iterator itSubnets = subnets.begin(); itSubnets != subnets.end(); ++itSubnets) {
        //        for (Address16 i = 3; i < MAX_NUMBER_OF_DEVICES; ++i) {
        //            Device * device = itSubnets->second->getDevice(i);
        const Address16Set& activeDevices = itSubnets->second->getActiveDevices();

        for (Address16Set::const_iterator it = activeDevices.begin(); it != activeDevices.end(); ++it) {
            Device* device = itSubnets->second->getDevice(*it);
            if (!device || !device->isEligibleForReport() || device->capabilities.isManager() || device->capabilities.isGateway()) {
                continue;
            }

            LOG_DEBUG(LOG_OI << "getContractsAndRoutes for: " << Address::toString(device->address32));

            marshallDevice(device, devicesStream, devicesCount);

//            ++devicesCount;
//
//            NetworkOrderStream contractsStream;
//            NetworkOrderStream routesStream;
//            int contractsCount = 0;
//
//            for (ContractIndexedAttribute::iterator itContracts = device->phyAttributes.contractsTable.begin(); itContracts
//                        != device->phyAttributes.contractsTable.end(); ++itContracts) {
//
//                if (itContracts->second.getValue() == NULL) {
//                    continue;
//                }
//                marshallContract(*(itContracts->second.getValue()), contractsStream, contractsCount);
//            }
//
//            for (RouteIndexedAttribute::iterator itRoutes = device->phyAttributes.routesTable.begin(); itRoutes
//                        != device->phyAttributes.routesTable.end(); ++itRoutes) {
//
//                if (itRoutes->second.getValue() == NULL) {
//                    continue;
//                }
//                marshallRoute(*(itRoutes->second.getValue()), device->capabilities.dllSubnetId, routesStream);
//            }
//
//            device->address128.marshall(devicesStream);
//            devicesStream.write((Uint8) contractsCount);
//            devicesStream.write((Uint8) device->phyAttributes.routesTable.size());
//            devicesStream.write(contractsStream.ostream.str());
//            devicesStream.write(routesStream.ostream.str());
        }
    }

    responseStream.write((Uint8) devicesCount);
    responseStream.write(devicesStream.ostream.str());

    sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(responseStream.ostream.str())), true);

    LOG_DEBUG(LOG_OI << "ContractsAndRoutes:" << bytes2string(Bytes(responseStream.ostream.str())));
}

void SMO::getCCABackoff(BytesPointer parameters) {
    NetworkOrderStream stream(parameters);
    Address128 deviceAddress;
    deviceAddress.unmarshall(stream);

    Device *device = engine->getDevice(engine->getAddress32(deviceAddress));
    if (!device) {
        LOG_ERROR("Device not found [" << Address_toStream(engine->getAddress32(deviceAddress)) << "] "
                    << deviceAddress.toString());
        sendExecuteResponseToRequester(indication, SFC::internalError, BytesPointer(new Bytes()), true);
        return;
    }

    NetworkOrderStream responseStream;
    for (int i = 0; i <= 15; ++i) { //16 channels
        responseStream.write(device->deviceStatistics.CCABackoffStatistics[i]);
    }
    sendExecuteResponseToRequester(indication, SFC::success, BytesPointer(new Bytes(responseStream.ostream.str())), true);
}

}
}
}
