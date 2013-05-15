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

#ifndef SECURITYLEVEL_H_
#define SECURITYLEVEL_H_

#include <string>
#include <boost/lexical_cast.hpp>
#include "Common/NEException.h"
#include "Common/logging.h"

using namespace Isa100::Common;

namespace Isa100 {
namespace Security {

namespace SecurityLevel {
/**
 * ISA_SP100_Preliminary-Draft_v2007-12-21 - Table 128 – TLDE-DATA.indication primitive parameters
 * Transport layer security processing done on the PDU. Reference the IEEE 802.15.4-2006
 * Security levels for the MAC in section 7.6.2.2.1 Table 95.
 */
enum SecurityLevel {
    NONE = 0x00,
    MIC_32 = 0x01,
    MIC_64 = 0x02,
    MIC_128 = 0x03,
    ENC = 0x04,
    ENC_MIC_32 = 0x05,
    ENC_MIC_64 = 0x06,
    ENC_MIC_128 = 0x07
};
}

struct SecurityLevelEntry {
    /**
     * Security level identifier
     */
    unsigned char id;

    /*
     * Security control	field b2 b1 b0
     */
    std::string controlField;

    /**
     * Security	attributes
     */
    std::string attributes;

    /**
     * Data	confidentiality
     */
    bool confidentiality;

    /**
     * Data authenticity (including length M of	authentication tag, in octets)
     */
    bool authenticity;

    SecurityLevelEntry(unsigned char id_, std::string controlField_, std::string attributes_,
                bool confidentiality_, bool authenticity_) :
                    id(id_), controlField(controlField_), attributes(attributes_), confidentiality(confidentiality_),
                    authenticity(authenticity_) {
    }
};

namespace detail {
class SecurityLevelEntryIdPredicate {
    public:
        SecurityLevelEntryIdPredicate(unsigned char id_) {
            id = id_;
        }
        bool operator()(SecurityLevelEntry& securityLevelEntry) {
            return securityLevelEntry.id == id;
        }
    private:
        unsigned char id;
};
}

/**
 * Represents the exception that is thrown when you try to get an SecurityLevel by
 * an id (of SecurityLevel) and SecurityLevelTable doesn't contain the requested SecurityLevel.
 */
struct SecurityLevelEntryNotFoundException: public NEException {
    public:
        SecurityLevelEntryNotFoundException(unsigned char id) :
            NEException((std::string) "The SecurityLevel with id=" + boost::lexical_cast<std::string>(id)
                        + " doesn't found!") {
        }
};

struct SecurityLevelTable {
    LOG_DEF("I.M.SecurityLevelTable")
    private:
        SecurityLevelTable() {

            SecurityLevelEntry entry0(0x00, "000", "None", false, false);
            securityLevelTable.push_back(entry0);

            SecurityLevelEntry entry1(0x01, "001", "MIC-32", false, true);
            securityLevelTable.push_back(entry1);

            SecurityLevelEntry entry2(0x02, "010", "MIC-64", false, true);
            securityLevelTable.push_back(entry2);

            SecurityLevelEntry entry3(0x03, "011", " MIC-128", false, true);
            securityLevelTable.push_back(entry3);

            SecurityLevelEntry entry4(0x04, "100", "ENC", true, false);
            securityLevelTable.push_back(entry4);

            SecurityLevelEntry entry5(0x05, "101", "ENC-MIC-32", true, true);
            securityLevelTable.push_back(entry5);

            SecurityLevelEntry entry6(0x06, "110", "ENC-MIC-64", true, true);
            securityLevelTable.push_back(entry6);

            SecurityLevelEntry entry7(0x07, "111", "ENC-MIC-128", true, true);
            securityLevelTable.push_back(entry7);
        }
    public:
        static SecurityLevelTable& instance() {
            static SecurityLevelTable instance;
            return instance;
        }

        SecurityLevelEntry getSecurityLevelEntry(unsigned char id) {
            SecurityLevelEntries::iterator it = std::find_if(securityLevelTable.begin(), securityLevelTable.end(),
                        detail::SecurityLevelEntryIdPredicate(id));
            if (it != securityLevelTable.end()) {
                return *it;
            }
            else {
                LOG_ERROR("SecurityLevelEntryNotFoundException id : " << id);
                throw SecurityLevelEntryNotFoundException(id);
            }
        }

        typedef std::vector<SecurityLevelEntry> SecurityLevelEntries;
        SecurityLevelEntries securityLevelTable;
};
}
}

#endif /*SECURITYLEVEL_H_*/
