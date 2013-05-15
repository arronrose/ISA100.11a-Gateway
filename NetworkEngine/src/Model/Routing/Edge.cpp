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

#include "Edge.h"

namespace boost {
    size_t hash_value(Uint16 const & t) {
        return t ;
    }
}


namespace NE {

namespace Model {

namespace Routing {

#warning TO DO --add real cost to edge

#define SRC_DEST_STREAM std::hex << source << "->" << destination << std::dec


Edge::Edge(Address32 _source, Address32  _destination, Status::StatusEnum _edgeStatus) :
         ID(0),
         source(_source),
         destination(_destination),
        reportedTraffic(0),
        traffic(0),
        rssi(0),
        rsqi(0),
        sent(0),
        received(0),
        failed(0),
        oldCostFactorFail(0),
        costFactorFail(0),
        statisticsCount(0),
        costFactorBattery(0),
        costFactorRetry(0),
        edgeStatus(_edgeStatus),
        evalAllocationFactor(0),
        radioQualityLevel(RsqiLevelEnum::POOR),
        groupNeighbor(false),
        changedGroupNeighbor(false),
        trafficProbability(0),
        hasMngLinks(false),
        publishDiagNo(0) {

	// LOG_DEBUG("create Edge: source=" << Address::toString(source->getNodeAddress32()) << ", dest=" << Address::toString(destination->getNodeAddress32()));

    checkNodes(_source, _destination);

    rssi = -127;
    rsqi = 0;
    sent = 0;
    received = 0;
    failed = 0;
    oldCostFactorFail = 100;
    costFactorFail = 100;

    statisticsCount = 0;

    traffic = 0;
    costFactorBattery = 1;
    costFactorRetry = 1;

    radioQualityLevel = RsqiLevelEnum::POOR;
    evalAllocationFactor = 0;

    hasMngLinks = false;

    publishDiagNo = 0;
}

Edge::Edge(Address16 _source, Address16  _destination, Status::StatusEnum _edgeStatus) :
    source(_source), destination(_destination), edgeStatus(_edgeStatus) {

    // LOG_DEBUG("create Edge: source=" << Address::toString(source->getNodeAddress32()) << ", dest=" << Address::toString(destination->getNodeAddress32()));

    checkNodes(_source, _destination);

    rssi = -127;
    rsqi = 0;
    sent = 0;
    received = 0;
    failed = 0;
    oldCostFactorFail = 100;
    costFactorFail = 100;

    statisticsCount = 0;

    traffic = 0;
    costFactorBattery = 1;
    costFactorRetry = 1;

    radioQualityLevel = RsqiLevelEnum::POOR;
    evalAllocationFactor = 0;

    hasMngLinks = false;

    publishDiagNo = 0;
}

Edge::Edge(Edge32 edge, Status::StatusEnum _edgeStatus):edgeStatus(_edgeStatus) {
    ID = edge;
    source = edge >> 16;
    destination = edge;
    hasMngLinks = false;
    publishDiagNo = 0;
}

Edge::~Edge() {
}

void Edge::resetEdge() {
    LOG_DEBUG("reset Edge " << *this );
    graphs.clear();
}

void Edge::resetEdgeDiagnosticsOnChangeParent() {
    publishDiagNo = 0;
    lastPublishes.reset();
}

void Edge::checkNodes(Address32 source, Address32 destination) {

    if (source == destination) {
        LOG_ERROR("You can not create an edge with the source = destination !!!");
        throw NEException("You can not create an edge with the source = destination !!!");
    }
}

bool Edge::operator==(Edge& edge) const {
    if (source == edge.getSource()) {
        if (destination == edge.getDestination()) {
            return true;
        }
    }

    return false;
}

bool Edge::existGraphOnEdge() {
    return graphs.size() != 0;
}

bool Edge::isGraphOnEdge(Uint16 graphId) {
    return(std::find(graphs.begin(), graphs.end(),graphId ) != graphs.end());
}

int Edge::getFailedTxPercent() const {
    LOG_INFO( " Edge::getFailedTxPercent sumOfFailedSent=" << (int)lastPublishes.sumOfFailedSent
    		<< ", sumOfSuccessSent=" << (int)lastPublishes.sumOfSuccessSent);

    if (lastPublishes.sumOfFailedSent + lastPublishes.sumOfSuccessSent > 0) {
        return (lastPublishes.sumOfFailedSent * 100) / (lastPublishes.sumOfFailedSent + lastPublishes.sumOfSuccessSent);
    }
    return 0;
}

int Edge::getFailedTxPercentShort() const {
    LOG_INFO( " Edge::getFailedTxPercentShort sumOfFailedSentShort=" << (int)lastPublishes.sumOfFailedSentShort
    		<< ", sumOfSuccessSentShort=" << (int)lastPublishes.sumOfSuccessSentShort);

    if (lastPublishes.sumOfFailedSentShort + lastPublishes.sumOfSuccessSentShort > 0) {
        return (lastPublishes.sumOfFailedSentShort * 100) / (lastPublishes.sumOfFailedSentShort + lastPublishes.sumOfSuccessSentShort);
    }
    return 0;
}

bool Edge::containsNode(Address32 nodeAddres32) {
    if (source == nodeAddres32 || destination == nodeAddres32) {
        return true;
    }

    return false;
}

/**
 * Spec: Isa100 D2b page 310
 * RSQI shall be reported as a qualitative assessment of signal quality, with higher number
 *indicating a better signal. A value of 1-63 indicates a poor signal, 64-127 a fair signal, 128-
 *191 a good signal, and 192-255 an excellent signal. A value of zero indicates that the chipset
 *does not support any signal quality diagnostics other than RSSI.
 *
 */
void Edge::updateRadioQualityLevel(){


    radioQualityLevel = calculateRadioQualityLevel(rsqi);

}


RsqiLevelEnum::RsqiLevel Edge::calculateRadioQualityLevel(Uint8 _rsqi) {
    if (_rsqi <= 63) {
         return RsqiLevelEnum::POOR;
    } else if (_rsqi <= 127) {
        return  RsqiLevelEnum::FAIR;
    } else if (_rsqi <= 191) {
        return RsqiLevelEnum::GOOD;
    } else {
        return RsqiLevelEnum::EXCELLENT;
    }
}

bool Edge::setRSQI(Uint8 rsqi_) {
    rsqi = rsqi_;
    updateRadioQualityLevel();

    LOG_DEBUG("setRSQI(" << SRC_DEST_STREAM << "): "<< (int)rsqi_ << " " << RsqiLevelEnum::toString(radioQualityLevel));

   return false;
}

bool Edge::addDiagnostics(Int8 rssi_, Uint8 rsqi_, Uint16 sent_, Uint16 received_, Uint16 failed_,
		Uint16 tresholdNoPublish,  Uint16 tresholdNoPublishShort) {

    LOG_DEBUG("addDiag( " << SRC_DEST_STREAM << "): - rssi=" << (int)rssi_ << ", rsqi=" << (int)rsqi_
                << ", sent=" << (int)sent_ << ", received=" << (int)received_ << ", failed=" << (int)failed_);

    rssi = rssi_; //reported as average by device
    rsqi = rsqi_; //reported as average by device

    if (sent_ >= (MAX_32BITS_VALUE - sent)) {
        sent = MAX_32BITS_VALUE;
    } else {
        sent += sent_;
    }

    if (received_ >= (MAX_32BITS_VALUE - received)) {
        received = MAX_32BITS_VALUE;
    } else {
        received += received_;
    }

    if (failed_ >= (MAX_32BITS_VALUE - failed)) {
        failed = MAX_32BITS_VALUE;
    } else {
        failed += failed_;
    }

    Uint16 successSentSize = lastPublishes.successSent.size();
    Uint16 failedSentSize = lastPublishes.failedSent.size();

    LOG_INFO(" BEFORE - addDiagnostics : " << SRC_DEST_STREAM
				<< ", rssi=" << (int)rssi_
				<< ", rsqi=" << (int)rsqi_
				<< ", sent=" << (int)sent_
				<< ", received=" << (int)received_
				<< ", failed=" << (int)failed_
				<< ", publishDiagNo=" << (int)publishDiagNo
                << ", sumOfFailedSent=" << (int)lastPublishes.sumOfFailedSent
                << ", sumOfSuccessSent=" << (int)lastPublishes.sumOfSuccessSent
                << ", sumOfFailedSentShort=" << (int)lastPublishes.sumOfFailedSentShort
                << ", sumOfSuccessSentShort=" << (int)lastPublishes.sumOfSuccessSentShort
                << ", countSuccessSent=" << (int)successSentSize
                << ", countFailedSent=" << (int)failedSentSize);

    if ( tresholdNoPublish !=  publishDiagNo ) {
        ++publishDiagNo;
        lastPublishes.successSent.push_back(sent_);
        lastPublishes.failedSent.push_back(failed_);
        lastPublishes.sumOfFailedSent += failed_;
        lastPublishes.sumOfSuccessSent += sent_;

        if (publishDiagNo <= tresholdNoPublishShort) {
            lastPublishes.sumOfFailedSentShort += failed_;
            lastPublishes.sumOfSuccessSentShort += sent_;
        } else {
            if (!lastPublishes.successSent.empty() && !lastPublishes.failedSent.empty()) {
                lastPublishes.sumOfFailedSentShort -=  lastPublishes.failedSent[failedSentSize - tresholdNoPublishShort];
                lastPublishes.sumOfSuccessSentShort -= lastPublishes.successSent[successSentSize - tresholdNoPublishShort];
                lastPublishes.sumOfFailedSentShort += failed_;
                lastPublishes.sumOfSuccessSentShort += sent_;
            }
        }
    } else {
        if (!lastPublishes.successSent.empty() && !lastPublishes.failedSent.empty()) {
            std::vector<Uint16>::iterator firstSuccessSent = lastPublishes.successSent.begin();
            std::vector<Uint16>::iterator firstFailedSent = lastPublishes.failedSent.begin();
            lastPublishes.sumOfFailedSent -= *firstFailedSent;
            lastPublishes.sumOfSuccessSent -= *firstSuccessSent;
            lastPublishes.sumOfFailedSent += failed_;
            lastPublishes.sumOfSuccessSent += sent_;

            lastPublishes.sumOfFailedSentShort -=  lastPublishes.failedSent[failedSentSize - tresholdNoPublishShort];
            lastPublishes.sumOfSuccessSentShort -= lastPublishes.successSent[successSentSize - tresholdNoPublishShort];
            lastPublishes.sumOfFailedSentShort += failed_;
            lastPublishes.sumOfSuccessSentShort += sent_;

            lastPublishes.successSent.erase(firstSuccessSent);
            lastPublishes.failedSent.erase(firstFailedSent);
            lastPublishes.successSent.push_back(sent_);
            lastPublishes.failedSent.push_back(failed_);

            LOG_INFO("AFTER - addDiagnostics" << ", sumOfFailedSent=" << (int)lastPublishes.sumOfFailedSent
                    << ", sumOfSuccessSent=" << (int)lastPublishes.sumOfSuccessSent
                    << ", sumOfFailedSentShort=" << (int)lastPublishes.sumOfFailedSentShort
                    << ", sumOfSuccessSentShort=" << (int)lastPublishes.sumOfSuccessSentShort);
        }
    }

    updateRadioQualityLevel();

    return false;
}



void Edge::addGraph(Uint16 graphId) {
    if(graphs.find(graphId) == graphs.end()) {
        graphs.insert(graphId);
    }
}

void Edge::deleteGraph(Uint16 graphId) {
	graphs.erase(graphId);
}

std::ostream& operator<<(std::ostream& stream, const Edge& edge) {
	stream << std::hex << (int)edge.source << "->" << (int)edge.destination;
        stream << ", rssi=" << std::dec << (int)edge.rssi;
        stream << ", rsqi=" << (int)edge.rsqi;
        stream << ", traffic=" << (int)edge.traffic;
        stream << ", sent=" << edge.sent;
        stream << ", received=" << edge.received;
        stream << ", failed=" << edge.failed;
        stream << ", status=" << Status::getStatusDescription(edge.edgeStatus);
        stream << ", groupNeighbor=" << edge.groupNeighbor;
        stream << ", costFactorFail=" << (int)edge.costFactorFail;
        stream << ", statisticsCount=" << (int)edge.statisticsCount;;
        stream << ", costFactorBattery=" << (int)edge.costFactorBattery;
        stream << ", costFactorRetry=" << (int)edge.costFactorRetry;
        stream << ", evalAllocationFactor=" <<(int)edge.evalAllocationFactor;
        stream << ", publishDiagNo=" << (int)edge.publishDiagNo;
    return stream;
}

}
}
}
