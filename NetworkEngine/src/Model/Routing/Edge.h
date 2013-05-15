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

#ifndef ROUTINGEDGE_H_
#define ROUTINGEDGE_H_

#include <boost/unordered_set.hpp>
#include "boost/shared_ptr.hpp"
#include <vector>

#include "Model/Routing/RouteTypes.h"
#include "Model/Routing/EdgeGraph.h"
#include "Model/Device.h"
#include "Common/logging.h"

namespace NE {

namespace Model {

namespace Routing {

typedef unsigned int Edge32;
#define MAX_PUBLISH_DIAG_COUNTED 3

/**
 * Spec: Isa100 D2b page 310
 * RSQI shall be reported as a qualitative assessment of signal quality, with higher number
 *indicating a better signal. A value of 1-63 indicates a poor signal, 64-127 a fair signal, 128-
 *191 a good signal, and 192-255 an excellent signal. A value of zero indicates that the chipset
 *does not support any signal quality diagnostics other than RSSI.
 *
 */
namespace RsqiLevelEnum {
enum RsqiLevel {
    EXCELLENT = 0, //!< GREAT
    GOOD = 32, //!< GOOD
    FAIR = 72, //!< BAD
    POOR = 128
//!< VERY_BAD
};

inline
std::string toString(RsqiLevelEnum::RsqiLevel level) {
    switch (level) {
        case RsqiLevelEnum::EXCELLENT:
            return "EXCEL";
        case RsqiLevelEnum::GOOD:
            return "GOOD";
        case RsqiLevelEnum::FAIR:
            return "FAIR";
        case RsqiLevelEnum::POOR:
        default:
            return "FAIR";
    }
}
}


struct LastPublishes {
    std::vector<Uint16> successSent;
    std::vector<Uint16> failedSent;
    Uint16 sumOfSuccessSent;
    Uint16 sumOfFailedSent;
    Uint16 sumOfSuccessSentShort;
    Uint16 sumOfFailedSentShort;

    LastPublishes() :
    	sumOfSuccessSent(0), sumOfFailedSent(0),
        sumOfSuccessSentShort(0), sumOfFailedSentShort(0) {

    }

    void reset() {
        successSent.clear();
        failedSent.clear();
        sumOfSuccessSent = 0;
        sumOfFailedSent = 0;
        sumOfSuccessSentShort = 0;
        sumOfFailedSentShort = 0;
    }


};

class Edge {

        LOG_DEF("I.M.G.Edge");

    private:

        /**
         * ID of the edge  = (source_address16 << 16 ) + destination_address16
         */
        Edge32 ID;

        /*
         * Source device's address
         */

        Address16 source;

        /*
         * Destination device's address
         */

        Address16 destination;
        /**
         * Edge information for every graph that go through the edge
         */

        GraphsSet graphs;
        /**
         * The traffic reported for last allocation on the edge.
         */
        Uint16 reportedTraffic;

        /**
         * The current traffic of the edge.
         */
        Uint16 traffic;

        /**
         * The average RSL
         */
        Int8 rssi;

        /**
         * The average signal quality.
         * Used for NeighborHealthReport.
         */
        Uint8 rsqi;

        /**
         * Succesfully sent packets.
         */
        Uint32 sent;

        /**
         * Succesfully received packets.
         */
        Uint32 received;

        /**
         * Failed packets.
         */
        Uint32 failed;

        /**
         * The fail factor, given by packet loss
         */
        Uint16 oldCostFactorFail;

        /**
         * The fail factor, given by packet loss
         */
        Uint16 costFactorFail;

        /**
         * Fhe number of statistics (diagnostics, candidates) the node has received
         * from the last evaluation
         */
        Uint8 statisticsCount;

        /**
         * This is a factor related to the battery of the node.
         */
        Uint16 costFactorBattery;

        /**
         * A retry factor.
         */
        Uint16 costFactorRetry;

        /**
         * The status of the edge in the subnet.
         */
        Status::StatusEnum edgeStatus;

        /**
         * The traffic allocation factor (traffic on edge = generated traffic * allocation factor) generated
         * by the currently evaluated graph route.
         * It is used on graph routing evaluation process.
         */
        Uint16 evalAllocationFactor;

        /**
         * The current cost on the edge; It is used on graph routing evaluation process.
         */
        RsqiLevelEnum::RsqiLevel radioQualityLevel;

        /**
         * True if the neighbor is in join Group Neighbor
         * This is transitory in the join process
         */
        bool groupNeighbor;

        /**
         * True if the group change value has been changed
         * This value is used to detect if a neighbor operation should be generated
         */
        bool changedGroupNeighbor;

        /*
         * traffic probability on a given graph
         */
        Uint16 trafficProbability;

        bool hasMngLinks;

        Uint8 publishDiagNo;

        LastPublishes lastPublishes;

    private:

        /**
         * Validates that the source and destination are not null and they are different,
         * so they can make a valid edge.
         */
        void checkNodes(Address32 source, Address32 destination);

    public:

        /**
         * Equal operator.
         * @param the Edge to compare with
         */
        bool operator==(Edge& edge) const;

        Edge(Address32 source, Address32 destination, Status::StatusEnum edgeStatus = Status::NEW);

        Edge(Address16 source, Address16 destination, Status::StatusEnum edgeStatus = Status::NEW);

        Edge(Edge32 edge, Status::StatusEnum edgeStatus = Status::NEW);

        virtual ~Edge();

        void resetEdge();

        bool existGraphOnEdge();

        bool isGraphOnEdge(Uint16 graphId);

        GraphsSet& getEdgeGraphs() {
            return graphs;
        }

        Address16 getSource() {
            return source;
        }

        Edge32 getID() {
            return ID;
        }
        void setSource(Address32 source) {
            this->source = source;
        }

        float getCost() {
            return 1;
        }

        Address16 getDestination() {
            return destination;
        }

        void setDestination(Address32 destination) {
            this->destination = destination;
        }

        Uint16 getTraffic() const {
            return traffic;
        }

        Status::StatusEnum getEdgeStatus() const {
            return edgeStatus;
        }

        void setEdgeStatus(Status::StatusEnum edgeStatus_) {
            edgeStatus = edgeStatus_;
        }

        Uint16 getCostFactorBattery() const {
            return costFactorBattery;
        }

        void setCostFactorBattery(Uint16 costFactorBattery) {
            this->costFactorBattery = costFactorBattery;
        }
        Uint16 getCostFactorFail() const {
            return costFactorFail;
        }

        void setCostFactorFail(Uint16 costFactorFail) {
            this->costFactorFail = costFactorFail;
        }

        Uint16 getCostFactorRetry() const {
            return costFactorRetry;
        }

        void setCostFactorRetry(Uint16 costFactorRetry) {
            this->costFactorRetry = costFactorRetry;
        }

        Uint16 getAllocationFactor() {
            return evalAllocationFactor;
        }

        void setAllocationFactor(Uint16 allocationFactor) {
            this->evalAllocationFactor = allocationFactor;
        }

        void addAllocationFactor(Uint16 evalAllocationFactor) {
            this->evalAllocationFactor += evalAllocationFactor;
        }

        void unsetHasMngLinks() {
            hasMngLinks = false;
        }

        void setHasMngLinks() {
            hasMngLinks = true;
        }

        bool HasMngLinks() {
            return hasMngLinks;
        }

        Uint8 getPublishDiasgNo() {
            return publishDiagNo;
        }

        int getFailedTxPercent() const;

        int getFailedTxPercentShort() const;

        bool isWorst(EdgePointer const &orig) const {
            if (!orig) return true;
            return getFailedTxPercentShort() > orig->getFailedTxPercentShort()
                || getFailedTxPercent() > orig->getFailedTxPercent();
        }

        int  getEvalEdgeCost(Uint8 k1factor, Uint8 noChilds) {
        	int edgeCost = (k1factor * radioQualityLevel + (100 - k1factor) * noChilds);
        	LOG_DEBUG( "Cost_of_edge("<< std::hex << (int) source << "," << std::hex << (int) destination << ")=" << std::dec << edgeCost);
        	return edgeCost;
        }

        RsqiLevelEnum::RsqiLevel getRadioQualityLevel() {
            return radioQualityLevel;
        }

        bool existsGraphTrafficOnEdge(Uint16 graphId) {
            return graphs.find(graphId) != graphs.end() && trafficProbability > 0;
        }

        Uint16 getGraphTrafficProbabilityOnEdge(Uint16 graphId) {
            return trafficProbability;
        }

    public:

        /**
         * Returns <code>true</code> if the edge has the node as the source OR as the destination.
         */
        bool containsNode(Address32 nodeAddress32);

        /**
         * Spec: Isa100 D2b page 310
         * RSQI shall be reported as a qualitative assessment of signal quality, with higher number
         *indicating a better signal. A value of 1-63 indicates a poor signal, 64-127 a fair signal, 128-
         *191 a good signal, and 192-255 an excellent signal. A value of zero indicates that the chipset
         *does not support any signal quality diagnostics other than RSSI.
         *
         */
        void updateRadioQualityLevel();

        RsqiLevelEnum::RsqiLevel calculateRadioQualityLevel(Uint8 _rsqi);

        std::string getRadioQualityLevelString();

        /**
         * Update the edge RSQI indicator.
         * @param rslQual
         */
        bool setRSQI(Uint8 rsqi);

        /**
         * Update the edge diagnostics.
         */
        bool addDiagnostics(Int8 rssi, Uint8 rsqi, Uint16 sent, Uint16 received, Uint16 failed,
        		Uint16 tresholdNoPublishLong, Uint16 tresholdNoPublishShort);

        /**
         * Add graph
         * @return
         */
        void addGraph(Uint16 graphId);

        /**
         * delete graph
         * @return
         */
        void deleteGraph(Uint16 graphId);

        Int8 getRSSI() {
            return rssi;
        }

        Uint8 getRSQI() {
            return rsqi;
        }

        Uint32 getSent() {
            return sent;
        }

        void resetSent() {
            sent = 0;
        }

        Uint32 getReceived() {
            return received;
        }

        void resetReceived() {
            received = 0;
        }

        Uint32 getFailedSent() {
            return failed;
        }

        Uint32 getNumberOfLastSentPackages() {
            return (lastPublishes.sumOfFailedSent + lastPublishes.sumOfSuccessSent);
        }

        Uint32 getNumberOfLastSentPackagesShort() {
            return (lastPublishes.sumOfFailedSentShort + lastPublishes.sumOfSuccessSentShort);
        }

        void resetFailedSent() {
            failed = 0;
        }

        void resetEdgeDiagnosticsOnChangeParent();

        void setGroupNeighbor(bool _groupNeighbor) {
            groupNeighbor = _groupNeighbor;
            changedGroupNeighbor = true;
        }

        bool isGroupNeighbor() {
            return groupNeighbor;
        }

        void resetChangedGroupNeighbor() {
            changedGroupNeighbor = false;
        }

        bool isChangedGroupNeighbor() {
            return changedGroupNeighbor;
        }

        /**
         * Returns a string representation of this Edge.
         */
        friend std::ostream& operator<<(std::ostream&, const Edge&);

};
typedef boost::shared_ptr<Edge> EdgePointer;
typedef std::list<Routing::EdgePointer> EdgesList;

}
}
}

#endif /* ROUTINGEDGE_H_ */
