#ifndef JSON_TYPE_LIST_HPP
#define JSON_TYPE_LIST_HPP

#include <concepts>

namespace json {

template <class... Ts>
struct type_list {};

namespace detail {
template <class, class>
struct in_type_list_impl;

template <class T, class... Ts>
struct in_type_list_impl<T, type_list<Ts...>> {
    constexpr static auto value = (std::same_as<T, Ts> || ...);
};

template <class T, class... Ts>
struct all_same_impl {
    constexpr static auto value = (std::is_same_v<T, Ts> && ...);
};

template <class, class>
struct visitable_impl;

template <class F, class... Ts>
struct visitable_impl<F, type_list<Ts...>> {
    constexpr static auto value = (std::is_invocable_v<F, Ts> && ...)
                                  && all_same_impl<std::invoke_result_t<F, Ts>...>::value;
};

template <class, class, class>
struct visitable2_impl;

template <class F, class... Ts, class... Us>
struct visitable2_impl<F, type_list<Ts...>, type_list<Us...>> {
    constexpr static auto value = (std::is_invocable_v<F, Ts, Us> && ...)
                                  && all_same_impl<std::invoke_result_t<F, Ts, Us>...>::value;
};


}

template <class... Ts>
concept all_same = detail::all_same_impl<Ts...>::value;

template <class F, class T>
concept visitable = detail::visitable_impl<F, T>::value;

template <class F, class T, class U>
concept visitable2 = detail::visitable2_impl<F, T, U>::value;

template <class T, class V>
concept in_type_list = detail::in_type_list_impl<T, V>::value;

}


#endif