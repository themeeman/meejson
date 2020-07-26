#ifndef JSON_LEXER_HPP
#define JSON_LEXER_HPP

#include <variant>
#include <cstdint>
#include <compare>
#include <vector>

#include "value.hpp"

namespace mee::json {

enum class symbol {
    lbracket,
    rbracket,
    lbrace,
    rbrace,
    comma,
    colon
};

struct token {
    std::variant<null, bool, std::int64_t, double, std::string, symbol> tok;
    std::int32_t line;
    std::int32_t col;

    constexpr auto operator==(const token&) const noexcept -> bool = default;
};

auto lex(std::string_view) noexcept -> result<std::vector<token>>;

}

#endif
