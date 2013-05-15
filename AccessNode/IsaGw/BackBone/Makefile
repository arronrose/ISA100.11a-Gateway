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



export TOP:=../..

include $(TOP)/sys_inc.mk

export ENCASING:=-DUSE_ENCASE_TTY

backbone.FILES   = BackBoneApp BackBoneCfg backbone DeFrgmTbl LowPANTunnel  MsgQueue IsaSocket BbrGlobal WaitingQueues $(TOP)/IsaGw/common/TtyLink
backbone.CXXFLAGS= $(ENCASING) -I$(TOP) -I../ -I. $(BOOST_CXXFLAGS)
#-I$(PROJ_TOP)
backbone.LDFLAGS = $(SHRD_LIB_LINK) $(LIBISA_LINK)
#-lpthread
backbone.SUBDIRS = $(SHRD_DIR)  $(LIBISA_DIR_SRC)
backbone.TESTS   := tests/TCcm

#$(PROJ_TOP)/common
#-L $(TOP)/Shared/ -lisa100 $(TOP)/Shared/libcommon.a

TARGET.PROGRAMS =backbone

#TCcm.FILES = $(PROJ_TOP)/common/Ccm $(PROJ_TOP)/common/rijndael-alg-fst

include $(TOP)/system.mk
