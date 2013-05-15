#include "TestSerialization.h"

#include <util/serialization/Serialization.h>
#include <util/serialization/NetworkOrder.h>
#include <hostapp/CommandsFactory.h>
#include <hostapp/serialization/GServiceUnserializer.h>

#include <string>
#include <sstream>
#include <iostream>

#include <boost/lexical_cast.hpp> //for converting numeric <-> to string
using namespace nisa100::util;
using namespace nisa100::hostapp;
using namespace nisa100::gateway;

void Test_binary_read_uint8()
{
	const NetworkOrder& network = NetworkOrder::Instance();

	boost::uint8_t data[] = { 255, 0, 1 };
	std::basic_istringstream<boost::uint8_t> stream(std::basic_string<boost::uint8_t>(data, sizeof(data)));

	boost::uint8_t value = binary_read<boost::uint8_t> (stream, network);
	ASSERT_EQUALM("failed to read boost::uint8!", (int) 255, (int) value);

	value = binary_read<boost::uint8_t> (stream, network);
	ASSERT_EQUALM("failed to read boost::uint8!", (int) 0, (int) value);

	value = binary_read<boost::uint8_t> (stream, network);
	ASSERT_EQUALM("failed to read boost::uint8!", (int) 1, (int) value);

	ASSERT_THROWS(binary_read<boost::uint8_t> (stream, network), NotEnoughBytesException);
	ASSERT_THROWS(binary_read<boost::uint8_t> (stream, network), NotEnoughBytesException);
}

void Test_binary_read_bytes()
{
	boost::uint8_t data[] = { 1, 2, 0, 255 };
	std::basic_istringstream<boost::uint8_t> stream(std::basic_string<boost::uint8_t>(data, sizeof(data)));

	boost::uint8_t buff[sizeof(data)];

	binary_read_bytes(stream, buff, 2);
	ASSERT_EQUALM_ARRAY("bytes from stream not ok!", data, buff, 2);

	binary_read_bytes(stream, buff, 2);
	ASSERT_EQUALM_ARRAY("bytes from stream not ok!", data + 2, buff, 2);

	ASSERT_THROWS(binary_read_bytes(stream, buff, 2), NotEnoughBytesException);
	ASSERT_THROWS(binary_read_bytes(stream, buff, 200), NotEnoughBytesException);
}

void Test_binary_read_combination()
{
	const NetworkOrder& network = NetworkOrder::Instance();

	boost::uint8_t data[] = { 0, 0, 1 };
	std::basic_istringstream<boost::uint8_t> stream(std::basic_string<boost::uint8_t>(data, sizeof(data)));

	boost::uint8_t byte0 = binary_read<boost::uint8_t> (stream, network);
	ASSERT_EQUALM("failed to read boost::uint8!", (int) 0, (int) byte0);

	boost::uint8_t byte1 = binary_read<boost::uint8_t> (stream, network);
	ASSERT_EQUALM("failed to read boost::uint8!", (int) 0, (int) byte1);

	boost::uint8_t byte2 = binary_read<boost::uint8_t> (stream, network);
	ASSERT_EQUALM("failed to read boost::uint8!", (int) 1, (int) byte2);

	boost::uint8_t buff[3];
	ASSERT_THROWS(binary_read_bytes(stream, buff, 3), NotEnoughBytesException);
}

CUTEX_DEFINE_SUITE_BEGIN(Serialization)
CUTEX_TEST(binary_read_uint8)
CUTEX_TEST(binary_read_bytes)
CUTEX_TEST(binary_read_combination)

CUTEX_DEFINE_SUITE_END(Serialization)
