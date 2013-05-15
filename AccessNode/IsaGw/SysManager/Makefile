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



TOP:=../..
PROJ_TOP:=..

SystemManager.KEEP	= 1
SystemManager.EXE	= SystemManager
SystemManager.SUBDIRS	= $(PROJ_TOP)/SysManager/lib
SystemManager.DEPS	= copy_binary

TARGET.PROGRAMS := SystemManager

include $(TOP)/system.mk
.PHONY:copy_binary
copy_binary:
	cp $(SystemManager.EXE) $(PROJ_TOP)
	if ! test -d $(TOP)/an ; then mkdir -p $(TOP)/an; fi
	cp -f sm_refreshLog.sh $(TOP)/an
	cp -f sm_command.sh $(TOP)/an
