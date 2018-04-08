#include "whatismyaddress.h"

#include "unity.h"
#include "unity_fixture.h"

#include <string.h>

TEST_GROUP(wima_response_parse);

TEST_SETUP(wima_response_parse)
{
}

TEST_TEAR_DOWN(wima_response_parse)
{
}

TEST(wima_response_parse, wima_response_parse_ShouldFailWithNonJSONMessage)
{
    char msg[] = "{\"id\":123,\"code\":200,\"body\":{\"ip\"";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        -1,
        wima_response_parse(msg, strlen(msg), &response)
    );
}

TEST(wima_response_parse, wima_response_parse_ShouldFailWithoutIdField)
{
    char msg[] = "{\"code\":200,\"body\":{\"ip\":\"192.168.1.1\",\"port\":12345}}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        -1,
        wima_response_parse(msg, strlen(msg), &response)
    );
}

TEST(wima_response_parse, wima_response_parse_ShouldFailWithoutCodeField)
{
    char msg[] = "{\"id\":123,\"body\":{\"ip\":\"192.168.1.1\",\"port\":12345}}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        -1,
        wima_response_parse(msg, strlen(msg), &response)
    );
}

TEST(wima_response_parse, wima_response_parse_ShouldFailBodyIsnotObject)
{
    char msg[] = "{\"id\":123,\"code\":200,\"body\":\"body\"}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        -1,
        wima_response_parse(msg, strlen(msg), &response)
    );
}

TEST(wima_response_parse, wima_response_parse_ShouldFailWithoutIPFieldinBody)
{
    char msg[] = "{\"id\":123,\"code\":200,\"body\":{\"port\":12345}}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        -1,
        wima_response_parse(msg, strlen(msg), &response)
    );
}

TEST(wima_response_parse, wima_response_parse_ShouldFailWithoutPortFieldinBody)
{
    char msg[] = "{\"id\":123,\"code\":200,\"body\":{\"ip\":\"192.168.1.1\"}}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        -1,
        wima_response_parse(msg, strlen(msg), &response)
    );
}

TEST(wima_response_parse, wima_response_parse_ShouldParseRightMessage)
{
    char msg[] = "{\"id\":2,\"code\":200,\"body\":{\"ip\":\"213.138.188.234\",\"port\":20634}}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        0,
        wima_response_parse(msg, strlen(msg), &response)
    );
    TEST_ASSERT_EQUAL(response.id, 2);
    TEST_ASSERT_EQUAL(response.code, 200);
    // we are in network endian
    uint32_t ip = 213 + 138*256 + 188*256*256 + (uint32_t) 234*256*256*256;
    TEST_ASSERT_EQUAL_MEMORY(&response.ip, &ip, sizeof(ip));
    TEST_ASSERT_EQUAL_UINT32(response.port, 20634);
}

TEST(wima_response_parse, wima_response_parse_ShouldParseRightMessageDifferentBodyOrder)
{
    char msg[] = "{\"id\":123,\"code\":200,\"body\":{\"port\":12345,\"ip\":\"192.168.1.1\"}}";
    wima_response_t response;
    TEST_ASSERT_EQUAL(
        0,
        wima_response_parse(msg, strlen(msg), &response)
    );
    TEST_ASSERT_EQUAL(response.id, 123);
    TEST_ASSERT_EQUAL(response.code, 200);
    // we are in network endian
    uint32_t ip = 192 + 168*256 + 1*256*256 + (uint32_t) 1*256*256*256;
    TEST_ASSERT_EQUAL_MEMORY(&response.ip, &ip, sizeof(ip));
    TEST_ASSERT_EQUAL_UINT32(response.port, 12345);
}
