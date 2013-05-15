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




#default version for RMP projects
b_no?=""
SW_VERSION=2.8.3.390$(b_no)


#version for isa project
ifeq "$(release)" "isa"
	SW_VERSION=2.5.4$(b_no)
#SW_VERSION=2.1.4$(b_no)
endif

#version for whart project
ifeq "$(release)" "whart"
  SW_VERSION=1.0.13$(b_no)
endif

export VERSION="\"$(SW_VERSION)\""

export hw?="any"
DIST_VERSION=$(SW_VERSION)_$(hw)

ifdef release
	DIST_VERSION:=$(DIST_VERSION)_$(release)
endif

