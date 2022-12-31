//
// Copyright (c) 2019-2022 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

//[example_unix_socket

#include <boost/mysql.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>
#include <tuple>

void print_employee(boost::mysql::row_view employee)
{
    std::cout << "Employee '" << employee.at(0) << " "   // first_name (string)
              << employee.at(1) << "' earns "            // last_name  (string)
              << employee.at(2) << " dollars yearly\n";  // salary     (double)
}

#define ASSERT(expr)                                          \
    if (!(expr))                                              \
    {                                                         \
        std::cerr << "Assertion failed: " #expr << std::endl; \
        exit(1);                                              \
    }

/**
 * UNIX sockets are only available on, er, UNIX systems. Typedefs for
 * UNIX socket-based connections are only available in UNIX systems.
 * Check for BOOST_ASIO_HAS_LOCAL_SOCKETS to know if UNIX socket
 * typedefs are available in your system.
 */
#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void main_impl(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <username> <password> [<socket-path>] [<company-id>]\n";
        exit(1);
    }

    const char* socket_path = argc >= 4 ? argv[3] : "/var/run/mysqld/mysqld.sock";

    // The company_id whose employees we will be listing. This
    // is user-supplied input, and should be treated as untrusted.
    const char* company_id = argc == 5 ? argv[4] : "HGS";

    /**
     * Connection parameters that tell us where and how to connect to the MySQL server.
     * There are two types of parameters:
     *   - UNIX-level connection parameters, identifying the UNIX socket to connect to.
     *   - MySQL level parameters: database credentials and schema to use.
     */
    boost::asio::local::stream_protocol::endpoint ep(socket_path);
    boost::mysql::handshake_params params(
        argv[1],                // username
        argv[2],                // password
        "boost_mysql_examples"  // database to use; leave empty or omit the parameter for no
                                // database
    );

    boost::asio::io_context ctx;
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tls_client);

    // Connection to the MySQL server, over a UNIX socket
    boost::mysql::unix_ssl_connection conn(ctx, ssl_ctx);
    conn.connect(ep, params);  // UNIX socket connect and MySQL handshake

    // We will be using company_id, which is untrusted user input, so we will use a prepared
    // statement.
    boost::mysql::unix_ssl_statement stmt;
    conn.prepare_statement(
        "SELECT first_name, last_name, salary FROM employee WHERE company_id = ?",
        stmt
    );

    // Execute the statement
    boost::mysql::resultset result;
    stmt.execute(std::make_tuple(company_id), result);

    // Print employees
    for (boost::mysql::row_view employee : result.rows())
    {
        print_employee(employee);
    }

    // Notify the MySQL server we want to quit, then close the underlying connection.
    conn.close();
}

#else

void main_impl(int, char**)
{
    std::cout << "Sorry, your system does not support UNIX sockets" << std::endl;
}

#endif

int main(int argc, char** argv)
{
    try
    {
        main_impl(argc, argv);
    }
    catch (const boost::system::system_error& err)
    {
        std::cerr << "Error: " << err.what() << ", error code: " << err.code() << std::endl;
        return 1;
    }
    catch (const std::exception& err)
    {
        std::cerr << "Error: " << err.what() << std::endl;
        return 1;
    }
}

//]
