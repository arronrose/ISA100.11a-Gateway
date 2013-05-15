/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "VersionFW.h"
#include <vector>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace nisa100 {
namespace hostapp {

static boost::uint8_t null_address[VersionFW::SIZE] =
{ 0, 0, 0};

VersionFW::VersionFW(const boost::uint8_t * version_)
{
	SetAddress(version_ ? version_ : null_address);
}

//the format should by like: MajorVersion.MinorVersion.RevisionNo
VersionFW::VersionFW(const std::string& textVersion_)
{
	typedef std::vector<std::string> StringList;
	StringList splitedFW;
	boost::split(splitedFW, textVersion_, boost::is_any_of(".") );
	if (splitedFW.size() != 3)
		THROW_EXCEPTION1(nlib::ArgumentException, boost::str(boost::format("Invalid FW version string: %1% !") % textVersion_));
	try
	{
		int i = 0;
		for (StringList::const_iterator it = splitedFW.begin(); it != splitedFW.end(); it++)
		{
			boost::uint8_t val;
			std::istringstream verstring(*it);
			verstring >> val;
			version[i] = val;
			//boost::lexical_cast<int>(it->parameterValue);
			i++;
		}
		textFW = textVersion_;
	}
	catch(std::exception& ex )
	{
		THROW_EXCEPTION1(nlib::ArgumentException, boost::str(boost::format("Invalid FW version string: %1% !") % textVersion_));
	}
}

const boost::uint8_t* VersionFW::Version() const
{
	return version;
}
const std::string VersionFW::ToString() const
{
	return textFW;
}

void VersionFW::SetAddress(const boost::uint8_t* const newFW)
{
	textFW.clear();
	std::copy(newFW, newFW + sizeof(version), version);
	try
	{
		for (int i = 0; i < SIZE; i++)
		{
			textFW.append( boost::str( boost::format("%1%") % (int)version[i] ) );
			if (i < SIZE - 1)
			{
				textFW.append(".");
			}
		}
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Unhandled exception=" << ex.what());
		throw;
	}
}

}
}
