#include "../include/meejson/except.hpp"
#include <string_view>
#include <sstream>

json::invalid_operation::invalid_operation(std::string_view lhs, std::string_view rhs, std::string_view op) {
    std::stringstream s;
    s << "Invalid Operation \"" << op << "\" for types \"" << lhs << "\" and \"" << rhs << "\"";
    msg = s.str();
}

auto json::invalid_operation::what() const noexcept -> const char* {
    return msg.c_str();
}

json::invalid_operation::invalid_operation(std::string_view v, std::string_view op) {
    std::stringstream s;
    s << "Invalid Operation \"" << op << "\" for type \"" << v << "\"";
    msg = s.str();
}

auto json::error::what() const noexcept -> std::string {
    std::stringstream ss;
    ss << msg << " (" << line << ':' << col << ')';
    return ss.str();
}

json::invalid_access::invalid_access(std::string_view item) {
    std::stringstream s;
    s << "Invalid Access with key \"" << item << "\"";
    msg = s.str();
}

auto json::invalid_access::what() const noexcept -> const char* {
    return msg.c_str();
}

