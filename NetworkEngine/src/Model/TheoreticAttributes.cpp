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
 * TheoreticAttributes.cpp
 *
 *  Created on: Oct 20, 2009
 *      Author: Catalin Pop
 */

#include "Model/TheoreticAttributes.h"
#include "Common/NEAddress.h"

namespace NE {

namespace Model {


BandwidthChunck::BandwidthChunck():
    setNo(0),
    freqNo(0),
    slotsReservationMap(0),
	usedForRouter(false)
    {}

BandwidthChunck::BandwidthChunck(Uint8 setNo_, Uint8 freqNo_):
    setNo(setNo_),
    freqNo(freqNo_),
    slotsReservationMap(0),
    usedForRouter(false)
    {}

Uint16 BandwidthChunck::getFirstAvailableSlot() {

    Uint16 i = 0;
    Uint32 nCrtReservation = slotsReservationMap;


    for (; nCrtReservation & 1; ++i, nCrtReservation >>= 1)
        ;

    return i;
}

std::ostream& operator<<(std::ostream& stream, const BandwidthChunck& bandwidthChunck) {
    stream << std::dec << "SET:" <<(int)bandwidthChunck.setNo
        << ", F:" << (int)bandwidthChunck.freqNo
        << ", MAP:" << std::hex << bandwidthChunck.slotsReservationMap
        << std::dec << ", R:" << bandwidthChunck.usedForRouter;
    return stream;
}

void TheoreticAttributes::addServedContract(EntityIndex contractKey, float traffic){
    ServedContractsMap::iterator itServedContract = servedContracts.find(contractKey);
    if (itServedContract == servedContracts.end()) {
        servedContracts.insert(ServedContractsMap::value_type(contractKey, traffic));
    } else {
        itServedContract->second += traffic;
    }
}

void TheoreticAttributes::removeServedContract(EntityIndex contractKey) {
    ServedContractsMap::iterator itServedContract = servedContracts.find(contractKey);
    if (itServedContract != servedContracts.end()) {
        servedContracts.erase(itServedContract);
    }
}

float TheoreticAttributes::getServedContractsTraffic() {
    float traffic = 0.0;
    ServedContractsMap::iterator itServedContract = servedContracts.begin();
    for (;itServedContract != servedContracts.end(); ++itServedContract){
        traffic += itServedContract->second;
    }
    return traffic;
}

void TheoreticAttributes::removeChunckInboundRouter(Uint8 bcSetNo, Uint8 bcFreqNo) {

    for(BCList::iterator it = mngChunckInboundRouter.begin(); it != mngChunckInboundRouter.end();) {
        if (it->setNo == bcSetNo && it->freqNo == bcFreqNo) {
            it = mngChunckInboundRouter.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TheoreticAttributes::removeChunckInboundNonRouter(Uint8 bcSetNo, Uint8 bcFreqNo) {

    for(BCList::iterator it = mngChunckInboundNonRouter.begin(); it != mngChunckInboundNonRouter.end();) {
        if (it->setNo == bcSetNo && it->freqNo == bcFreqNo) {
            it = mngChunckInboundNonRouter.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TheoreticAttributes::deleteCandidate(Address16 candidate) {

    for(Uint8 i = 0; i< DEFAULT_MAX_NO_CANDIDATES; ++i) {
        if(candidates[i] == candidate) {
            deleteCandidateFromPosition(i);
        }
    }
}


void TheoreticAttributes::deleteCandidateFromPosition(Uint8 position) {
    if(position >= DEFAULT_MAX_NO_CANDIDATES) {
        return;
    }

    if((DEFAULT_MAX_NO_CANDIDATES -1)  ==  position) {
        candidates[ position ] = 0;
        return;
    }

    memmove(&candidates[position], &candidates[position+1], (DEFAULT_MAX_NO_CANDIDATES - position -1)*sizeof(Address16));
    candidates[(DEFAULT_MAX_NO_CANDIDATES - position)] = 0;
    candidates[ DEFAULT_MAX_NO_CANDIDATES -1 ] = 0;
}



void TheoreticAttributes::clearCandidates() {
    memset(candidates, 0, sizeof(candidates));
}

bool TheoreticAttributes::isCandidateOnList(Address16 candidate) {
    for(Uint8 i = 0; i< DEFAULT_MAX_NO_CANDIDATES; ++i) {
        if(candidates[i] == candidate) {
           return true;
        }
    }

    return false;
}

bool TheoreticAttributes::candidateIsBadRate(Address16 candidate) {
    for(BadRateDevicesList::const_iterator itDev = devicesToIgnore.begin(); itDev != devicesToIgnore.end(); ++itDev) {
        if((*itDev).device16 == candidate) {
            return true;
        }
    }

    return false;
}

void TheoreticAttributes::getServedContracts(EntityIndexList& servedContractsList){
	LinkToContractMap::const_iterator itLinkAssociation = linkToContract.begin();
	for (; itLinkAssociation != linkToContract.end(); ++itLinkAssociation){
		servedContractsList.push_back(itLinkAssociation->second);
	}
}

void TheoreticAttributes::getLinksForContract(EntityIndex& contract, EntityIndexList& linksList){
    LinkToContractMap::const_iterator itLinkAssociation = linkToContract.begin();
    for (; itLinkAssociation != linkToContract.end(); ++itLinkAssociation){
        if(itLinkAssociation->second == contract) {
            linksList.push_back(itLinkAssociation->first);
        }
    }
}


Uint16 TheoreticAttributes::getNoOfUdoContractsAtOneSecond() {
	Uint16 noUdoContracts = 0;
    ContractToLinksMap::const_iterator itContract2Link =  udoContract2Links.begin();
    for (; itContract2Link != udoContract2Links.end(); ++itContract2Link){
        if(!itContract2Link->second.empty()) {
        	++noUdoContracts;
        }
    }
    return noUdoContracts;
}

std::ostream& operator<<(std::ostream& stream, const TheoreticAttributes& thAttributes) {
    BCList::const_iterator it;
    stream << std::endl << "TH mngChunckInboundNonRouter={";
    for(it = thAttributes.mngChunckInboundNonRouter.begin(); it != thAttributes.mngChunckInboundNonRouter.end(); ++it) {
        stream << "{" << *it << "}, ";
    }
    stream << "}";

    stream << std::endl << "TH mngChunckInboundRouter={";
    for(it = thAttributes.mngChunckInboundRouter.begin(); it != thAttributes.mngChunckInboundRouter.end(); ++it) {
        stream << "{" << *it << "}, ";
    }
    stream << "}";

    stream << std::endl << "TH mngChunckOutbound={";
    for(it = thAttributes.mngChunckOutbound.begin(); it != thAttributes.mngChunckOutbound.end(); ++it) {
        stream << "{" << *it << "}, ";
    }
    stream << "}";

    stream << std::endl << "TH childID=" << (int)thAttributes.childID;

    stream << std::endl << "TH Candidates={";
    for(Uint8 i = 0; i< DEFAULT_MAX_NO_CANDIDATES; ++i) {
        if(thAttributes.candidates[i] ) {
            stream << std::hex << thAttributes.candidates[i] << ", ";
        }
    }
    stream << "}";

    stream << std::endl << "TH BadRateIgnoredDevices={";
	for( BadRateDevicesList::const_iterator itIgnore = thAttributes.devicesToIgnore.begin(); itIgnore != thAttributes.devicesToIgnore.end(); ++itIgnore) {
		if(itIgnore->device16 ) {
			stream << std::hex << itIgnore->device16 << ", ";
		}
	}
    stream << "}";

    stream << std::endl << "TH servedContracts={";
    for(ServedContractsMap::const_iterator it = thAttributes.servedContracts.begin(); it != thAttributes.servedContracts.end(); ++it) {
        stream << std::hex << it->first << "=" << std::dec << it->second << ", ";
    }
    stream << "}";

    stream << std::endl << "TH linkToContract={";
    for(LinkToContractMap::const_iterator it = thAttributes.linkToContract.begin(); it != thAttributes.linkToContract.end(); ++it) {
        stream << std::hex << it->first << "=" << it->second << ", ";
    }
    stream << "}";

    stream << std::endl << "TH contractLinks={";
    for(ContractToLinksMap::const_iterator it = thAttributes.contractLinks.begin(); it != thAttributes.contractLinks.end(); ++it) {
        stream << std::hex << it->first << "=" << "(";
        const LinksList& linksList = it->second;
        for (LinksList::const_iterator itLink = linksList.begin(); itLink != linksList.end(); ++itLink){
            stream << *itLink << ", ";
        }
        stream << ") ";
    }
    stream << "}";


    return stream;
}

}  // namespace Model

}  // namespace NE

