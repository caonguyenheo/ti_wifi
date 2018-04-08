#include "unity.h"
#include "unity_fixture.h"

#include "hw_types.h"
#include "flash.h"
#include <string.h>

static char file[100];
#define FILE    "mcubootinfo.bin"
#define FILE1   "mcuimg.bin"
#define FILE2   "mcuimg1.bin"
#define FILE3   "mcuimg2.bin"

TEST_GROUP(flash);

TEST_SETUP(flash)
{
    strncpy(file, FILE, strlen(FILE));
}

TEST_TEAR_DOWN(flash)
{
}

TEST(flash, flash_ShouldRemoveAnyFile)
{
    TEST_ASSERT_EQUAL(0, fileRemove(file));
}

TEST(flash, flash_CheckNonExistingFile)
{
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_FALSE(fileIsExist(file));
}

TEST(flash, flash_CreateANewFile)
{
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));
}

TEST(flash, flash_CreateSeveralFiles)
{
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));

    memset(file, 0, sizeof(file));
    strncpy(file, FILE1, strlen(FILE1));
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));

    memset(file, 0, sizeof(file));
    strncpy(file, FILE2, strlen(FILE2));
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));

    memset(file, 0, sizeof(file));
    strncpy(file, FILE3, strlen(FILE3));
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));
}

TEST(flash, flash_FileExistingAfterCreated)
{
    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));
    TEST_ASSERT_TRUE(fileIsExist(file));
}

TEST(flash, flash_AbleToWrite)
{
    char write_msg[] = "hello world";
    TEST_ASSERT_EQUAL(0, fileCreate(file));
    TEST_ASSERT_EQUAL(strlen(write_msg), fileWrite(file, write_msg, strlen(write_msg)));
}

TEST(flash, flash_ReadTheRightMessageWrote)
{
    char write_msg[] = "hello world";
    int32_t write_msg_len = strlen(write_msg);
    char read_msg[100];

    memset(read_msg, 0, sizeof(read_msg));

    TEST_ASSERT_EQUAL(0, fileRemove(file));
    TEST_ASSERT_EQUAL(0, fileCreate(file));
    TEST_ASSERT_EQUAL(write_msg_len, fileWrite(file, write_msg, write_msg_len));
    TEST_ASSERT_EQUAL(write_msg_len, fileRead(file, read_msg, sizeof(read_msg)));
    TEST_ASSERT_EQUAL_STRING_LEN(write_msg, read_msg, write_msg_len);
}
