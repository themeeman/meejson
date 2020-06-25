#include "array.hpp"

#include <iostream>
#include <algorithm>

json::array::array(const array& arr) : array(arr.begin(), arr.end()) {}
json::array::array(size_type n, const value& v) : m_arr(n) {
    for (auto& ptr : m_arr) {
        ptr = detail::make_box<value>(v);
    }
}

json::array::array(std::initializer_list<value> list) : array(list.begin(), list.end()) {}

auto json::array::operator=(const array& arr) -> array& {
    resize(0);
    reserve(arr.size());
    for (const auto& val : arr.m_arr) {
        push_back(*val);
    }
    return *this;
}

auto json::array::operator=(std::initializer_list<value> list) -> array& {
    resize(0);
    reserve(list.size());
    for (const auto& val : list) {
        push_back(val);
    }
    return *this;
}

auto json::array::get_allocator() const -> allocator_type {
    return allocator_type();
}

auto json::array::at(size_type i) -> reference {
    return *m_arr.at(i);
}

auto json::array::at(size_type i) const -> const_reference {
    return *m_arr.at(i);
}

auto json::array::operator[](size_type i) noexcept -> reference {
    return *m_arr[i];
}

auto json::array::operator[](size_type i) const noexcept -> const_reference {
    return *m_arr[i];
}

auto json::array::front() noexcept -> reference {
    return *m_arr.front();
}

auto json::array::front() const noexcept -> const_reference {
    return *m_arr.front();
}

auto json::array::back() noexcept -> reference {
    return *m_arr.back();
}

auto json::array::back() const noexcept -> const_reference {
    return *m_arr.back();
}

auto json::array::begin() noexcept -> iterator {
    return array_iterator(m_arr.begin());
}

auto json::array::begin() const noexcept -> const_iterator {
    return const_array_iterator(m_arr.begin());
}

auto json::array::cbegin() const noexcept -> const_iterator {
    return const_array_iterator(m_arr.cbegin());
}

auto json::array::end() noexcept -> iterator {
    return array_iterator(m_arr.end());
}

auto json::array::end() const noexcept -> const_iterator {
    return const_array_iterator(m_arr.end());
}

auto json::array::cend() const noexcept -> const_iterator {
    return const_array_iterator(m_arr.cend());
}

auto json::array::rbegin() noexcept -> reverse_iterator {
    return reverse_iterator(end());
}

auto json::array::rbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(end());
}

auto json::array::crbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(cend());
}

auto json::array::rend() noexcept -> reverse_iterator {
    return reverse_iterator(begin());
}

auto json::array::rend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(begin());
}

auto json::array::crend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator(cbegin());
}

[[nodiscard]] auto json::array::empty() const noexcept -> bool {
    return m_arr.empty();
}

auto json::array::size() const noexcept -> size_type {
    return m_arr.size();
}

auto json::array::max_size() const noexcept -> size_type {
    return m_arr.max_size();
}

void json::array::reserve(size_type n) {
    m_arr.reserve(n);
}

auto json::array::capacity() const noexcept -> size_type {
    return m_arr.size();
}

void json::array::shrink_to_fit() {
    m_arr.shrink_to_fit();
}

void json::array::clear() noexcept {
    m_arr.clear();
}

auto json::array::insert(const_iterator it, const value& v) -> iterator {
    return array_iterator(m_arr.insert(it.get_base(), detail::make_box<value>(v)));
}

auto json::array::insert(const_iterator it, value&& v) -> iterator {
    return array_iterator(m_arr.insert(it.get_base(), detail::make_box<value>(std::move(v))));
}

auto json::array::erase(const_iterator it) -> iterator {
    return array_iterator(m_arr.erase(it.get_base()));
}

void json::array::push_back(const value& v) {
    m_arr.push_back(detail::make_box<value>(v));
}

void json::array::push_back(value&& v) {
    m_arr.push_back(detail::make_box<value>(std::move(v)));
}

void json::array::pop_back() {
    m_arr.pop_back();
}

void json::array::resize(size_type n) {
    auto old_size = size();
    m_arr.resize(n);
    for (auto i = old_size; i < n; i++) {
        m_arr[i] = detail::make_box<value>();
    }
}

void json::array::resize(size_type n, const value& v) {
    auto old_size = size();
    m_arr.resize(n);
    for (auto i = old_size; i < n; i++) {
        m_arr[i] = detail::make_box<value>(v);
    }
}

auto json::operator<<(std::ostream& os, const array& arr) noexcept -> std::ostream& {
    os << '[';
    auto it = arr.begin();
    if (it != arr.end()) {
        os << *it;
        it++;
        for (; it != arr.end(); it++) {
            os << ',' << *it;
        }
    }
    os << ']';
    return os;
}
