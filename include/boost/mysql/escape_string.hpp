//
// Copyright (c) 2019-2023 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_ESCAPE_STRING_HPP
#define BOOST_MYSQL_ESCAPE_STRING_HPP

#include <boost/mysql/error_code.hpp>
#include <boost/mysql/string_view.hpp>

#include <boost/mysql/detail/config.hpp>

#include <boost/config.hpp>

#include <string>

namespace boost {
namespace mysql {

struct character_set;

/**
 * \brief (EXPERIMENTAL) Identifies the context which a string is being escaped for.
 */
enum class quoting_context : char
{
    /// The string is surrounded by double quotes.
    double_quote = '"',

    /// The string is surrounded by single quotes.
    single_quote = '\'',

    /// The string is surrounded by backticks.
    backtick = '`',
};

/**
 * \brief (EXPERIMENTAL) Escapes a string, making it safe for query composition.
 * \details
 * Given a string `input`, computes a string with special characters
 * escaped, and places it in `output`. This function is a low-level building
 * block for composing client-side queries with runtime string values without
 * incurring in SQL injection vulnerabilities.
 * \n
 * For instance, to compose a valid query from `SELECT * FROM employee WHERE company = "<runtime_value>"`,
 * where `runtime_value` is an untrusted runtime string, `runtime_value` should be escaped
 * using this function before concatenating strings. Otherwise, a malicious `runtime_value`
 * will be able to run arbitrary SQL statements in your server.
 * \n
 * Escaping rules are different depending on the context a string is
 * being used in. `quot_ctx` identifies where the string will appear in
 * a query. Possible values are: \n
 * \li \ref quoting_context::double_quote : the string is surrounded by
 *     double quotes. For example:
 *     \code SELECT * FROM employee WHERE company = "<runtime_value>" \endcode
 * \li \ref quoting_context::single_quote : the string is surrounded by
 *     single quotes. For example:
 *     \code SELECT * FROM employee WHERE company = '<runtime_value>' \endcode
 * \li \ref quoting_context::backtick : the string is surrounded by
 *     backticks. This may happen when escaping identifiers. For example:
 *     \code SELECT `<runtime_column>` FROM employee \endcode
 * \n
 * By default, MySQL treats backslash characters as escapes in string values
 * (for instance, the string `"\n"` is treated as a newline). This behavior is
 * enabled by default, but can be disabled by enabling the
 * <a
 *    href="https://dev.mysql.com/doc/refman/8.0/en/sql-mode.html#sqlmode_no_backslash_escapes">`NO_BACKSLASH_ESCAPES`</a>
 * SQL mode. When enabled, backslashes no longer have a special meaning, which changes
 * the escaping rules. `backslash_escapes` should be set to `true` if backslashes represent
 * escapes (i.e. `NO_BACKSLASH_ESCAPES` is not enabled), and `false` otherwise.
 * Servers report whether this mode is enabled to clients. \ref any_connection::backslash_escapes
 * can be used to retrieve the value to be used for this parameter.
 * \n
 * MySQL can be configured to treat double-quoted strings as identifiers instead of values.
 * This is enabled by activating the
 * <a href="https://dev.mysql.com/doc/refman/8.0/en/sql-mode.html#sqlmode_ansi_quotes">`ANSI_QUOTES`</a> or
 * <a href="https://dev.mysql.com/doc/refman/8.0/en/sql-mode.html#sqlmode_ansi">`ANSI`</a> SQL modes.
 * Servers don't report whether this mode is enabled to clients.
 * This SQL mode is not directly supported by this function.
 * \n
 * `charset` should identify the connection's character set (as given by the
 * `character_set_client` session variable). The character set is used to iterate
 * over the input string. It must be an ASCII-compatible character set (like \ref utf8mb4_charset).
 * All character sets allowed by `character_set_client` satisfy this requirement.
 * \n
 * \par Exception safety
 * Basic guarantee. Memory allocations may throw.
 *
 * \par Complexity
 * Linear in `input.size()`.
 *
 * \par Errors
 * \ref client_errc::invalid_encoding if `input` contains a string
 * that is not valid according to `charset`.
 */
BOOST_ATTRIBUTE_NODISCARD BOOST_MYSQL_DECL error_code escape_string(
    string_view input,
    const character_set& charset,
    bool backslash_escapes,
    quoting_context quot_ctx,
    std::string& output
);

}  // namespace mysql
}  // namespace boost

#ifdef BOOST_MYSQL_HEADER_ONLY
#include <boost/mysql/impl/escape_string.ipp>
#endif

#endif
