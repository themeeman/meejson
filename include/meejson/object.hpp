#ifndef OBJECT_JSON_HPP
#define OBJECT_JSON_HPP

#include "box.hpp"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <ranges>

namespace mee::json {

namespace detail {

template <class T>
struct ptr_wrapper {
    explicit ptr_wrapper(const T& val) : m_ptr(val) {}
    auto operator*() noexcept -> T& {
        return m_ptr;
    }

    auto operator*() const noexcept -> const T& {
        return m_ptr;
    }

    auto operator->() noexcept -> T* {
        return std::addressof(m_ptr);
    }

    auto operator->() const noexcept -> const T* {
        return std::addressof(m_ptr);
    }

private:
    T m_ptr;
};

template <class Value>
struct key_value_ref {
    key_value_ref(std::pair<const std::string, detail::box<Value>>* p) : m_pair(p) {}

    [[nodiscard]] auto first() const noexcept -> const std::string& {
        return m_pair->first;
    }

    auto second() const noexcept -> Value& {
        return *m_pair->second;
    }

    template<std::size_t I>
    decltype(auto) get() const noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    auto operator&() const noexcept -> ptr_wrapper<key_value_ref> {
        return ptr_wrapper(*this);
    }

    operator std::pair<const std::string, Value>() const noexcept {
        return {m_pair->first, *m_pair->second};
    }

private:
    std::pair<const std::string, detail::box<Value>>* m_pair;
};

template <class Value>
struct const_key_value_ref {
    const_key_value_ref(const std::pair<const std::string, detail::box<Value>>* p) : m_pair(p) {}

    [[nodiscard]] auto first() const noexcept -> const std::string& {
        return m_pair->first;
    }

    [[nodiscard]] auto second() const noexcept -> const Value& {
        return *m_pair->second;
    }

    template<std::size_t I>
    const auto& get() const noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    auto operator&() const noexcept -> ptr_wrapper<const_key_value_ref> {
        return ptr_wrapper(*this);
    }

    operator std::pair<const std::string, Value>() const noexcept {
        return {m_pair->first, *m_pair->second};
    }
private:
    const std::pair<const std::string, detail::box<Value>>* m_pair;
};

template <bool IsConst, class Value>
struct object_iterator {
    using base_type = typename std::unordered_map<std::string, box<Value>>::iterator;
    using const_base_type = typename std::unordered_map<std::string, box<Value>>::const_iterator;
    using difference_type = std::conditional_t<IsConst, typename const_base_type::difference_type, typename base_type::difference_type>;
    using value_type = std::pair<const std::string, Value>;
    using reference = std::conditional_t<IsConst, const_key_value_ref<Value>, key_value_ref<Value>>;
    using pointer = detail::ptr_wrapper<reference>;
    using iterator_category = std::forward_iterator_tag;

    object_iterator() noexcept = default;
    object_iterator(const object_iterator&) noexcept = default;

    explicit object_iterator(base_type it) noexcept : m_iter(it) {}
    explicit object_iterator(const_base_type it) noexcept requires IsConst : m_iter(it) {}

    template <bool B> requires (IsConst || !B)
    object_iterator(const object_iterator<B, Value>& it) noexcept : m_iter(it.m_iter) {}

    auto operator==(const object_iterator& other) const noexcept -> bool {
        return m_iter == other.m_iter;
    }

    auto operator*() const noexcept -> reference {
        return reference(&*m_iter);
    }

    auto operator->() const noexcept -> pointer {
        return &reference(&*m_iter);
    }

    auto operator++(int) noexcept -> object_iterator {
        return object_iterator<IsConst, Value>(m_iter++);
    }

    auto operator++() noexcept -> object_iterator& {
        ++m_iter;
        return *this;
    }

    auto get_base() const noexcept {
        return m_iter;
    }

private:
    std::conditional_t<IsConst, const_base_type, base_type> m_iter;
};
}

template <class Value>
using object_iterator = detail::object_iterator<false, Value>;

template <class Value>
using const_object_iterator = detail::object_iterator<true, Value>;

template <class Value>
struct basic_object {
    using key_type = std::string;
    using mapped_type = Value;
    using value_type = std::pair<const std::string, Value>;
    using size_type = typename std::unordered_map<std::string, detail::box<Value>>::size_type;
    using difference_type = typename std::unordered_map<std::string, detail::box<Value>>::difference_type;
    using hasher = std::hash<std::string>;
    using key_equal = std::equal_to<std::string>;
    using allocator_type = std::allocator<std::pair<const std::string, detail::box<Value>>>;
    using reference = detail::key_value_ref<Value>;
    using const_reference = detail::const_key_value_ref<Value>;
    using pointer = detail::ptr_wrapper<reference>;
    using const_pointer = detail::ptr_wrapper<const_reference>;
    using iterator = object_iterator<Value>;
    using const_iterator = const_object_iterator<Value>;

    static_assert(std::input_iterator<iterator>);
    static_assert(std::input_iterator<const_iterator>);

    basic_object() noexcept = default;
    basic_object(const basic_object& other) : basic_object(other.begin(), other.end()) {}
    basic_object(basic_object&&) noexcept = default;
    explicit basic_object(size_type n) : m_obj(n) {}
    basic_object(std::initializer_list<value_type> list) : basic_object(list.begin(), list.end()) {}

    template <class Iter>
    basic_object(Iter first, Iter last) requires std::input_iterator<Iter> {
        for (; first != last; first++) {
            const auto& [k, v] = *first;
            m_obj.try_emplace(k, detail::make_box<Value>(v));
        }
    }

    auto operator=(const basic_object& other) -> basic_object& {
        clear();
        for (const auto& [k, v] : other.m_obj) {
            m_obj.try_emplace(k, detail::make_box<Value>(*v));
        }
        return *this;
    }

    auto operator=(std::initializer_list<value_type> list) -> basic_object& {
        clear();
        for (const auto& [k, v] : list) {
            m_obj.try_emplace(k, detail::make_box<Value>(v));
        }
        return *this;
    }

    auto get_allocator() const noexcept -> allocator_type {
        return allocator_type();
    }

    auto begin() noexcept -> iterator {
        return iterator(m_obj.begin());
    }

    auto begin() const noexcept -> const_iterator {
        return const_iterator(m_obj.begin());
    }

    auto cbegin() const noexcept -> const_iterator {
        return const_iterator(m_obj.cbegin());
    }

    auto end() noexcept -> iterator {
        return iterator(m_obj.end());
    }

    auto end() const noexcept -> const_iterator {
        return const_iterator(m_obj.end());
    }

    auto cend() const noexcept -> const_iterator {
        return const_iterator(m_obj.cend());
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return m_obj.empty();
    }

    auto size() const noexcept -> size_type {
        return m_obj.size();
    }

    auto max_size() const noexcept -> size_type {
        return m_obj.max_size();
    }

    void clear() noexcept {
        m_obj.clear();
    }

    auto insert(const value_type& v) -> std::pair<iterator, bool> {
        auto p = m_obj.insert(std::pair(v.first, detail::make_box<Value>(v.second)));
        return std::pair(iterator(p.first), p.second);
    }

    auto insert(value_type&& v) -> std::pair<iterator, bool> {
        auto p = m_obj.insert(std::pair(std::move(v.first), detail::make_box<Value>(std::move(v.second))));
        return std::pair(iterator(p.first), p.second);
    }

    auto erase(const_iterator it) -> iterator {
        return iterator(m_obj.erase(it.get_base()));
    }

    auto erase(const key_type& k) -> bool {
        return m_obj.erase(k);
    }

    void swap(basic_object& o) noexcept {
        m_obj.swap(o.m_obj);
    }

    void merge(basic_object& o) {
        m_obj.merge(o.m_obj);
    }

    void merge(basic_object&& o) {
        m_obj.merge(std::move(o.m_obj));
    }

    auto at(const key_type& k) -> Value& {
        return *m_obj.at(k);
    }

    auto at(const key_type& k) const -> const Value& {
        return *m_obj.at(k);
    }

    auto operator[](const key_type& k) -> Value& {
        auto& val = m_obj[k];
        if (!bool(val))
            val = detail::make_box<Value>();
        return *val;
    }

    auto operator[](key_type&& k) -> Value& {
        auto& val = m_obj[std::move(k)];
        if (!bool(val))
            val = detail::make_box<Value>();
        return *val;
    }

    auto find(const key_type& k) -> iterator {
        return iterator(m_obj.find(k));
    }

    auto find(const key_type& k) const -> const_iterator {
        return const_iterator(m_obj.find(k));
    }

    [[nodiscard]] auto contains(const key_type& k) const -> bool {
        return m_obj.contains(k);
    }

    template <class P> requires std::is_constructible_v<value_type, P&&>
    auto insert(P&& value) -> std::pair<iterator, bool> {
        auto p = std::pair(std::forward<P>(value));
        auto p2 = m_obj.insert(std::pair(p.first, detail::make_box<Value>(p.second)));
        return std::pair(iterator(p2.first), p2.second);
    }

    template <class M> requires std::is_constructible_v<mapped_type, M&&>
    auto insert_or_assign(const key_type& k, M&& obj) -> std::pair<iterator, bool> {
        auto p = m_obj.insert_or_assign(k, detail::make_box<Value>(std::forward<M>(obj)));
        return std::pair(iterator(p.first), p.second);
    }

    template <class M> requires std::is_constructible_v<mapped_type, M&&>
    auto insert_or_assign(key_type&& k, M&& obj) -> std::pair<iterator, bool> {
        auto p = m_obj.insert_or_assign(std::move(k), detail::make_box<Value>(std::forward<M>(obj)));
        return std::pair(iterator(p.first), p.second);
    }

    template <class... Args>
    auto emplace(const key_type& k, Args&&... args) -> std::pair<iterator, bool> {
        auto p = m_obj.try_emplace(k, detail::make_box<Value>(std::forward<Args>(args)...));
        return std::pair(iterator(p.first), p.second);
    }

    template <class... Args>
    auto emplace(key_type&& k, Args&&... args) -> std::pair<iterator, bool> {
        auto p = m_obj.try_emplace(std::move(k), detail::make_box<Value>(std::forward<Args>(args)...));
        return std::pair(iterator(p.first), p.second);
    }

    template <class V>
    friend auto operator<<(std::ostream&, const basic_object<V>&) noexcept -> std::ostream&;

    auto operator==(const basic_object& other) const noexcept -> bool {
        if (size() != other.size()) {
            return false;
        }
        return std::all_of(begin(), end(), [&other](auto ref) {
            const auto& [key, val] = ref;
            auto o = other.find(key);
            return o != other.end() && val == o->second();
        });
    }
private:
    std::unordered_map<std::string, detail::box<Value>> m_obj;
};

template <class Value>
auto operator<<(std::ostream& os, const basic_object<Value>& obj) noexcept -> std::ostream& {
    os << '{';
    auto iter = obj.begin();
    if (iter != obj.end()) {
        os << "\"" << iter->first() << "\"" << ':' << iter->second();
        iter++;
        for (; iter != obj.end(); iter++) {
            const auto& [key, val] = *iter;
            os << ',' << "\"" << key << "\"" << ':' << val;
        }
    }
    os << '}';
    return os;
}

}

namespace std {
    template <class Value> struct tuple_size<mee::json::detail::key_value_ref<Value>> : std::integral_constant<size_t, 2> { };

    template <class Value> struct tuple_element<0, mee::json::detail::key_value_ref<Value>> { using type = std::string; };
    template <class Value> struct tuple_element<1, mee::json::detail::key_value_ref<Value>> { using type = Value; };

    template <class Value> struct tuple_size<mee::json::detail::const_key_value_ref<Value>> : std::integral_constant<size_t, 2> { };

    template <class Value> struct tuple_element<0, mee::json::detail::const_key_value_ref<Value>> { using type = std::string; };
    template <class Value> struct tuple_element<1, mee::json::detail::const_key_value_ref<Value>> { using type = Value; };
}

#endif