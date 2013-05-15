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




#tunnel-path
TUNNEL_PATH = ./tunnel
GATEWAY_PATH = $(TUNNEL_PATH)/gateway
SERIALIZ_PATH = $(TUNNEL_PATH)/util/serialization
FLOW_PATH = $(TUNNEL_PATH)/flow
SERVICES_PATH = $(TUNNEL_PATH)/services
PROCESSOR_PATH = $(TUNNEL_PATH)/processor
SERIALIZATION_PATH = $(TUNNEL_PATH)/serialization
TASKS_PATH = $(TUNNEL_PATH)/tasks
LOG_PATH = $(TUNNEL_PATH)/log


#tunnel-files
TUNNEL_FILES = $(GATEWAY_PATH)/GeneralPacket $(GATEWAY_PATH)/GChannel $(GATEWAY_PATH)/Crc32c $(GATEWAY_PATH)/AsyncTCPSocket
TUNNEL_FILES += $(SERIALIZ_PATH)/NetworkOrder
TUNNEL_FILES += $(TUNNEL_PATH)/TunnelIO
TUNNEL_FILES += $(FLOW_PATH)/TasksPool $(FLOW_PATH)/Request $(FLOW_PATH)/ServicesFactory $(FLOW_PATH)/RequestProcessor $(FLOW_PATH)/TrackingManager
TUNNEL_FILES += $(SERVICES_PATH)/GSession $(SERVICES_PATH)/GLease $(SERVICES_PATH)/GClientServer_C $(SERVICES_PATH)/GClientServer_S
TUNNEL_FILES += $(PROCESSOR_PATH)/GServiceInProcessor $(PROCESSOR_PATH)/GServiceOutProcessor
TUNNEL_FILES += $(SERIALIZATION_PATH)/GServiceSerializer $(SERIALIZATION_PATH)/GServiceUnserializer
TUNNEL_FILES += $(TASKS_PATH)/SendCSRequest $(TASKS_PATH)/CreateLease $(TASKS_PATH)/OpenSession
TUNNEL_FILES += $(LOG_PATH)/Log


#for debug
#fp_translator.CXXFLAGS += -g


CPP_LIB_TRUNK = $(TOP)/../cpplib/trunk

fp_translator.FILES   += $(TUNNEL_FILES) 
fp_translator.CXXFLAGS += -I$(CPP_LIB_TRUNK)/nlib

fp_translator.CXXFLAGS += -Wno-shadow
