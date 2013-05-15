#
# Copyright (C) 2013 Nivis LLC.
# Email:   opensource@nivis.com
# Website: http://www.nivis.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# Redistribution and use in source and binary forms must retain this
# copyright notice.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#



# Makefile for Shared folder
export TOP=..

-include $(TOP)/sys_inc.mk

shared.FILES = app Common log IdsBooking UtilsSolo \
	Config DevMem ModulesActivity\
        pipe link FileLock \
        IniParser CfgBuffVar PacketStream ProtocolPacket h \
        AlarmEx TcpSocket  Socket UdpSocket \
        ServerSocket PersistentInteger PersistentMsgId HashTable\
        linklog Utils log2flash CmdLineArgs\
	SignalsMgr SimpleStream MicroSec log_callback\
	RuleDefFile EventsStruct Events\
	DSTHandler

shared.TESTS  = TPacketStream
shared.CXXFLAGS= $(BOOST_CXXFLAGS) -I$(BOOST_INCLUDE_PATH) -I$(CPPLIB_DIR)/nlib/ -I$(TOP)

TARGET.LIBRARIES := shared

# so far no use of SqliteUtil on arm
TARGET.SUBDIRS := SqliteUtil

include $(TOP)/system.mk


GLOBAL.LDFLAGS = $(SHRD_LIB_LINK) ../UnitTest/test_fmwk/mockpp/lib/libmockpp.so.36

TPacketStream.FILES=PacketStream
