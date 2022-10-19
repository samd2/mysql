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

void print_employee(boost::mysql::row_view employee)
{
    std::cout << "Employee '" << employee[0] << " "   // first_name (string)
              << employee[1] << "' earns "            // last_name  (string)
              << employee[2] << " dollars yearly\n";  // salary     (double)
}

#define ASSERT(expr)                                          \
    if (!(expr))                                              \
    {                                                         \
        std::cerr << "Assertion failed: " #expr << std::endl; \
        exit(1);                                              \
    }

/* UNIX sockets are only available in, er, UNIX systems. Typedefs for
 * UNIX socket-based connections are only available in UNIX systems.
 * Check for BOOST_ASIO_HAS_LOCAL_SOCKETS to know if UNIX socket
 * typedefs are available in your system.
 */
#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS

void main_impl(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <username> <password> [<socket-path>]\n";
        exit(1);
    }

    const char* socket_path = "/var/run/mysqld/mysqld.sock";
    if (argc == 4)
    {
        socket_path = argv[3];
    }

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

    const char* sql = "SELECT first_name, last_name, salary FROM employee WHERE company_id = 'HGS'";
    boost::mysql::unix_ssl_resultset result;
    conn.query(sql, result);

    // Get all the rows in the resultset
    boost::mysql::rows all_rows;
    result.read_all(all_rows);
    for (const auto& employee : all_rows)
    {
        print_employee(employee);
    }

    // We can issue any SQL statement, not only SELECTs. In this case, the returned
    // resultset will have no fields and no rows
    sql = "UPDATE employee SET salary = 10000 WHERE first_name = 'Underpaid'";
    conn.query(sql, result);
    ASSERT(result.complete());  // UPDATE queries never return rows

    // Check we have updated our poor intern salary
    conn.query("SELECT salary FROM employee WHERE first_name = 'Underpaid'", result);
    result.read_all(all_rows);
    double salary = all_rows.at(0).at(0).as_double();
    ASSERT(salary == 10000);

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
