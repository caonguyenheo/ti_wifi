#include "unity_fixture.h"

static void RunAllTests(void)
{
    /* ota cinatic */
    RUN_TEST_GROUP(ParseVersionResponse);
    RUN_TEST_GROUP(convert_version);
}

int main(int argc, const char * argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}