#include "unity_fixture.h"

TEST_GROUP_RUNNER(flash)
{
    RUN_TEST_CASE(flash, flash_ShouldRemoveAnyFile);
    RUN_TEST_CASE(flash, flash_CheckNonExistingFile);
    RUN_TEST_CASE(flash, flash_CreateANewFile);
    RUN_TEST_CASE(flash, flash_CreateSeveralFiles);
    RUN_TEST_CASE(flash, flash_FileExistingAfterCreated);
    RUN_TEST_CASE(flash, flash_AbleToWrite);
    RUN_TEST_CASE(flash, flash_ReadTheRightMessageWrote);
}