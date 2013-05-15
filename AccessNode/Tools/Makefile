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



TOP:=../
PROJ_TOP:=.

-include $(TOP)/sys_inc.mk

SINGLE_FILE_PROGRAMS=atomic_act exec_sh_tout ini_editor ini_update  

TARGET.PROGRAMS= log2flash  load_versa_node
TARGET.SUBDIRS=$(SHRD_DIR)
TARGET.ALL=udp_listen

ifeq ($(hw),vr900)
    TARGET.SUBDIRS += watchdog
    TARGET.PROGRAMS+= backupdb
endif

TARGET.PROGRAMS += $(SINGLE_FILE_PROGRAMS)

define BUILD_SINGLE_FILE
$(1).FILES=$(1)
$(1).LDFLAGS=$$(SHRD_LIB_LINK)
#$(1).SUBDIRS=$$(SHRD_DIR)
endef
$(foreach prog,$(SINGLE_FILE_PROGRAMS),$(eval $(call BUILD_SINGLE_FILE,$(prog))) )

log_maint.FILES=log_archive_maintainer
#log_maint.SUBDIRS=$(SHRD_DIR)
log_maint.LDFLAGS=$(SHRD_LIB_LINK)

udp_listen.FILES=udp_listen
#udp_listen.SUBDIRS=$(SHRD_DIR)
udp_listen.LDFLAGS=$(SHRD_LIB_LINK)

log2flash.FILES=log2flashapp
#log2flash.SUBDIRS=$(SHRD_DIR)
log2flash.CXXFLAGS=-DSTANDALONE
log2flash.LDFLAGS=$(SHRD_LIB_LINK)

smtp_send.FILES=smtp_send $(SHRD_DIR)/SmtpSupport
#smtp_send.SUBDIRS=$(SHRD_DIR)
smtp_send.LDFLAGS=$(SHRD_LIB) $(UDNSLIB)

wifi_ftp.FILES=wifi_ftp WiFiLink WiFiFtpClient
#wifi_ftp.SUBDIRS=$(SHRD_DIR)
wifi_ftp.LDFLAGS=$(SHRD_LIB_LINK)

email_sender.FILES=email_sender
#email_sender.SUBDIRS=$(SHRD_DIR)
email_sender.LDFLAGS=$(SHRD_LIB_LINK)

watchdog.FILES=Watchdog
#watchdog.SUBDIRS=$(SHRD_DIR)
watchdog.CXXFLAGS=-I$(TOP)
watchdog.LDFLAGS=$(SHRD_LIB_LINK)

local_msp_loader.FILES=local_msp_loader bslcomm
#local_msp_loader.SUBDIRS=$(SHRD_DIR)
local_msp_loader.LDFLAGS=$(SHRD_LIB_LINK)

load_versa_node.FILES=load_versa_node VersaNode
#load_versa_node.SUBDIRS=$(SHRD_DIR)
load_versa_node.LDFLAGS=$(SHRD_LIB_LINK)

gprs_prov_info.FILES=gprs_prov_info
gprs_prov_info.LDFLAGS=$(SHRD_LIB_LINK)

backupdb.FILES= backupdb
backupdb.CXXFLAGS=-I$(AUX_LIBS_DIR_INC)/sqlite
backupdb.LDFLAGS=-lsqlite3

include $(TOP)/system.mk
