//
// Copyright (c) 2019-2023 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_DETAIL_CHARACTER_SET_HPP
#define BOOST_MYSQL_DETAIL_CHARACTER_SET_HPP

#include <boost/mysql/string_view.hpp>

#include <boost/mysql/detail/config.hpp>

#include <cstddef>

namespace boost {
namespace mysql {
namespace detail {

inline std::size_t next_char_latin1(string_view) noexcept { return 1; }
BOOST_MYSQL_DECL std::size_t next_char_utf8mb4(string_view input) noexcept;

}  // namespace detail
}  // namespace mysql
}  // namespace boost

#endif
