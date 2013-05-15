#include <cutex/cutex.h>
#include <nlib/log.h>

#include "util/serialization/TestSerialization.h"
#include "hostapp/serialization/TestGServiceUnserializer.h"
#include "hostapp/model/TestCommand.h"
#include "hostapp/model/TestDevice.h"


CUTEX_DEFINE_SUITE_BEGIN(Monitor_Host)
	CUTEX_SUITE(Device)
	CUTEX_SUITE(Command)
	CUTEX_SUITE(Serialization)
	CUTEX_SUITE(GServiceUnserializer)	
CUTEX_DEFINE_SUITE_END(Monitor_Host)


#include <iostream>
#include <queue>
#include <string>

struct _Suite
{
	_Suite(const TestSuite* suite_, const std::string& path_)
		: suite(suite_), path(path_)
		{
		}
	const TestSuite* suite;
	const std::string path;
};

inline int RunTests(const TestSuite* rootSuite)
{
	cute::suite mainSuite;

	std::queue<_Suite> pendingSuites;
	pendingSuites.push(_Suite(rootSuite, rootSuite->suiteName));

	while (!pendingSuites.empty())
	{
		_Suite suite = pendingSuites.front();
		pendingSuites.pop();

		for (int i = 0; i < suite.suite->testsCount; i++)
		{
			Test& test = suite.suite->tests[i];
			if (test.isSuite)
			{
				TestSuite* testSuite =  ((TestSuiteFunction)test.fnTest)();
				pendingSuites.push(_Suite(testSuite, suite.path + "/" + testSuite->suiteName));
			}
			else
			{
				mainSuite += cute::test((TestFunction)test.fnTest, suite.path + "/" + test.testName);
			}
		}
	}

	return cute::runner<cute::my_listener>()(mainSuite, "main")	? 0: 1;
}


int main(int argc, char* argv[])
{
	LOG_INIT_UNITTEST();

	return RunTests(CUTEX_GET_SUITE(Monitor_Host));
}
