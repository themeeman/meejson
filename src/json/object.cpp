#include "object.hpp"
#include <iostream>

json::object::object(size_type n) : m_obj(n) {}

json::object::object(const object& other) : object(other.begin(), other.end()) {}

json::object::object(std::initializer_list<value_type> list) : object(list.begin(), list.end()) {}

auto json::object::operator=(const object& other) -> object& {
    clear();
    for (const auto& [k, v] : other.m_obj) {
        m_obj.try_emplace(k, detail::make_box<value>(*v));
    }
    return *this;
}

auto json::object::operator=(std::initializer_list<value_type> list) -> object& {
    clear();
    for (const auto& [k, v] : list) {
        m_obj.try_emplace(k, detail::make_box<value>(v));
    }
    return *this;
}

auto json::object::get_allocator() const noexcept -> allocator_type {
    return allocator_type();
}

auto json::object::begin() noexcept -> iterator {
    return object_iterator(m_obj.begin());
}

auto json::object::begin() const noexcept -> const_iterator {
    return const_object_iterator(m_obj.begin());
}

auto json::object::cbegin() const noexcept -> const_iterator {
    return const_object_iterator(m_obj.cbegin());
}

auto json::object::end() noexcept -> iterator {
    return object_iterator(m_obj.end());
}

auto json::object::end() const noexcept -> const_iterator {
    return const_object_iterator(m_obj.end());
}

auto json::object::cend() const noexcept -> const_iterator {
    return const_object_iterator(m_obj.cend());
}

[[nodiscard]] auto json::object::empty() const noexcept -> bool {
    return m_obj.empty();
}

auto json::object::size() const noexcept -> size_type {
    return m_obj.size();
}

auto json::object::max_size() const noexcept -> size_type {
    return m_obj.max_size();
}

void json::object::clear() noexcept {
    m_obj.clear();
}

auto json::object::insert(const value_type& v) -> std::pair<iterator, bool> {
    auto p = m_obj.insert(std::pair(v.first, detail::make_box<value>(v.second)));
    return std::pair(object_iterator(p.first), p.second);
}

auto json::object::insert(value_type&& v) -> std::pair<iterator, bool> {
    auto p = m_obj.insert(std::pair(std::move(v.first), detail::make_box<value>(std::move(v.second))));
    return std::pair(object_iterator(p.first), p.second);
}

auto json::object::erase(const_iterator it) -> iterator {
    return object_iterator(m_obj.erase(it.get_base()));
}

auto json::object::erase(const key_type& k) -> bool {
    return m_obj.erase(k);
}

void json::object::swap(object& o) noexcept {
    m_obj.swap(o.m_obj);
}

void json::object::merge(object& o) {
    m_obj.merge(o.m_obj);
}

void json::object::merge(object&& o) {
    m_obj.merge(std::move(o.m_obj));
}

auto json::object::at(const key_type& k) -> value& {
    return *m_obj.at(k);
}

auto json::object::at(const key_type& k) const -> const value& {
    return *m_obj.at(k);
}

auto json::object::operator[](const key_type& k) -> value& {
    auto& val = m_obj[k];
    if (!bool(val))
        val = detail::make_box<value>();
    return *val;
}

auto json::object::operator[](key_type&& k) -> value& {
    auto& val = m_obj[std::move(k)];
    if (!bool(val))
        val = detail::make_box<value>();
    return *val;
}

auto json::object::find(const key_type& k) -> iterator {
    return object_iterator(m_obj.find(k));
}

auto json::object::find(const key_type& k) const -> const_iterator {
    return const_object_iterator(m_obj.find(k));
}

auto json::object::contains(const key_type& k) const -> bool {
    return m_obj.contains(k);
}

auto json::operator<<(std::ostream& os, const object& obj) noexcept -> std::ostream& {
    os << '{';
    auto iter = obj.begin();
    if (iter != obj.end()) {
        std::cout << "\"" << iter->first() << "\"" << ':' << iter->second();
        iter++;
        for (; iter != obj.end(); iter++) {
            const auto& [key, val] = *iter;
            std::cout << ',' << "\"" << key << "\"" << ':' << val;
        }
    }
    os << '}';
    return os;
}