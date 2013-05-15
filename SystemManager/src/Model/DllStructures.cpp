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
 * DllStructures.cpp
 *
 *  Created on: Oct 15, 2009
 *      Author: Sorin.Bidian, catalin.pop
 */
#include "DllStructures.h"

using namespace NE::Misc::Marshall;

namespace Isa100 {
namespace Model {

Candidates::Candidates() :
    candidatesCount(0) {
}

void Candidates::unmarshall(NE::Misc::Marshall::InputStream& stream) {
    stream.read(candidatesCount);
    for (int i = 0; i < candidatesCount; ++i) {
        NeighborRadio neighborRadio;
        neighborRadio.neighbor = Isa100::Common::Utils::unmarshallExtDLUint(stream);
        stream.read(neighborRadio.radio);

        neighborRadioList.push_back(neighborRadio);
    }
}

std::string Candidates::toString() {
    std::ostringstream stream;
    stream << "Candidates {";
    stream << "candidatesCount=" << (int) candidatesCount;
    for (int i = 0; i < candidatesCount; ++i) {
        stream << ", neighborRadioList[" << i << "]:";
        stream << " neighbor=" << std::hex << (int) neighborRadioList[i].neighbor;
        stream << ", radio=" << (int) neighborRadioList[i].radio;
    }
    stream << "}";

    return stream.str();
}

std::ostream& operator<<(std::ostream& stream, const Candidates& candidates){
	stream << "Candidates {";
	stream << "count=" << (int) candidates.candidatesCount;
	for (int i = 0; i < candidates.candidatesCount; ++i) {
		stream << ", neighborRadioList[" << i << "]:";
		stream << " neighbor=" << std::hex << (int) candidates.neighborRadioList[i].neighbor;
		stream << ", radio=" << (int) candidates.neighborRadioList[i].radio;
	}
	stream << "}";

	return stream;
}

Summary::Summary() :
    rssi(-127), rsqi(0), rxDPDU(0), txSuccessful(0), txFailed(0), txCCA_Backoff(0), txNACK(0), clockSigma(0) {
}

void Summary::unmarshall(InputStream& stream) {
    stream.read(rssi);
    stream.read(rsqi);
    rxDPDU = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    txSuccessful = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    txFailed = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    txCCA_Backoff = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    txNACK = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    stream.read(clockSigma);
}

std::ostream& operator<<(std::ostream& stream, const Summary& summary){
	int fail = 0;
	if (((summary.txSuccessful + summary.txFailed +  summary.txNACK) != 0) && (summary.txFailed != 0 )) {
		fail = (summary.txFailed * 100) / (summary.txSuccessful + summary.txFailed +  summary.txNACK);
	}
    stream << "Summary {";
    stream << "failed="<< std::dec << (int)fail <<"%";
    stream << ", rssi=" << std::dec << (int) summary.rssi;
    stream << ", rsqi=" << (int) summary.rsqi;
    stream << ", rxDPDU=" << (int) summary.rxDPDU;
    stream << ", txScs=" << (int) summary.txSuccessful;
    stream << ", txFld=" << (int) summary.txFailed;
    stream << ", txCCA=" << (int) summary.txCCA_Backoff;
    stream << ", txNACK=" << (int) summary.txNACK;
    stream << ", clkSig=" << (int) summary.clockSigma;
    stream << "}";

    return stream;
}

ClockDetail::ClockDetail() :
    clockBias(0), clockCount(0), clockTimeout(0), clockOutliers(0) {

}

void ClockDetail::unmarshall(InputStream& stream) {
    stream.read(clockBias);
    clockCount = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    clockTimeout = Isa100::Common::Utils::unmarshallExtDLUint(stream);
    clockOutliers = Isa100::Common::Utils::unmarshallExtDLUint(stream);
}

std::ostream& operator<<(std::ostream& stream, const ClockDetail& clock){
    stream << "ClockDetail {";
    stream << "Bias=" << std::dec << (int) clock.clockBias;
    stream << ", Count=" << (int) clock.clockCount;
    stream << ", Timeout=" << (int) clock.clockTimeout;
    stream << ", Outliers=" << (int) clock.clockOutliers;
    stream << "}";

    return stream;
}

NeighborDiag::NeighborDiag(Uint16 index_) :
    diagLevel(3),//default configured for Summary and Clock. Is updated in unmarshall
    index(index_) {

}

void NeighborDiag::unmarshall(InputStream& stream, Uint8 diagLevel_) {
    //index is parsed separately in DO
    diagLevel = diagLevel_;

    if (diagLevel & 0x01) {
        summary.unmarshall(stream);
    }
    if (diagLevel & 0x02) {
        clockDetail.unmarshall(stream);
    } else if (diagLevel > 0x03) { //0x03 is for both summary and clockDetail
        THROW_EX(NE::Common::NEException, "Invalid diag level= " << (int)diagLevel << ".");
    }
}

std::ostream& operator<<(std::ostream& stream, const NeighborDiag& diag){
	stream << "NeighborDiag {";
	stream << "index=" << std::hex << (int) diag.index;
	if (diag.diagLevel & 0x01) {
	    stream << ", " << diag.summary;
	}
	if (diag.diagLevel & 0x02) {
	    stream << ", " << diag.clockDetail;
	}
	stream << "}";
	return stream;
}

ChannelDiag::ChannelDiag()
    : count(0) {
}

void ChannelDiag::marshall(NE::Misc::Marshall::OutputStream& stream) {
    stream.write(count);
    for (int i = 0; i <= 15; ++i) { //16 channels
        stream.write(channelTransmissionList[i].noAck);
        stream.write(channelTransmissionList[i].ccaBackoff);
    }
}

void ChannelDiag::unmarshall(NE::Misc::Marshall::InputStream& stream) {
    stream.read(count);
    for (int i = 0; i <= 15; ++i) { //16 channels
        ChannelTransmission channelTransmission;
        stream.read(channelTransmission.noAck);
        stream.read(channelTransmission.ccaBackoff);

        channelTransmissionList.push_back(channelTransmission);
    }
}

std::ostream& operator<<(std::ostream& stream, const ChannelDiag& diag){
    stream << "ChannelDiag (noAck,CCA){";
    stream << "cnt=" << (int) diag.count;
    for (int i = 0; i <= 15; ++i) {
        stream << " C" << i << "(";
        if (diag.channelTransmissionList[i].noAck == 0){
            stream <<  " ";
        } else {
            stream <<  (int) diag.channelTransmissionList[i].noAck;
        }
        stream << ", ";
        if (diag.channelTransmissionList[i].ccaBackoff == 0){
            stream << "";
        } else {
            stream << (int) diag.channelTransmissionList[i].ccaBackoff;
        }
        stream << ")";
    }
    stream << "}";

    return stream;
}

}
}

