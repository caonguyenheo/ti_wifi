#include "whatismyaddress.h"

#include "unity.h"
#include "unity_fixture.h"

#include <string.h>


TEST_GROUP(wima_request_serialize);

TEST_SETUP(wima_request_serialize)
{
}

TEST_TEAR_DOWN(wima_request_serialize)
{
}

TEST(wima_request_serialize, wima_request_serialize_CouldReturnFailWhenMsgIsTooSmall)
{
    char uri[] = "an uri";
    wima_request_t request = {
        .type = REQUEST_GET,
        .id = 1,
        .uri = uri
    };
    char msg[39];
    // 39 = strlen("{\"request\":\"get\",\"id\":1,\"uri\":\"an uri\"}");
    
    TEST_ASSERT_EQUAL(
        -1,
        wima_request_serialize(&request, msg, sizeof(msg))
    );
}
TEST(wima_request_serialize, wima_request_serialize_CouldReturnFailWhenRequestDontHaveUri)
{
    wima_request_t request = {
        .type = REQUEST_GET,
        .id = 1
    };
    char msg[10];
    
    TEST_ASSERT_EQUAL(
        -1,
        wima_request_serialize(&request, msg, sizeof(msg))
    );
}

TEST(wima_request_serialize, wima_request_serialize_CouldGenerateRightMessage)
{
    char uri[] = "an uri";
    
    wima_request_t request = {
        .type = REQUEST_GET,
        .id = 1,
        .uri = uri
    };
    char msg[40];
    //  = strlen("{\"type\":\"get\",\"id\":1,\"uri\":\"an uri\"}") + strlen("\0");
    char expected_msg[] = "{\"request\":\"get\",\"id\":1,\"uri\":\"an uri\"}";
    
    TEST_ASSERT_EQUAL(
        0,
        wima_request_serialize(&request, msg, sizeof(msg))
    );
    
    TEST_ASSERT_EQUAL_STRING(msg, expected_msg);
}
 