//
// Copyright (c) 2019-2023 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_CHARACTER_SET_HPP
#define BOOST_MYSQL_CHARACTER_SET_HPP

#include <boost/mysql/string_view.hpp>

#include <boost/mysql/detail/character_set.hpp>
#include <boost/mysql/detail/config.hpp>

#include <cstddef>

namespace boost {
namespace mysql {

/**
 * \brief (EXPERIMENTAL) Represents a MySQL character set.
 * \details
 * By default, you should always use \ref utf8mb4_charset, unless there is
 * a strong reason not to. This struct allows you to extend this library
 * with character sets that are not supported out of the box.
 */
struct character_set
{
    /**
     * \brief The character set name, as a NULL-terminated string.
     * \details
     * This should match the character set name in MySQL. This is the string
     * you specify when issuing `SET NAMES` statements. You can find available
     * character sets using the `SHOW CHARACTER SET` statement.
     */
    const char* name;

    /**
     * \brief Obtains the given string's first character size.
     * \details
     * Given an input string `s`, this function must return the number of
     * bytes that the first character in `s` spans, or 0 in case of error.
     * `s` is guaranteed to be a non-empty string (`s.size() > 0`).
     * \n
     * In some character sets (like UTF-8), not all byte sequences represent
     * valid characters. If this function finds an invalid byte sequence while
     * trying to interpret the first character, it should return 0 to signal the error.
     * \n
     * This function must not throw exceptions or have side effects.
     * \n
     * \par Function signature
     * The function signature should be: `std::size_t (*next_char)(string_view) noexcept`
     */
    std::size_t (*next_char)(string_view) noexcept;
};

/// (EXPERIMENTAL) The utf8mb4 character set (and the one you should use by default).
constexpr character_set utf8mb4_charset{"utf8mb4", detail::next_char_utf8mb4};

/// (EXPERIMENTAL) The latin1 character set.
constexpr character_set latin1_charset{"latin1", detail::next_char_latin1};

}  // namespace mysql
}  // namespace boost

#ifdef BOOST_MYSQL_HEADER_ONLY
#include <boost/mysql/impl/character_set.ipp>
#endif

#endif
