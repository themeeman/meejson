#ifndef JSON_JSON_HPP
#define JSON_JSON_HPP

#include <string>
#include <compare>
#include <variant>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <iostream>
#include <concepts>
#include <iterator>

namespace json {

namespace detail {

template <class, class>
struct in_variant_impl;

template <class T, class... Ts>
struct in_variant_impl<T, std::variant<Ts...>> {
    constexpr static auto value = (std::is_same_v<T, Ts> || ...);
};

template <class T, class V>
concept in_variant = in_variant_impl<T, V>::value;

template <class, class>
struct visitable_impl;

template <class F, class T, class... Ts>
struct visitable_impl<F, std::variant<T, Ts...>> {
    constexpr static auto value = std::is_invocable_v<F, T> 
        && (std::is_invocable_v<F, Ts> && ...) 
        && (std::is_same_v<std::invoke_result_t<F, T>, std::invoke_result_t<F, Ts>> && ...);
};

template <class F, class V>
concept visitable = visitable_impl<F, V>::value;
}

template <class T>
struct optional_ref {
    constexpr optional_ref(std::nullptr_t = nullptr) : m_ptr(nullptr) {}
    constexpr optional_ref(T* t) : m_ptr(t) {}

    constexpr explicit operator bool() const noexcept {
        return bool(m_ptr);
    }

    constexpr auto operator*() const noexcept -> T& {
        return *m_ptr;
    }

    constexpr auto operator->() const noexcept -> T* {
        return m_ptr;
    }
private:
    T* m_ptr;
};

struct null {
    constexpr auto operator<=>(const null&) const noexcept -> std::strong_ordering = default;
};

struct value;

struct array_iterator {
    using difference_type = std::iterator_traits<std::vector<std::unique_ptr<value>>::iterator>::difference_type;
    using value_type = value;
    using pointer = value*;
    using reference = value&;
    using iterator_category = std::iterator_traits<std::vector<std::unique_ptr<value>>::iterator>::iterator_category;

    explicit array_iterator(std::vector<std::unique_ptr<value>>::iterator it);

    auto operator<=>(const array_iterator&) const noexcept -> std::strong_ordering = default;

    auto operator*() noexcept -> value&;
    auto operator*() const noexcept -> const value&;
    auto operator->() noexcept -> value*;
    auto operator->() const noexcept -> const value*;
    auto operator[](difference_type) noexcept -> value&;
    auto operator[](difference_type) const noexcept -> const value&;
    auto operator++() noexcept -> array_iterator&;
    auto operator++(int) noexcept -> array_iterator;
    auto operator--() noexcept -> array_iterator&;
    auto operator--(int) noexcept -> array_iterator;

    auto operator+=(difference_type) noexcept -> array_iterator&;
    auto operator-=(difference_type) noexcept -> array_iterator&;

    friend auto operator+(array_iterator, difference_type) noexcept -> array_iterator;
    friend auto operator+(difference_type, array_iterator) noexcept -> array_iterator;
    friend auto operator-(array_iterator, difference_type) noexcept -> array_iterator;
    friend auto operator-(array_iterator, array_iterator) noexcept -> difference_type;
private:
    std::vector<std::unique_ptr<value>>::iterator m_iter;
};

auto operator+(array_iterator, array_iterator::difference_type) noexcept -> array_iterator;
auto operator+(array_iterator::difference_type, array_iterator) noexcept -> array_iterator;
auto operator-(array_iterator, array_iterator::difference_type) noexcept -> array_iterator;
auto operator-(array_iterator, array_iterator) noexcept -> array_iterator::difference_type;

struct array {
    using value_type = value;
    using allocator_type = std::allocator<value>;
    using size_type = std::vector<value>::size_type;
    using difference_type = std::vector<value>::difference_type;
    using reference = value&;
    using const_reference = const value&;
    using pointer = value*;
    using const_pointer = const value*;
    using iterator = array_iterator;
    using const_iterator = const array_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    
    array() = default;
    array(const array&);
    array(array&&) = default;
    explicit array(size_type);
    array(size_type, const value& v);

    template <class Iter>
    array(Iter begin, Iter end) {
        reserve(end - begin);
        for (; begin != end; begin++) {
            push_back(*begin);
        }
    }
    
    array(std::initializer_list<value>);

    auto operator=(const array&) -> array&;
    auto operator=(array&&) -> array& = default;
    auto operator=(std::initializer_list<value>) -> array&;

    auto get_allocator() const -> allocator_type;
    auto at(size_type) -> reference;
    auto at(size_type) const -> const_reference;
    auto operator[](size_type) noexcept -> reference;
    auto operator[](size_type) const noexcept -> const_reference;
    auto front() noexcept -> reference;
    auto front() const noexcept -> const_reference;
    auto back() noexcept -> reference;
    auto back() const noexcept -> const_reference;

    auto begin() noexcept -> iterator;
    auto begin() const noexcept -> const_iterator;
    auto cbegin() const noexcept -> const_iterator;
    auto end() noexcept -> iterator;
    auto end() const noexcept -> const_iterator;
    auto cend() const noexcept -> const_iterator;

    auto rbegin() noexcept -> reverse_iterator;
    auto rbegin() const noexcept -> const_reverse_iterator;
    auto crbegin() const noexcept -> const_reverse_iterator;
    auto rend() noexcept -> reverse_iterator;
    auto rend() const noexcept -> const_reverse_iterator;
    auto crend() const noexcept -> const_reverse_iterator;

    [[nodiscard]] auto empty() const noexcept -> bool;
    auto size() const noexcept -> size_type;
    auto max_size() const noexcept -> size_type;
    void reserve(size_type);
    auto capacity() const noexcept -> size_type;
    void shrink_to_fit();

    void clear() noexcept;
    auto insert(const_iterator, const value&) -> iterator;
    auto insert(const_iterator, value&&) -> iterator;

    template <class T> requires std::is_constructible_v<value, std::remove_reference_t<T>>
    auto emplace(const_iterator pos, T&& arg) -> iterator {
        return m_arr.emplace(pos, std::forward<T>(arg));
    }

    auto erase(const_iterator) -> iterator;
    void push_back(const value&);
    void push_back(value&&);

    template <class T> requires std::is_constructible_v<value, std::remove_reference_t<T>>
    auto emplace_back(T&& arg) -> reference {
        return m_arr.emplace_back(std::forward<T>(arg));
    }
private:
    std::vector<std::unique_ptr<value>> m_arr;
};

struct value {
    using array = std::vector<std::unique_ptr<value>>;
    using object = std::unordered_map<std::string, std::unique_ptr<value>>;
    using value_type = std::variant<null, std::int64_t, double, std::string, array, object>;

    constexpr value() noexcept = default;
    constexpr value(const value&);
    value(value&&) noexcept = default;

    template <class T> requires std::is_constructible_v<value_type, std::remove_reference_t<T>>
    constexpr explicit value(T&& t) noexcept : m_val(std::forward<T>(t)) {}

    constexpr auto operator=(const value&) -> value&;
    auto operator=(value&&) noexcept -> value& = default;

    template <class T> requires std::is_assignable_v<value_type, std::remove_reference_t<T>>
    constexpr auto operator=(T&&) noexcept -> value&;

    template <class T> requires detail::in_variant<T, value_type>
    constexpr auto get() -> T& {
        return std::get<T>(m_val);
    }

    template <class T> requires detail::in_variant<T, value_type>
    constexpr auto get() const -> const T& {
        return std::get<T>(m_val);
    }

    template <class T> requires detail::in_variant<T, value_type>
    constexpr auto get_if() noexcept -> optional_ref<T> {
        return holds<T>() ? &get<T>() : nullptr;
    }

    template <class T> requires detail::in_variant<T, value_type>
    constexpr auto get_if() const noexcept -> optional_ref<const T> {
        return holds<T>() ? &get<T>() : nullptr;
    }

    template <class T> requires detail::in_variant<T, value_type>
    constexpr auto holds() const noexcept -> bool {
        return std::holds_alternative<T>(m_val);
    }

    template <class F> requires detail::visitable<F, value_type>
    constexpr friend auto visit(F&&, const value&);

    friend auto operator<<(std::ostream&, const value&) noexcept -> std::ostream&;
private:
    value_type m_val;
};

template <class F> requires detail::visitable<F, value::value_type>
constexpr auto visit(F&& f, const value& v) {
    return std::visit(std::forward<F>(f), v.m_val);
}

auto operator<<(std::ostream&, const value&) noexcept -> std::ostream&;

}

#endif