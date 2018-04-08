#include "whatismyaddress.h"

#include "unity.h"
#include "unity_fixture.h"

#include <string.h>

uint32_t version_bin;

TEST_GROUP(wima_decode);

TEST_SETUP(wima_decode)
{
}

TEST_TEAR_DOWN(wima_decode)
{
}

TEST(wima_decode, wima_decode_CouldReturnRightDecodeIpAddress)
{
    struct in_addr ip;
    struct in_addr expected_decode_ip;
    struct in_addr decoded_ip;
    
    ip.s_addr = 192 + 168*256 + 1*256*256 + (uint32_t)1*256*256*256;
    expected_decode_ip.s_addr = 63 + 87*256 + 254*256*256 + (uint32_t)254*256*256*256;
    decoded_ip = wima_decode_ip(&ip),
    
    TEST_ASSERT_EQUAL_MEMORY(
        &expected_decode_ip,
        &decoded_ip,
        sizeof(struct in_addr)
    );
}

TEST(wima_decode, wima_decode_CouldReturnRightDecodePortAddress)
{
    uint16_t port = 1234;
    uint16_t expected_decode_port = 64301;
    
    TEST_ASSERT_EQUAL_UINT16(
        expected_decode_port,
        wima_decode_port(&port)
    );
}
