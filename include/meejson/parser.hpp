#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <string_view>
#include "value.hpp"
#include "lexer.hpp"

namespace json {
auto parse(std::string_view) noexcept -> result<value>;
auto parse(const std::vector<token>&) noexcept -> result<value>;
}

#endif
