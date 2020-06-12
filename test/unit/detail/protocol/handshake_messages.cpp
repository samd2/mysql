//
// Copyright (c) 2019-2020 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "serialization_test_common.hpp"
#include "boost/mysql/detail/protocol/handshake_messages.hpp"

using namespace boost::mysql::test;
using namespace boost::mysql::detail;
using boost::mysql::collation;

namespace
{

constexpr std::uint8_t handshake_auth_plugin_data [] = {
    0x52, 0x1a, 0x50, 0x3a, 0x4b, 0x12, 0x70, 0x2f,
    0x03, 0x5a, 0x74, 0x05, 0x28, 0x2b, 0x7f, 0x21,
    0x43, 0x4a, 0x21, 0x62
};

constexpr std::uint32_t hanshake_caps =
        CLIENT_LONG_PASSWORD |
        CLIENT_FOUND_ROWS |
        CLIENT_LONG_FLAG |
        CLIENT_CONNECT_WITH_DB |
        CLIENT_NO_SCHEMA |
        CLIENT_COMPRESS |
        CLIENT_ODBC |
        CLIENT_LOCAL_FILES |
        CLIENT_IGNORE_SPACE |
        CLIENT_PROTOCOL_41 |
        CLIENT_INTERACTIVE |
        CLIENT_IGNORE_SIGPIPE |
        CLIENT_TRANSACTIONS |
        CLIENT_RESERVED | // old flag, but set in this frame
        CLIENT_SECURE_CONNECTION | // old flag, but set in this frame
        CLIENT_MULTI_STATEMENTS |
        CLIENT_MULTI_RESULTS |
        CLIENT_PS_MULTI_RESULTS |
        CLIENT_PLUGIN_AUTH |
        CLIENT_CONNECT_ATTRS |
        CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA |
        CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS |
        CLIENT_SESSION_TRACK |
        CLIENT_DEPRECATE_EOF |
        CLIENT_REMEMBER_OPTIONS;

INSTANTIATE_TEST_SUITE_P(Handshake, DeserializeSpaceTest, ::testing::Values(
    serialization_testcase(handshake_packet{
        string_null("5.7.27-0ubuntu0.19.04.1"), // server version
        2, // connection ID
        handshake_packet::auth_buffer_type(makesv(handshake_auth_plugin_data)),
        hanshake_caps,
        static_cast<std::uint8_t>(collation::latin1_swedish_ci),
        static_cast<std::uint16_t>(SERVER_STATUS_AUTOCOMMIT),
        string_null("mysql_native_password")
    }, {
      0x35, 0x2e, 0x37, 0x2e, 0x32, 0x37, 0x2d, 0x30,
      0x75, 0x62, 0x75, 0x6e, 0x74, 0x75, 0x30, 0x2e,
      0x31, 0x39, 0x2e, 0x30, 0x34, 0x2e, 0x31, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x52, 0x1a, 0x50, 0x3a,
      0x4b, 0x12, 0x70, 0x2f, 0x00, 0xff, 0xf7, 0x08,
      0x02, 0x00, 0xff, 0x81, 0x15, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
      0x5a, 0x74, 0x05, 0x28, 0x2b, 0x7f, 0x21, 0x43,
      0x4a, 0x21, 0x62, 0x00, 0x6d, 0x79, 0x73, 0x71,
      0x6c, 0x5f, 0x6e, 0x61, 0x74, 0x69, 0x76, 0x65,
      0x5f, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72,
      0x64, 0x00
    }, "regular")
), test_name_generator);

constexpr std::uint8_t handshake_response_auth_data [] = {
    0xfe, 0xc6, 0x2c, 0x9f, 0xab, 0x43, 0x69, 0x46,
    0xc5, 0x51, 0x35, 0xa5, 0xff, 0xdb, 0x3f, 0x48,
    0xe6, 0xfc, 0x34, 0xc9
};

constexpr std::uint32_t handshake_response_caps =
        CLIENT_LONG_PASSWORD |
        CLIENT_LONG_FLAG |
        CLIENT_LOCAL_FILES |
        CLIENT_PROTOCOL_41 |
        CLIENT_INTERACTIVE |
        CLIENT_TRANSACTIONS |
        CLIENT_SECURE_CONNECTION |
        CLIENT_MULTI_STATEMENTS |
        CLIENT_MULTI_RESULTS |
        CLIENT_PS_MULTI_RESULTS |
        CLIENT_PLUGIN_AUTH |
        CLIENT_CONNECT_ATTRS |
        CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA |
        CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS |
        CLIENT_SESSION_TRACK |
        CLIENT_DEPRECATE_EOF;

INSTANTIATE_TEST_SUITE_P(HandshakeResponse, SerializeTest, ::testing::Values(
    serialization_testcase(handshake_response_packet{
        handshake_response_caps,
        16777216, // max packet size
        static_cast<std::uint8_t>(collation::utf8_general_ci),
        string_null("root"),
        string_lenenc(makesv(handshake_response_auth_data)),
        string_null(""), // Irrelevant, not using connect with DB
        string_null("mysql_native_password") // auth plugin name
    }, {
        0x85, 0xa6, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x72, 0x6f, 0x6f, 0x74, 0x00, 0x14, 0xfe, 0xc6,
        0x2c, 0x9f, 0xab, 0x43, 0x69, 0x46, 0xc5, 0x51,
        0x35, 0xa5, 0xff, 0xdb, 0x3f, 0x48, 0xe6, 0xfc,
        0x34, 0xc9, 0x6d, 0x79, 0x73, 0x71, 0x6c, 0x5f,
        0x6e, 0x61, 0x74, 0x69, 0x76, 0x65, 0x5f, 0x70,
        0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64, 0x00
    }, "without_database", handshake_response_caps),

    serialization_testcase(handshake_response_packet{
        handshake_response_caps | CLIENT_CONNECT_WITH_DB,
        16777216, // max packet size
        static_cast<std::uint8_t>(collation::utf8_general_ci),
        string_null("root"),
        string_lenenc(makesv(handshake_response_auth_data)),
        string_null("database"), // database name
        string_null("mysql_native_password") // auth plugin name
    }, {
        0x8d, 0xa6, 0xff, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x72, 0x6f, 0x6f, 0x74, 0x00, 0x14, 0xfe, 0xc6,
        0x2c, 0x9f, 0xab, 0x43, 0x69, 0x46, 0xc5, 0x51,
        0x35, 0xa5, 0xff, 0xdb, 0x3f, 0x48, 0xe6, 0xfc,
        0x34, 0xc9, 0x64, 0x61, 0x74, 0x61, 0x62, 0x61,
        0x73, 0x65, 0x00, 0x6d, 0x79, 0x73, 0x71, 0x6c,
        0x5f, 0x6e, 0x61, 0x74, 0x69, 0x76, 0x65, 0x5f,
        0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64,
        0x00
    }, "with_database", handshake_response_caps | CLIENT_CONNECT_WITH_DB)
), test_name_generator);

constexpr std::uint8_t auth_switch_request_auth_data [] = {
    0x49, 0x49, 0x7e, 0x51, 0x5d, 0x1f, 0x19, 0x6a,
    0x0f, 0x5a, 0x63, 0x15, 0x3e, 0x28, 0x31, 0x3e,
    0x3c, 0x79, 0x09, 0x7c
};

INSTANTIATE_TEST_SUITE_P(AuthSwitchRequest, DeserializeTest, testing::Values(
    serialization_testcase(auth_switch_request_packet{
        string_null("mysql_native_password"),
        string_eof(makesv(auth_switch_request_auth_data))
    }, {
        0x6d, 0x79, 0x73,
        0x71, 0x6c, 0x5f, 0x6e, 0x61, 0x74, 0x69, 0x76,
        0x65, 0x5f, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f,
        0x72, 0x64, 0x00, 0x49, 0x49, 0x7e, 0x51, 0x5d,
        0x1f, 0x19, 0x6a, 0x0f, 0x5a, 0x63, 0x15, 0x3e,
        0x28, 0x31, 0x3e, 0x3c, 0x79, 0x09, 0x7c, 0x00
    }, "regular")
), test_name_generator);

constexpr std::uint8_t auth_switch_response_auth_data [] = {
    0xba, 0x55, 0x9c, 0xc5, 0x9c, 0xbf, 0xca, 0x06,
    0x91, 0xff, 0xaa, 0x72, 0x59, 0xfc, 0x53, 0xdf,
    0x88, 0x2d, 0xf9, 0xcf
};

INSTANTIATE_TEST_SUITE_P(AuthSwitchResponse, SerializeTest, testing::Values(
    serialization_testcase(auth_switch_response_packet{
        string_eof(makesv(auth_switch_response_auth_data))
    }, {
        0xba, 0x55, 0x9c, 0xc5, 0x9c, 0xbf, 0xca, 0x06,
        0x91, 0xff, 0xaa, 0x72, 0x59, 0xfc, 0x53, 0xdf,
        0x88, 0x2d, 0xf9, 0xcf
    }, "regular")
), test_name_generator);

constexpr std::uint32_t ssl_request_caps =
        CLIENT_LONG_FLAG |
        CLIENT_LOCAL_FILES |
        CLIENT_PROTOCOL_41 |
        CLIENT_INTERACTIVE |
        CLIENT_SSL |
        CLIENT_TRANSACTIONS |
        CLIENT_SECURE_CONNECTION |
        CLIENT_MULTI_STATEMENTS |
        CLIENT_MULTI_RESULTS |
        CLIENT_PS_MULTI_RESULTS |
        CLIENT_PLUGIN_AUTH |
        CLIENT_CONNECT_ATTRS |
        CLIENT_SESSION_TRACK |
        (1UL << 29);

INSTANTIATE_TEST_SUITE_P(SslRequest, SerializeTest, ::testing::Values(
    serialization_testcase(ssl_request{
        ssl_request_caps,
        0x1000000,
        45,
        string_fixed<23>{}
    }, {

        0x84, 0xae, 0x9f, 0x20, 0x00, 0x00, 0x00, 0x01,
        0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, "default")
), test_name_generator);

INSTANTIATE_TEST_SUITE_P(AuthMoreData, DeserializeTest, ::testing::Values(
    serialization_testcase(auth_more_data_packet{
        string_eof("abc")
    }, {
        0x61, 0x62, 0x63
    }, "default")
), test_name_generator);


} // anon namespace
