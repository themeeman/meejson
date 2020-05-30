#include "json.hpp"

#include <compare>

template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

constexpr json::value::value(const value& other) : m_val(json::visit(overload{
    [](const value::object& obj) {
        auto copy = value::object();
        std::for_each(begin(obj), end(obj), 
            [&copy](const auto& pair) mutable { copy.emplace(pair.first, std::make_unique<value>(*pair.second)); });
        return value::value_type(copy);
    },
    [](const value::array& arr) {
        auto copy = value::array();
        copy.reserve(arr.size());
        std::for_each(begin(arr), end(arr), 
            [&copy](const auto& val) mutable { copy.push_back(std::make_unique<value>(*val)); });
        return value::value_type(copy);
    },
    [](const auto& val) { return value::value_type(val); },
}, other)) {}

auto json::operator<<(std::ostream& os, const value& v) noexcept -> std::ostream& {
    json::visit(overload{
        [&os](const value::object& obj) {
            os << '{';
            auto it = begin(obj);
            if (it != end(obj)) {
                std::cout << "\"" << (*it).first << "\"" << ':' << *(*it).second;
                it++;
                for (; it != end(obj); it++) {
                    std::cout << ',';
                    std::cout << "\"" << (*it).first << "\"" << ':' << *(*it).second;
                }
            }
            os << '}';
        },
        [&os](const value::array& arr) {
            os << '[';
            auto it = begin(arr);
            if (it != end(arr)) {
                std::cout << **it;
                it++;
                for (; it != end(arr); it++) {
                    std::cout << ',' << **it;
                }
            }
            os << ']';
        },
        [&os](const std::string& s) { os << "\"" << s << "\""; },
        [&os](null) { os << "null"; },
        [&os](const auto& val) { std::cout << val; },
    }, v);
    return os;
}
