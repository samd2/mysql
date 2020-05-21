//
// Copyright (c) 2019-2020 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "boost/mysql/mysql.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_future.hpp>
#include <iostream>

using boost::mysql::error_code;
using boost::mysql::error_info;


#ifdef BOOST_ASIO_HAS_CO_AWAIT

/**
 * For this example, we will be using the 'boost_mysql_examples' database.
 * You can get this database by running db_setup.sql.
 * This example assumes you are connecting to a localhost MySQL server.
 *
 * This example uses asynchronous functions with C++20 coroutines
 * (boost::asio::use_awaitable).
 *
 * This example assumes you are already familiar with the basic concepts
 * of mysql-asio (tcp_connection, resultset, rows, values). If you are not,
 * please have a look to the query_sync.cpp example.
 *
 * In this library, all asynchronous operations follow Boost.Asio universal
 * asynchronous models, and thus may be used with callbacks, Boost stackful
 * coroutines, C++20 coroutines or futures.
 * The handler signature is always one of:
 *   - void(error_code): for operations that do not have a "return type" (e.g. handshake)
 *   - void(error_code, T): for operations that have a "return type" (e.g. query, for which
 *     T = resultset<StreamType>).
 *
 * All asynchronous operations accept a last optional error_info* parameter. error_info
 * contains additional diagnostic information returned by the server. If you
 * pass a non-nullptr value, it will be populated in case of error if any extra information
 * is available.
 *
 * Design note: handler signatures in Boost.Asio should have two parameters, at
 * most, and the first one should be an error_code - otherwise some of the asynchronous
 * features (e.g. coroutines) won't work. This is why error_info is not part of any
 * of the handler signatures.
 */

void print_employee(const boost::mysql::row& employee)
{
    std::cout << "Employee '"
              << employee.values()[0] << " "                   // first_name (type std::string_view)
              << employee.values()[1] << "' earns "            // last_name  (type std::string_view)
              << employee.values()[2] << " dollars yearly\n";  // salary     (type double)
}

/**
 * A boost::asio::io_context plus a thread that calls context.run().
 * We encapsulate this here to ensure correct shutdown even in case of
 * error (exception), when we should first reset the work guard, to
 * stop the io_context, and then join the thread. Failing to do so
 * may cause your application to not stop (if the work guard is not
 * reset) or to terminate badly (if the thread is not joined).
 */
class application
{
    boost::asio::io_context ctx_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard_;
    std::thread runner_;
public:
    application(): guard_(ctx_.get_executor()), runner_([this] { ctx_.run(); }) {}
    application(const application&) = delete;
    application(application&&) = delete;
    application& operator=(const application&) = delete;
    application& operator=(application&&) = delete;
    ~application()
    {
        guard_.reset();
        runner_.join();
    }
    boost::asio::io_context& context() { return ctx_; }
};

/**
 * Our coroutine. It must have a return type of boost::asio::awaitable<T>.
 * Our coroutine does not communicate any result back, so T=void.
 * Remember that you do not have to explicitly create any awaitable<void> in
 * your function. Instead, the return type is fed to std::coroutine_traits
 * to determine the semantics of the coroutine, like the promise type.
 * Asio already takes care of all this for us.
 *
 * The coroutine will suspend every time we call one of the asynchronous functions, saving
 * all information it needs for resuming. When the asynchronous operation completes,
 * the coroutine will resume in the point it was left.
 *
 * The return type of an asynchronous operation that uses boost::asio::use_awaitable
 * as completion token is a boost::asio::awaitable<T>, where T
 * is the second argument to the handler signature for the asynchronous operation.
 * For example, connection::query has a handler
 * signature of void(error_code, resultset<Stream>), so async_query will return
 * a boost::asio::awaitable<boost::mysql::resultset<Stream>>. The return type of
 * calling co_await on such a expression would be a boost::mysql::resultset<Stream>.
 * If any of the asyncrhonous operations fail, an exception will be raised
 * within the coroutine.
 */
boost::asio::awaitable<void> start_query(
    boost::asio::executor ex,
    const boost::asio::ip::tcp::endpoint& ep,
    const boost::mysql::connection_params& params
)
{
    boost::mysql::tcp_connection conn (ex);

    // Connect to server
    co_await conn.async_connect(ep, params, boost::asio::use_awaitable);

    // Issue the query to the server. Note that async_query returns a
    // boost::asio::awaitable<boost::mysql::tcp_resultset>
    const char* sql = "SELECT first_name, last_name, salary FROM employee WHERE company_id = 'HGS'";
    boost::mysql::tcp_resultset result =
        co_await conn.async_query(sql, boost::asio::use_awaitable);

    /**
     * Get all rows in the resultset. We will employ resultset::async_fetch_one(),
     * which returns a single row at every call. The returned row is a pointer
     * to memory owned by the resultset, and is re-used for each row. Thus, returned
     * rows remain valid until the next call to async_fetch_one(). When no more
     * rows are available, async_fetch_one returns nullptr.
     */
    while (const boost::mysql::row* row =
        co_await result.async_fetch_one(boost::asio::use_awaitable))
    {
        print_employee(*row);
    }

    // Notify the MySQL server we want to quit, then close the underlying connection.
    co_await conn.async_close(boost::asio::use_awaitable);
}

void main_impl(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <username> <password>\n";
        exit(1);
    }

    // io_context plus runner thread
    application app;

    // Connection parameters
    boost::asio::ip::tcp::endpoint ep (
        boost::asio::ip::address_v4::loopback(), // host
        boost::mysql::default_port                 // port
    );
    boost::mysql::connection_params params (
        argv[1],               // username
        argv[2],               // password
        "boost_mysql_examples" // database to use; leave empty or omit the parameter for no database
    );

    /**
     * The entry point. We spawn a thread of execution to run our
     * coroutine using boost::asio::co_spawn. We pass in a function returning
     * a boost::asio::awaitable<void>, as required.
     *
     * We pass in a callback to co_spawn. It will be called when
     * the coroutine completes, with an exception_ptr if there was any error
     * during execution. We use a promise to wait for the coroutine completion
     * and transmit any raised exception.
     */
    auto executor = app.context().get_executor();
    std::promise<void> prom;
    boost::asio::co_spawn(executor, [executor, ep, params] {
        return start_query(executor, ep, params);
    }, [&prom](std::exception_ptr err) {
        prom.set_exception(std::move(err));
    });
    prom.get_future().get();
}

#else

void main_impl(int, char**)
{
    std::cout << "Sorry, your compiler does not support C++20 coroutines" << std::endl;
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

