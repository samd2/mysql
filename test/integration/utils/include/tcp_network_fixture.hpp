//
// Copyright (c) 2019-2022 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_TEST_INTEGRATION_UTILS_INCLUDE_TCP_NETWORK_FIXTURE_HPP
#define BOOST_MYSQL_TEST_INTEGRATION_UTILS_INCLUDE_TCP_NETWORK_FIXTURE_HPP

#include <boost/mysql/tcp.hpp>

#include "get_endpoint.hpp"
#include "integration_test_common.hpp"
#include "streams.hpp"

namespace boost {
namespace mysql {
namespace test {

struct tcp_network_fixture : network_fixture_base
{
    boost::mysql::tcp_connection conn;

    tcp_network_fixture() : conn(ctx.get_executor()) {}

    void connect() { conn.connect(get_endpoint<tcp_socket>(), params); }

    void start_transaction()
    {
        resultset result;
        conn.query("START TRANSACTION", result);
    }
};

}  // namespace test
}  // namespace mysql
}  // namespace boost

#endif