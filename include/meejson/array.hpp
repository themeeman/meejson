#ifndef ARRAY_JSON_HPP
#define ARRAY_JSON_HPP

#include <iterator>
#include <memory>
#include <vector>
#include <type_traits>
#include <iostream>
#include <ranges>

#include "box.hpp"

namespace json {

namespace detail {

template <bool IsConst, class Value>
struct array_iterator {
    using base_type = typename std::vector<box<Value>>::iterator;
    using const_base_type = typename std::vector<box<Value>>::const_iterator;
    using difference_type = std::conditional_t<IsConst, typename const_base_type::difference_type, typename base_type::difference_type>;
    using value_type = Value;
    using pointer = std::conditional_t<IsConst, const Value*, Value*>;
    using reference = std::conditional_t<IsConst, const Value&, Value&>;
    using iterator_category = std::random_access_iterator_tag;

    array_iterator() noexcept = default;
    array_iterator(const array_iterator&) noexcept = default;
    explicit array_iterator(base_type it) noexcept : m_iter(it) {}
    explicit array_iterator(const_base_type it) noexcept requires IsConst : m_iter(it) {}

    template <bool B> requires (IsConst || !B)
    array_iterator(const array_iterator<B, Value>& it) noexcept : m_iter(it.m_iter) {}

    auto operator==(const array_iterator& other) const noexcept -> bool = default;

    auto operator<=>(const array_iterator& other) const noexcept -> std::strong_ordering {
        return m_iter <=> other.m_iter;
    }

    auto operator*() const noexcept -> reference {
        return **m_iter;
    }

    auto operator->() const noexcept -> pointer {
        return &**m_iter;
    }

    auto operator[](difference_type n) const noexcept -> reference {
        return *m_iter[n];
    }

    auto operator++() noexcept -> array_iterator& {
        ++m_iter;
        return *this;
    }

    auto operator++(int) noexcept -> array_iterator {
        return array_iterator<IsConst, Value>(m_iter++);
    }

    auto operator--() noexcept -> array_iterator& {
        --m_iter;
        return *this;
    }

    auto operator--(int) noexcept -> array_iterator {
        return array_iterator<IsConst, Value>(m_iter--);
    }

    auto operator+=(difference_type n) noexcept -> array_iterator& {
        m_iter += n;
        return *this;
    }

    auto operator-=(difference_type n) noexcept -> array_iterator& {
        m_iter -= n;
        return *this;
    }

    template <bool B, class V>
    friend auto operator+(array_iterator<B, V>, typename array_iterator<B, V>::difference_type) noexcept -> array_iterator<B, V>;

    template <bool B, class V>
    friend auto operator+(typename array_iterator<B, V>::difference_type, array_iterator<B, V>) noexcept -> array_iterator<B, V>;

    template <bool B, class V>
    friend auto operator-(array_iterator<B, V>, typename array_iterator<B, V>::difference_type) noexcept -> array_iterator<B, V>;

    template <bool B, class V>
    friend auto operator-(array_iterator<B, V>, array_iterator<B, V>) noexcept -> typename array_iterator<B, V>::difference_type;

    [[nodiscard]] auto get_base() const noexcept -> std::conditional_t<IsConst, const_base_type, base_type> {
        return m_iter;
    }

private:
    std::conditional_t<IsConst, const_base_type, base_type> m_iter;
};

template <bool IsConst, class Value>
auto operator+(array_iterator<IsConst, Value> it, typename array_iterator<IsConst, Value>::difference_type n) noexcept -> array_iterator<IsConst, Value> {
    return array_iterator<IsConst, Value>(it.m_iter + n);
}

template <bool IsConst, class Value>
auto operator+(typename array_iterator<IsConst, Value>::difference_type n, array_iterator<IsConst, Value> it) noexcept -> array_iterator<IsConst, Value> {
    return array_iterator<IsConst, Value>(n + it.m_iter);
}

template <bool IsConst, class Value>
auto operator-(array_iterator<IsConst, Value> it, typename array_iterator<IsConst, Value>::difference_type n) noexcept -> array_iterator<IsConst, Value> {
    return array_iterator<IsConst, Value>(it.m_iter - n);
}

template <bool IsConst, class Value>
auto operator-(array_iterator<IsConst, Value> it1, array_iterator<IsConst, Value> it2) noexcept -> typename array_iterator<IsConst, Value>::difference_type {
    return it1.m_iter - it2.m_iter;
}

}

template <class Value>
using array_iterator = detail::array_iterator<false, Value>;

template <class Value>
using const_array_iterator = detail::array_iterator<true, Value>;

template <class Value>
struct basic_array {
    using value_type = Value;
    using allocator_type = std::allocator<Value>;
    using size_type = typename std::vector<Value>::size_type;
    using difference_type = typename std::vector<Value>::difference_type;
    using reference = Value&;
    using const_reference = const Value&;
    using pointer = Value*;
    using const_pointer = const Value*;
    using iterator = array_iterator<Value>;
    using const_iterator = const_array_iterator<Value>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_assert(std::random_access_iterator<iterator>);
    static_assert(std::random_access_iterator<const_iterator>);

    basic_array() = default;
    basic_array(const basic_array& arr) : basic_array(arr.begin(), arr.end()) {}
    basic_array(basic_array&&) noexcept = default;

    template <class Iter> requires std::input_iterator<Iter>
    basic_array(Iter begin, Iter end) {
        for (; begin != end; begin++) {
            push_back(*begin);
        }
    }

    template <class Iter> requires std::random_access_iterator<Iter>
    basic_array(Iter begin, Iter end) {
        reserve(end - begin);
        for (; begin != end; begin++) {
            push_back(*begin);
        }
    }

    basic_array(size_type n, const Value& v) : m_arr(n) {
        for (auto& ptr : m_arr) {
            ptr = detail::make_box<Value>(v);
        }
    }

    basic_array(std::initializer_list<Value> list) : basic_array(list.begin(), list.end()) {}

    auto operator=(const basic_array& arr) -> basic_array& {
        resize(0);
        reserve(arr.size());
        for (const auto& val : arr.m_arr) {
            push_back(*val);
        }
        return *this;
    }

    auto operator=(std::initializer_list<Value> list) -> basic_array& {
        resize(0);
        reserve(list.size());
        for (const auto& val : list) {
            push_back(val);
        }
        return *this;
    }

    auto get_allocator() const -> allocator_type {
        return allocator_type();
    }

    auto at(size_type i) -> reference {
        return *m_arr.at(i);
    }

    auto at(size_type i) const -> const_reference {
        return *m_arr.at(i);
    }

    auto operator[](size_type i) noexcept -> reference {
        return *m_arr[i];
    }

    auto operator[](size_type i) const noexcept -> const_reference {
        return *m_arr[i];
    }

    auto front() noexcept -> reference {
        return *m_arr.front();
    }

    auto front() const noexcept -> const_reference {
        return *m_arr.front();
    }

    auto back() noexcept -> reference {
        return *m_arr.back();
    }

    auto back() const noexcept -> const_reference {
        return *m_arr.back();
    }

    auto begin() noexcept -> iterator {
        return iterator(m_arr.begin());
    }

    auto begin() const noexcept -> const_iterator {
        return const_iterator(m_arr.begin());
    }

    auto cbegin() const noexcept -> const_iterator {
        return const_iterator(m_arr.cbegin());
    }

    auto end() noexcept -> iterator {
        return iterator(m_arr.end());
    }

    auto end() const noexcept -> const_iterator {
        return const_iterator(m_arr.end());
    }

    auto cend() const noexcept -> const_iterator {
        return const_iterator(m_arr.cend());
    }

    auto rbegin() noexcept -> reverse_iterator {
        return reverse_iterator(end());
    }

    auto rbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(end());
    }

    auto crbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(cend());
    }

    auto rend() noexcept -> reverse_iterator {
        return reverse_iterator(begin());
    }

    auto rend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(begin());
    }

    auto crend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(cbegin());
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return m_arr.empty();
    }

    auto size() const noexcept -> size_type {
        return m_arr.size();
    }

    auto max_size() const noexcept -> size_type {
        return m_arr.max_size();
    }

    void reserve(size_type n) {
        m_arr.reserve(n);
    }

    auto capacity() const noexcept -> size_type {
        return m_arr.size();
    }

    void shrink_to_fit() {
        m_arr.shrink_to_fit();
    }

    void clear() noexcept {
        m_arr.clear();
    }

    auto insert(const_iterator it, const Value& v) -> iterator {
        return iterator(m_arr.insert(it.get_base(), detail::make_box<Value>(v)));
    }

    auto insert(const_iterator it, Value&& v) -> iterator {
        return iterator(m_arr.insert(it.get_base(), detail::make_box<Value>(std::move(v))));
    }

    auto erase(const_iterator it) -> iterator {
        return iterator(m_arr.erase(it.get_base()));
    }

    void push_back(const Value& v) {
        m_arr.push_back(detail::make_box<Value>(v));
    }

    void push_back(Value&& v) {
        m_arr.push_back(detail::make_box<Value>(std::move(v)));
    }

    void pop_back() {
        m_arr.pop_back();
    }

    void resize(size_type n) {
        auto old_size = size();
        m_arr.resize(n);
        for (auto i = old_size; i < n; i++) {
            m_arr[i] = detail::make_box<Value>();
        }
    }

    void resize(size_type n, const Value& v) {
        auto old_size = size();
        m_arr.resize(n);
        for (auto i = old_size; i < n; i++) {
            m_arr[i] = detail::make_box<Value>(v);
        }
    }

    template <class T> requires std::is_constructible_v<Value, std::remove_reference_t<T>>
    auto emplace(const_iterator pos, T&& arg) -> iterator {
        return m_arr.emplace(pos, std::forward<T>(arg));
    }

    template <class T> requires std::is_constructible_v<Value, std::remove_reference_t<T>>
    auto emplace_back(T&& arg) -> reference {
        return *m_arr.emplace_back(detail::make_box<Value>(std::forward<T>(arg)));
    }

    template <class V>
    friend void swap(basic_array<V>&, basic_array<V>&) noexcept;

    auto operator==(const basic_array& other) const noexcept -> bool {
        return std::equal(begin(), end(), other.begin(), other.end());
    }

    auto operator<=>(const basic_array& other) const noexcept -> std::partial_ordering {
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
    }

private:
    std::vector<detail::box<Value>> m_arr;
};

template <class V>
void swap(basic_array<V>& lhs, basic_array<V>& rhs) noexcept {
    std::swap_ranges(lhs.begin(), rhs.end(), rhs.begin());
}

template <class V>
auto operator<<(std::ostream& os, const basic_array<V>& arr) noexcept -> std::ostream& {
    os << '[';
    auto iter = arr.begin();
    if (iter != arr.end()) {
        std::cout << *iter;
        iter++;
        for (; iter != arr.end(); iter++) {
            std::cout << ',' << *iter;
        }
    }
    os << ']';
    return os;
}
}

#endif