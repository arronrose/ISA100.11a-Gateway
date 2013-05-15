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
 * Device.cpp
 *
 *  Created on: Sep 16, 2009
 *      Author: Catalin Pop, ion.ticus, beniamin.tecar,sorin.bidian
 */
#ifndef DEVICE_H_
#define DEVICE_H_

#include <boost/unordered_set.hpp>
#include <list>

#include "Common/NEAddress.h"
#include "Common/NETypes.h"
#include "Common/logging.h"
#include "Model/Capabilities.h"
#include "Model/model.h"
#include "Model/TheoreticAttributes.h"
#include "Model/Operations/IEngineOperation.h"
#include <boost/noncopyable.hpp>

namespace NE {
namespace Model {

#define ADDRESS16_MANAGER 1
#define ADDRESS16_GATEWAY 2

namespace DeviceStatus {
enum DeviceStatus {
    NOT_JOINED = 1,
    DELETED = 2,
    MARKED_FOR_REMOVE = 3,
    JOIN_REQUEST_RECEIVED = 4,
    NET_JOIN_RECEIVED = 5,
    CONTRACT_JOIN_RECEIVED = 6,
    JOIN_CONFIRMED = 7,
//set when SM received joinConfirm request
//    JOIN_CONFIRMED_DETAILS_UPDATED = 8 //set after SM has read detail attributes (tag, vendor_id, capabilities, dlmo.metadata)
//    JOIN_CONFIRMED_AND_ROLE_ACTIVATED = 9 //set for routers when the routing role is activated (added advertise & join links)
};


/**
 * Returns the name of the status.
 */
inline std::string toString(DeviceStatus status) {

    if (status == DeviceStatus::NOT_JOINED) {
        return "NOT_JOINED";
    } else if (status == DeviceStatus::JOIN_REQUEST_RECEIVED) {
        return "SECURITY_JOIN_REQ_RECEIVED";
    } else if (status == DeviceStatus::NET_JOIN_RECEIVED) {
        return "NETWORK_JOIN_REQ_RECEIVED";
    } else if (status == DeviceStatus::CONTRACT_JOIN_RECEIVED) {
        return "CONTRACT_JOIN_REQ_RECEIVED";
    } else if (status == DeviceStatus::JOIN_CONFIRMED) {
        return "JOIN_CONFIRMED";
    } else if (status == DeviceStatus::MARKED_FOR_REMOVE) {
        return "MARKED_FOR_REMOVE";
    } else if (status == DeviceStatus::DELETED) {
        return "DELETED";
    } else {
        return "UNKNOWN";
    }
}

}

namespace StatusForReports {
    enum StatusForReports {
        SEC_JOIN_REQUEST_RECEIVED = 4, //Security join request received
        SEC_JOIN_RESPONSE_SENT = 5, //Security join response sent
        SM_JOIN_RECEIVED = 6, // Network join request received (System_Manager_Join method)
        SM_JOIN_RESPONSE_SENT = 7, //Network join response sent
        SM_CONTRACT_JOIN_RECEIVED = 8, //Join contract request received (System_Manager_Contract method)
        SM_CONTRACT_JOIN_RESPONSE_SENT = 9, //Join contract response sent (System_Manager_Contract method)
        SEC_CONFIRM_RECEIVED = 10, // Security join confirmation received (Security_Confirm)
        SEC_CONFIRM_RESPONSE_SENT = 11, // Security join confirmation response sent (Security_Confirm)
        JOINED_AND_CONFIGURED = 20 // Joined & Configured & All info available
    };
    inline std::string toString(NE::Model::StatusForReports::StatusForReports status) {

        if (status == NE::Model::StatusForReports::SEC_JOIN_REQUEST_RECEIVED) {
            return "SEC_JOIN_REQUEST_RECEIVED";
        } else if (status == NE::Model::StatusForReports::SEC_JOIN_RESPONSE_SENT) {
            return "SEC_JOIN_RESPONSE_SENT";
        } else if (status == NE::Model::StatusForReports::SM_JOIN_RECEIVED) {
            return "SM_JOIN_RECEIVED";
        } else if (status == NE::Model::StatusForReports::SM_JOIN_RESPONSE_SENT) {
            return "SM_JOIN_RESPONSE_SENT";
        } else if (status == NE::Model::StatusForReports::SM_CONTRACT_JOIN_RECEIVED) {
            return "SM_CONTRACT_JOIN_RECEIVED";
        } else if (status == NE::Model::StatusForReports::SM_CONTRACT_JOIN_RESPONSE_SENT) {
            return "SM_CONTRACT_JOIN_RESPONSE_SENT";
        } else if (status == NE::Model::StatusForReports::SEC_CONFIRM_RECEIVED) {
            return "SEC_CONFIRM_RECEIVED";
        } else if (status == NE::Model::StatusForReports::SEC_CONFIRM_RESPONSE_SENT) {
            return "SEC_CONFIRM_RESPONSE_SENT";
        } else if (status == NE::Model::StatusForReports::JOINED_AND_CONFIGURED) {
            return "JOINED_AND_CONFIGURED";
        } else {
            return "UNKNOWN";
        }
    }

}
namespace RoleActivationStatus {
enum RoleActivationStatusEnum {
    NOT_ACTIVE = 0,// not sent advertise
    ACTIVE = 1,//set advertise at normal rate
    ACTIVE_SLOW = 2//set advertise at slow rate (1/30s)
};
}

/**
 * Holds DPDUs statistics.
 */
struct DeviceStatistics {
    Uint32 DPDUsTransmitted;

    Uint32 DPDUsReceived;

    Uint32 DPDUsFailedTransmission;

    Uint32 DPDUsFailedReception;

    Uint8 CCABackoffStatistics[16];

    DeviceStatistics() : DPDUsTransmitted(0), DPDUsReceived(0), DPDUsFailedTransmission(0), DPDUsFailedReception(0) {
        memset(CCABackoffStatistics, 0, 16);
    }
};

class Device: public boost::noncopyable {
        LOG_DEF("I.M.Device");

    private:

        bool hasVistited;
        bool dirtyInboundFlag;

        float inboundAppTraffic;
        float outboundAppTraffic;
        NE::Model::Operations::OperationsList unsentOperations;

        int lastContractID;
        int lastLinkID;
        int lastIndexID;
        int lastNetworkRouteID;
        int lastATTID;
        int lastRouteID;

        bool   acceleratedFlag;
        Uint32 lastTimeAccessed;

        /**
         * DPDUs transmitted since last interrogation for NetworkHealth report.
         * Used for global statistics of the network.
         */
        Uint32 transmissionsSinceLastReport;

        /**
         * DPDUs failed transmissions since last interrogation for NetworkHealth report.
         * Used for global statistics of the network.
         */
        Uint32 failedTransmissionsSinceLastReport;

    public:

        Address64 address64;

        Address32 address32;

        Address128 address128;

        Address32 joinedBackbone32;

        Address32 parent32;

        Capabilities capabilities;

        DeviceStatus::DeviceStatus status;

        Uint32 startTime;

        Uint32 joinConfirmTime;

        DeviceStatistics deviceStatistics;

        PhyAttributes phyAttributes;

        TheoreticAttributes theoAttributes;

        bool hasChanged;

        /**
         * Set for routers when the routing role is activated (added advertise & join links).
         */
        RoleActivationStatus::RoleActivationStatusEnum hasRoleActivated;

        bool hasMetadataUpdated;

        /**
         * Device status used for reports.
         */
        StatusForReports::StatusForReports statusForReports;

        /**
         * The TAI of the last received packet from the device.
         */
        Uint32 lastPacketTAI;

        /**
         * Contains the publishers for this device.
         */
        NE::Common::Address32Set publishers;

        Uint16 joinsCount;

        /**
         * Counts joins in which device reaches status JOINED_AND_CONFIGURED.
         */
        Uint32 fullJoinsCount;

        /**
         * default = false; it is set to true if the device's role has changed
         */
        bool roleChanged;


        /**
         * Max number of wrong publish then the device is reconfigured for publish.
         */
        static const Uint8 wrongPublishThreshold = 3;

        /**
         * Number of wrong publish received.
         */
        Uint8 nrOfwrongPublishReceived;

        /**
         * Time stamp when device was changed to fast discovery. Value 0 if is not fast discovery.
         */
        Uint32 fastDiscoveryTime;

    public:

        Device(const Capabilities& deviceCapabilities);

        /**
         * Method used when generating GSAP reports. A device should be included in the reports only after its details are updated.
         *
         * @return true if the device's details are updated
         */
        bool isEligibleForReport() const {
            return (statusForReports == StatusForReports::JOINED_AND_CONFIGURED);
        }

        void setCapabilities(const Capabilities& deviceCapabilities) {
            capabilities = deviceCapabilities;
        }

        void setStatistics(Uint8 transmited, Uint8 received, Uint8 failedTransmission, Uint8 failedReception);

        void updateStatistics(Uint8 transmited, Uint8 received, Uint8 failedTransmission, Uint8 failedReception);

        void updateStatistics(const std::vector<Uint8>& channelsCCABackoffs);

        /**
         * Resets transmissionsSinceLastReport and failedTransmissionsSinceLastReport.
         * Called after these values are used in the NetworkHealth report.
         */
        void resetStatisticsSinceLastReport();

        Uint32 getTransmissionsSinceLastReport() {
            return transmissionsSinceLastReport;
        }

        Uint32 getFailedTransmissionsSinceLastReport() {
            return failedTransmissionsSinceLastReport;
        }

        bool isJoinConfirmed(void) const {
            return (status == DeviceStatus::JOIN_CONFIRMED);
        }
        bool isNewStatus(void) const {
            return (status == DeviceStatus::JOIN_REQUEST_RECEIVED);
        }
        bool isRemoveStatus(void) const {
            return (status == DeviceStatus::MARKED_FOR_REMOVE);
        }
        bool isDeletedStatus(void) const {
            return (status == DeviceStatus::DELETED);
        }

        bool isAlreadyVisited(void) const {
            return hasVistited;
        }
        void setVisited(void) {
            hasVistited = true;
        }
        void unsetVisited(void) {
            hasVistited = false;
        }

        bool isDirtyInbound(void) const {
            return dirtyInboundFlag;
        }

        void setDirtyInbound(void) {
            dirtyInboundFlag = true;
            inboundAppTraffic = 0;
            outboundAppTraffic = 0;
        }
        void unsetDirtyInbound(void) {
            dirtyInboundFlag = false;
        }



        //        MetaDataAttributesPointer getMetaDataAttributes();
        //
        //        void setMetaDataAttributes(MetaDataAttributes& _metaDataAttributes);

        void setMetaDataAttributesUsedSuperframes(Uint16 usedSuperFrames);

        void setMetaDataAttributesUsedDiagnostics(Uint16 usedDiagnostics);

        Uint16 getNextContractID();

        Uint16 getNextLinkID();

        Uint16 getNextNetworkRouteID();

        Uint16 getNextRouteID();

        /**
         * Generates a new index for the PhySessionKey entity.
         * @return
         */
        Uint16 getNextKeysTableIndex();

        /**
         * Gets the greatest keyID out of the keyIDs with the specified peer.
         * @param peer
         * @return
         */
        Uint16 getGreatestKeyIDwithPeer(Device *peerDevice, int tsapSrc, int tsapDest);

        Uint16 getNextAddressTranslationID();

        void getEdges(Address16Set &targetsList);
        Uint8 getEdgesNo();

        Uint8 getMinInboundBC_Router();

        Uint8 getMaxOutBoundBC( Uint8 joinReservedSet);

        bool isUsedBcSetNumber(Uint8 setNumber);

        float getAllocatedInboundLink(Address16 peerDevice,Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role);
        float getAllocatedInboundLink2Roles(Address16 peerDevice, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role1, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role2);
        float getAllocatedOutboundLink(Address16 peerDevice, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role );
        float getInboundAppTraffic(Uint16 slotsPerSec);
        float getOutboundAppTraffic(Uint16 slotsPerSec);

        float getOutboundAppTrafficLocalLoop(Uint16 slotsPerSec);

        PhyLink * getPhyLink(EntityIndex linkIndex, bool& isPendingStatus);
        PhyContract * getPhyContract(EntityIndex contractIndex, bool& isPending);

        bool existsChannelHoppingIndex(EntityIndex &index) {
            return (phyAttributes.channelHoppingTable.find(index) != phyAttributes.channelHoppingTable.end());
        }

        bool existsSuperframeIndex(EntityIndex &index) {
            return (phyAttributes.superframesTable.find(index) != phyAttributes.superframesTable.end());
        }

        Uint16 getOutBoundGraph(Address16 device) const;

        bool  deviceHasOutboundGraph(Address32 device);

        Address16  getGraphOwner(Uint16 graphId);

        bool existsPhyGraph(Uint64 entityIndexGraph);

        bool existsPhyNeighbor(Uint64 entityIndexNeighbor);

        bool existsTransmisionManagementLink(Address16 destination, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction);

        void incrementChildId() { ++theoAttributes.childID; }

        void decrementChildId() { --theoAttributes.childID; }

        bool hasNeighbor(Address16 neighbor);


        bool hasCandidate(Address16 candidate);

        bool hasOnlyOneContractWithPeer(Address32 peer, Uint16 contractId );

        bool hasMultipleContractsWithPeerOnSameTSAP(Address32 peer,  int sourceTSAP, int destinationTSAP);


        PhyLink * getNotFullTxMngLink(Address16 peerDevice, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction);
        PhyLink * getTxMngLink( Address16 peerDevice, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction );
        PhyLink * getFirstTxMngLink(Address16 peerDevice, Tdma::TdmaLinkDir::TdmaLinkDirEnum direction, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role);
        PhyLink * getPeerRxLink(const PhyLink * txLink);
        /**
         * Find a link given the following parameters: direction, type, role, chOffset, offset.
         * It returns the phy link if found, and NULL otherwise. If link is found the entityIndexLink will contains the entity index of the link.
         * @param direction
         * @param type
         * @param role
         * @param chOffset
         * @param offset
         * @param entityIndexLink - OUT param: entity index of found link.
         * @return
         */
        PhyLink * findLink(Uint8 direction, Uint8 type, Tdma::TdmaLinkTypes::TdmaLinkTypesEnum role, Uint8 chOffset, Uint16 offset, EntityIndex& entityIndexLink);

        Uint16 getInboundGraphId();

        bool  getAcceleratedFlag() const { return acceleratedFlag; }
        void  unsetAcceleratedFlag() { acceleratedFlag = false; }
        void  setAcceleratedFlag() { acceleratedFlag = true; }
        void  setLastTimeAccessed(Uint32 currentTime) { lastTimeAccessed = currentTime; }
        Uint32 getLastTimeAccessed() const { return lastTimeAccessed ; }

        bool wasCommunicationInLastInterval(Uint16 interval ){
            return  wasCommunicationInLastInterval(interval, (Uint32)time(NULL)) ;
        }
        bool wasCommunicationInLastInterval(Uint16 interval, Uint32 currentTime ){
            return ( (getLastTimeAccessed() + interval) > currentTime );
        }

        /**
         * Add the operation to the unsentOperation list. This list is used for checking the entities that are not yet part of physical model.
         */
        void addUnsentOperation(const NE::Model::Operations::IEngineOperationPointer& operation) { unsentOperations.push_back(operation); }
        void removeUnsentOperation(const NE::Model::Operations::IEngineOperationPointer& operation){ unsentOperations.remove(operation); }
        NE::Model::Operations::OperationsList
            getUnsentOperations(){return unsentOperations;}

        /**
         * Obtains the link period. If the period from schedule is 0 use the period from superframe.
         * @param link
         * @return
         */
        int getLinkPeriod(const PhyLink * link) const;

        void getClockSourceNeighbors(Address16 &prefered, Address16 &backup);

        bool existsContract(Uint16 contractID);

        /**
         * Returns a string representation of this Device.
         */
        friend std::ostream& operator<<(std::ostream&, const Device& device);




};

typedef boost::shared_ptr<Device> DevicePointer;
typedef std::list<Address32> DevicesList;

}
}

#endif /*DEVICE_H_*/
