#include "unity_fixture.h"

TEST_GROUP_RUNNER(wima)
{
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldFailWithNonJSONMessage)
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldFailWithoutIdField)
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldFailWithoutCodeField)
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldFailBodyIsnotObject)
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldFailWithoutIPFieldinBody)
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldFailWithoutPortFieldinBody)
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldParseRightMessage);
    RUN_TEST_CASE(wima_response_parse, wima_response_parse_ShouldParseRightMessageDifferentBodyOrder);
    
    RUN_TEST_CASE(wima_request_serialize, wima_request_serialize_CouldReturnFailWhenMsgIsTooSmall);
    RUN_TEST_CASE(wima_request_serialize, wima_request_serialize_CouldReturnFailWhenRequestDontHaveUri);
    RUN_TEST_CASE(wima_request_serialize, wima_request_serialize_CouldGenerateRightMessage);
    
    RUN_TEST_CASE(wima_decode, wima_decode_CouldReturnRightDecodeIpAddress);
    RUN_TEST_CASE(wima_decode, wima_decode_CouldReturnRightDecodePortAddress);
}