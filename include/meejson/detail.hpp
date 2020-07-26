#ifndef JSON_DETAIL_HPP
#define JSON_DETAIL_HPP

#include <type_traits>

namespace mee::json::detail {

template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;
}

#endif
