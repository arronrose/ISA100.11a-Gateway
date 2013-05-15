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



# the next rules: clean, exe_copy exe_strip $(SHRD_LIB)  are in ../modules_shrd.mk
# the next rules: dep  are in ../common.mk
# It's a MUST to exist var EXE with proper value
# you can exlude modules from TARGET.MODULES like this:
# make EXCLUDE='SysManager Gateway' will compile only the BackBone module
TOP=..
-include $(TOP)/sys_inc.mk

TARGET.SUBDIRS :=  ISA100 Gateway BackBone Monitor_Host

# fp_translator

export ENCASING:=-DUSE_ENCASE_TTY

include $(TOP)/system.mk


