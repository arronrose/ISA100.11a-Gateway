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

#ifndef ISA100ADDRESS_H_
#define ISA100ADDRESS_H_

#include "Common/NETypes.h"
#include "Misc/Marshall/Stream.h"
#include <boost/unordered_set.hpp>
#include <set>
#include <list>

using namespace NE::Misc::Marshall;

#define Address_toStream(address)\
    std::hex << address << std::dec

namespace NE {
namespace Common {

namespace Address {

inline
Address32 createAddress32(Uint16 subnetId, Uint16 address16) {
    return ((subnetId << 16) + address16);
}

inline void toString(Address32 address32, std::string &addressString) {
    std::ostringstream stream;
    stream << std::hex << (long long) address32;
    addressString = stream.str();
}
inline void toString(Address32 address32, std::ostream& stream) {
    stream << std::hex << (long long) address32;
}

inline std::string toString(Address32 address32) {
    std::ostringstream stream;
    stream << std::hex << (long long) address32;
    return stream.str();
}
inline Uint16 getAddress16(const Address32 address32) {
    return (address32 & 0x0000FFFF);
}

inline Uint16 extractSubnet(Address32 address32) {
    return address32 >> 16;
}
}

/**
 * 64 bits address used as EUI-64 address.
 */
struct Address64 {
        /** the length of the address in octets */
        static const Uint8 LENGTH = 8;

        /** the length of the address represented as string */
        static const Uint8 STRING_LENGTH = 64 / 4 + 64 / 16 - 1;

        /** the length of the address represented as string without ':' separators */
        static const Uint8 SHORT_STRING_LENGTH = 64 / 4;

        /** the value of the address */
        Uint8 value[8];

        std::string address64String;

        /**
         * Initializes the address to 0x0000:0x0000:0x0000:0x0000
         */
        Address64();

        /**
         *
         */
        void marshall(OutputStream& stream) const;

        /**
         *
         */
        void unmarshall(InputStream& stream);

        /**
         * Initializes the address from a binary format.
         * @param the binary value of the address
         */
        void loadBinary(Uint8 tmpValue[LENGTH]);

        /**
         * Load the address from format FFFF:FFFF:FFFF:FFFF.
         * @param tmpValue the string value of the address
         * @throws Isa100::Common::InvalidArgumentException if the length
         * of tmpValue is not Address64::STRING_LENGTH.
         */
        void loadString(const std::string& tmpValue);

        /**
         * Load the address from format FFFFFFFFFFFFFFFF.
         * @param tmpValue
         */
        void loadShortString(const std::string& tmpValue);

        static Address64 createFromString(const std::string& tmpValue);
        static Address64 createFromShortString(const std::string& tmpValue);

        /**
         * Check if the address match the input format.
         * Example: FFFF:FFFF:FFFF:XXXX match the address FFFF:FFFF:FFFF:FFFF
         *          FFFF:FFFF:FFDD:XXXX doesn't match the address FFFF:FFFF:FFFF:FFFF
         */
        bool match(const std::string& format) const;

        /**
         * Less operator required by map container.
         * @param compare the address compare with
         */
        bool operator<(const Address64 &compare) const;

        /**
         * Equal operator.
         * @param compare the address compare with
         */
        bool operator==(const Address64 &address) const;

        /**
         * Formats the value in format hexa
         */
        void createAddressString();

        /**
         * returns the hexa formated value
         * @return
         */
        const std::string& toString() const;
};

/**
 * 128 bits address used as IPV6 address.
 */
struct Address128 {
        /** the length of the address in octets */
        static const Uint8 LENGTH = 16;

        /** the length of the address represented as string */
        static const Uint8 STRING_LENGTH = 128 / 4 + 128 / 16 - 1;

        /** the value of the address */
        Uint8 value[LENGTH];

        /** the address128 in string format */
        std::string stringAddress128;

        /**
         * Initializes the address to 0x0000:0x0000:0x0000:0x0000:0x0000:0x0000:0x0000:0x0000
         */
        Address128();

        /**
         *
         */
        void marshall(OutputStream& stream) const;

        /**
         *
         */
        void unmarshall(InputStream& stream);

        /**
         * Initializes the address from a binary format
         * @param the binary value of the address
         */
        void loadBinary(Uint8 tmpValue[LENGTH]);

        /**
         * Load the address from format FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF.
         * @param tmpValue the string value of the address
         * @throws Isa100::Common::InvalidArgumentException if the length
         * of tmpValue is not Address128::STRING_LENGTH.
         */
        void loadString(const std::string& tmpValue);

        /**
         *
         */
        static Address128 createFromString(const std::string& tmpValue);

        /**
         * Less operator required by map container.
         * @param compare the address compare with
         */
        bool operator<(const Address128 &compare) const;

        /**
         * Equal operator.
         * @param compare the address compare with
         */
        bool operator==(const Address128 &address) const;

        /**
         * format address to string
         */
        void setAddressString();

        /**
         * Formats the value in format hexa.
         */
        const std::string& toString() const;

        /**
         *
         */
        static unsigned char getSize();
};

class AddressNotFoundException: public NEException {
    public:

        AddressNotFoundException(const Address128& address) :
            NEException("AddressNotFoundException : The Address " + address.toString() + " does not exist!") {
        }

        AddressNotFoundException(const Address64& address) :
            NEException("AddressNotFoundException : The Address " + address.toString() + " does not exist!") {
        }

        AddressNotFoundException(const Address32& address) :
            NEException("AddressNotFoundException : The Address " + Address::toString(address) + " does not exist!") {
        }
};

typedef std::set<Address16> Address16Set;
typedef std::set<Address32> Address32Set;
typedef std::list<Uint32> Uint32List;
typedef std::list<Address16> Address16List;
typedef std::set<Uint16> Uint16Set;

typedef unsigned short GraphID16; // graphId
typedef std::set<GraphID16> GraphsSet;
typedef std::list<GraphID16> GraphsList;


}
}
#endif /*ISA100ADDRESS_H_*/

