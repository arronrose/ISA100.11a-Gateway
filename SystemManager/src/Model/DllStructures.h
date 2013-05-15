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
 * DllStructures.h
 *
 *  Created on: Oct 15, 2009
 *      Author: Sorin.Bidian, catalin.pop
 */

#ifndef DLLSTRUCTURES_H_
#define DLLSTRUCTURES_H_

#include "Common/Utils/DllUtils.h"
#include "Misc/Marshall/NetworkOrderStream.h"

namespace Isa100 {
namespace Model {

struct Candidates {
    /**
     * The number of neighbors that have been discovered.
     */
    Uint8 candidatesCount;

    struct NeighborRadio {
        /**
         * The 16-bit address of candidate neighbor in the DL subnet.
         */
        Uint16 neighbor; //ExtDLUint
        /**
         * The quality of the radio signal from neighbor.
         */
        Uint8 radio;
    };
    std::vector<NeighborRadio> neighborRadioList;

    Candidates();

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

    std::string toString();
};
std::ostream& operator<<(std::ostream& stream, const Candidates& candidates);

struct Summary {
    Int8 rssi;       //level -- signal strength
    Uint8 rsqi;      //level -- signal quality
    Uint16 rxDPDU;          //count; ExtDLUint -- Count of valid DPDUs received from neighbor, excluding ACKs, NACKs, and DPDUs without DSDU payloads
    Uint16 txSuccessful;    //count; ExtDLUint -- Count of successful unicast transmissions to the neighbor, where an ACK was received in response
    Uint16 txFailed;        //count; ExtDLUint -- Count of DPDU unicast transmissions, where no ACK or NACK was received in response
    Uint16 txCCA_Backoff;   //count; ExtDLUint -- Count of unicast transmissions that were aborted due to CCA. These aborted transmissions are not included in TxFailed
    Uint16 txNACK;          //count; ExtDLUint -- Count of NACKs received, not included in TxFailed
    Int16 clockSigma; //level -- estimate of standard deviation of clock corrections, in units of 2-20 s

    Summary();

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

};
std::ostream& operator<<(std::ostream& stream, const Summary& summary);

struct ClockDetail {
    Int16 clockBias; //level
    Uint16 clockCount;      //count; ExtDLUint
    Uint16 clockTimeout;    //count; ExtDLUint
    Uint16 clockOutliers;   //count; ExtDLUint

    ClockDetail();

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

};
std::ostream& operator<<(std::ostream& stream, const ClockDetail& clock);


struct NeighborDiag {
    LOG_DEF("I.M.NeighborDiag");

    Uint8 diagLevel;

    /*
     * Unique identifier.
     */
    Uint16 index; //ExtDLUint

    Summary summary;

    ClockDetail clockDetail;

    NeighborDiag(Uint16 index_);

    void unmarshall(NE::Misc::Marshall::InputStream& stream, Uint8 diagLevel);

//    std::string toString();
};

std::ostream& operator<<(std::ostream& stream, const NeighborDiag& diag);



struct ChannelDiag {

    /**
     * Number of attempted unicast transmissions for all channels.
     */
    Uint16 count;

    struct ChannelTransmission {
        /**
         * Percentage of time transmissions on channel that did not receive an ACK.
         */
        Int8 noAck;

        /**
         * Percentage of time transmissions on channel aborted due to CCA.
         */
        Int8 ccaBackoff;
    };

    std::vector<ChannelTransmission> channelTransmissionList;

    ChannelDiag();

    void marshall(NE::Misc::Marshall::OutputStream& stream);

    void unmarshall(NE::Misc::Marshall::InputStream& stream);

};
std::ostream& operator<<(std::ostream& stream, const ChannelDiag& diag);


}
}

#endif /* DLLSTRUCTURES_H_ */
