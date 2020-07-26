#include "../include/meejson/parser.hpp"
#include "../include/meejson/type_list.hpp"

using json::detail::overload;
using namespace std::literals;

namespace {

auto to_string(const json::token& t) noexcept -> std::string {
    using json::symbol;
    return std::visit(overload{
        [](symbol t) {
            switch (t) {
                case symbol::lbracket:
                    return "["s;
                case symbol::rbracket:
                    return "]"s;
                case symbol::lbrace:
                    return "{"s;
                case symbol::rbrace:
                    return "}"s;
                case symbol::colon:
                    return ":"s;
                case symbol::comma:
                    return ","s;
            };
            std::terminate();
        },
        [](const std::string &s) { return '"' + s + '"'; },
        [](json::null) { return "null"s; },
        [](bool b) { return b ? "true"s : "false"s; },
        [](const auto &x) { return std::to_string(x); }
    }, t.tok);
}

template<class... Ts, class T>
requires json::in_type_list<T, json::type_list<Ts...>>
constexpr auto operator==(const std::variant<Ts...>& lhs, const T& rhs) noexcept {
    return std::visit(overload{
        [rhs](const T &x) { return x == rhs; },
        [](const auto &) { return false; },
    }, lhs);
}

struct Parser {
    Parser(std::vector<json::token>::const_iterator iter, std::vector<json::token>::const_iterator end) noexcept
    : m_iter(iter), m_end(end) {}

    auto parse() noexcept -> json::result<json::value> {
        if (m_iter == m_end) {
            return json::error(1, 1, "Parser Error: Unable to parse empty string");
        }
        auto val = parse_value();
        if (!val) {
            return val.error();
        }
        if (m_iter != m_end) {
            const auto&[t, line, col] = *m_iter;
            return json::error(line, col, "Parser Error: Unexpected token " + to_string(*m_iter));
        }
        return *val;
    }

    auto parse_value() noexcept -> json::result<json::value> {
        if (m_iter == m_end) {
            const auto &last = *(m_iter - 1);
            return json::error(last.line, last.col, "Parser Error: Unexpected end of tokens");
        }
        return std::visit(overload{
        [this](json::symbol t) -> json::result<json::value> {
            switch (t) {
                case json::symbol::lbrace:
                    return parse_object();
                case json::symbol::lbracket:
                    return parse_array();
                default:
                    return json::error(m_iter->line, m_iter->col,
                                       "Parser Error: Unexpected token " + to_string(*m_iter));
            }
        },
        [this](const auto &val) -> json::result<json::value> {
            m_iter++;
            return json::value(val);
        },
        }, m_iter->tok);
    }

    auto parse_array() noexcept -> json::result<json::value> {
        return parse_aggregate<json::array>(
            [](Parser& self) { return self.parse_value(); },
            [](json::array& arr, auto&& val) { return arr.push_back(std::forward<decltype(val)>(val)); },
        json::symbol::rbracket
        );
    }

    auto parse_object() noexcept -> json::result<json::value> {
        return parse_aggregate<json::object>(
            [](Parser& self) { return self.parse_key_value_pair(); },
            [](json::object& arr, auto&& val) { return arr.insert(std::forward<decltype(val)>(val)); },
        json::symbol::rbrace
        );
    }

    template<class T, class Parse, class Add>
    auto parse_aggregate(Parse&& parse, Add&& add, json::symbol end) -> json::result<json::value> {
        auto arr = T();
        m_iter++;
        if (m_iter == m_end) {
            const auto &last = *(m_iter - 1);
            return json::error(last.line, last.col, "Parser Error: Unexpected end of tokens, expected value");
        }
        if (m_iter->tok == end) {
            m_iter++;
            return json::value(arr);
        } else {
            if (auto val = parse(*this)) {
                add(arr, *val);
            } else {
                return val.error();
            }
        }

        while (m_iter != m_end) {
            if (m_iter->tok == end) {
                m_iter++;
                return json::value(arr);
            } else if (m_iter->tok == json::symbol::comma) {
                m_iter++;
                if (auto val = parse(*this)) {
                    add(arr, *val);
                } else {
                    return val.error();
                }
            } else {
                const auto& tok = *m_iter;
                return json::error(tok.line, tok.col, "Unexpected token '" + to_string(tok) + "' Expected ','");
            }
        }
        const auto& last = *(m_iter - 1);
        return json::error(last.line, last.col, "Parser Error: Unexpected end of tokens");
    }

    auto parse_key_value_pair() noexcept -> json::result<std::pair<std::string, json::value>> {
        const auto &key = *m_iter;
        if (!std::holds_alternative<std::string>(key.tok)) {
            return json::error(key.line, key.col,
                               "Parser Error: Invalid object key '" + to_string(key) + "', expecting string.");
        }
        m_iter++;
        if (m_iter == m_end) {
            return json::error(key.line, key.col, "Parser Error: Unexpected end of tokens, expecting ':'");
        }
        if (m_iter->tok != json::symbol::colon) {
            return json::error(m_iter->line, m_iter->col,
                               "Parser Error: Unexpected token '" + to_string(*m_iter) + "' expected ':'");
        }
        m_iter++;
        if (m_iter == m_end) {
            return json::error(key.line, key.col, "Parser Error: Unexpected end of tokens, expecting value");
        }
        if (auto val = parse_value()) {
            return std::pair(std::get<std::string>(key.tok), *val);
        } else {
            return val.error();
        }
    }

private:
    std::vector<json::token>::const_iterator m_iter;
    std::vector<json::token>::const_iterator m_end;
};

}

auto json::parse(std::string_view s) noexcept -> json::result<json::value> {
    auto toks = json::lex(s);
    if (toks) {
        return json::parse(*toks);
    } else {
        return toks.error();
    }
}

auto json::parse(const std::vector<json::token>& toks) noexcept -> json::result<json::value> {
    return Parser(toks.begin(), toks.end()).parse();
}
