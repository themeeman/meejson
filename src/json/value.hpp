#ifndef JSON_VALUE_HPP
#define JSON_VALUE_HPP

#include <variant>
#include <type_traits>
#include <string>
#include <cstdint>

#include "box.hpp"
#include "array.hpp"
#include "object.hpp"

namespace json {

namespace detail {

template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

template <class... Ts>
struct type_list {};

template <class, class>
struct in_type_list_impl;

template <class T, class... Ts>
struct in_type_list_impl<T, type_list<Ts...>> {
    constexpr static auto value = (std::is_same_v<T, Ts> || ...);
};

template <class T, class V>
concept in_type_list = in_type_list_impl<T, V>::value;

template <class, class>
struct visitable_impl;

template <class F, class T, class... Ts>
struct visitable_impl<F, type_list<T, Ts...>> {
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

auto operator<<(std::ostream& os, null) noexcept -> std::ostream&;

struct array;
struct object;

struct value {
    using types = detail::type_list<null, bool, std::int64_t, double, std::string, array, object>;
    using value_type = std::variant<null, bool, std::int64_t, double, std::string, detail::box<array>, detail::box<object>>;

    constexpr value() noexcept = default;
    constexpr value(const json::value&);
    value(value&&) noexcept = default;

    template <class T> requires (detail::in_type_list<std::remove_cvref_t<T>, types> && !detail::in_type_list<std::remove_cvref_t<T>, detail::type_list<array, object>>)
    constexpr explicit value(T&& t) noexcept : m_val(std::forward<T>(t)) {}

    template <class T> requires (detail::in_type_list<std::remove_cvref_t<T>, detail::type_list<array, object>>)
    constexpr explicit value(T&& t) noexcept : m_val(detail::make_box<std::remove_cvref_t<T>>(std::forward<T>(t))) {}

    template <class Int> requires (!std::is_same_v<Int, int64_t> && !std::is_same_v<Int, bool> && std::is_integral_v<Int>)
    constexpr explicit value(Int i) noexcept : m_val(int64_t(i)) {}

    template <class Fp> requires (!std::is_same_v<Fp, double> && std::is_floating_point_v<Fp>)
    constexpr explicit value(Fp f) noexcept : m_val(double(f)) {}

    template <class S> requires (!std::is_same_v<std::remove_cvref_t<S>, std::string> && std::is_constructible_v<std::string, S>)
    constexpr explicit value(S&& s) noexcept : m_val(std::string(std::forward<S>(s))) {}

    constexpr auto operator=(const value&) -> value&;
    auto operator=(value&&) noexcept -> value& = default;

    template <class T> requires (detail::in_type_list<std::remove_cvref_t<T>, types> && !detail::in_type_list<std::remove_cvref_t<T>, detail::type_list<array, object>>)
    constexpr auto operator=(T&& t) noexcept -> value& {
        m_val = std::forward<T>(t);
        return *this;
    }

    template <class T> requires (detail::in_type_list<std::remove_cvref_t<T>, detail::type_list<array, object>>)
    constexpr auto operator=(T&& t) noexcept -> value& {
        m_val = detail::make_box<std::remove_cvref_t<T>>(std::forward<T>(t));
        return *this;
    }

    template <class Int> requires (!std::is_same_v<Int, int64_t> && !std::is_same_v<Int, bool> && std::is_integral_v<Int>)
    constexpr auto operator=(Int i) noexcept -> value& {
        m_val = int64_t(i);
        return *this;
    }

    template <class Fp> requires (!std::is_same_v<Fp, double> && std::is_floating_point_v<Fp>)
    constexpr auto operator=(Fp f) noexcept -> value& {
        m_val = double(f);
        return *this;
    }

    template <class S> requires (!std::is_same_v<std::remove_cvref_t<S>, std::string> && std::is_constructible_v<std::string, S>)
    constexpr auto operator=(S&& s) noexcept -> value& {
        m_val = std::string(std::forward<S>(s));
        return *this;
    }

    template <class T> requires detail::in_type_list<T, types>
    constexpr auto get() -> T& {
        if constexpr (std::is_same_v<T, array> || std::is_same_v<T, object>) {
            return *std::get<detail::box<T>>(m_val);
        } else {
            return std::get<T>(m_val);
        }
    }

    template <class T> requires detail::in_type_list<T, types>
    constexpr auto get() const -> const T& {
        if constexpr (std::is_same_v<T, array> || std::is_same_v<T, object>) {
            return *std::get<detail::box<T>>(m_val);
        } else {
            return std::get<T>(m_val);
        }
    }

    template <class T> requires detail::in_type_list<T, types>
    constexpr auto get_if() noexcept -> optional_ref<T> {
        return holds<T>() ? get<T>() : nullptr;
    }

    template <class T> requires detail::in_type_list<T, types>
    constexpr auto get_if() const noexcept -> optional_ref<const T> {
        return holds<T>() ? get<T>() : nullptr;
    }

    template <class T> requires detail::in_type_list<T, types>
    constexpr auto holds() const noexcept -> bool {
        if constexpr (std::is_same_v<T, array> || std::is_same_v<T, object>) {
            return std::holds_alternative<detail::box<T>>(m_val);
        } else {
            return std::holds_alternative<T>(m_val);
        }
    }

    template <class F> requires detail::visitable<F, value::types>
    constexpr friend auto visit(F&&, const value&);

    friend auto operator<<(std::ostream&, const value&) noexcept -> std::ostream&;

    auto operator[](std::string_view) -> value&;
    auto operator[](std::string_view) const -> const value&;
    auto operator[](std::size_t) -> value&;
    auto operator[](std::size_t) const -> const value&;

    constexpr friend auto operator+(const value&, const value&) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator+(const value&, Int) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator+(Int, const value&) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator+(const value&, Fp) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator+(Fp, const value&) -> value;

    constexpr friend auto operator-(const value&, const value&) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator-(const value&, Int) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator-(Int, const value&) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator-(const value&, Fp) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator-(Fp, const value&) -> value;

    constexpr friend auto operator*(const value&, const value&) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator*(const value&, Int) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator*(Int, const value&) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator*(const value&, Fp) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator*(Fp, const value&) -> value;

    constexpr friend auto operator/(const value&, const value&) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator/(const value&, Int) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator/(Int, const value&) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator/(const value&, Fp) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr friend auto operator/(Fp, const value&) -> value;

    constexpr friend auto operator%(const value&, const value&) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator%(const value&, Int) -> value;

    template <class Int> requires std::is_integral_v<Int>
    constexpr friend auto operator%(Int, const value&) -> value;

    constexpr auto operator+=(const value&);

    template <class Int> requires std::is_integral_v<Int>
    constexpr auto operator+=(Int) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr auto operator+=(Fp) -> value;

    constexpr auto operator-=(const value&);

    template <class Int> requires std::is_integral_v<Int>
    constexpr auto operator-=(Int) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr auto operator-=(Fp) -> value;

    constexpr auto operator*=(const value&);

    template <class Int> requires std::is_integral_v<Int>
    constexpr auto operator*=(Int) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr auto operator*=(Fp) -> value;
value_typer/=(const value&);

    template <class Int> requires std::is_integral_v<Int>
    constexpr auto operator/=(Int) -> value;

    template <class Fp> requires std::is_floating_point_v<Fp>
    constexpr auto operator/=(Fp) -> value;

    constexpr auto operator%=(const value&);

    template <class Int> requires std::is_integral_v<Int>
    constexpr auto operator%=(Int) -> value;

    constexpr auto operator++() -> value&;
    constexpr auto operator++(int) -> value;
    constexpr auto operator--() -> value&;
    constexpr auto operator--(int) -> value;

    constexpr explicit operator bool() const noexcept;
    constexpr auto operator==(const value&) const noexcept -> bool;
    constexpr auto operator<(const value&) const -> bool;

private:
    value_type m_val;
};

template <class F> requires detail::visitable<F, value::types>
constexpr auto visit(F&& f, const value& v) {
    return std::visit(detail::overload{
        [f = std::forward<F>(f)](const detail::box<array>& arr) { return f(*arr); },
        [f = std::forward<F>(f)](const detail::box<object>& arr) { return f(*arr); },
        [f = std::forward<F>(f)]<class T> (T&& val) { return f(std::forward<T>(val)); },
    }, v.m_val);
}

auto operator<<(std::ostream&, const value&) noexcept -> std::ostream&;

constexpr json::value::value(const value& other) : m_val(json::visit(json::detail::overload{
    [](const json::object& obj) {
        return value::value_type(detail::make_box<object>(obj));
    },
    [](const json::array& arr) {
        return value::value_type(detail::make_box<array>(arr));
    },
    [](const auto& val) { return value::value_type(val); },
}, other)) {}

constexpr auto operator+(const value&, const value&) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator+(const value&, Int) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator+(Int, const value&) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator+(const value&, Fp) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator+(Fp, const value&) -> value;

constexpr auto operator-(const value&, const value&) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator-(const value&, Int) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator-(Int, const value&) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator-(const value&, Fp) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator-(Fp, const value&) -> value;

constexpr auto operator*(const value&, const value&) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator*(const value&, Int) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator*(Int, const value&) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator*(const value&, Fp) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator*(Fp, const value&) -> value;

constexpr auto operator/(const value&, const value&) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator/(const value&, Int) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator/(Int, const value&) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator/(const value&, Fp) -> value;

template <class Fp> requires std::is_floating_point_v<Fp>
constexpr auto operator/(Fp, const value&) -> value;

constexpr auto operator%(const value&, const value&) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator%(const value&, Int) -> value;

template <class Int> requires std::is_integral_v<Int>
constexpr auto operator%(Int, const value&) -> value;

namespace literals {
    auto operator""_value(unsigned long long x) noexcept -> json::value;
    auto operator""_value(long double x) noexcept -> json::value;
    auto operator""_value(const char* s, std::size_t n) noexcept -> json::value;
}

}

#endif