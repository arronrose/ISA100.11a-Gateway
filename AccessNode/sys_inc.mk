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




#for explanations about default.mk look in main Makefile
-include $(TOP)/default.mk
##################################

#hw?=any
dist?=mesh

#####################################################################################
# HW Specific assign default value to host variable
####################################################################################
export RELEASES:= isa whart
export HW:= vr900 pc i386

ifeq "$(hw)" "vr900"
	host?=m68k-unknown-linux-uclibc
endif

ifeq "$(hw)" "pc"
	override  hw:=i386
endif

ifeq "$(hw)" "i386"
	host?=i386
	link?=static
	fs_tree?=rel
endif


#variables which could appear on cmd line
#an - fs tree for AN (ER)
#rel - relative fs tree on i386 or cyg.
fs_tree  ?=an
static_libc ?=0

ifndef link
link     ?=dynamic
#ifneq ($(findstring exe_strip,$(MAKECMDGOALS)),)
#else ifneq ($(findstring exe_copy,$(MAKECMDGOALS)),)
#else ifneq ($(findstring clean_local,$(MAKECMDGOALS)),)
#else
#$(warning *** WARN: $$(link) undefined. Defaulting to $(link).)
#endif
endif


OUTPUT_DIR?=out
AN_DIR      ?=$(abspath $(TOP)/an)
AN_LIB_DIR  ?=$(abspath $(AN_DIR)/lib)
SHRD_DIR    ?=$(abspath $(TOP)/Shared)
OUT_SHRD_DIR?=$(abspath $(TOP)/$(OUTPUT_DIR)/$(host)/Shared)
CONFIG_DIR  ?=$(abspath $(TOP)/config/FW_$(dist)_HW_$(hw)/release_$(release))

EXE_DST_DIR :=$(abspath $(AN_DIR))
ifdef WWW
EXE_DST_DIR :=$(abspath $(AN_DIR)/www/wwwroot)
endif

AUX_LIBS_DIR_BASE	?=$(TOP)/../AuxLibs
AUX_LIBS_DIR_INC	=$(AUX_LIBS_DIR_BASE)/include
AUX_LIBS_DIR_LIB	=$(AUX_LIBS_DIR_BASE)/lib/$(host)

LIBISA_DIR_SRC ?=$(abspath $(TOP)/IsaGw/ISA100)
LIBISA_DIR_OUT ?=$(abspath $(TOP)/$(OUTPUT_DIR)/$(host)/IsaGw/ISA100)
LIBGSAP_DIR_SRC ?=$(abspath $(TOP)/ProtocolTranslators/LibGSAP)
LIBGSAP_DIR_OUT ?=$(abspath $(TOP)/$(OUTPUT_DIR)/$(host)/ProtocolTranslators/LibGSAP)

CPPLIB_DIR	=$(TOP)/../cpplib/trunk
BOOST_INCLUDE_PATH 	=$(CPPLIB_DIR)/boost_1_36_0/
BOOST_LIB_PATH 	=
BOOST_CXXFLAGS = -I$(BOOST_INCLUDE_PATH)

ifeq "$(hw)" "vr900"
	BOOST_CXXFLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif

ifeq "$(host)" "arm"
	BOOST_CXXFLAGS += -DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS 
endif


#variable "link" MUST NOT change after this point
ifeq "$(link)" "static"
	_LIBEXT :=.a
	SHRD_LIB_LINK=$(OUT_SHRD_DIR)/libshared$(_LIBEXT)
	LIBISA_LINK=$(LIBISA_DIR_OUT)/libisa100$(_LIBEXT)
	AXTLS_LIB_LINK=$(AUX_LIBS_DIR_LIB)/libaxtls$(_LIBEXT)
	LIBGSAP_LINK=$(LIBGSAP_DIR_OUT)/libgsap$(_LIBEXT)
else
	_LIBEXT :=.so
	SHRD_LIB_LINK=-L$(OUT_SHRD_DIR) -lshared
	LIBISA_LINK=-L$(LIBISA_DIR_OUT) -lisa100
	AXTLS_LIB_LINK=-L$(AUX_LIBS_DIR_LIB) -laxtls
	LIBGSAP_LINK=-L$(LIBGSAP_DIR_OUT) -lgsap
endif
SHRD_LIB=$(SHRD_LIB_LINK)

HASSQLITE:=vr900_isa;i386_isa;vr900_whart;
HAS_SQLITE:=$(findstring $(hw)_$(release);,$(HASSQLITE))


define GLOB
$(shell find $(1) -iname $(2) | sed -e 's/\.cpp$$\|\.c$$//g;')
endef


#GLOBAL_DEP:=Makefile system.mk sys_inc.mk

export host hw dist fs_tree static_libc
export AN_DIR AN_LIB_DIR SHRD_DIR OUT_SHRD_DIR EXE_DST_DIR OUTPUT_DIR _EXEDIR _OBJDIR LIBISA_DIR_SRC LIBISA_DIR_OUT
export _LIBEXT SHRD_LIB_LINK SHRD_LIB LIBISA_LINK BOOST_INCLUDE_PATH BOOST_LIB_PATH GLOBAL_DEP LIBGSAP_DIR_OUT LIBGSAP_LINK
