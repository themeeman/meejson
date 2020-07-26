#include "gtest/gtest.h"
#include "../include/meejson/parser.hpp"

namespace json = mee::json;

using namespace std::literals;
using namespace json::literals;

TEST(parser_test, valid_input) {
    const auto inputs = std::array{
      std::pair("null"sv, json::value()),
      std::pair("true"sv, json::value(true)),
      std::pair("false"sv, json::value(false)),
      std::pair("5"sv, json::value(5)),
      std::pair("-2"sv, json::value(-2)),
      std::pair("1.61803398875"sv, json::value(1.61803398875)),
      std::pair("-1e-3"sv, json::value(-0.001)),
      std::pair("1E3"sv, json::value(1000.0)),
      std::pair("1.5e-2"sv, json::value(0.015)),
      std::pair("1e+010"sv, json::value(10000000000.0)),
      std::pair(R"("Hello World")"sv, json::value("Hello World")),
      std::pair(R"("\" \\ \b \f \n \r \t")"sv, json::value("\" \\ \b \f \n \r \t")),
      std::pair(R"("\u3053\u3093\u306B\u3061\u306F\u4E16\u754C")"sv, json::value("こんにちは世界")),
      std::pair(R"("こんにちは世界")"sv, json::value("こんにちは世界")),
      std::pair(R"([])"sv, json::value(json::array())),
      std::pair(R"([1, null, false, "A", 3.1415])"sv, json::value{json::value(1), json::value(), json::value(false), json::value("A"), json::value(3.1415)}),
      std::pair(R"({})"sv, json::value(json::object())),
      std::pair(R""({"Aaa": 3, "Bbb": 2, "Ccc": 1})""sv, json::value{{"Aaa", 3_value}, {"Bbb", 2_value}, {"Ccc", 1_value}}),
      std::pair("[[[[[[[[[[[[[[[3]]]]]]]]]]]]]]]"sv, json::value{{{{{{{{{{{{{{{3_value}}}}}}}}}}}}}}}),
    };

    for (const auto& [s, res] : inputs) {
        auto val = json::parse(s);
        EXPECT_TRUE(val);
        if (val) {
            EXPECT_EQ(*val, res);
        }
    }
}

TEST(parser_test, invalid_input) {
    const auto inputs = std::array{
        ""sv,
        ".3"sv,
        "3."sv,
        "."sv,
        "010"sv,
        "aaaaaaaaaaa"sv,
        R"(")"sv,
        "fals"sv,
        R"("\ugggg")"sv,
        "["sv,
        "]"sv,
        "{"sv,
        "}"sv,
        "3 5"sv,
        "[1 2 3 4 5]"sv,
        "{1: 2, true: false, {}: []}"sv
    };
    for (const auto s : inputs) {
        EXPECT_FALSE(json::parse(s));
    }
}