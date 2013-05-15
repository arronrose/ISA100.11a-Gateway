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
 * smTypes.h
 *
 *  Created on: Feb 17, 2009
 *      Author: mulderul(catalin.pop), sorin.bidian, andrei.petrut, flori.parauan, ion.ticus
 */

#ifndef SMTYPES_H_
#define SMTYPES_H_
#include "Common/NETypes.h"


namespace Isa100 {

namespace Common {

/**
 * Bit challenge type.
 */
static const Uint8 CHALLENGE_BYTES_NO = 16;
typedef Uint8 Uint128[16];


namespace TSAP {
enum TSAP_Enum{
	TSAP_DMAP = 0,
	TSAP_SMAP = 1,
	TSAP_UAP1 = 1,
	TSAP_UAP2 = 2,
	TSAP_UAP3 = 3,
	TSAP_UAP4 = 4,
	TSAP_UAP5 = 5,
	TSAP_UAP6 = 6,
	TSAP_UAP7 = 7,
	TSAP_UAP8 = 8,
	TSAP_UAP10 = 10,
	TSAP_UAP11 = 11,
	TSAP_UAP12 = 12,
	TSAP_UAP13 = 13,
	TSAP_UAP14 = 14,
	TSAP_UAP15 = 15,
	TSAP_NOT_SET = 255
};

inline TSAP_Enum fromInt( int tsap)
{
    return (TSAP_Enum )( tsap);
}
inline TSAP_Enum fromPort( Uint16 port)
{
    return (TSAP_Enum )( port - 0xF0B0);
}

inline Uint16 toPort( TSAP_Enum  tsap)
{
    return (Uint16)(tsap + 0xF0B0);
}
}  // namespace TSAP

namespace ServicePriority {
enum ServicePriority {
    high = 0,
    medium = 1,
    low = 2
};
}//end namespace ServicePriority

namespace ServiceType {
    enum ServiceType {
        publish = 0,
        alertReport = 1,
        alertAcknowledge = 2,
        read = 3,
        write = 4,
        execute = 5,
        tunnel = 6
        //concatenate = 7,
    };

    inline ServiceType::ServiceType fromInt(Uint8 value){
        switch (value) {
        case ServiceType::publish : return ServiceType::publish;
        case ServiceType::alertReport : return ServiceType::alertReport;
        case ServiceType::alertAcknowledge : return ServiceType::alertAcknowledge;
        case ServiceType::read : return ServiceType::read;
        case ServiceType::write : return ServiceType::write;
        case ServiceType::execute : return ServiceType::execute;
        case ServiceType::tunnel : return ServiceType::tunnel;
        default: return ServiceType::execute;
        }
    }

    inline std::string toString(Isa100::Common::ServiceType::ServiceType serviceInfo){
        switch(serviceInfo){
            case Isa100::Common::ServiceType::publish : return "P";
            case Isa100::Common::ServiceType::alertReport : return "AR";
            case Isa100::Common::ServiceType::alertAcknowledge : return "AA";
            case Isa100::Common::ServiceType::read : return "R";
            case Isa100::Common::ServiceType::write : return "W";
            case Isa100::Common::ServiceType::execute : return "E";
            case Isa100::Common::ServiceType::tunnel :return "T";
            default : return "?";
         }
    }
}//end namespace ServiceType

namespace PrimitiveType {
    enum PrimitiveTypeEnum {
        request = 0,
        response = 1
    };
    inline PrimitiveType::PrimitiveTypeEnum fromInt(Uint8 value){
        switch (value) {
        case PrimitiveType::request : return PrimitiveType::request;
        case PrimitiveType::response : return PrimitiveType::response;
        default: return PrimitiveType::request;
        }
    }

    inline std::string toString(Isa100::Common::PrimitiveType::PrimitiveTypeEnum value){
        switch (value) {
            case PrimitiveType::request : return "Req";
            case PrimitiveType::response : return "Res";
            default: return "?";
        }
    }
}//end namespace PrimitiveType

namespace AddressingMode {
	enum AddressingModeEnum {
		fourBitMode = 0,
		eightBitMode = 1,
		sixteenBitMode = 2,
		inferredBitMode = 3
	};

    inline AddressingMode::AddressingModeEnum fromInt(Uint8 value){
        switch (value) {
        case AddressingMode::fourBitMode : return AddressingMode::fourBitMode;
        case AddressingMode::eightBitMode : return AddressingMode::eightBitMode;
        case AddressingMode::sixteenBitMode : return AddressingMode::sixteenBitMode;
        case AddressingMode::inferredBitMode : return AddressingMode::inferredBitMode;
        default: return AddressingMode::eightBitMode;
        }
    }

	inline Uint8 getSize(Isa100::Common::AddressingMode::AddressingModeEnum addressingMode){
		switch (addressingMode)
		{
		case AddressingMode::fourBitMode : return 1;
		case AddressingMode::eightBitMode : return 2;
		case AddressingMode::sixteenBitMode : return 4;
		case AddressingMode::inferredBitMode : return 0;
		default : return 0;
		}
	}

	inline std::string toString(Isa100::Common::AddressingMode::AddressingModeEnum addressingMode){
		switch (addressingMode)
		{
		case AddressingMode::fourBitMode : return "fourBitMode";
		case AddressingMode::eightBitMode : return "eightBitMode";
		case AddressingMode::sixteenBitMode : return "sixteenBitMode";
		case AddressingMode::inferredBitMode : return "inferredBitMode";
		default : return "?";
		}
	}
}//end namespace AddressingMode

namespace TransportPorts {

enum TransportPortsEnum{
	PORT_DMAP = 0xF0B0, //int 61616
	PORT_SMAP = 0xF0B1  //int 61617
};
inline TransportPorts::TransportPortsEnum fromTSAP(TSAP::TSAP_Enum tsap)
{
	return (TransportPorts::TransportPortsEnum)(tsap | 0xF0B0);
}

}  // namespace TransportPorts

}  // namespace Common

}  // namespace Isa100


#endif /* SMTYPES_H_ */
