#include "TestDevice.h"

#include <hostapp/model/Device.h>

using namespace nisa100::hostapp;

void Test_copy_devicechannel()
{
	ChannelValue originalChannel;
	originalChannel.dataType=ChannelValue::cdtUInt16;
	originalChannel.value.uint16=123;
	
	ChannelValue newChannel = originalChannel;
	ASSERT_EQUALM("failed to copy channel object data!", "123", newChannel.ToString());
}

CUTEX_DEFINE_SUITE_BEGIN(Device)

CUTEX_TEST(copy_devicechannel)

CUTEX_DEFINE_SUITE_END(Device)
