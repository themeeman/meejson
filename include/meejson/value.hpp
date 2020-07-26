#ifndef JSON_VALUE_HPP
#define JSON_VALUE_HPP

#include <variant>
#include <type_traits>
#include <string>
#include <cstdint>
#include <functional>
#include <concepts>
#include <string>

#include "box.hpp"
#include "array.hpp"
#include "object.hpp"
#include "except.hpp"
#include "detail.hpp"
#include "type_list.hpp"

namespace mee::json {

template <class>
struct is_value;

template <class T>
concept integral = !std::same_as<T, bool> && std::integral<T>;

template <class T>
concept arithmetic = integral<T> || std::floating_point<T>;

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

    constexpr auto unwrap() const -> T& {
        if (!m_ptr)
            std::terminate();
        return *m_ptr;
    }

private:
    T* m_ptr;
};

struct null {
    constexpr auto operator<=>(const null&) const noexcept -> std::strong_ordering = default;
};

inline auto operator<<(std::ostream& os, null) noexcept -> std::ostream& {
    os << "null";
    return os;
}

template <class Value, class T> requires in_type_list<T, typename Value::types>
struct type_name;

template <class Value>
struct type_name<Value, typename Value::null_type> {
    constexpr static auto value = std::string_view("null");
};

template <class Value>
struct type_name<Value, typename Value::bool_type> {
    constexpr static auto value = std::string_view("boolean");
};

template <class Value>
struct type_name<Value, typename Value::int_type> {
    constexpr static auto value = std::string_view("integer");
};

template <class Value>
struct type_name<Value, typename Value::float_type> {
    constexpr static auto value = std::string_view("float");
};

template <class Value>
struct type_name<Value, typename Value::string_type> {
    constexpr static auto value = std::string_view("string");
};

template <class Value>
struct type_name<Value, typename Value::array_type> {
    constexpr static auto value = std::string_view("array");
};

template <class Value>
struct type_name<Value, typename Value::object_type> {
    constexpr static auto value = std::string_view("object");
};

template <class F, class Value> requires is_value<Value>::value && visitable<F, typename Value::types>
constexpr auto visit(F&& f, const Value& v);

template <class F, class Value> requires is_value<Value>::value && visitable2<F, typename Value::types, typename Value::types>
constexpr auto visit(F&& f, const Value& v1, const Value& v2);

namespace detail {

template <class Value, class F>
void apply_value_assign(Value& self, const Value& other, F&& f, std::string_view op) {
    json::visit(detail::overload{
        [&self, f = std::forward<F>(f)] <arithmetic T, arithmetic U>(T x, U y)
            { self = f(x, y); },
        [op]<class T, class U>(const T&, const U&)
            { throw json::invalid_operation(type_name<Value, T>::value, type_name<Value, U>::value, op); }
    }, self, other);
}

template <class Value, arithmetic T, class F>
void apply_value_assign(Value& self, T other, F&& f, std::string_view op) {
    json::visit(detail::overload{
        [&self, other, f = std::forward<F>(f)] <arithmetic U>(const U& x)
            { self = f(x, other); },
        [op]<class U>(const U&) {
            if constexpr (json::integral<T>) {
                throw json::invalid_operation(type_name<Value, U>::value, type_name<Value, typename Value::int_type>::value, op);
            } else {
                throw json::invalid_operation(type_name<Value, U>::value, type_name<Value, typename Value::float_type>::value, op);
            }
        }
    }, self);
}

}
template <class T>
struct type_t {
    using type = T;

    constexpr auto operator==(type_t) const noexcept -> bool {
        return true;
    }

    template <class U>
    constexpr auto operator==(type_t<U>) const noexcept -> bool {
        return false;
    }
};

template <class T>
constexpr static auto type = type_t<T>();

template <template <class...> class Temp>
struct templ_t {
    template <class... Ts>
    using templ = Temp<Ts...>;

    constexpr auto operator==(templ_t) const noexcept -> bool {
        return true;
    }

    template <template <class...> class U>
    constexpr auto operator==(templ_t<U>) const noexcept -> bool {
        return false;
    }
};

template <template <class...> class Temp>
constexpr static auto templ = templ_t<Temp>();

template <class IntType = std::int64_t,
          class FloatType = double,
          class StringType = std::string,
          template <class> class ArrayType = basic_array,
          template <class> class ObjectType = basic_object>
struct value_args {
    type_t<IntType> int_type{};
    type_t<FloatType> float_type{};
    type_t<StringType> string_type{};
    templ_t<ArrayType> array_type{};
    templ_t<ObjectType> object_type{};

    constexpr auto operator==(const value_args&) const noexcept -> bool = default;
};

template <class IntType = std::int64_t,
    class FloatType = double,
    class StringType = std::string,
    template <class> class ArrayType = basic_array,
    template <class> class ObjectType = basic_object>
struct basic_value {
#if 0
    constexpr static auto args = Args;

    using null_type = null;
    using bool_type = bool;
    using int_type = typename decltype(Args.int_type)::type;
    using float_type = typename decltype(Args.float_type)::type;
    using string_type = typename decltype(Args.string_type)::type;
    using array_type = typename decltype(Args.array_type)::template templ<basic_value>;
    using object_type = typename decltype(Args.object_type)::template templ<basic_value>;
#endif

    using null_type = null;
    using bool_type = bool;
    using int_type = IntType;
    using float_type = FloatType;
    using string_type = StringType;
    using array_type = ArrayType<basic_value>;
    using object_type = ObjectType<basic_value>;

    using numbers = type_list<int_type, float_type>;
    using primitives = type_list<null_type, bool_type, int_type, float_type, string_type>;
    using aggregates = type_list<array_type, object_type>;
    using types = type_list<null_type, bool_type, int_type, float_type, string_type, array_type, object_type>;
    using value_type = std::variant<null_type, bool_type, int_type, float_type, string_type, detail::box<array_type>, detail::box<object_type>>;

    template <class T> requires in_type_list<T, types>
    constexpr static auto type_name_v = type_name<basic_value, T>::value;

    constexpr basic_value() noexcept : m_val() {}
    basic_value(const basic_value& other) : m_val(std::visit(detail::overload{
        []<class T>(const detail::box<T>& val) { return value_type(val.clone()); },
        [](const auto& val) { return value_type(val); },
    }, other.m_val)) {}

    constexpr basic_value(basic_value&& other) noexcept : m_val(std::move(other.m_val)) {}

    template <class T> requires in_type_list<std::remove_cvref_t<T>, primitives>
    constexpr explicit basic_value(T&& t) noexcept : m_val(std::forward<T>(t)) {}

    template <class T> requires in_type_list<std::remove_cvref_t<T>, aggregates>
    constexpr explicit basic_value(T&& t) noexcept : m_val(detail::make_box<std::remove_cvref_t<T>>(std::forward<T>(t))) {}

    template <json::integral Int> requires (!std::same_as<Int, int_type>)
    constexpr explicit basic_value(Int i) noexcept : m_val(int_type(i)) {}

    template <std::floating_point Fp> requires (!std::same_as<Fp, float_type>)
    constexpr explicit basic_value(Fp f) noexcept : m_val(float_type(f)) {}

    template <class S> requires (!std::same_as<std::remove_cvref_t<S>, string_type> && std::constructible_from<string_type, S>)
    constexpr explicit basic_value(S&& s) noexcept : m_val(string_type(std::forward<S>(s))) {}

    constexpr basic_value(std::initializer_list<basic_value> list) : m_val(detail::make_box<array_type>(list)) {}
    constexpr basic_value(std::initializer_list<std::pair<const string_type, basic_value>> list) : m_val(detail::make_box<object_type>(list)) {}

    auto operator=(const basic_value& other) -> basic_value& {
        m_val = std::visit(detail::overload{
            []<class T>(const detail::box<T>& val) { return value_type(val.clone()); },
            [](const auto& val) { return value_type(val); },
        }, other.m_val);
        return *this;
    }

    auto operator=(basic_value&&) noexcept -> basic_value& = default;

    template <class T> requires in_type_list<std::remove_cvref_t<T>, types>
    constexpr auto operator=(T&& t) noexcept -> basic_value& {
        if constexpr (in_type_list<std::remove_cvref_t<T>, primitives>) {
            m_val = std::forward<T>(t);
        } else {
            m_val = detail::make_box<std::remove_cvref_t<T>>(std::forward<T>(t));
        }
        return *this;
    }

    template <json::integral Int> requires (!std::same_as<Int, int_type>)
    constexpr auto operator=(Int i) noexcept -> basic_value& {
        m_val = int_type(i);
        return *this;
    }

    template <std::floating_point Fp> requires (!std::same_as<Fp, float_type>)
    constexpr auto operator=(Fp f) noexcept -> basic_value& {
        m_val = float_type(f);
        return *this;
    }

    template <class S> requires (!std::same_as<std::remove_cvref_t<S>, string_type> && std::constructible_from<std::string, S>)
    constexpr auto operator=(S&& s) noexcept -> basic_value& {
        m_val = string_type(std::forward<S>(s));
        return *this;
    }

    constexpr auto operator=(std::initializer_list<basic_value> list) noexcept -> basic_value& {
        m_val = detail::make_box<array_type>(list);
        return *this;
    }

    constexpr auto operator=(std::initializer_list<std::pair<string_type, basic_value>> list) noexcept -> basic_value& {
        m_val = detail::make_box<object_type>(list);
        return *this;
    }

    [[nodiscard]] auto type_name() const noexcept -> std::string_view {
        return json::visit([]<class T>(const T&) { return type_name_v<T>; }, *this);
    }

    template <class T> requires in_type_list<T, types>
    constexpr auto get() -> T& {
        if constexpr (in_type_list<T, aggregates>) {
            return *std::get<detail::box<T>>(m_val);
        } else {
            return std::get<T>(m_val);
        }
    }

    template <class T> requires in_type_list<T, types>
    constexpr auto get() const -> const T& {
        if constexpr (in_type_list<T, aggregates>) {
            return *std::get<detail::box<T>>(m_val);
        } else {
            return std::get<T>(m_val);
        }
    }

    template <class T> requires in_type_list<T, types>
    constexpr auto get_if() noexcept -> optional_ref<T> {
        return holds<T>() ? &get<T>() : nullptr;
    }

    template <class T> requires in_type_list<T, types>
    constexpr auto get_if() const noexcept -> optional_ref<const T> {
        return holds<T>() ? &get<T>() : nullptr;
    }

    template <class T> requires in_type_list<T, primitives>
    [[nodiscard]] constexpr auto holds() const noexcept -> bool {
        return std::holds_alternative<T>(m_val);
    }

    template <class T> requires in_type_list<T, aggregates>
    [[nodiscard]] constexpr auto holds() const noexcept -> bool {
        return std::holds_alternative<detail::box<T>>(m_val);
    }

    template <class F, class Value> requires is_value<Value>::value && visitable<F, typename Value::types>
    constexpr friend auto visit(F&& f, const Value& v);

    template <class F, class Value> requires is_value<Value>::value && visitable2<F, typename Value::types, typename Value::types>
    constexpr friend auto visit(F&& f, const Value& v1, const Value& v2);

    auto operator[](std::string_view s) -> basic_value& {
        auto obj = get_if<object_type>();
        if (!obj) {
            throw invalid_operation(type_name(), "[string]");
        }
        auto iter = obj->find(s);
        if (iter != obj->end()) {
            return iter->second();
        } else {
            throw invalid_access(s);
        }
    }

    auto operator[](std::string_view s) const -> const basic_value& {
        auto obj = get_if<object_type>();
        if (!obj) {
            throw invalid_operation(type_name(), "[string]");
        }
        auto iter = obj->find(s);
        if (iter != obj->end()) {
            return iter->second();
        } else {
            throw invalid_access(s);
        }
    }

    auto operator[](std::size_t i) -> basic_value& {
        auto arr = get_if<array_type>();
        if (!arr) {
            throw invalid_operation(type_name(), "[index]");
        }
        if (i < arr->size()) {
            return (*arr)[i];
        } else {
            throw invalid_access(std::to_string(i));
        }
    }

    auto operator[](std::size_t i) const -> const basic_value& {
        auto arr = get_if<array_type>();
        if (!arr) {
            throw invalid_operation(type_name(), "[index]");
        }
        if (i < arr->size()) {
            return (*arr)[i];
        } else {
            throw invalid_access(std::to_string(i));
        }
    }

    auto operator+=(const basic_value& other) -> basic_value& {
        detail::apply_value_assign(*this, other, std::plus(), "+=");
        return *this;
    }

    auto operator-=(const basic_value& other) -> basic_value& {
        detail::apply_value_assign(*this, other, std::minus(), "-=");
        return *this;
    }

    auto operator*=(const basic_value& other) -> basic_value& {
        detail::apply_value_assign(*this, other, std::multiplies(), "*=");
        return *this;
    }

    auto operator/=(const basic_value& other) -> basic_value& {
        detail::apply_value_assign(*this, other, std::divides(), "/=");
        return *this;
    }

    auto operator%=(const basic_value& other) -> basic_value& {
        json::visit(detail::overload{
            [this](int_type x, int_type y) { *this = x % y; },
            []<class T, class U>(const T&, const U&)
                { throw json::invalid_operation(type_name_v<T>, type_name_v<U>, "%="); }
        }, *this, other);
        return *this;
    }

    template <arithmetic T>
    auto operator+=(T x) -> basic_value& {
        detail::apply_value_assign(*this, x, std::plus(), "+=");
        return *this;
    }

    template <arithmetic T>
    auto operator-=(T x) -> basic_value& {
        detail::apply_value_assign(*this, x, std::minus(), "-=");
        return *this;
    }

    template <arithmetic T>
    auto operator*=(T x) -> basic_value& {
        detail::apply_value_assign(*this, x, std::multiplies(), "*=");
        return *this;
    }

    template <arithmetic T>
    auto operator/=(T x) -> basic_value& {
        detail::apply_value_assign(*this, x, std::divides(), "/=");
        return *this;
    }

    template <json::integral Int>
    auto operator%=(Int x) -> basic_value& {
        json::visit(detail::overload{
            [this, x](int_type y) { *this = y % x; },
            []<class T>(const T&) { throw json::invalid_operation(type_name_v<T>, type_name_v<int_type>, "%="); }
        }, *this);
        return *this;
    }

    auto operator++() -> basic_value& {
        operator+=(1);
        return *this;
    }

    auto operator++(int) -> basic_value {
        auto copy = *this;
        operator++();
        return copy;
    }

    auto operator--() -> basic_value& {
        operator-=(1);
        return *this;
    }

    auto operator--(int) -> basic_value {
        auto copy = *this;
        operator--();
        return copy;
    }

    explicit operator bool() const noexcept {
        return json::visit(detail::overload{
            [](const array_type& arr) { return !arr.empty(); },
            [](const object_type& obj) { return !obj.empty(); },
            [](const string_type& s) { return !s.empty(); },
            [](null_type) { return false; },
            [](auto x) { return bool(x); },
        }, *this);
    }

    auto operator==(const basic_value& other) const noexcept -> bool {
        return json::visit(detail::overload{
            []<class T>(const T& lhs, const T& rhs) { return lhs == rhs; },
            [](arithmetic auto lhs, arithmetic auto rhs) { return lhs == rhs; },
            [](const auto&, const auto&) { return false; },
        }, *this, other);
    }

    template <in_type_list<types> T> requires (!arithmetic<T>)
    auto operator==(const T& other) const noexcept -> bool {
        return json::visit(detail::overload{
            [&other](const T& val) { return other == val; },
            [](const auto&) { return false; },
        }, *this);
    }

    template <arithmetic T>
    auto operator==(T other) const noexcept -> bool {
        return json::visit(detail::overload{
            [other](arithmetic auto val) { return other == val; },
            [](const auto&) { return false; },
        }, *this);
    }

    template <class S> requires (!std::same_as<std::remove_cvref_t<S>, string_type> && !std::convertible_to<S, basic_value> && std::equality_comparable_with<string_type, S>)
    auto operator==(S&& other) const noexcept -> bool {
        return json::visit(detail::overload{
            [other = std::forward<S>(other)](const string_type& val) { return std::forward<S>(other) == val; },
            [](const auto&) { return false; },
        }, *this);
    }

    auto operator==(std::initializer_list<basic_value> list) const noexcept -> bool {
        return json::visit(detail::overload{
            [list](const array_type& arr) { return std::equal(arr.begin(), arr.end(), list.begin(), list.end()); },
            [](const auto&) { return false; },
        }, *this);
    }

    auto operator==(std::initializer_list<std::pair<string_type, basic_value>> list) const noexcept -> bool {
        return json::visit(detail::overload{
            [list](const object_type& arr) { return arr == list; },
            [](const auto&) { return false; },
        }, *this);
    }

    auto operator<=>(const basic_value& other) const noexcept -> std::partial_ordering {
        return json::visit(detail::overload{
            [](const object_type& lhs, const object_type& rhs) {
                return (lhs == rhs) ? std::partial_ordering::equivalent : std::partial_ordering::unordered;
            },
            []<class T> (const T& lhs, const T& rhs) -> std::partial_ordering {
                static_assert(in_type_list<T, types>);
                return lhs <=> rhs;
            },
            [](const auto&, const auto&) { return std::partial_ordering::unordered; },
        }, *this, other);
    }

    template <in_type_list<types> T>
    auto operator<=>(const T& other) const noexcept -> std::partial_ordering {
        return json::visit(detail::overload{
            [&other](const T& val) -> std::partial_ordering { return val <=> other; },
            [](const auto&) { return std::partial_ordering::unordered; },
        }, *this);
    }


    auto operator<=>(const object_type& other) const noexcept -> std::partial_ordering {
        return json::visit(detail::overload{
            [&other](const object_type& obj) {
                return (obj == other) ? std::partial_ordering::equivalent : std::partial_ordering::unordered;
            },
            [](const auto&) { return std::partial_ordering::unordered; }
        }, *this);
    }

    template <json::integral T> requires (!std::same_as<T, int_type>)
    auto operator<=>(T other) const noexcept -> std::partial_ordering {
        return json::visit(detail::overload{
            [other](int_type val) -> std::partial_ordering { return val <=> other; },
            [](const auto&) { return std::partial_ordering::unordered; },
        }, *this);
    }

    template <std::floating_point T> requires (!std::same_as<T, float_type>)
    auto operator<=>(T other) const noexcept -> std::partial_ordering {
        return json::visit(detail::overload{
            [other](float_type val) -> std::partial_ordering { return val <=> other; },
            [](const auto&) { return std::partial_ordering::unordered; },
        }, *this);
    }

    template <class S> requires (!std::same_as<std::remove_cvref_t<S>, string_type> && !std::convertible_to<S, basic_value> && std::totally_ordered_with<string_type, S>)
    auto operator<=>(S&& other) const noexcept -> std::partial_ordering {
        return json::visit(detail::overload{
            [other = std::forward<S>(other)](const string_type& val) -> std::partial_ordering { return val <=> std::forward<S>(other); },
            [](const auto&) { return std::partial_ordering::unordered; },
        }, *this);
    }

    constexpr auto get_null() noexcept -> null_type& {
        return get<null_type>();
    }

    constexpr auto get_null() const noexcept -> const null_type& {
        return get<null_type>();
    }

    constexpr auto get_bool() noexcept -> bool_type& {
        return get<float_type>();
    }

    constexpr auto get_bool() const noexcept -> const bool_type& {
        return get<bool_type>();
    }

    constexpr auto get_int() noexcept -> int_type& {
        return get<int_type>();
    }

    constexpr auto get_int() const noexcept -> const int_type& {
        return get<int_type>();
    }

    constexpr auto get_float() noexcept -> float_type& {
        return get<float_type>();
    }

    constexpr auto get_float() const noexcept -> const float_type& {
        return get<float_type>();
    }

    constexpr auto get_string() noexcept -> string_type& {
        return get<string_type>();
    }

    constexpr auto get_string() const noexcept -> const string_type& {
        return get<string_type>();
    }

    constexpr auto get_array() noexcept -> array_type& {
        return get<array_type>();
    }

    constexpr auto get_array() const noexcept -> const array_type& {
        return get<array_type>();
    }

    constexpr auto get_object() noexcept -> object_type& {
        return get<object_type>();
    }

    constexpr auto get_object() const noexcept -> const object_type& {
        return get<object_type>();
    }

    constexpr auto get_if_null() noexcept -> optional_ref<null_type> {
        return get_if<null_type>();
    }

    constexpr auto get_if_null() const noexcept -> optional_ref<const null_type> {
        return get_if<null_type>();
    }

    constexpr auto get_if_bool() noexcept -> optional_ref<bool_type> {
        return get_if<float_type>();
    }

    constexpr auto get_if_bool() const noexcept -> optional_ref<const bool_type> {
        return get_if<bool_type>();
    }

    constexpr auto get_if_int() noexcept -> optional_ref<int_type> {
        return get_if<int_type>();
    }

    constexpr auto get_if_int() const noexcept -> optional_ref<const int_type> {
        return get_if<int_type>();
    }

    constexpr auto get_if_float() noexcept -> optional_ref<float_type> {
        return get_if<float_type>();
    }

    constexpr auto get_if_float() const noexcept -> optional_ref<const float_type> {
        return get_if<float_type>();
    }

    constexpr auto get_if_string() noexcept -> optional_ref<string_type> {
        return get_if<string_type>();
    }

    constexpr auto get_if_string() const noexcept -> optional_ref<const string_type> {
        return get_if<string_type>();
    }

    constexpr auto get_if_array() noexcept -> optional_ref<array_type> {
        return get_if<array_type>();
    }

    constexpr auto get_if_array() const noexcept -> optional_ref<const array_type> {
        return get_if<array_type>();
    }

    constexpr auto get_if_object() noexcept -> optional_ref<object_type> {
        return get_if<object_type>();
    }

    constexpr auto get_if_object() const noexcept -> optional_ref<const object_type> {
        return get_if<object_type>();
    }

private:
    value_type m_val;
};

using value = basic_value<>;

template <class V>
struct is_value : std::false_type {};

template <class I, class F, class S, template <class> class A, template <class> class O>
struct is_value<basic_value<I, F, S, A, O>> : std::true_type {};

template <class V>
constexpr static auto is_value_v = is_value<V>::value;

using array = basic_array<value>;
using object = basic_object<value>;

template <class F, class Value> requires is_value<Value>::value && visitable<F, typename Value::types>
constexpr auto visit(F&& f, const Value& v) {
    return std::visit(detail::overload{
        [f = std::forward<F>(f)]<class T>(const detail::box<T>& b) { return f(*b); },
        [f = std::forward<F>(f)](const auto& val) {
            static_assert(in_type_list<std::remove_cvref_t<decltype(val)>, typename Value::types>);
            return f(val);
        },
    }, v.m_val);
}

template <class F, class Value> requires is_value<Value>::value && visitable2<F, typename Value::types, typename Value::types>
constexpr auto visit(F&& f, const Value& v1, const Value& v2) {
    using detail::box;
    return std::visit(detail::overload{
        [f = std::forward<F>(f)]<class T, class U>(const box<T>& lhs, const box<U>& rhs) { return f(*lhs, *rhs); },
        [f = std::forward<F>(f)]<class T>(const box<T>& lhs, const auto& rhs) {
            static_assert(in_type_list<std::remove_cvref_t<decltype(rhs)>, typename Value::types>);
            return f(*lhs, rhs);
        },
        [f = std::forward<F>(f)]<class T>(const auto& lhs, const box<T>& rhs) {
            static_assert(in_type_list<std::remove_cvref_t<decltype(lhs)>, typename Value::types>);
            return f(lhs, *rhs);
        },
        [f = std::forward<F>(f)](const auto& lhs, const auto& rhs) {
            static_assert(in_type_list<std::remove_cvref_t<decltype(lhs)>, typename Value::types>);
            static_assert(in_type_list<std::remove_cvref_t<decltype(rhs)>, typename Value::types>);
            return f(lhs, rhs);
        }
    }, v1.m_val, v2.m_val);
}

namespace detail {
template <class Value, class T, class F> requires is_value_v<Value> && in_type_list<T, typename Value::numbers>
void apply_value_assign(Value& self, T other, F&& f, std::string_view op) {
    json::visit(detail::overload{
        [&self, other, f = std::forward<F>(f)]<arithmetic U>(U x)
            { self = f(x, other); },
        [op]<class U>(const U&)
            { throw json::invalid_operation(Value::template type_name_v<T>, Value::template type_name_v<U>, op); }
    }, self, other);
}
}

template <class Value> requires is_value_v<Value>
auto operator<<(std::ostream& os, const Value& v) noexcept -> std::ostream& {
    auto p = os.precision(std::numeric_limits<double>::max_digits10);
    auto b = os.flags(std::ios_base::boolalpha);
    json::visit(json::detail::overload{
        [&os](const std::string& s) { os << "\"" << s << "\""; },
        [&os](const auto& val) { os << val; },
    }, v);
    os.precision(p);
    os.flags(b);
    return os;
}

namespace detail {

template <class Value, arithmetic T, class F> requires is_value_v<Value>
auto apply_value(const Value& lhs, T rhs, F&& f, std::string_view op) {
    return json::visit(detail::overload{
        [rhs, f = std::forward<F>(f)]<arithmetic U> (const U& x)
            { return Value(f(x, rhs)); },
        [op]<class U> (const U&) -> Value {
            if constexpr (json::integral<T>) {
                throw invalid_operation(Value::template type_name_v<U>, Value::template type_name_v<typename Value::int_type>, op);
            } else {
                throw invalid_operation(Value::template type_name_v<U>, Value::template type_name_v<typename Value::float_type>, op);
            }
        }
    }, lhs);
}

template <class Value, arithmetic T, class F> requires is_value_v<Value>
auto apply_value(T lhs, const Value& rhs, F&& f, std::string_view op) {
    return json::visit(detail::overload{
        [lhs, f = std::forward<F>(f)]<arithmetic U>(U x)
            { return Value(f(lhs, x)); },
        [op]<class U>(const U&) -> Value {
            if constexpr (json::integral<T>) {
                throw invalid_operation(Value::template type_name_v<typename Value::int_type>, Value::template type_name_v<U>, op);
            } else {
                throw invalid_operation(Value::template type_name_v<typename Value::float_type>, Value::template type_name_v<U>, op);
            }
        }
    }, rhs);
}

template <class Value, class F> requires is_value_v<Value>
auto apply_value(const Value& lhs, const Value& rhs, F&& f , std::string_view op) {
    return json::visit(detail::overload{
        [f = std::forward<F>(f)]<arithmetic U, arithmetic V> (U x, V y)
            { return Value(f(x, y)); },
        [op]<class U, class V>(const U&, const V&) -> Value {
            throw invalid_operation(Value::template type_name_v<U>, Value::template type_name_v<V>, op);
        }
    }, lhs, rhs);
}

}

template <class Value> requires is_value_v<Value>
auto operator+(const Value& lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::plus(), "+");
}

template <class Value> requires is_value_v<Value>
auto operator-(const Value& lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::minus(), "-");
}

template <class Value> requires is_value_v<Value>
auto operator*(const Value& lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::multiplies(), "*");
}

template <class Value> requires is_value_v<Value>
auto operator/(const Value& lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::divides(), "/");
}

template <class Value> requires is_value_v<Value>
auto operator%(const Value& lhs, const Value& rhs) -> Value {
    return visit(detail::overload{
        [](typename Value::int_type x, typename Value::int_type y) { return basic_value(x % y); },
        []<class T, class U>(const T&, const U&) -> value {
            throw invalid_operation(Value::template type_name_v<T>, Value::template type_name_v<U>, "%");
        }
    }, lhs, rhs);
}

template <class Value> requires is_value_v<Value>
auto operator+(const Value& v) -> Value {
    return visit(detail::overload{
        []<arithmetic T>(T x)
            { return basic_value(+x); },
        []<class T>(const T&) -> value
            { throw invalid_operation(Value::template type_name_v<T>, "+"); }
    }, v);
}

template <class Value> requires is_value_v<Value>
auto operator-(const Value& v) -> Value {
    return visit(detail::overload{
        []<arithmetic T>(T x)
            { return basic_value(-x); },
        []<class T>(const T&) -> value
            { throw invalid_operation(Value::template type_name_v<T>, "-"); }
    }, v);
}

template <class Value, arithmetic T> requires is_value_v<Value>
auto operator+(const Value& lhs, T rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::plus(), "+");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator+(T lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::plus(), "+");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator-(const Value& lhs, T rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::minus(), "-");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator-(T lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::minus(), "-");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator*(const Value& lhs, T rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::multiplies(), "*");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator*(T lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::multiplies(), "*");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator/(const Value& lhs, T rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::divides(), "/");
}

template <arithmetic T, class Value> requires is_value_v<Value>
auto operator/(T lhs, const Value& rhs) -> Value {
    return detail::apply_value(lhs, rhs, std::divides(), "/");
}

template <json::integral Int, class Value> requires is_value_v<Value>
auto operator%(const Value& lhs, Int rhs) -> Value {
    return visit(detail::overload{
        [rhs](typename Value::int_type x) { return basic_value(x % rhs); },
        []<class T>(const T&) -> Value
            { throw invalid_operation(Value::template type_name_v<T>, Value::template type_name_v<typename Value::int_type>, "%"); }
    }, lhs);
}

template <json::integral Int, class Value> requires is_value_v<Value>
auto operator%(Int lhs, const Value& rhs) -> Value {
    return visit(detail::overload{
        [lhs](typename Value::int_type x) { return basic_value(lhs % x); },
        []<class T>(const T&) -> Value
            { throw invalid_operation(Value::template type_name_v<typename Value::int_type>, Value::template type_name_v<T>, "%"); }
    }, rhs);
}

namespace literals {

inline auto operator""_value(unsigned long long x) noexcept -> value {
    return value(int64_t(x));
}

inline auto operator""_value(long double x) noexcept -> value {
    return value(double(x));
}

inline auto operator""_value(const char* s, std::size_t n) noexcept -> value {
    return value(std::string(s, n));
}

}

}

#endif