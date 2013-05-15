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
 * @author beniamin.tecar, radu.pop, sorin.bidian, catalin.pop
 */
#include "OperationPDUsVisitor.h"
#include "EntitiesHelper.h"
#include "AL/ObjectsIDs.h"
#include "ASL/PDUUtils.h"
#include "Common/HandleFactory.h"
#include "Common/AttributesIDs.h"
#include "Common/MethodsIDs.h"
#include "Common/SmSettingsLogic.h"
#include "Common/Utils/DllUtils.h"
#include "Common/Objects/ExtensibleAttributeIdentifier.h"
#include "Model/EngineProvider.h"

using namespace Isa100::AL;
using namespace Isa100::ASL;
using namespace Isa100::Security;
using namespace Isa100::Common;
using namespace Isa100::Common::MethodsID;
using namespace Isa100::Common::Objects;
using namespace NE::Model;
using namespace NE::Model::Operations;

namespace Isa100 {
namespace Model {

struct AlertPolicy {
        bool enabled;
        Uint16 neiMinUnicast; //ExtDlUint
        Uint8 neiErrThresh;
        Uint16 chanMinUnicast; //ExtDlUint
        Uint8 noAckThresh;
        Uint8 ccaBackoffThresh;

        AlertPolicy(bool enabled_, Uint16 neiMinUnicast_, Uint8 neiErrThresh_, Uint16 chanMinUnicast_, Uint8 noAckThresh_,
                    Uint8 ccaBackoffThresh_) :
            enabled(enabled_), neiMinUnicast(neiMinUnicast_), neiErrThresh(neiErrThresh_), chanMinUnicast(chanMinUnicast_),
                        noAckThresh(noAckThresh_), ccaBackoffThresh(ccaBackoffThresh_) {

        }

        void marshall(NE::Misc::Marshall::OutputStream& stream) {
            stream.write(enabled);
            Isa100::Common::Utils::marshallExtDLUint(stream, neiMinUnicast);
            stream.write(neiErrThresh);
            Isa100::Common::Utils::marshallExtDLUint(stream, chanMinUnicast);
            stream.write(noAckThresh);
            stream.write(ccaBackoffThresh);
        }
};

/**
 * ISA_100_11a_Draft_D2a-final.pdf - p.145.
 */
ContractResponsePointer createContractResponse(NE::Model::PhyContract& contract) {

    ContractResponsePointer contractResponse(new Isa100::Model::ContractResponse());

    contractResponse->contractRequestID = contract.requestID;
    contractResponse->responseCode = contract.responseCode;
    contractResponse->contractID = contract.contractID;
    contractResponse->communicationServiceType = contract.communicationServiceType;
    contractResponse->contract_Activation_Time = contract.contract_Activation_Time;
    contractResponse->assigned_Contract_Life = contract.assigned_Contract_Life;
    contractResponse->assigned_Contract_Priority = contract.assigned_Contract_Priority;
    contractResponse->assigned_Max_NSDU_Size = contract.assigned_Max_NSDU_Size;
    contractResponse->assigned_Reliability_And_PublishAutoRetransmit = contract.assigned_Reliability_And_PublishAutoRetransmit;
    contractResponse->assignedPeriod = contract.assignedPeriod;
    contractResponse->assignedPhase = contract.assignedPhase;
    contractResponse->assignedDeadline = contract.assignedDeadline;
    contractResponse->assignedCommittedBurst = contract.assignedCommittedBurst;
    contractResponse->assignedExcessBurst = contract.assignedExcessBurst;
    contractResponse->assigned_Max_Send_Window_Size = contract.assigned_Max_Send_Window_Size;

    return contractResponse;
}

/**
 * ISA_100_11a_Draft_D2a-final.pdf - p.151.
 */
ContractDataPointer createContractData(NE::Model::PhyContract& contract) {

    ContractDataPointer contractData(new Isa100::Model::ContractData());

    contractData->contractID = contract.contractID;
    contractData->contractStatus = (ContractStatus::ContractStatus) contract.responseCode;
    contractData->serviceType = contract.communicationServiceType;
    contractData->contractActivationTime = contract.contract_Activation_Time;
    contractData->sourceSAP = contract.sourceSAP + 0xF0B0;
    contractData->destinationAddress = contract.destination128;
    contractData->destinationSAP = contract.destinationSAP + 0xF0B0;
    contractData->assigned_Contract_Life = contract.assigned_Contract_Life;
    contractData->assigned_Contract_Priority = contract.assigned_Contract_Priority;
    contractData->assigned_Max_NSDU_Size = contract.assigned_Max_NSDU_Size;
    contractData->assigned_Reliability_And_PublishAutoRetransmit = contract.assigned_Reliability_And_PublishAutoRetransmit;
    contractData->assignedPeriod = contract.assignedPeriod;
    contractData->assignedPhase = contract.assignedPhase;
    contractData->assignedDeadline = contract.assignedDeadline;
    contractData->assignedCommittedBurst = contract.assignedCommittedBurst;
    contractData->assignedExcessBurst = contract.assignedExcessBurst;
    contractData->assigned_Max_Send_Window_Size = contract.assigned_Max_Send_Window_Size;

    return contractData;
}

OperationPDUsVisitor::OperationPDUsVisitor(Isa100::AL::ObjectID::ObjectIDEnum sourceObjectID_,
            Isa100::Security::SecurityManager& securityManager_) :
    sourceObjectID(sourceObjectID_), securityManager(securityManager_) {

}

Isa100::AL::ObjectID::ObjectIDEnum OperationPDUsVisitor::getObject(EntityType::EntityTypeEnum entityType) {

    Isa100::AL::ObjectID::ObjectIDEnum objectID;
    switch (entityType) {
        case EntityType::Contract:
        case EntityType::ClientServerRetryTimeout:
        case EntityType::ClientServerRetryMaxTimeoutInterval:
        case EntityType::ClientServerRetryCount:
        case EntityType::PowerSupply:
        case EntityType::SerialNumber:
        case EntityType::Vendor_ID:
        case EntityType::Model_ID:
        case EntityType::Software_Revision_Information:
        case EntityType::PackagesStatistics:
        case EntityType::JoinReason:
        case EntityType::PingInterval:
        case EntityType::ContractsTable_MetaData:
        case EntityType::AssignedRole:
            objectID = ObjectID::ID_DMO;
        break;
        case EntityType::NetworkContract:
        case EntityType::NetworkRoute:
        case EntityType::AddressTranslation:
            objectID = ObjectID::ID_NLMO;
        break;
        case EntityType::Link:
        case EntityType::AdvJoinInfo:
        case EntityType::AdvSuperframe:
        case EntityType::Neighbour:
        case EntityType::Route:
        case EntityType::Superframe:
        case EntityType::Graph:
        case EntityType::BlackListChannels:
        case EntityType::ChannelHopping:
        case EntityType::Neighbor_MetaData:
        case EntityType::Superframe_MetaData:
        case EntityType::Graph_MetaData:
        case EntityType::Link_MetaData:
        case EntityType::Route_MetaData:
        case EntityType::Diag_MetaData:
        case EntityType::Candidate:
        case EntityType::ChannelDiag:
        case EntityType::NeighborDiag:
        case EntityType::DeviceCapability:
        case EntityType::QueuePriority:
        case EntityType::DLMO_MaxLifetime:
        case EntityType::DLMO_IdleChannels:
        case EntityType::DLMO_DiscoveryAlert:
            objectID = ObjectID::ID_DLMO;
        break;
        case EntityType::HRCO_CommEndpoint:
        case EntityType::HRCO_Publish:
            objectID = ObjectID::ID_HRCO;
        break;
        case EntityType::SessionKey:
        case EntityType::MasterKey:
        case EntityType::SubnetKey:
            objectID = ObjectID::ID_DSMO;
        break;
        default:
            throw NEException("Unknown entity type!");
    }
    return objectID;
}

Isa100::AL::ObjectID::ObjectIDEnum OperationPDUsVisitor::getObject(NE::Model::Operations::IEngineOperation& operation) {
    return getObject(getEntityType(operation.getEntityIndex()));
}

Uint16 OperationPDUsVisitor::getMethod(NE::Model::Operations::IEngineOperation& operation) {
    EntityType::EntityTypeEnum entityType = getEntityType(operation.getEntityIndex());
    EngineOperationType::EngineOperationTypeEnum operationType = operation.getType();

    Uint16 methodID = 0;

    switch (entityType) {
        case EntityType::Neighbour:
        case EntityType::Route:
        case EntityType::ChannelHopping:
        case EntityType::Link:
        case EntityType::Superframe:
        case EntityType::Graph:
        case EntityType::QueuePriority:
            methodID = DLMOMethodID::Write_Row;
        break;

        case EntityType::Contract:
            if (operationType == EngineOperationType::WRITE_ATTRIBUTE) {
                methodID = DMOMethodID::Modify_Contract;
            } else if (operationType == EngineOperationType::DELETE_ATTRIBUTE) {
                methodID = DMOMethodID::Terminate_Contract;
            }
        break;
        case EntityType::NetworkContract:
            if (operationType == EngineOperationType::WRITE_ATTRIBUTE) {
                methodID = NLMOMethodID::Set_row_ContractTable;
            } else if (operationType == EngineOperationType::DELETE_ATTRIBUTE) {
                methodID = NLMOMethodID::Delete_row_ContractTable;
            }
        break;
        case EntityType::AddressTranslation:
            if (operationType == EngineOperationType::WRITE_ATTRIBUTE) {
                methodID = NLMOMethodID::Set_row_ATT;
            } else if (operationType == EngineOperationType::DELETE_ATTRIBUTE) {
                methodID = NLMOMethodID::Delete_row_ATT;
            }
        break;
        case EntityType::NetworkRoute:
            if (operationType == EngineOperationType::WRITE_ATTRIBUTE) {
                methodID = NLMOMethodID::Set_row_RT;
            } else if (operationType == EngineOperationType::DELETE_ATTRIBUTE) {
                methodID = NLMOMethodID::Delete_row_RT;
            }
        break;
        case EntityType::SessionKey:
        case EntityType::MasterKey:
        case EntityType::SubnetKey:
            if (operationType == EngineOperationType::WRITE_ATTRIBUTE) {
                methodID = DSMOMethodId::NEW_KEY;
            } else if (operationType == EngineOperationType::DELETE_ATTRIBUTE) {
                methodID = DSMOMethodId::DELETE_KEY;
            }
        break;
        default:
        break;
    }

    if (!methodID) {
        throw NEException("Unknown entity type!");
    }

    return methodID;
}

Uint16 OperationPDUsVisitor::getAttribute(EntityType::EntityTypeEnum entityType) {

    Uint16 attributeID;
    switch (entityType) {
        case EntityType::AdvJoinInfo:
            attributeID = DLMO_Attributes::AdvJoinInfo;
        break;
        case EntityType::AdvSuperframe:
            attributeID = DLMO_Attributes::AdvSuperframe;
        break;
        case EntityType::ClientServerRetryTimeout:
            attributeID = DMO_Attributes::Contract_Request_Timeout;
        break;
        case EntityType::ClientServerRetryMaxTimeoutInterval:
            attributeID = DMO_Attributes::Max_Retry_Timeout_Interval;
        break;
        case EntityType::ClientServerRetryCount:
            attributeID = DMO_Attributes::Max_ClientServer_Retries;
        break;
        case EntityType::BlackListChannels:
            attributeID = DLMO_Attributes::IdleChannels;
        break;
        case EntityType::PowerSupply:
            attributeID = DMO_Attributes::Power_Supply_Status;
        break;
        case EntityType::SerialNumber:
            attributeID = DMO_Attributes::Serial_Number;
        break;
        case EntityType::Vendor_ID:
            attributeID = DMO_Attributes::Vendor_ID;
        break;
        case EntityType::Model_ID:
            attributeID = DMO_Attributes::Model_ID;
        break;
        case EntityType::Software_Revision_Information:
            attributeID = DMO_Attributes::Software_Revision_Information;
        break;
        case EntityType::PackagesStatistics:
            attributeID = DMO_Attributes::PackagesStatistics;
        break;
        case EntityType::JoinReason:
            attributeID = DMO_Attributes::JoinReason;
        break;
        case EntityType::PingInterval:
            attributeID = DMO_Attributes::PingInterval;
        break;
        case EntityType::AssignedRole:
            attributeID = DMO_Attributes::Assigned_Role;
        break;
        case EntityType::ContractsTable_MetaData:
            attributeID = DMO_Attributes::Metadata_Contracts_Table;
        break;
        case EntityType::Neighbor_MetaData:
            attributeID = DLMO_Attributes::NeighborMeta;
        break;
        case EntityType::Superframe_MetaData:
            attributeID = DLMO_Attributes::SuperframeMeta;
        break;
        case EntityType::Graph_MetaData:
            attributeID = DLMO_Attributes::GraphMeta;
        break;
        case EntityType::Link_MetaData:
            attributeID = DLMO_Attributes::LinkMeta;
        break;
        case EntityType::Route_MetaData:
            attributeID = DLMO_Attributes::RouteMeta;
        break;
        case EntityType::Diag_MetaData:
            attributeID = DLMO_Attributes::DiagMeta;
        break;
        case EntityType::Candidate:
            attributeID = DLMO_Attributes::Candidates;
        break;
        case EntityType::ChannelDiag:
            attributeID = DLMO_Attributes::ChannelDiag;
        break;
        case EntityType::NeighborDiag:
            attributeID = DLMO_Attributes::NeighborDiag;
        break;
        case EntityType::DLMO_MaxLifetime:
            attributeID = DLMO_Attributes::MaxLifetime;
        break;
        case EntityType::DLMO_DiscoveryAlert:
            attributeID = DLMO_Attributes::DiscoveryAlert;
        break;
        case EntityType::HRCO_CommEndpoint:
            attributeID = HRCO_Attributes::CommunicationEndpoint;
        break;
        case EntityType::HRCO_Publish:
            attributeID = HRCO_Attributes::ArrayOfObjectAttributeIndexAndSize;
        break;
        case EntityType::DeviceCapability:
            attributeID = DLMO_Attributes::DeviceCapability;
        break;
        case EntityType::QueuePriority:
            attributeID = DLMO_Attributes::QueuePriority;
        break;
        case EntityType::DLMO_IdleChannels:
            attributeID = DLMO_Attributes::IdleChannels;
        break;

        default:
            throw NEException("Unknown entity type!");
    }
    return attributeID;

}

Uint16 OperationPDUsVisitor::getAttribute(IEngineOperation& operation) {

    return getAttribute(getEntityType(operation.getEntityIndex()));
}

OperationPDUsVisitor::~OperationPDUsVisitor() {
}

void OperationPDUsVisitor::generateConfirmationTimeoutCommands(SubnetSettings& subnetSettings) {

    if (subnetSettings.alertTimeout == 10) {
        return;
    }

    NE::Misc::Marshall::NetworkOrderStream stream;
    stream.write((Uint8) subnetSettings.alertTimeout);

    AppHandle reqID = HandleFactory().CreateHandle();
    Isa100::ASL::PDU::ClientServerPDUPointer apdu1(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));
    ExtensibleAttributeIdentifier targetAttribute1(ARMO_Attributes::Confirmation_Timeout_Device_Diagnostics);
    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest1(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute1, BytesPointer(
                new Bytes(stream.ostream.str()))));
    apdu1 = Isa100::ASL::PDUUtils::appendWriteRequest(apdu1, writeRequest1);
    pduList.push_back(apdu1);

    reqID = HandleFactory().CreateHandle();
    Isa100::ASL::PDU::ClientServerPDUPointer apdu2(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));
    ExtensibleAttributeIdentifier targetAttribute2(ARMO_Attributes::Confirmation_Timeout_Comm_Diagnostics);
    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest2(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute2, BytesPointer(
                new Bytes(stream.ostream.str()))));
    apdu2 = Isa100::ASL::PDUUtils::appendWriteRequest(apdu2, writeRequest2);
    pduList.push_back(apdu2);

    reqID = HandleFactory().CreateHandle();
    Isa100::ASL::PDU::ClientServerPDUPointer apdu3(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));
    ExtensibleAttributeIdentifier targetAttribute3(ARMO_Attributes::Confirmation_Timeout_Security);
    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest3(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute3, BytesPointer(
                new Bytes(stream.ostream.str()))));
    apdu3 = Isa100::ASL::PDUUtils::appendWriteRequest(apdu3, writeRequest3);
    pduList.push_back(apdu3);

    reqID = HandleFactory().CreateHandle();
    Isa100::ASL::PDU::ClientServerPDUPointer apdu4(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));
    ExtensibleAttributeIdentifier targetAttribute4(ARMO_Attributes::Confirmation_Timeout_Process);
    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest4(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute4, BytesPointer(
                new Bytes(stream.ostream.str()))));
    apdu4 = Isa100::ASL::PDUUtils::appendWriteRequest(apdu4, writeRequest4);
    pduList.push_back(apdu4);
}

bool OperationPDUsVisitor::visitAlertOperation(NE::Model::Operations::WriteAttributeOperation& operation) {
    NE::Misc::Marshall::NetworkOrderStream stream;

    Uint16 subnet = NE::Common::Address::extractSubnet(getDeviceAddress(operation.getEntityIndex()));
    SubnetSettings& subnetSettings = SmSettingsLogic::instance().getSubnetSettings(subnet);

    AppHandle reqID;

    //configure ARMO timeout attributes (2, 5, 8, 11)
    generateConfirmationTimeoutCommands(subnetSettings);

    //create the endpoint with destination gateway - used for gateway alerts and for manager alerts redirected from config
    PhyAlertCommunicationEndpoint gwEndpoint;
    Device *gw = Isa100::Model::EngineProvider::getEngine()->getDevice(ADDRESS16_GATEWAY);
    if (gw) {
        gwEndpoint.remoteAddress = gw->address128;
        gwEndpoint.remotePort = subnetSettings.alertReceivingPort;
        gwEndpoint.remoteObjectID = subnetSettings.alertReceivingObjectID;
    } else {
        LOG_ERROR("Gateway not found (addr=" << Address_toStream(ADDRESS16_GATEWAY)
                    << "). Skipping configuration of alerts for gateway.");
    }

    PhyAlertCommunicationEndpoint* phyCommEndpoint = (PhyAlertCommunicationEndpoint*) operation.getPhysicalEntity();

    if (subnetSettings.enableAlertsForGateway == false) {
        phyCommEndpoint->remoteAddress = SmSettingsLogic::instance().managerAddress128;
        phyCommEndpoint->remotePort = 0xF0B1; //ARO object is in SMAP
        phyCommEndpoint->remoteObjectID = ObjectID::ID_ARO;
    } else {
        if (gw) {
            phyCommEndpoint->remoteAddress = gwEndpoint.remoteAddress;
            phyCommEndpoint->remotePort = gwEndpoint.remotePort;
            phyCommEndpoint->remoteObjectID = gwEndpoint.remoteObjectID;
        } else {
            LOG_ERROR("Skipping configuration of manager alerts redirected to gateway.");
            return false;
        }
    }

    marshallEntity(*phyCommEndpoint, stream);

    //comm alerts
    reqID = HandleFactory().CreateHandle();

    Isa100::ASL::PDU::ClientServerPDUPointer apdu2(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));

    ExtensibleAttributeIdentifier targetAttribute2(ARMO_Attributes::Alert_Master_Comm_Diagnostics);

    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest2(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute2, BytesPointer(
                new Bytes(stream.ostream.str()))));

    apdu2 = Isa100::ASL::PDUUtils::appendWriteRequest(apdu2, writeRequest2);
    pduList.push_back(apdu2);

    //security alerts
    reqID = HandleFactory().CreateHandle();

    Isa100::ASL::PDU::ClientServerPDUPointer apdu3(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));

    ExtensibleAttributeIdentifier targetAttribute3(ARMO_Attributes::Alert_Master_Security);

    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest3(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute3, BytesPointer(
                new Bytes(stream.ostream.str()))));

    apdu3 = Isa100::ASL::PDUUtils::appendWriteRequest(apdu3, writeRequest3);
    pduList.push_back(apdu3);

    //create one PDU for dlmo.AlertPolicy configuration
    reqID = HandleFactory().CreateHandle();

    Isa100::ASL::PDU::ClientServerPDUPointer apduPolicy(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, ObjectID::ID_DLMO, //
                reqID));

    NE::Misc::Marshall::NetworkOrderStream streamPolicy;

    AlertPolicy alertPolicy(SmSettingsLogic::instance().alertsEnabled, //
                SmSettingsLogic::instance().alertsNeiMinUnicast, //
                SmSettingsLogic::instance().alertsNeiErrThresh, //
                SmSettingsLogic::instance().alertsChanMinUnicast, //
                SmSettingsLogic::instance().alertsNoAckThresh, //
                SmSettingsLogic::instance().alertsCcaBackoffThresh);

    alertPolicy.marshall(streamPolicy);

    ExtensibleAttributeIdentifier targetAttributePolicy(DLMO_Attributes::AlertPolicy);

    Isa100::ASL::PDU::WriteRequestPDUPointer
                writeRequestPolicy(new Isa100::ASL::PDU::WriteRequestPDU(targetAttributePolicy, BytesPointer(
                            new Bytes(streamPolicy.ostream.str()))));

    apduPolicy = PDUUtils::appendWriteRequest(apduPolicy, writeRequestPolicy);
    pduList.push_back(apduPolicy);

    //configure gateway endpoints

    if (!gw) {
        return true;
    }

    NE::Misc::Marshall::NetworkOrderStream gwStream;
    marshallEntity(gwEndpoint, gwStream);

    //device diagnostics alerts
    reqID = HandleFactory().CreateHandle();

    Isa100::ASL::PDU::ClientServerPDUPointer apdu1(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));

    ExtensibleAttributeIdentifier targetAttribute1(ARMO_Attributes::Alert_Master_Device_Diagnostics);

    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest1(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute1, BytesPointer(
                new Bytes(gwStream.ostream.str()))));

    apdu1 = PDUUtils::appendWriteRequest(apdu1, writeRequest1);
    pduList.push_back(apdu1);

    //process alerts
    reqID = HandleFactory().CreateHandle();

    Isa100::ASL::PDU::ClientServerPDUPointer apdu4(new Isa100::ASL::PDU::ClientServerPDU( //
                Isa100::Common::PrimitiveType::request, //
                Isa100::Common::ServiceType::write, //
                sourceObjectID, //
                ObjectID::ID_ARMO, //
                reqID));

    ExtensibleAttributeIdentifier targetAttribute4(ARMO_Attributes::Alert_Master_Process);

    Isa100::ASL::PDU::WriteRequestPDUPointer writeRequest4(new Isa100::ASL::PDU::WriteRequestPDU(targetAttribute4, BytesPointer(
                new Bytes(gwStream.ostream.str()))));

    apdu4 = PDUUtils::appendWriteRequest(apdu4, writeRequest4);
    pduList.push_back(apdu4);

    return true;
}

bool OperationPDUsVisitor::visitWriteAttributeOperation(NE::Model::Operations::WriteAttributeOperation& operation) {

    EntityIndex entityIndex = operation.getEntityIndex();

    //treat alert operation separately because the same entity generates multiple commands
    if (getEntityType(entityIndex) == EntityType::ARMO_CommEndpoint) {
        return visitAlertOperation(operation);
    }

    Uint16 index = getIndex(entityIndex);

    Uint16 reqID = HandleFactory().CreateHandle();
    NE::Misc::Marshall::NetworkOrderStream stream;

    marshallWrite(operation, stream);

    Isa100::ASL::PDU::ClientServerPDUPointer apdu;
    if (index == 0) { // simple attribute

        apdu.reset(
                    new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::write, sourceObjectID, getObject(
                                operation), //Isa100::AL::ObjectID::ID_DMO,
                    reqID));

        Isa100::ASL::PDU::WriteRequestPDUPointer
                    writeRequest(new Isa100::ASL::PDU::WriteRequestPDU(getAttribute(operation), BytesPointer(
                                new Bytes(stream.ostream.str()))));

        apdu = Isa100::ASL::PDUUtils::appendWriteRequest(apdu, writeRequest);
    } else { // indexed attribute

        apdu.reset(
                    new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::execute, sourceObjectID, getObject(
                                operation), // ObjectID::ID_DLMO,
                    reqID));

        Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest(
                    new Isa100::ASL::PDU::ExecuteRequestPDU(getMethod(operation), BytesPointer(new Bytes(stream.ostream.str()))));

        apdu = PDUUtils::appendExecuteRequest(apdu, executeRequest);
    }

    pduList.push_back(apdu);

    return true;
}

bool OperationPDUsVisitor::visitReadAttributeOperation(NE::Model::Operations::ReadAttributeOperation& operation) {

    Uint16 reqID = HandleFactory().CreateHandle();
    NE::Misc::Marshall::NetworkOrderStream stream;

    Isa100::ASL::PDU::ClientServerPDUPointer apdu;
    apdu.reset(
                new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::read, sourceObjectID, getObject(
                            operation), reqID));

    Uint16 attributeID = getAttribute(operation);
    ExtensibleAttributeIdentifier eai(attributeID);
    Isa100::ASL::PDU::ReadRequestPDUPointer readRequest(new Isa100::ASL::PDU::ReadRequestPDU(eai));

    apdu = Isa100::ASL::PDUUtils::appendReadRequest(apdu, readRequest);

    pduList.push_back(apdu);

    return true;
}

bool OperationPDUsVisitor::visitDeleteAttributeOperation(NE::Model::Operations::DeleteAttributeOperation& operation) {

    EntityIndex entityIndex = operation.getEntityIndex();
    Uint16 index = getIndex(entityIndex);

    Uint16 reqID = HandleFactory().CreateHandle();
    NE::Misc::Marshall::NetworkOrderStream stream;

    marshallDelete(operation, stream);

    Isa100::ASL::PDU::ClientServerPDUPointer apdu;
    if (index == 0) { // simple attribute

        apdu.reset(
                    new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::write, sourceObjectID, getObject(
                                operation), // Isa100::AL::ObjectID::ID_DMO,
                    reqID));

        Isa100::ASL::PDU::WriteRequestPDUPointer
                    writeRequest(new Isa100::ASL::PDU::WriteRequestPDU(getAttribute(operation), BytesPointer(
                                new Bytes(stream.ostream.str()))));

        apdu = Isa100::ASL::PDUUtils::appendWriteRequest(apdu, writeRequest);
    } else { // indexed attribute

        apdu.reset(
                    new Isa100::ASL::PDU::ClientServerPDU(Isa100::Common::PrimitiveType::request, Isa100::Common::ServiceType::execute, sourceObjectID, getObject(
                                operation), // ObjectID::ID_DLMO,
                    reqID));

        Isa100::ASL::PDU::ExecuteRequestPDUPointer executeRequest(
                    new Isa100::ASL::PDU::ExecuteRequestPDU(getMethod(operation), BytesPointer(new Bytes(stream.ostream.str()))));

        apdu = PDUUtils::appendExecuteRequest(apdu, executeRequest);
    }

    pduList.push_back(apdu);

    return true;
}

void OperationPDUsVisitor::marshallWrite(NE::Model::Operations::WriteAttributeOperation& operation,
            NE::Misc::Marshall::NetworkOrderStream& stream) {

    EntityType::EntityTypeEnum entityType = getEntityType(operation.getEntityIndex());
    if (operation.getPhysicalEntity() == NULL) {
        LOG_ERROR("Physical entity NULL on marshal operation. Op:" << operation.getEntityIndex());
        return;
    }

    switch (entityType) {
        case EntityType::Contract: { // Add Contract

            Uint16 attributeID = DMO_Attributes::Contracts_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhyContract* contract = (PhyContract*) operation.getPhysicalEntity();
            stream.write(contract->contractID); // write index separately

            ContractDataPointer contractData = createContractData(*contract);
            contractData->marshall(stream);

            break;
        }
        case EntityType::NetworkContract: { // Add NetworkContract

            Uint16 attributeID = NLMO_Attributes::Contract_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            //indexes are sent twice
            PhyNetworkContract* networkContract = (PhyNetworkContract*) operation.getPhysicalEntity();

            stream.write(networkContract->contractID);
            networkContract->sourceAddress.marshall(stream);

            marshallEntity(*networkContract, stream);
            break;
        }
        case EntityType::AddressTranslation: { // Add ATT row

            Uint16 attributeID = NLMO_Attributes::Address_Translation_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            //indexes are sent twice
            PhyAddressTranslation* addressTranslation = (PhyAddressTranslation*) operation.getPhysicalEntity();
            addressTranslation->longAddress.marshall(stream);

            marshallEntity(*addressTranslation, stream);

            break;
        }
        case EntityType::Neighbour: { // Add Neighbour

            Uint16 attributeID = DLMO_Attributes::Neighbor; //24;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhyNeighbor* neighbor = (PhyNeighbor*) operation.getPhysicalEntity();
            stream.write(neighbor->index); // write index separately

            marshallEntity(*neighbor, stream);
            break;
        }
        case EntityType::Route: { // Add Route

            Uint16 attributeID = DLMO_Attributes::Route; //30;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhyRoute* route = (PhyRoute*) operation.getPhysicalEntity();
            stream.write(route->index); // write index separately

            marshallEntity(*route, stream);
            break;
        }
        case EntityType::NetworkRoute: { // Add NetworkRoute

            Uint16 attributeID = NLMO_Attributes::Route_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhyNetworkRoute* route = (PhyNetworkRoute*) operation.getPhysicalEntity();
            route->destination.marshall(stream);

            marshallEntity(*route, stream);
            break;
        }
        case EntityType::Link: { // Add Link

            Uint16 attributeID = DLMO_Attributes::Link;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());
            PhyLink* link = (PhyLink*) operation.getPhysicalEntity();
            stream.write(link->index);

            marshallEntity(*link, stream);
            break;
        }
        case EntityType::AdvJoinInfo: { // Add AdvInfo

            PhyAdvJoinInfo* advJoinInfo = (PhyAdvJoinInfo*) operation.getPhysicalEntity();
            marshallEntity(*advJoinInfo, stream);
            break;
        }
        case EntityType::QueuePriority: {

            PhyQueuePriority* queuePriority = (PhyQueuePriority*) operation.getPhysicalEntity();
            marshallEntity(*queuePriority, stream);
            break;
        }
        case EntityType::AdvSuperframe:
        case EntityType::ClientServerRetryTimeout:
        case EntityType::ClientServerRetryMaxTimeoutInterval:
        case EntityType::ContractsTable_MetaData:
        case EntityType::AssignedRole:
        case EntityType::ClientServerRetryCount:
        case EntityType::DLMO_MaxLifetime:
        case EntityType::DLMO_IdleChannels:
        case EntityType::PingInterval: {

            PhyUint16 * phyUint16 = (PhyUint16*) operation.getPhysicalEntity();
            marshallEntity(*phyUint16, stream);
            break;
        }

        case EntityType::DLMO_DiscoveryAlert: {

            PhyUint8 * phyUint8 = (PhyUint8*) operation.getPhysicalEntity();
            marshallEntity(*phyUint8, stream);
            break;
        }
        case EntityType::Superframe: {

            Uint16 attributeID = Isa100::Common::DLMO_Attributes::Superframe; //0x23;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhySuperframe* superframe = (PhySuperframe*) operation.getPhysicalEntity();
            stream.write(superframe->index);//index is superframe ID

            marshallEntity(*superframe, stream);
            break;
        }
        case EntityType::Graph: {

            Uint16 attributeID = Isa100::Common::DLMO_Attributes::Graph; //28;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhyGraph* graph = (PhyGraph*) operation.getPhysicalEntity();
            stream.write(graph->index);

            marshallEntity(*graph, stream);
            break;
        }
        case EntityType::HRCO_CommEndpoint: {

            PhyCommunicationAssociationEndpoint* phyCommEndpoint =
                        (PhyCommunicationAssociationEndpoint*) operation.getPhysicalEntity();
            phyCommEndpoint->remoteAddress = SmSettingsLogic::instance().managerAddress128;
            phyCommEndpoint->remotePort = 0xF0B1; //dispersion object is in SMAP
            phyCommEndpoint->remoteObjectID = ObjectID::ID_DO1; //a dispersion object; publish receiver
            phyCommEndpoint->staleDataLimit = 0;
            phyCommEndpoint->publicationPeriod = SmSettingsLogic::instance().publishPeriod;
            SmSettingsLogic::instance().publishPhase += 5;
            if (SmSettingsLogic::instance().publishPhase >= 100) {
                SmSettingsLogic::instance().publishPhase = 0;
            }
            phyCommEndpoint->idealPublicationPhase = SmSettingsLogic::instance().publishPhase;
            phyCommEndpoint->publishAutoRetransmit = 1; //transmit whenever possible
            phyCommEndpoint->configurationStatus = 1;

            marshallEntity(*phyCommEndpoint, stream);
            break;
        }
        case EntityType::HRCO_Publish: {

            PhyEntityIndexList* phyEntityIndexList = (PhyEntityIndexList*) operation.getPhysicalEntity();
            for (std::list<EntityIndex>::iterator it = phyEntityIndexList->value.begin(); it != phyEntityIndexList->value.end(); ++it) {
                EntityType::EntityTypeEnum entityType = getEntityType(*it);
                Uint16 index = getIndex(*it);

                stream.write((Uint16) getObject(entityType));
                stream.write((Uint16) getAttribute(entityType));
                stream.write((Uint16) index);
                stream.write((Uint16) 0);
            }
            break;
        }
        case EntityType::BlackListChannels: {

            PhyBlacklistChannels* blacklistChannels = (PhyBlacklistChannels*) operation.getPhysicalEntity();

            Uint16 config = 0;
            for (std::vector<Uint8>::iterator it = blacklistChannels->seq.begin(); it != blacklistChannels->seq.end(); it++) {

                config += 1 << (*it);
            }
            stream.write(config); // write value
            break;
        }
        case EntityType::ChannelHopping: {

            Uint16 attributeID = Isa100::Common::DLMO_Attributes::Ch;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            PhyChannelHopping* channelHopping = (PhyChannelHopping*) operation.getPhysicalEntity();

            stream.write(channelHopping->index);
            Utils::marshallExtDLUint(stream, channelHopping->index);

            Uint8 channelHoppingSize = channelHopping->length;
            stream.write((Uint8) channelHoppingSize);

            Uint8 index = 0;
            while (index < channelHoppingSize) {
                Uint8 writeValue = 0;
                writeValue |= (channelHopping->seq[index++] & 0x0F);
                if (index < channelHoppingSize) {
                    writeValue |= ((channelHopping->seq[index++] << 4) & 0xF0);
                }

                stream.write(writeValue);
            }
            break;
        }
        case EntityType::SessionKey: {

            PhySessionKey* phySessionKey = (PhySessionKey*) operation.getPhysicalEntity();
            SecurityKeyAndPolicies securityKeyAndPolicies;
            securityManager.createNewSessionKey(*phySessionKey, securityKeyAndPolicies);
            marshallEntity(securityKeyAndPolicies, stream);
            break;
        }
        case EntityType::MasterKey: {

            Address32 deviceAddress = getDeviceAddress(operation.getEntityIndex());

            PhySpecialKey* phyMasterKey = (PhySpecialKey*) operation.getPhysicalEntity();
            SecurityKeyAndPolicies securityKeyAndPolicies;
            securityManager.prepareMasterKey(*phyMasterKey, deviceAddress, securityKeyAndPolicies);
            marshallEntity(securityKeyAndPolicies, stream);
            break;
        }
        case EntityType::SubnetKey: {

            Address32 deviceAddress = getDeviceAddress(operation.getEntityIndex());

            PhySpecialKey* phySubnetKey = (PhySpecialKey*) operation.getPhysicalEntity();
            SecurityKeyAndPolicies securityKeyAndPolicies;
            securityManager.prepareSubnetKey(*phySubnetKey, deviceAddress, securityKeyAndPolicies);
            marshallEntity(securityKeyAndPolicies, stream);
            break;
        }
        default: {
            std::ostringstream streamEx;
            streamEx << "Unknown operation=" << operation;
            throw NEException(streamEx.str());
        }
    }
}

void OperationPDUsVisitor::marshallDelete(NE::Model::Operations::DeleteAttributeOperation& operation,
            NE::Misc::Marshall::NetworkOrderStream& stream) {

    Device* device = EngineProvider::getEngine()->getDevice(operation.getOwner());
    if (!device) {
        LOG_ERROR("The operation owner found! operatin=" << operation);
        return;
    }

    EntityType::EntityTypeEnum entityType = getEntityType(operation.getEntityIndex());
    std::ostringstream hexEntityIndexStream;
    hexEntityIndexStream << std::hex << operation.getEntityIndex();

    switch (entityType) {
        case EntityType::Contract: { // Delete Contract

            Uint16 attributeID = DMO_Attributes::Contracts_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            //The Contract_ID element shall be used as the Index_1 input argument
            //PhyContract* contract = (PhyContract*) operation.getPhysicalEntity();
            //Uint16 index = contract->contractID;
            Uint16 index = getIndex(operation.getEntityIndex());
            stream.write(index);
            break;
        }
        case EntityType::NetworkContract: { // Delete NetworkContract

            Uint16 attributeID = NLMO_Attributes::Contract_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            NetworkContractIndexedAttribute::iterator it = device->phyAttributes.networkContractsTable.find(
                        operation.getEntityIndex());
            if (it != device->phyAttributes.networkContractsTable.end() && it->second.getValue()) {
                //PhyNetworkContract* networkContract = (PhyNetworkContract*) operation.getPhysicalEntity();
                PhyNetworkContract* networkContract = it->second.getValue();
                stream.write(networkContract->contractID);
                networkContract->sourceAddress.marshall(stream);
            } else {
                LOG_ERROR("delete Network Contract fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
            break;
        }
        case EntityType::Neighbour: { // Delete Neighbour

            Uint16 attributeID = DLMO_Attributes::Neighbor; //24;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            Uint16 index = getIndex(operation.getEntityIndex());
            stream.write(index);
            break;
        }
        case EntityType::Route: { // Delete Route

            Uint16 attributeID = DLMO_Attributes::Route; //30;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            Uint16 index = getIndex(operation.getEntityIndex());
            stream.write(index);
            break;
        }
        case EntityType::NetworkRoute: { // Delete NetworkRoute

            NetworkRouteIndexedAttribute::iterator it = device->phyAttributes.networkRoutesTable.find(operation.getEntityIndex());
            if (it != device->phyAttributes.networkRoutesTable.end() && it->second.getValue()) {
                Uint16 attributeID = NLMO_Attributes::Route_Table;
                stream.write(attributeID);
                stream.write(operation.getTaiCutOver());

                PhyNetworkRoute* route = it->second.getValue();
                route->destination.marshall(stream);
            } else {
                LOG_ERROR("delete Network Route fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
            break;
        }
        case EntityType::Link: { // Delete Link

            Uint16 attributeID = Isa100::Common::DLMO_Attributes::Link;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            Uint16 index = getIndex(operation.getEntityIndex());
            stream.write(index);
            break;
        }
        case EntityType::ChannelHopping: { // Delete ChannelHopping

            Uint16 attributeID = Isa100::Common::DLMO_Attributes::Ch;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            Uint16 index = getIndex(operation.getEntityIndex());
            stream.write(index);
            break;
        }
        case EntityType::Graph: { // Delete Graph

            Uint16 attributeID = Isa100::Common::DLMO_Attributes::Graph;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            Uint16 index = getIndex(operation.getEntityIndex());
            stream.write(index);
            break;
        }
        case EntityType::AddressTranslation: { // Delete AddressTranslation

            Uint16 attributeID = NLMO_Attributes::Address_Translation_Table;
            stream.write(attributeID);
            stream.write(operation.getTaiCutOver());

            AddressTranslationIndexedAttribute::iterator it = device->phyAttributes.addressTranslationTable.find(
                        operation.getEntityIndex());
            if (it != device->phyAttributes.addressTranslationTable.end() &&  it->second.getValue()) {
                PhyAddressTranslation* addressTranslation = it->second.getValue();
                addressTranslation->longAddress.marshall(stream);
            } else {
                LOG_ERROR("delete AddressTranslation fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
            break;
        }
        case EntityType::SessionKey: {
            // delete key
            SessionKeyIndexedAttribute::iterator it = device->phyAttributes.sessionKeysTable.find(operation.getEntityIndex());
            if (it != device->phyAttributes.sessionKeysTable.end() && it->second.getValue()) {
                PhySessionKey* phySessionKey = it->second.getValue();
                SecurityDeleteKeyReq securityDeleteKeyReq;
                securityManager.getKeyDeletionParams(*phySessionKey, securityDeleteKeyReq);
                marshallEntity(securityDeleteKeyReq, stream);
            } else {
                LOG_ERROR("delete SessionKey fail! the entity index " << hexEntityIndexStream.str() << " not found!");
            }
            break;
        }
        default: {
            std::ostringstream streamEx;
            streamEx << "Unknown operation=" << operation;
            throw NEException(streamEx.str());
        }
    }
}

}
}
