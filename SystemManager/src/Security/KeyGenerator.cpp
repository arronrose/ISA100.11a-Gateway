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
 * @author catalin.pop, ion.ticus, andrei.petrut
 */
#include "Security/KeyGenerator.h"
#include <sys/times.h> 

namespace Isa100 {
namespace Security {

KeyGenerator::KeyGenerator() {

    struct tms t;
    srand( (time(NULL) + times(&t)) ^ 0x802A15B4);

    this->nextChallenge = rand();
}

KeyGenerator::~KeyGenerator() {
}

SecurityKey KeyGenerator::generateKey() {
    SecurityKey securityKey;
    struct tms t;
    int clkRand = ( time(NULL) + times(&t) ) ^ 0x1AE2190E; // scramble the key

    *(Uint32*)(securityKey.value+0)  = rand() ^ clkRand;
    *(Uint32*)(securityKey.value+4)  = rand() ^ *(Uint32*)(securityKey.value+0);
    *(Uint32*)(securityKey.value+8)  = rand() ^ *(Uint32*)(securityKey.value+4);
    *(Uint32*)(securityKey.value+12) = rand() ^ *(Uint32*)(securityKey.value+12);

    return securityKey;
}

Uint32 KeyGenerator::getNextChallenge() {

    struct tms t;
    nextChallenge = rand() ^ ( time(NULL) + times(&t) ) ^ 0x9471B543; // scramble the key

    std::string challengeString;
    Type::toString(nextChallenge, challengeString);
    LOG_DEBUG("getNextChallenge() : " << challengeString );
    return nextChallenge;
}

void KeyGenerator::setNextChallenge(Uint32 nextChallenge) {
    std::string challengeString;
    Type::toString(nextChallenge, challengeString);
    LOG_DEBUG("setNextChallenge : " <<  challengeString);
    this->nextChallenge = nextChallenge;
}
}
}
