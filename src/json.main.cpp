#include "json/json.hpp"
#include <iostream>
#include <variant>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <string_view>

using namespace std::literals;
using namespace json::literals;

int main() {
    const auto obj = json::array();
    std::cout << obj << '\n';
}