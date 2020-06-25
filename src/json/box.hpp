#ifndef JSON_BOX_HPP
#define JSON_BOX_HPP

#include <memory>
#include <utility>

namespace json {

namespace detail {

template <class T>
struct box {
    constexpr box(std::nullptr_t = nullptr) noexcept {}
    box(const box&) = delete;
    box(box&&) noexcept = default;

    template <class U>
    box(box<U>&& b) : m_ptr(std::move(b)) {}

    auto operator=(const box&) -> box& = delete;
    auto operator=(box&&) -> box& = default;

    ~box() noexcept = default;

    auto operator*() noexcept -> T& {
        return *m_ptr;
    }

    auto operator*() const noexcept -> const T& {
        return *m_ptr;
    }

    auto operator->() noexcept -> T* {
        return &*m_ptr;
    }

    auto operator->() const noexcept -> const T* {
        return &*m_ptr;
    }

    explicit operator bool() const noexcept {
        return bool(m_ptr);
    }

    template <class U, class... Args>
    friend auto make_box(Args&&... args) -> box<U>;
private:
    std::unique_ptr<T> m_ptr;
};

template <class T, class... Args>
auto make_box(Args&&... args) -> box<T> {
    auto b = box<T>();
    b.m_ptr = std::make_unique<T>(std::forward<Args>(args)...);
    return b;
}

}
}

#endif