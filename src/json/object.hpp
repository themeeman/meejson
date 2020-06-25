#ifndef OBJECT_JSON_HPP
#define OBJECT_JSON_HPP

#include "value.hpp"
#include "box.hpp"
#include <unordered_map>

namespace json {

struct value;

namespace detail {

template <class T>
struct ptr_wrapper {
    explicit ptr_wrapper(const T& val) : m_ptr(val) {}
    auto operator*() noexcept -> T& {
        return m_ptr;
    }

    auto operator*() const noexcept -> const T& {
        return m_ptr;
    }

    auto operator->() noexcept -> T* {
        return std::addressof(m_ptr);
    }

    auto operator->() const noexcept -> const T* {
        return std::addressof(m_ptr);
    }

private:
    T m_ptr;
};

struct key_value_ref {
    key_value_ref(std::pair<const std::string, detail::box<json::value>>* p) : m_pair(p) {}

    auto first() const noexcept -> const std::string& {
        return m_pair->first;
    }

    auto second() noexcept -> value& {
        return *m_pair->second;
    }

    auto second() const noexcept -> const value& {
        return *m_pair->second;
    }

    template<std::size_t I>
    auto& get() & noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    template<std::size_t I>
    const auto& get() const & noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    template<std::size_t I>
    auto&& get() && noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    template<std::size_t I>
    const auto&& get() const && noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    auto operator&() noexcept -> ptr_wrapper<key_value_ref> {
        return ptr_wrapper(*this);
    }

    auto operator&() const noexcept -> ptr_wrapper<key_value_ref> {
        return ptr_wrapper(*this);
    }
    
private:
    std::pair<const std::string, detail::box<value>>* m_pair;
};

struct const_key_value_ref {
    const_key_value_ref(const std::pair<const std::string, detail::box<json::value>>* p) : m_pair(p) {}

    auto first() const noexcept -> const std::string& {
        return m_pair->first;
    }

    auto second() const noexcept -> const value& {
        return *m_pair->second;
    }

    template<std::size_t I>
    auto& get() & noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    template<std::size_t I>
    const auto& get() const & noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    template<std::size_t I>
    auto&& get() && noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    template<std::size_t I>
    const auto&& get() const && noexcept {
        static_assert(I == 0 || I == 1);
        if constexpr (I == 0) return first();
        else return second();
    }

    auto operator&() noexcept -> ptr_wrapper<const_key_value_ref> {
        return ptr_wrapper(*this);
    }

    auto operator&() const noexcept -> ptr_wrapper<const_key_value_ref> {
        return ptr_wrapper(*this);
    }
    
private:
    const std::pair<const std::string, detail::box<value>>* m_pair;
};

template <bool IsConst>
struct object_iterator {
    using base_type = std::unordered_map<std::string, box<value>>::iterator;
    using const_base_type = std::unordered_map<std::string, box<value>>::const_iterator;
    using difference_type = std::conditional_t<IsConst, const_base_type::difference_type, base_type::difference_type>;
    using value_type = std::pair<const std::string, json::value>;
    using reference = std::conditional_t<IsConst, const_key_value_ref, key_value_ref>;
    using pointer = detail::ptr_wrapper<reference>;
    using iterator_category = std::forward_iterator_tag;

    object_iterator(const object_iterator&) noexcept = default;
    explicit object_iterator(base_type it) noexcept : m_iter(it) {}
    explicit object_iterator(const_base_type it) noexcept requires IsConst : m_iter(it) {}

    template <bool B> requires (IsConst || !B)
    object_iterator(const object_iterator<B>& it) noexcept : m_iter(it.m_iter) {}

    auto operator==(const object_iterator& other) const noexcept -> bool {
        return m_iter == other.m_iter;
    }

    auto operator*() const noexcept -> reference {
        return reference(&*m_iter);
    }

    auto operator->() const noexcept -> pointer {
        return &reference(&*m_iter);
    }

    auto operator++(int) noexcept -> object_iterator {
        return object_iterator<IsConst>(m_iter++);
    }

    auto operator++() noexcept -> object_iterator& {
        ++m_iter;
        return *this;
    }

    auto get_base() const noexcept {
        return m_iter;
    }

private:
    std::conditional_t<IsConst, const_base_type, base_type> m_iter;
};
}

using object_iterator = detail::object_iterator<false>;
using const_object_iterator = detail::object_iterator<true>;

struct object {
    using key_type = std::string;
    using mapped_type = value;
    using value_type = std::pair<const std::string, value>;
    using size_type = std::unordered_map<std::string, detail::box<value>>::size_type;
    using difference_type = std::unordered_map<std::string, detail::box<value>>::difference_type;
    using hasher = std::hash<std::string>;
    using key_equal = std::equal_to<std::string>;
    using allocator_type = std::allocator<std::pair<const std::string, detail::box<json::value>>>;
    using reference = detail::key_value_ref;
    using const_reference = detail::const_key_value_ref;
    using pointer = detail::ptr_wrapper<reference>;
    using const_pointer = detail::ptr_wrapper<const_reference>;
    using iterator = object_iterator;
    using const_iterator = const_object_iterator;

    auto operator==(const object&) noexcept -> bool;

    object() noexcept = default;
    explicit object(size_type);
    object(const object&);
    object(object&&) noexcept = default;
    object(std::initializer_list<value_type>);
    
    template <class Iter> 
    object(Iter first, Iter last) {
        for (; first != last; first++) {
            const auto& [k, v] = *first;
            m_obj.try_emplace(k, detail::make_box<value>(v));
        }
    }

    auto operator=(const object&) -> object&;
    auto operator=(std::initializer_list<value_type>) -> object&;

    auto get_allocator() const noexcept -> allocator_type;
    auto begin() noexcept -> iterator;
    auto begin() const noexcept -> const_iterator;
    auto cbegin() const noexcept -> const_iterator;
    auto end() noexcept -> iterator;
    auto end() const noexcept -> const_iterator;
    auto cend() const noexcept -> const_iterator;
    [[nodiscard]] auto empty() const noexcept -> bool;
    auto size() const noexcept -> size_type;
    auto max_size() const noexcept -> size_type;
    void clear() noexcept;
    auto insert(const value_type&) -> std::pair<iterator, bool>;
    auto insert(value_type&&) -> std::pair<iterator, bool>;

    template <class P> requires std::is_constructible_v<value_type, P&&>
    auto insert(P&& value) -> std::pair<iterator, bool> {
        auto p = std::pair(std::forward<P>(p));
        auto p2 = m_obj.insert(std::pair(p.first, *p.second));
        return std::pair(object_iterator(p.first), p.second);
    }

    template <class M> requires std::is_constructible_v<mapped_type, M&&>
    auto insert_or_assign(const key_type& k, M&& obj) -> std::pair<iterator, bool> {
        auto p = m_obj.insert_or_assign(k, detail::make_box<value>(std::forward<M>(obj)));
        return std::pair(object_iterator(p.first), p.second);
    }

    template <class M> requires std::is_constructible_v<mapped_type, M&&>
    auto insert_or_assign(key_type&& k, M&& obj) -> std::pair<iterator, bool> {
        auto p = m_obj.insert_or_assign(std::move(k), detail::make_box<value>(std::forward<M>(obj)));
        return std::pair(object_iterator(p.first), p.second);
    }

    template <class... Args>
    auto emplace(const key_type& k, Args&&... args) -> std::pair<iterator, bool> {
        auto p = m_obj.try_emplace(k, detail::make_box<value>(std::forward<Args>(args)...));
        return std::pair(object_iterator(p.first), p.second);
    }

    template <class... Args>
    auto emplace(key_type&& k, Args&&... args) -> std::pair<iterator, bool> {
        auto p = m_obj.try_emplace(std::move(k), detail::make_box<value>(std::forward<Args>(args)...));
        return std::pair(object_iterator(p.first), p.second);
    }

    auto erase(const_iterator) -> iterator;
    auto erase(const key_type&) -> bool;
    void swap(object&) noexcept;
    void merge(object&);
    void merge(object&&);
    auto at(const key_type&) -> value&;
    auto at(const key_type&) const -> const value&;
    auto operator[](const key_type&) -> value&;
    auto operator[](key_type&&) -> value&;
    auto find(const key_type&) -> iterator;
    auto find(const key_type&) const -> const_iterator;
    auto contains(const key_type&) const -> bool;

    friend auto operator<<(std::ostream&, const object&) noexcept -> std::ostream&;

private:
    std::unordered_map<std::string, detail::box<value>> m_obj;
};

auto operator<<(std::ostream&, const object&) noexcept -> std::ostream&;

}

namespace std {
    template <> struct tuple_size<json::detail::key_value_ref> : std::integral_constant<size_t, 2> { };

    template <> struct tuple_element<0, json::detail::key_value_ref> { using type = std::string; };
    template <> struct tuple_element<1, json::detail::key_value_ref> { using type = json::value; };

    template <> struct tuple_size<json::detail::const_key_value_ref> : std::integral_constant<size_t, 2> { };

    template <> struct tuple_element<0, json::detail::const_key_value_ref> { using type = std::string; };
    template <> struct tuple_element<1, json::detail::const_key_value_ref> { using type = json::value; };
}

#endif