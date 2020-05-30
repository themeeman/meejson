#include "json/json.hpp"
#include <iostream>
#include <variant>
#include <string>
#include <unordered_map>

using namespace std::literals;

int main() {
    //auto v = json::value(json::value::object{
    //    {"Name"s, std::make_unique<json::value>("Tom"s)},
    //    {"Age"s, std::make_unique<json::value>(18)}
    //});
    auto arr = json::value::array();
    arr.emplace_back(std::make_unique<json::value>(3.14));
    arr.emplace_back(std::make_unique<json::value>(1));
    arr.emplace_back(std::make_unique<json::value>(json::value::array()));
    auto v = json::value(std::move(arr));
    std::cout << *(*v.get_if<json::value::array>())[0] << '\n';
}