#include "TestGServiceUnserializer.h"

#include <util/serialization/Serialization.h>
#include <hostapp/serialization/GServiceUnserializer.h>

#include <string>
#include <sstream>
#include <iostream>

void RethrownException()
{
	try
	{
		THROW_EXCEPTION0(nisa100::util::NotEnoughBytesException);
	}
	catch (nlib::Exception& ex)
	{
		THROW_EXCEPTION1(nisa100::hostapp::SerializationException, ex.Message());
	}
}
void Test_RethrownException()
{
	ASSERT_THROWS(RethrownException(), nisa100::hostapp::SerializationException);
}


void FailedToDeserializeGetFwVersion()
{
	nisa100::hostapp::GServiceUnserializer unserializer;

	nisa100::gateway::GeneralPacket packet;
	packet.version = 0;
	packet.serviceType = (nisa100::gateway::GeneralPacket::GServiceTypes) 0x88;
	packet.trackingID = 1;
	uint8_t data[] = { 0, 0, 0 };
	packet.data = std::basic_string<boost::uint8_t>(data, sizeof(data));
	packet.dataSize = packet.data.size();

	nisa100::hostapp::AbstractGServicePtr response(
		new nisa100::hostapp::GClientServer<nisa100::hostapp::GetFirmwareVersion>);

	unserializer.Unserialize(*response, packet);
}
void Test_FailedToDeserializeGetFwVersion()
{
	ASSERT_THROWS(FailedToDeserializeGetFwVersion(), nisa100::hostapp::SerializationException);
}

CUTEX_DEFINE_SUITE_BEGIN(GServiceUnserializer)

//CUTEX_TEST(RethrownException)

CUTEX_TEST(FailedToDeserializeGetFwVersion)

CUTEX_DEFINE_SUITE_END(GServiceUnserializer)
