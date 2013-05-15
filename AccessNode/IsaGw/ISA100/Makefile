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

-include $(TOP)/sys_inc.mk

LIBISA_VERSION="\"1.0.0.5"\"

isa100.FILES = aslde aslsrvc Ccm dmap dmap_dmo dmap_utils dmap_armo dmap_dpo nlme porting provision uap_mo \
   rijndael-alg-fst slme tlde tlme callbacks $(TOP)/Shared/MicroSec $(TOP)/Shared/UdpSocket $(TOP)/Shared/Socket $(TOP)/Shared/UtilsSolo  $(TOP)/Shared/log_callback

isa100.CFLAGS = -DBACKBONE_SUPPORT -DLIBISA_VERSION=$(LIBISA_VERSION) -Wunknown-pragmas -std=c99
isa100.CXXFLAGS= -DDLL_KEY_ID_NOT_AUTHENTICATED -DBACKBONE_SUPPORT -DCALLBACK_LOG -DLIBISA_VERSION=$(LIBISA_VERSION) -I$(TOP) $(BOOST_CXXFLAGS)

TARGET.LIBRARIES=isa100

include $(TOP)/system.mk
