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
 * @author george.petrehus, radu.pop, catalin.pop
 */
#include "SignalsManager.h"
#include <iostream>
#include <signal.h>

namespace run_lib {

SignalsManager::SigInfo SignalsManager::m_pSigVector[MAX_SIG_NUM];

SignalsManager::SignalsManager() {
}

SignalsManager::~SignalsManager() {
}

int SignalsManager::Install(int nSigNum, sighandler_t p_pSigHandler) {
    sighandler_t oldHandler = ::signal(nSigNum, GenericHandler);

    if (oldHandler == SIG_ERR) {
        return 0;
    }
    m_pSigVector[nSigNum].m_nStatus = INSTALLED;
    m_pSigVector[nSigNum].m_nRaised = 0;
    m_pSigVector[nSigNum].m_pSigHandler = p_pSigHandler;
    m_pSigVector[nSigNum].m_pOldSigHandler = oldHandler;

    return 1;
}

bool SignalsManager::IsInstalled(int nSigNum, sighandler_t p_pSigHandler) {
    return m_pSigVector[nSigNum].m_nStatus == INSTALLED && (p_pSigHandler == 0
                || m_pSigVector[nSigNum].m_pOldSigHandler == p_pSigHandler);
}

int SignalsManager::Ignore(int nSigNum) {
    if (::signal(nSigNum, SIG_IGN) == SIG_ERR) {
        return 0;
    }
    m_pSigVector[nSigNum].m_nStatus = IGNORED;
    m_pSigVector[nSigNum].m_nRaised = 0;
    m_pSigVector[nSigNum].m_pSigHandler = 0;

    return 1;
}

int SignalsManager::Uninstall(int nSigNum) {
    if (::signal(nSigNum, SIG_DFL) == SIG_ERR) {
        return 0;
    }
    m_pSigVector[nSigNum].m_nStatus = DEFAULT;
    m_pSigVector[nSigNum].m_nRaised = 0;
    m_pSigVector[nSigNum].m_pSigHandler = 0;

    return 1;
}

void SignalsManager::GenericHandler(int nSigNum) {
    Install(nSigNum, m_pSigVector[nSigNum].m_pSigHandler);// reinstall the handler

    if (m_pSigVector[nSigNum].m_pSigHandler) {
        try {
            m_pSigVector[nSigNum].m_pSigHandler(nSigNum);
        } catch (...) {
            std::cout << "Exception on handler for signal:" << nSigNum;
        }
    }
    Raise(nSigNum);
}

}
