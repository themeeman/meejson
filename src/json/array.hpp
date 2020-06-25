#ifndef ARRAY_JSON_HPP
#define ARRAY_JSON_HPP

#include "value.hpp"
#include <iterator>
#include <memory>
#include <vector>
#include <type_traits>

namespace json {

struct value;

namespace detail {

template <bool IsConst>
struct array_iterator {
    using base_type = std::vector<box<json::value>>::iterator;
    using const_base_type = std::vector<box<json::value>>::const_iterator;
    using difference_type = std::conditional_t<IsConst, const_base_type::difference_type, base_type::difference_type>;
    using value_type = json::value;
    using pointer = std::conditional_t<IsConst, const json::value*, json::value*>;
    using reference = std::conditional_t<IsConst, const json::value&, json::value&>;
    using iterator_category = std::random_access_iterator_tag;

    array_iterator(const array_iterator&) noexcept = default;
    explicit array_iterator(base_type it) noexcept : m_iter(it) {}
    explicit array_iterator(const_base_type it) noexcept requires IsConst : m_iter(it) {}

    template <bool B> requires (IsConst || !B)
    array_iterator(const array_iterator<B>& it) noexcept : m_iter(it.m_iter) {}

    auto operator==(const array_iterator& other) const noexcept -> bool {
        return m_iter == other.m_iter;
    }

    auto operator<(const array_iterator& other) const noexcept -> bool {
        return m_iter < other.m_iter;
    }

    auto operator*() const noexcept -> reference {
        return **m_iter;
    }

    auto operator->() const noexcept -> pointer {
        return &**m_iter;
    }

    auto operator[](difference_type n) const noexcept -> reference {
        return *m_iter[n];
    }

    auto operator++() noexcept -> array_iterator& {
        ++m_iter;
        return *this;
    }

    auto operator++(int) noexcept -> array_iterator {
        return array_iterator<IsConst>(m_iter++);
    }

    auto operator--() noexcept -> array_iterator& {
        --m_iter;
        return *this;
    }

    auto operator--(int) noexcept -> array_iterator {
        return array_iterator<IsConst>(m_iter--);
    }

    auto operator+=(difference_type n) noexcept -> array_iterator& {
        m_iter += n;
        return *this;
    }

    auto operator-=(difference_type n) noexcept -> array_iterator& {
        m_iter -= n;
        return *this;
    }

    template <bool B>
    friend auto operator+(array_iterator<B>, typename array_iterator<B>::difference_type) noexcept -> array_iterator<B>;

    template <bool B>
    friend auto operator+(typename array_iterator<B>::difference_type, array_iterator<B>) noexcept -> array_iterator<B>;

    template <bool B>
    friend auto operator-(array_iterator<B>, typename array_iterator<B>::difference_type) noexcept -> array_iterator<B>;

    template <bool B>
    friend auto operator-(array_iterator<B>, array_iterator<B>) noexcept -> typename array_iterator<B>::difference_type;

    auto get_base() const noexcept -> std::conditional_t<IsConst, const_base_type, base_type> {
        return m_iter;
    }

private:
    std::conditional_t<IsConst, const_base_type, base_type> m_iter;
};

template <bool IsConst>
auto operator+(array_iterator<IsConst> it, typename array_iterator<IsConst>::difference_type n) noexcept -> array_iterator<IsConst> {
    return array_iterator<IsConst>(it.m_iter + n);
}

template <bool IsConst>
auto operator+(typename array_iterator<IsConst>::difference_type n, array_iterator<IsConst> it) noexcept -> array_iterator<IsConst> {
    return array_iterator<IsConst>(n + it.m_iter);
}

template <bool IsConst>
auto operator-(array_iterator<IsConst> it, typename array_iterator<IsConst>::difference_type n) noexcept -> array_iterator<IsConst> {
    return array_iterator<IsConst>(it.m_iter - n);
}

template <bool IsConst>
auto operator-(array_iterator<IsConst> it1, array_iterator<IsConst> it2) noexcept -> typename array_iterator<IsConst>::difference_type {
    return it1.m_iter - it2.m_iter;
}

}

using array_iterator = detail::array_iterator<false>;
using const_array_iterator = detail::array_iterator<true>;

struct array {
    using value_type = value;
    using allocator_type = std::allocator<value>;
    using size_type = std::vector<value>::size_type;
    using difference_type = std::vector<value>::difference_type;
    using reference = value&;
    using const_reference = const value&;
    using pointer = value*;
    using const_pointer = const value*;
    using iterator = array_iterator;
    using const_iterator = const_array_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    
    array() = default;
    array(const array&);
    array(array&&) = default;
    explicit array(size_type);
    array(size_type, const value&);

    template <class Iter>
    array(Iter begin, Iter end) {
        reserve(end - begin);
        for (; begin != end; begin++) {
            push_back(*begin);
        }
    }
    
    array(std::initializer_list<value>);

    auto operator=(const array&) -> array&;
    auto operator=(array&&) -> array& = default;
    auto operator=(std::initializer_list<value>) -> array&;

    auto operator<=>(const array&) const noexcept; 
    auto get_allocator() const -> allocator_type;
    auto at(size_type) -> reference;
    auto at(size_type) const -> const_reference;
    auto operator[](size_type) noexcept -> reference;
    auto operator[](size_type) const noexcept -> const_reference;
    auto front() noexcept -> reference;
    auto front() const noexcept -> const_reference;
    auto back() noexcept -> reference;
    auto back() const noexcept -> const_reference;

    auto begin() noexcept -> iterator;
    auto begin() const noexcept -> const_iterator;
    auto cbegin() const noexcept -> const_iterator;
    auto end() noexcept -> iterator;
    auto end() const noexcept -> const_iterator;
    auto cend() const noexcept -> const_iterator;
    auto rbegin()  noexcept -> reverse_iterator;
    auto rbegin() const noexcept -> const_reverse_iterator;
    auto crbegin() const noexcept -> const_reverse_iterator;
    auto rend() noexcept -> reverse_iterator;
    auto rend() const noexcept -> const_reverse_iterator;
    auto crend() const noexcept -> const_reverse_iterator;

    [[nodiscard]] auto empty() const noexcept -> bool;
    auto size() const noexcept -> size_type;
    auto max_size() const noexcept -> size_type;
    void reserve(size_type);
    auto capacity() const noexcept -> size_type;
    void shrink_to_fit();

    void clear() noexcept;
    auto insert(const_iterator, const value&) -> iterator;
    auto insert(const_iterator, value&&) -> iterator;

    template <class T> requires std::is_constructible_v<value, std::remove_reference_t<T>>
    auto emplace(const_iterator pos, T&& arg) -> iterator {
        return m_arr.emplace(pos, std::forward<T>(arg));
    }

    auto erase(const_iterator) -> iterator;
    void push_back(const value&);
    void push_back(value&&);

    template <class T> requires std::is_constructible_v<value, std::remove_reference_t<T>>
    auto emplace_back(T&& arg) -> reference {
        return *m_arr.emplace_back(detail::make_box<value>(std::forward<T>(arg)));
    }

    void pop_back();
    void resize(size_type);
    void resize(size_type, const value&);
    friend void swap(const value&, const value&) noexcept;

private:
    std::vector<detail::box<value>> m_arr;
};

void swap(const value&, const value&) noexcept;
auto operator<<(std::ostream&, const array&) noexcept -> std::ostream&;
}

#endif