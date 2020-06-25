#include "value.hpp"

#include <compare>
#include <iostream>

auto json::operator<<(std::ostream& os, null) noexcept -> std::ostream& {
    os << "null";
    return os;
}

auto json::operator<<(std::ostream& os, const value& v) noexcept -> std::ostream& {
    auto p = os.precision(std::numeric_limits<double>::max_digits10);
    os << std::boolalpha;
    json::visit(json::detail::overload{
        [&os](const std::string& s) { os << "\"" << s << "\""; },
        [&os](const auto& val) { os << val; },
    }, v);
    os.precision(p);
    return os;
}

auto json::value::operator[](std::string_view s) -> value& {
    return get<object>()[std::string(s)];
}

auto json::literals::operator""_value(unsigned long long x) noexcept -> json::value {
    return value(int64_t(x));
}

auto json::literals::operator""_value(long double x) noexcept -> json::value {
    return value(double(x));
}

auto json::literals::operator""_value(const char* s, std::size_t n) noexcept -> json::value {
    return value(std::string(s, n));
}