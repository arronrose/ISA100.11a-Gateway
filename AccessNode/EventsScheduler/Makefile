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



export TOP:=..

-include $(TOP)/sys_inc.mk

SINGLE_FILE_PROGRAMS=rule_file_mgr sched_util
TARGET.PROGRAMS=scheduler
TARGET.ALL=

TARGET.PROGRAMS += $(SINGLE_FILE_PROGRAMS)

define BUILD_SINGLE_FILE
$(1).FILES=$(1)
$(1).LDFLAGS=$$(SHRD_LIB_LINK)
$(1).SUBDIRS=$$(SHRD_DIR)
$(1).CXXFLAGS=-I$(TOP)
endef
$(foreach prog,$(SINGLE_FILE_PROGRAMS),$(eval $(call BUILD_SINGLE_FILE,$(prog))) )


scheduler.FILES= AppS 
scheduler.SUBDIRS=$(SHRD_DIR)
scheduler.LDFLAGS=$(SHRD_LIB_LINK)
scheduler.CXXFLAGS=-I$(TOP)

include $(TOP)/system.mk

# end of file
