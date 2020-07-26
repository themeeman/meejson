#ifndef JSON_EXCEPT_HPP
#define JSON_EXCEPT_HPP

#include <exception>
#include <string>
#include <variant>
#include <string_view>

#include "type_list.hpp"

namespace json {

struct invalid_operation : std::exception {
    invalid_operation() = delete;
    invalid_operation(std::string_view v, std::string_view op);
    invalid_operation(std::string_view lhs, std::string_view rhs, std::string_view op);

    [[nodiscard]] auto what() const noexcept -> const char* override;
private:
    std::string msg;
};

struct error {
    std::int32_t line;
    std::int32_t col;
    std::string msg;

    [[nodiscard]] auto what() const noexcept -> std::string;
};

struct invalid_access : std::exception {
    invalid_access() = delete;
    invalid_access(std::string_view item);

    [[nodiscard]] auto what() const noexcept -> const char* override;
private:
    std::string msg;
};

template <class T>
struct result {
    explicit(false) result(const T& val) : m_var(val) {}
    explicit(false) result(const error& err) : m_var(err) {}

    explicit operator bool() noexcept {
        return std::holds_alternative<T>(m_var);
    }

    auto operator*() -> T& {
        return std::get<T>(m_var);
    }

    auto operator*() const -> const T& {
        return std::get<T>(m_var);
    }

    auto operator->() -> T* {
        return &std::get<T>(m_var);
    }

    auto operator->() const -> const T* {
        return &std::get<T>(m_var);
    }

    auto error() -> json::error& {
        return std::get<json::error>(m_var);
    }

    [[nodiscard]] auto error() const -> const json::error& {
        return std::get<json::error>(m_var);
    }

    template <class F> requires visitable<F, type_list<T, json::error>>
    auto visit(F&& f) const {
        return std::visit(std::forward<F>(f), m_var);
    }

private:
    std::variant<T, json::error> m_var;
};

}

#endif
