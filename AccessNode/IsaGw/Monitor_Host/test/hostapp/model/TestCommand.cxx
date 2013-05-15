#include "TestCommand.h"

#include <hostapp/model/Command.h>

#include <string>

using namespace nisa100::hostapp;

void Test_get_App_error_description()
{		
	int responseCode = Command::rsFailure_GatewayInvalidContract;
	std::string errorDescription = FormatResponseCode(responseCode);	
	ASSERT_EQUALM("failed to complete description of code!", "App(-12)-contract expired or invalid", errorDescription);
}

void Test_get_Device_error_description()
{	
	int sfcCode = 23;
	int responseCode = (Command::rsFailure_DeviceError - sfcCode);
	std::string errorDescription = FormatResponseCode(responseCode);	
	ASSERT_EQUALM("failed to complete description of code!", "SFC(23)-problem with sensor detected", errorDescription);
}

void Test_get_ExtendedDevice_error_description()
{	
	int sfcCode = 0xFF01;
	int responseCode = (Command::rsFailure_DeviceError - sfcCode);
	std::string errorDescription = FormatResponseCode(responseCode);	
	ASSERT_EQUALM("failed to complete description of code!", "SFCEx(1)-unknown extended device error", errorDescription);
}


CUTEX_DEFINE_SUITE_BEGIN(Command)
CUTEX_TEST(get_App_error_description)
CUTEX_TEST(get_Device_error_description)
CUTEX_TEST(get_ExtendedDevice_error_description)

CUTEX_DEFINE_SUITE_END(Command)
