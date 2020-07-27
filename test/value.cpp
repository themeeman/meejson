#include "gtest/gtest.h"
#include "../include/meejson/value.hpp"

namespace json = mee::json;

using namespace std::literals;
using namespace json::literals;

#define EXPECT_UNORDERED(val1, val2) EXPECT_EQ((val1) <=> (val2), std::partial_ordering::unordered)
#define ASSERT_UNORDERED(val1, val2) ASSERT_EQ((val1) <=> (val2), std::partial_ordering::unordered)

template <auto V>
struct constant {
    constexpr static auto value = V;
};

template <class F, class Tuple>
constexpr void for_each_tuple(F&& f, Tuple&& tuple) noexcept {
    constexpr auto size = std::tuple_size_v<std::remove_cvref_t<Tuple>>;
    [&f, &tuple]<auto... Is>(std::index_sequence<Is...>){
        (f(std::get<Is>(tuple)), ...);
    }(std::make_index_sequence<size>());
}

template <class F, class Tuple1, class Tuple2>
constexpr void for_each_tuple(F&& f, Tuple1&& tuple1, Tuple2&& tuple2) noexcept {
    for_each_tuple([&f, &tuple2](const auto& val1) {
        for_each_tuple([&f, &val1](const auto& val2) {
            f(val1, val2);
        }, tuple2);
    }, tuple1);
}

template <class T, class U, class V>
struct triple {
    T first;
    U second;
    V third;

    triple() = default;
    triple(const T& t, const U& u, const V& v) : first(t), second(u), third(v) {}
};

template <class T, class U, class V>
triple(T, U, V) -> triple<T, U, V>;

TEST(value_test, comparison) {
    enum class ord { EQ, NE, LT, GT, LE, GE, UNORDERED };
    const auto inputs = std::tuple(
        triple(2, 2, ord::EQ),
        triple(2, 3, ord::NE),
        triple(json::null(), json::null(), ord::EQ),
        triple(3.5, 3.5, ord::EQ),
        triple(3, 3.0, ord::EQ),
        triple("Hello World", "Hello World", ord::EQ),
        triple("Hello World", "Hello World"sv, ord::EQ),
        triple("", "", ord::EQ),
        triple(json::array{1_value, 2_value}, json::array{1_value, 2_value}, ord::EQ),
        triple(json::array{2_value, 1_value}, json::array{1_value, 2_value}, ord::NE),
        triple(json::array(), json::array(), ord::EQ),
        triple(json::object(), json::object(), ord::EQ),
        triple(json::object{{"a", 1_value}, {"b", 2_value}}, json::object{{"b", 2_value}, {"a", 1_value}}, ord::EQ),
        triple(json::object{{"a", 5_value}, {"b", 2_value}}, json::object{{"b", 2_value}, {"a", 1_value}}, ord::NE),

        triple(2, 3, ord::LT),
        triple(2.71828, 3.14159, ord::LT),
        triple("bcd", "abc", ord::GT),
        triple(2, 2, ord::GE),
        triple(2, 2, ord::LE),
        triple(2, 3.5, ord::LT),
        triple(15.5, -1, ord::GT),
        triple(true, false, ord::GT),
        triple(json::array{1_value, 2_value}, json::array{1_value, 10_value}, ord::LT),
        triple(json::array{1_value, 2_value}, json::array{0_value, "AAA"_value}, ord::GT),
        triple(2, "Hello", ord::UNORDERED),
        triple(3.0, true, ord::UNORDERED),
        triple(json::object{{"Two", 2_value}}, json::array{1_value, "AAA"_value}, ord::UNORDERED),
        triple(2, json::array{1_value, 2_value}, ord::UNORDERED)
    );

#define CASE(O) case ord::O: \
    EXPECT_##O(json::value(lhs), json::value(rhs)); \
    EXPECT_##O(lhs, json::value(rhs)); \
    EXPECT_##O(json::value(lhs), rhs); \
    break;

    for_each_tuple([](const auto& input){
        const auto& [lhs, rhs, o] = input;
        switch (o) {
            CASE(EQ)
            CASE(NE)
            CASE(LT)
            CASE(GT)
            CASE(LE)
            CASE(GE)
            CASE(UNORDERED)
        }
    }, inputs);

#undef CASE
}

TEST(value_test, arithmetic) {
    const auto inputs = std::tuple(
        std::pair(1, 2),
        std::pair(1, 2.5),
        std::pair(1.5, 2.5),
        std::pair(3, "3"),
        std::pair("3", "2"),
        std::pair(3, true),
        std::pair(true, false),
        std::pair(json::array{1_value, 2_value, 3_value}, json::value(2)),
        std::pair(json::null(), json::null()),
        std::pair(json::object{{"a", 1_value}, {"b", 2_value}}, json::object{{"b", 2_value}})
    );

    constexpr auto funcs = std::tuple(
        std::pair(std::plus(), [](auto& lhs, const auto& rhs) -> auto& { return lhs += rhs; }),
        std::pair(std::minus(), [](auto& lhs, const auto& rhs) -> auto& { return lhs -= rhs; }),
        std::pair(std::multiplies(), [](auto& lhs, const auto& rhs) -> auto& { return lhs *= rhs; }),
        std::pair(std::divides(), [](auto& lhs, const auto& rhs) -> auto& { return lhs /= rhs; }),
        std::pair(std::modulus(), [](auto& lhs, const auto& rhs) -> auto& { return lhs %= rhs; }),
        std::pair([](const auto& x, const auto& y) { void(y + 1); return x + 1; }, [](auto& x, const auto& y) -> auto& {
            auto copy = y;
            void(++copy);
            return ++x;
        }),
        std::pair([](const auto& x, const auto& y) { void(y + 1); return x - 1; }, [](auto& x, const auto& y) -> auto& {
            auto copy = y;
            void(--copy);
            return --x;
        })
    );

    for_each_tuple([](const auto& i, const auto& f) {
        const auto& [lhs, rhs] = i;
        const auto& [op, assign] = f;
        constexpr auto is_modulus = std::same_as<std::remove_cvref_t<decltype(op)>, std::modulus<>>;
        constexpr auto lhs_int = json::integral<std::remove_cvref_t<decltype(lhs)>>;
        constexpr auto rhs_int = json::integral<std::remove_cvref_t<decltype(rhs)>>;

        constexpr auto lhs_arith = json::arithmetic<std::remove_cvref_t<decltype(lhs)>>;
        constexpr auto rhs_arith = json::arithmetic<std::remove_cvref_t<decltype(rhs)>>;
        if constexpr ((is_modulus && lhs_int && rhs_int) || (!is_modulus && lhs_arith && rhs_arith)) {
            EXPECT_EQ(op(json::value(lhs), json::value(rhs)), json::value(op(lhs, rhs)));
            EXPECT_EQ(op(json::value(lhs), rhs), json::value(op(lhs, rhs)));
            EXPECT_EQ(op(lhs, json::value(rhs)), json::value(op(lhs, rhs)));

            EXPECT_EQ(op(json::value(rhs), json::value(lhs)), json::value(op(rhs, lhs)));
            EXPECT_EQ(op(json::value(rhs), lhs), json::value(op(rhs, lhs)));
            EXPECT_EQ(op(rhs, json::value(lhs)), json::value(op(rhs, lhs)));

            auto val1 = json::value(lhs);
            auto val2 = json::value(rhs);
            EXPECT_EQ(&assign(val1, val2), &val1);
            EXPECT_EQ(val1, op(json::value(lhs), json::value(rhs)));
            val1 = json::value(lhs);

            EXPECT_EQ(&assign(val1, rhs), &val1);
            EXPECT_EQ(val1, op(json::value(lhs), rhs));
            val1 = json::value(lhs);

            EXPECT_EQ(&assign(val2, val1), &val2);
            EXPECT_EQ(val2, op(json::value(rhs), json::value(lhs)));
            val2 = json::value(rhs);

            EXPECT_EQ(&assign(val2, lhs), &val2);
            EXPECT_EQ(val2, op(json::value(rhs), lhs));
            val2 = json::value(rhs);
        } else {
            auto v1 = json::value(lhs);
            auto v2 = json::value(rhs);
            EXPECT_ANY_THROW(op(v1, v2));
            EXPECT_ANY_THROW(op(v2, v1));
            EXPECT_ANY_THROW(assign(v1, v2));
            EXPECT_ANY_THROW(assign(v2, v1));
        }
    }, inputs, funcs);

    for_each_tuple([](const auto& i) {
        const auto& [lhs, rhs] = i;
        auto v1 = json::value(lhs);
        auto v2 = json::value(rhs);
        EXPECT_EQ(&(v1 = v2), &v1);
        EXPECT_EQ(v1, json::value(rhs));
        v1 = json::value(lhs);
        EXPECT_EQ(&(v1 = rhs), &v1);
        EXPECT_EQ(v1, json::value(rhs));
        v1 = json::value(lhs);
        EXPECT_EQ(&(v2 = v1), &v2);
        EXPECT_EQ(v2, json::value(lhs));
        v2 = json::value(rhs);
        EXPECT_EQ(&(v2 = lhs), &v2);
        EXPECT_EQ(v2, json::value(lhs));

    }, inputs);

    EXPECT_EQ(-json::value(1), json::value(-1));
    EXPECT_EQ(+json::value(1), json::value(1));
    EXPECT_EQ(-json::value(1.5), json::value(-1.5));
    EXPECT_EQ(+json::value(1.5), json::value(1.5));

    EXPECT_ANY_THROW(+json::value("A"));
    EXPECT_ANY_THROW(-json::value("A"));
}

TEST(value_test, assignment) {


}