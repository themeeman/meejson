#include "../include/meejson/lexer.hpp"
#include <array>
#include <concepts>
using namespace std::literals;

namespace mee {

namespace {

template<class T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template<class To, class From>
constexpr auto cast(const From& x) noexcept -> To {
    return x;
}

template<std::integral T> requires (sizeof(T) == 1)
struct utf8 {
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using difference_type = std::ptrdiff_t;
    using size_type = std::int8_t;

    constexpr utf8() noexcept: m_size(1), m_bytes{} {}

    constexpr explicit utf8(std::uint16_t x) noexcept: utf8(cast<std::uint32_t>(x)) {}

    constexpr explicit utf8(std::array<std::uint8_t, 3> x) : utf8(
    cast<std::uint32_t>(x[2] + (x[1] << 8) + (x[0] << 16))) {}

    constexpr explicit utf8(std::uint32_t x) : m_bytes{} {
        if (x <= 0x007F) {
            m_size = 1;
            m_bytes[0] = x;
        } else if (x <= 0x07FF) {
            const auto byte0 = cast<std::uint8_t>((x << 16) >> 24);
            const auto byte1 = cast<std::uint8_t>((x << 24) >> 24);
            m_size = 2;
            m_bytes[0] = (0b110 << 5) + (byte0 << 2) + (byte1 >> 6);
            m_bytes[1] = (0b10 << 6) + (byte1 & 0b00111111);
        } else if (x <= 0xFFFF) {
            const auto byte0 = cast<std::uint8_t>((x << 16) >> 24);
            const auto byte1 = cast<std::uint8_t>((x << 24) >> 24);
            m_size = 3;
            m_bytes[0] = (0b1110 << 4) + (byte0 >> 4);
            m_bytes[1] = (0b10 << 6) + ((byte0 & 0b00001111) << 2) + (byte1 >> 6);
            m_bytes[2] = (0b10 << 6) + (byte1 & 0b00111111);
        } else if (x <= 0x10FFFF) {
            const auto byte0 = cast<std::uint8_t>((x << 8) >> 24);
            const auto byte1 = cast<std::uint8_t>((x << 16) >> 24);
            const auto byte2 = cast<std::uint8_t>((x << 24) >> 24);
            m_size = 4;
            m_bytes[0] = (0b11110 << 3) + ((byte0 & 0b00011100) >> 2);
            m_bytes[1] = (0b10 << 6) + ((byte0 & 0b11) << 4) + (byte1 >> 4);
            m_bytes[2] = (0b10 << 6) + ((byte1 & 0b1111) << 2) + (byte2 >> 6);
            m_bytes[3] = (0b10 << 6) + (byte2 & 0b111111);
        } else {
            throw std::invalid_argument(std::to_string(x) + " is not representable as a UTF-8 code point");
        }
    }

    constexpr explicit operator std::basic_string_view<T>() noexcept {
        return std::basic_string_view<T>(m_bytes, m_size);
    }

    constexpr auto begin() noexcept -> iterator {
        return m_bytes;
    }

    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator {
        return m_bytes;
    }

    [[nodiscard]] constexpr auto cbegin() const noexcept -> iterator {
        return m_bytes;
    }

    constexpr auto end() noexcept -> iterator {
        return m_bytes + m_size;
    }

    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
        return m_bytes + m_size;
    }

    [[nodiscard]] constexpr auto cend() const noexcept -> iterator {
        return m_bytes + m_size;
    }

private:
    std::int8_t m_size;
    T m_bytes[4];
};

struct Lexer {
    Lexer(std::string_view::const_iterator begin, std::string_view::const_iterator end) : m_iter(begin), m_end(end) {}

    constexpr static auto is_int(char c) noexcept -> bool {
        return c >= '0' && c <= '9';
    }

    constexpr static auto is_hex(char c) noexcept -> bool {
        return is_int(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    constexpr static auto is_whitespace(char c) noexcept -> bool {
        return c == 0x20 || c == 0x0A || c == 0x0D || c == 0x09;
    }

    constexpr static auto is_alpha(char c) noexcept -> bool {
        return c >= 'a' && c <= 'z';
    }

    constexpr static auto is_exponent(char c) noexcept -> bool {
        return c == 'e' || c == 'E';
    }

    void advance() noexcept {
        m_iter++;
        m_col++;
    }

    auto lex() noexcept -> json::result<std::vector<json::token>> {
        auto vec = std::vector<json::token>();
        while (m_iter != m_end) {
            if (*m_iter == '{') {
                vec.emplace_back(json::symbol::lbrace, m_line, m_col);
                advance();
            } else if (*m_iter == '}') {
                vec.emplace_back(json::symbol::rbrace, m_line, m_col);
                advance();
            } else if (*m_iter == '[') {
                vec.emplace_back(json::symbol::lbracket, m_line, m_col);
                advance();
            } else if (*m_iter == ']') {
                vec.emplace_back(json::symbol::rbracket, m_line, m_col);
                advance();
            } else if (*m_iter == ':') {
                vec.emplace_back(json::symbol::colon, m_line, m_col);
                advance();
            } else if (*m_iter == ',') {
                vec.emplace_back(json::symbol::comma, m_line, m_col);
                advance();
            } else if (is_int(*m_iter) || *m_iter == '-') {
                if (auto tok = lex_number()) {
                    vec.push_back(*tok);
                } else {
                    return tok.error();
                }
            } else if (*m_iter == '"') {
                if (auto tok = lex_string()) {
                    vec.push_back(*tok);
                } else {
                    return tok.error();
                }
            } else if (*m_iter == '\n') {
                m_col = 1;
                m_line++;
                m_iter++;
            } else if (is_whitespace(*m_iter)) {
                advance();
            } else if (*m_iter == 'n' || *m_iter == 't' || *m_iter == 'f') {
                if (auto tok = lex_literal()) {
                    vec.push_back(*tok);
                } else {
                    return tok.error();
                }
            } else {
                return json::error(m_line, m_col, "Lexer Error: Unexpected Token\""s + *m_iter + '"');
            }
        }
        return vec;
    }

    auto lex_number() noexcept -> json::result<json::token> {
        auto line = m_line;
        auto col = m_col;
        auto s = std::string();
        auto is_float = false;
        if (*m_iter == '-') {
            s.push_back('-');
            advance();
        }
        if (*m_iter == '0') {
            s.push_back('0');
            advance();
        } else {
            s += lex_digits();
        }
        if (*m_iter == '.') {
            is_float = true;
            s.push_back('.');
            advance();
            auto digits = lex_digits();
            if (digits.empty()) {
                return json::error(line, col, "Lexer error: Invalid number literal \"" + s + "\"");
            }
            s += digits;
        }
        if (is_exponent(*m_iter)) {
            is_float = true;
            s.push_back('e');
            advance();
            if (*m_iter == '-') {
                s.push_back('-');
                advance();
            }
            auto digits = lex_digits();
            if (digits.empty()) {
                return json::error(line, col, "Lexer error: Invalid number literal \"" + s + "\"");
            }
            s += digits;
        }
        return is_float
               ? json::token(std::stod(s), line, col)
               : json::token(std::int64_t(std::stoll(s)), line, col);
    }

    auto lex_string() noexcept -> json::result<json::token> {
        auto line = m_line;
        auto col = m_col;
        advance();
        std::string s;
        while (m_iter != m_end && *m_iter != '"') {
            if (*m_iter == '\\') {
                advance();
                if (*m_iter == 'u') {
                    advance();
                    if (auto bytes = lex_unicode()) {
                        s += std::string_view(*bytes);
                    } else {
                        return bytes.error();
                    }
                } else {
                    if (auto c = lex_escape()) {
                        s.push_back(*c);
                    } else {
                        return c.error();
                    }
                }
            } else {
                s.push_back(*m_iter);
                advance();
            }
        }
        advance();
        return json::token(s, line, col);
    }

    auto lex_literal() noexcept -> json::result<json::token> {
        auto line = m_line;
        auto col = m_col;
        auto err = [line, col](std::string_view s) {
            auto e = "Lexer Error: Unknown literal \""s;
            e += s;
            return json::error(line, col, e + '"');
        };
        if (*m_iter == 'n' || *m_iter == 't') {
            if (m_end - m_iter < 4) {
                return err(std::string_view(m_iter, m_end - m_iter));
            }
            auto s = std::string_view(m_iter, 4);
            if (s == "null") {
                m_iter += 4;
                m_col += 4;
                return json::token({}, line, col);
            } else if (s == "true") {
                m_iter += 4;
                m_col += 4;
                return json::token(true, line, col);
            } else {
                return err(s);
            }
        } else {
            if (m_end - m_iter < 5) {
                return err(std::string_view(m_iter, m_end - m_iter));
            }
            auto s = std::string_view(m_iter, 5);
            if (s == "false") {
                m_iter += 5;
                m_col += 5;
                return json::token(false, line, col);
            } else {
                return err(s);
            }
        }
    }

    auto lex_digits() noexcept -> std::string {
        std::string s;
        while (m_iter != m_end && is_int(*m_iter)) {
            s.push_back(*m_iter);
            advance();
        }
        return s;
    }

    auto lex_escape() noexcept -> json::result<char> {
        auto c = *m_iter;
        advance();
        switch (c) {
            case '"':
                return '"';
            case '\\':
                return '\\';
            case 'b':
                return '\b';
            case 'f':
                return '\f';
            case 'n':
                return '\n';
            case 'r':
                return '\r';
            case 't':
                return '\t';
        }
        return json::error(m_line, m_col, "Lexer Error: Invalid escape character "s + '\\' + c);
    }

    auto lex_unicode() noexcept -> json::result<utf8<char>> {
        auto s = std::string();
        for (auto i = 0; i < 4; i++) {
            auto c = *m_iter;
            if (!is_hex(c)) {
                return json::error(m_line, m_col, "Lexer Error: Invalid hex character "s + c);
            }
            s.push_back(c);
            advance();
        }
        return utf8<char>(cast<std::uint16_t>(std::stoi(s, 0, 16)));
    }

private:
    std::string_view::const_iterator m_iter;
    std::string_view::const_iterator m_end;
    std::int32_t m_line = 1;
    std::int32_t m_col = 1;
};

}

auto json::lex(std::string_view s) noexcept -> result <std::vector<token>> {
    return Lexer(s.cbegin(), s.cend()).lex();
}

}