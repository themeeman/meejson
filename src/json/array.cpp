#include "json.hpp"

json::array_iterator::array_iterator(std::vector<std::unique_ptr<value>>::iterator it) : m_iter(it) {}

auto json::array_iterator::operator*() noexcept -> value& {
    return **m_iter;
}

auto json::array_iterator::operator*() const noexcept -> const value& {
    return **m_iter;
}

auto json::array_iterator::operator->() noexcept -> value* {
    return &**m_iter;
}

auto json::array_iterator::operator->() const noexcept -> const value* {
    return &**m_iter;
}

auto json::array_iterator::operator[](difference_type n) noexcept -> value& {
    return *m_iter[n];
}

auto json::array_iterator::operator[](difference_type n) const noexcept -> const value& {
    return *m_iter[n];
}

auto json::array_iterator::operator++() noexcept -> array_iterator& {
    ++m_iter;
    return *this;
}

auto json::array_iterator::operator++(int) noexcept -> array_iterator {
    return array_iterator(m_iter++);
}

auto json::array_iterator::operator--() noexcept -> array_iterator& {
    --m_iter;
    return *this;
}

auto json::array_iterator::operator--(int) noexcept -> array_iterator {
    return array_iterator(m_iter--);
}

auto json::array_iterator::operator+=(difference_type n) noexcept -> array_iterator& {
    m_iter += n;
    return *this;
}

auto json::array_iterator::operator-=(difference_type n) noexcept -> array_iterator& {
    m_iter -= n;
    return *this;
}

auto json::operator+(array_iterator it, array_iterator::difference_type n) noexcept -> array_iterator {
    return array_iterator(it.m_iter + n);
}

auto json::operator+(array_iterator::difference_type n, array_iterator it) noexcept -> array_iterator {
    return array_iterator(n + it.m_iter);
}

auto json::operator-(array_iterator it, array_iterator::difference_type n) noexcept -> array_iterator {
    return array_iterator(it.m_iter - n);
}

auto json::operator-(array_iterator it1, array_iterator it2) noexcept -> array_iterator::difference_type {
    return it1.m_iter - it2.m_iter;
}

json::array::array(const array& arr) : array(arr.begin(), arr.end()) {}
json::array::array(std::initializer_list<value> list) : array(list.begin(), list.end()) {}
