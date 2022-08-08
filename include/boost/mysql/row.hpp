//
// Copyright (c) 2019-2022 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_ROW_HPP
#define BOOST_MYSQL_ROW_HPP

#include <boost/mysql/detail/auxiliar/bytestring.hpp>
#include <boost/mysql/detail/auxiliar/container_equals.hpp>
#include <boost/mysql/field_view.hpp>
#include <algorithm>
#include <boost/utility/string_view_fwd.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <vector>

namespace boost {
namespace mysql {

/**
 * \brief Represents a row returned from a database operation.
 * \details A row is a collection of values, plus a buffer holding memory
 * for the string [reflink value]s.
 *
 * Call [refmem row values] to get the actual sequence of
 * [reflink value]s the row contains.
 *
 * There will be the same number of values and in the same order as fields
 * in the SQL query that produced the row. You can get more information
 * about these fields using [refmem resultset fields].
 *
 * If any of the values is a string, it will be represented as a `string_view`
 * pointing into the row's buffer. These string values will be valid as long as
 * the [reflink row] object containing the memory they point to is alive and valid. Concretely:
 * - Destroying the row object invalidates the string values.
 * - Move assigning against the row invalidates the string values.
 * - Calling [refmem row clear] invalidates the string values.
 * - Move-constructing a [reflink row] from the current row does **not**
 *   invalidate the string values.
 *
 * Default constructible and movable, but not copyable.
 */
class row
{
    std::vector<field_view> fields_;
    std::vector<char> string_buffer_;
public:
    row() = default;
    row(const row&) = delete; // TODO
    row(row&&) = default;
    row& operator=(const row&) = delete; // TODO
    row& operator=(row&&) = default;
    ~row() = default;

    using iterator = const field_view*;
    using const_iterator = iterator;
    // TODO: add other standard container members when we add field and field_view

    iterator begin() const noexcept { return fields_.data(); }
    iterator end() const noexcept { return fields_.data() + fields_.size(); }
    field_view at(std::size_t i) const { return fields_.at(i); }
    field_view operator[](std::size_t i) const noexcept { return fields_[i]; }
    field_view front() const noexcept { return fields_.front(); }
    field_view back() const noexcept { return fields_.back(); }
    bool empty() const noexcept { return fields_.empty(); }
    std::size_t size() const noexcept { return fields_.size(); }

    iterator insert(iterator before, field_view v);
    iterator insert(iterator before, std::initializer_list<field_view> v);
    template <class FwdIt>
    iterator insert(iterator before, FwdIt first, FwdIt last);

    iterator replace(iterator pos, field_view v);
    iterator replace(iterator first, iterator last, std::initializer_list<field_view> v);
    template <class FwdIt>
    iterator replace(iterator first, iterator last, FwdIt other_first, FwdIt other_last);

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(field_view v);
    void pop_back();



    /**
     * \brief Clears the row object.
     * \details Clears the value array and the memory buffer associated to this row.
     * After calling this operation, [refmem row values] will be the empty array. Any
     * pointers, references and iterators to elements in [refmem row values] will be invalidated.
     * Any string values using the memory held by this row will also become invalid. 
     */
    void clear() noexcept
    {
        fields_.clear();
        string_buffer_.clear();
    }

    // TODO: hide these
    const std::vector<field_view>& fields() const noexcept { return fields_; }
    std::vector<field_view>& values() noexcept { return fields_; }
    void copy_strings()
    {
        // Calculate size
        std::size_t size = 0;
        for (const auto& f: fields_)
        {
            const boost::string_view* str = f.if_string();
            if (str)
            {
                size += str->size();
            }
        }

        // Make space
        string_buffer_.resize(size);

        // Copy the strings
        std::size_t offset = 0;
        for (auto& f: fields_)
        {
            const boost::string_view* str = f.if_string();
            if (str)
            {
                std::memcpy(string_buffer_.data() + offset, str->data(), str->size());
                f = field_view(boost::string_view(string_buffer_.data() + offset, str->size()));
                offset += str->size();
            }
        }
    }
};


class row_view
{
    const field_view* fields_ {};
    std::size_t size_ {};
public:
    row_view() = default;
    row_view(const field_view* f, std::size_t size) noexcept : fields_ {f}, size_{size} {}
    row_view(const row& r) noexcept :
        fields_(r.begin()),
        size_(r.size())
    {
    }

    using iterator = const field_view*;
    using const_iterator = iterator;

    iterator begin() const noexcept { return fields_; }
    iterator end() const noexcept { return fields_ + size_; }
    field_view at(std::size_t i) const;
    field_view operator[](std::size_t i) const noexcept { return fields_[i]; }
    field_view front() const noexcept { return fields_[0]; }
    field_view back() const noexcept { return fields_[size_ - 1]; }
    bool empty() const noexcept { return size_ == 0; }
    std::size_t size() const noexcept { return size_; }
};



class rows
{
    std::vector<field_view> fields_;
    std::vector<char> string_buffer_;
    std::size_t num_columns_ {};

    void rebase_strings(const char* old_buffer_base)
    {
        const char* new_buffer_base = string_buffer_.data();
        auto diff = new_buffer_base - old_buffer_base;
        if (diff)
        {
            for (auto& f: fields_)
            {
                const boost::string_view* str = f.if_string();
                if (str)
                {
                    f = field_view(boost::string_view(
                        str->data() + diff,
                        str->size()
                    ));
                }
            }
        }
    }

public:
    rows() = default;
    rows(std::size_t num_columns) noexcept : num_columns_(num_columns) {}
    rows(const rows&) = delete; // TODO
    rows(rows&&) = default;
    const rows& operator=(const rows&) = delete; // TODO
    rows& operator=(rows&&) = default;

    class iterator;
    using const_iterator = iterator;
    // TODO: add other standard container members

    iterator begin() const noexcept { return iterator(this, 0); }
    iterator end() const noexcept { return iterator(this, size()); }
    row_view at(std::size_t i) const { /* TODO: check idx */ return (*this)[i]; }
    row_view operator[](std::size_t i) const noexcept
    {
        std::size_t offset = num_columns_ * i;
        return row_view(fields_.data() + offset, num_columns_);
    }
    row_view front() const noexcept { return (*this)[0]; }
    row_view back() const noexcept { return (*this)[size() - 1]; }
    bool empty() const noexcept { return fields_.empty(); }
    std::size_t size() const noexcept { return fields_.size() / num_columns_; }

    // TODO: hide these
    std::vector<field_view>& fields() noexcept { return fields_; }
    void copy_strings(std::size_t field_offset)
    {
        // Calculate the extra size required for the new strings
        std::size_t size = 0;
        for (const auto* f = fields_.data() + field_offset; f != fields_.data() + fields_.size(); ++f)
        {
            const boost::string_view* str = f->if_string();
            if (str)
            {
                size += str->size();
            }
        }

        // Make space and rebase the old strings
        const char* old_buffer_base = string_buffer_.data();
        std::size_t old_buffer_size = string_buffer_.size();
        string_buffer_.resize(size);
        rebase_strings(old_buffer_base);
        
        // Copy the strings
        std::size_t offset = old_buffer_size;
        for (auto& f: fields_)
        {
            const boost::string_view* str = f.if_string();
            if (str)
            {
                std::memcpy(string_buffer_.data() + offset, str->data(), str->size());
                f = field_view(boost::string_view(string_buffer_.data() + offset, str->size()));
                offset += str->size();
            }
        }
    }
    void clear() noexcept
    {
        fields_.clear();
        string_buffer_.clear();
    }



    class iterator
    {
        const rows* obj_;
        std::size_t row_num_;
    public:
        using value_type = row_view; // TODO: should this be row?
        using reference = row_view;
        using pointer = void const*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        // TODO: this can be made random access

        iterator(const rows* obj, std::size_t rownum) noexcept : obj_(obj), row_num_(rownum) {}

        iterator& operator++() noexcept { ++row_num_; return *this; }
        iterator operator++(int) noexcept { return iterator(obj_, row_num_ + 1); }
        iterator& operator--() noexcept { --row_num_; return *this; }
        iterator operator--(int) noexcept { return iterator(obj_, row_num_ - 1); }
        reference operator*() const noexcept { return (*obj_)[row_num_]; }
        bool operator==(const iterator& rhs) const noexcept { return obj_ == rhs.obj_&& row_num_ == rhs.row_num_; }
        bool operator!=(const iterator& rhs) const noexcept { return !(*this == rhs); }
    };
};


class rows_view
{
    const field_view* fields_ {};
    std::size_t num_values_ {};
    std::size_t num_columns_ {};
public:
    rows_view() = default;
    rows_view(const field_view* fields, std::size_t num_values, std::size_t num_columns) noexcept :
        fields_(fields),
        num_values_(num_values),
        num_columns_(num_columns)
    {
    }

    class iterator;
    using const_iterator = iterator;
    // TODO: add other standard container members

    iterator begin() const noexcept { return iterator(this, 0); }
    iterator end() const noexcept { return iterator(this, size()); }
    row_view at(std::size_t i) const { /* TODO: check idx */ return (*this)[i]; }
    row_view operator[](std::size_t i) const noexcept
    {
        std::size_t offset = num_columns_ * i;
        return row_view(fields_ + offset, num_columns_);
    }
    row_view front() const noexcept { return (*this)[0]; }
    row_view back() const noexcept { return (*this)[size() - 1]; }
    bool empty() const noexcept { return num_values_ == 0; }
    std::size_t size() const noexcept { return num_values_ / num_columns_; }

    class iterator
    {
        const rows_view* obj_;
        std::size_t row_num_;
    public:
        using value_type = row_view; // TODO: should this be row?
        using reference = row_view;
        using pointer = void const*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        // TODO: this can be made random access

        iterator(const rows_view* obj, std::size_t rownum) noexcept : obj_(obj), row_num_(rownum) {}

        iterator& operator++() noexcept { ++row_num_; return *this; }
        iterator operator++(int) noexcept { return iterator(obj_, row_num_ + 1); }
        iterator& operator--() noexcept { --row_num_; return *this; }
        iterator operator--(int) noexcept { return iterator(obj_, row_num_ - 1); }
        reference operator*() const noexcept { return (*obj_)[row_num_]; }
        bool operator==(const iterator& rhs) const noexcept { return obj_ == rhs.obj_&& row_num_ == rhs.row_num_; }
        bool operator!=(const iterator& rhs) const noexcept { return !(*this == rhs); }
    };
};


/**
 * \relates row
 * \brief Compares two rows.
 */
inline bool operator==(const row& lhs, const row& rhs)
{
    return detail::container_equals(lhs.fields(), rhs.fields());
}

/**
 * \relates row
 * \brief Compares two rows.
 */
inline bool operator!=(const row& lhs, const row& rhs) { return !(lhs == rhs); }

/**
 * \relates row
 * \brief Streams a row.
 */
inline std::ostream& operator<<(std::ostream& os, const row& value)
{
    os << '{';
    const auto& arr = value.fields();
    if (!arr.empty())
    {
        os << arr[0];
        for (auto it = std::next(arr.begin()); it != arr.end(); ++it)
        {
            os << ", " << *it;
        }
    }
    return os << '}';
}

} // mysql
} // boost



#endif
