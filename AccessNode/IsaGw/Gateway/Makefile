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



export TOP=../..

include $(TOP)/sys_inc.mk

isa_gw.FILES = AlertSvc BulkTransferSvc ClientServerSvc crc32c gw GwMgmtSvc \
		GwApp GwUAP GSAP GSAPList GwUtil InterfaceObjMgr ISAConfig LeaseMgmtSvc \
		MsgTracker PublishSubscribeSvc Service SessionMgmtSvc SystemReportSvc \
		ServerService TimeSvc TcpGSAP SAPStruct YDevList $(SHRD_DIR)/TcpSecureSocket
		#TcpServerTh  $(TOP)/Shared/PThreadWrapper

isa_gw.CXXFLAGS= -I$(TOP) $(BOOST_CXXFLAGS) -I$(AUX_LIBS_DIR_INC)

isa_gw.LDFLAGS =  $(SHRD_LIB_LINK) $(LIBISA_LINK) $(AXTLS_LIB_LINK)
#-lpthread
isa_gw.SUBDIRS = $(SHRD_DIR) $(LIBISA_DIR_SRC)

isa_gw.TESTS   = tests

TARGET.PROGRAMS	:= isa_gw

include $(TOP)/system.mk
