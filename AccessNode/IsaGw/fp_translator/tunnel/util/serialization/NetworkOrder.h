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

#ifndef NETWORKORDER_H_
#define NETWORKORDER_H_


#include <boost/cstdint.hpp> //used for inttypes

namespace tunnel {
namespace util {

/**
 * @class Transforms integer values from host representation into network order and reverse.
 * To : Host to Network
 * From: Host from Network.
 */
class NetworkOrder
{
public:
	static const NetworkOrder& Instance();

	boost::uint8_t To(const boost::uint8_t& value) const;
	boost::uint8_t From(const boost::uint8_t& value) const;

	boost::int8_t To(const boost::int8_t& value) const;
	boost::int8_t From(const boost::int8_t& value) const;

	boost::uint16_t To(const boost::uint16_t& value) const;
	boost::uint16_t From(const boost::uint16_t& value) const;

	boost::int16_t To(const boost::int16_t& value) const;
	boost::int16_t From(const boost::int16_t& value) const;

	boost::uint32_t To(const boost::uint32_t& value) const;
	boost::uint32_t From(const boost::uint32_t& value) const;

	boost::int32_t To(const boost::int32_t& value) const;
	boost::int32_t From(const boost::int32_t& value) const;

private:
	NetworkOrder()
	{
	}
	NetworkOrder(const NetworkOrder&);
};

}//namespace util
}//namespace nisa100

#endif /*NETWORKORDER_H_*/
