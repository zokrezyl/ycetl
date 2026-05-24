#pragma once

#include <cstddef>
#include <functional>
#include <utility>

#include <ycetl/dynamic_array.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

namespace pq_detail {

// std::push_heap / std::pop_heap aren't constexpr until C++26 (and even
// then only via specific implementations). Inline siftup / siftdown so
// priority_queue works in constant evaluation on current toolchains.
template <typename C, typename Cmp>
constexpr void sift_up(C &c, std::size_t i, Cmp &cmp)
{
    while (i > 0) {
        std::size_t parent = (i - 1) / 2;
        if (cmp(c[parent], c[i])) {
            auto tmp = std::move(c[parent]);
            c[parent] = std::move(c[i]);
            c[i] = std::move(tmp);
            i = parent;
        } else {
            break;
        }
    }
}

template <typename C, typename Cmp>
constexpr void sift_down(C &c, std::size_t i, std::size_t n, Cmp &cmp)
{
    for (;;) {
        std::size_t l = 2 * i + 1;
        std::size_t r = 2 * i + 2;
        std::size_t best = i;
        if (l < n && cmp(c[best], c[l])) best = l;
        if (r < n && cmp(c[best], c[r])) best = r;
        if (best == i) break;
        auto tmp = std::move(c[best]);
        c[best] = std::move(c[i]);
        c[i] = std::move(tmp);
        i = best;
    }
}

} // namespace pq_detail

// Binary-heap adapter on dynamic_array. Default Compare is std::less<T>,
// matching std::priority_queue (top() returns the largest element).
template <typename T,
          typename Container = dynamic_array<T>,
          typename Compare   = std::less<T>>
class priority_queue {
public:
    using container_type  = Container;
    using value_compare   = Compare;
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T &;
    using const_reference = const T &;

private:
    container_type _c;
    value_compare  _cmp;

public:
    constexpr priority_queue() = default;

    explicit constexpr priority_queue(const Compare &cmp) : _c(), _cmp(cmp) {}

    template <typename Memory>
    explicit constexpr priority_queue(Memory &m) : _c(m), _cmp() {}

    template <typename Memory>
    constexpr priority_queue(Memory &m, const Compare &cmp)
        : _c(m), _cmp(cmp) {}

    constexpr bool      empty() const { return _c.size() == 0; }
    constexpr size_type size()  const { return _c.size(); }

    constexpr const_reference top() const { return _c[0]; }

    constexpr void push(const T &v) {
        _c.push_back(v);
        pq_detail::sift_up(_c, _c.size() - 1, _cmp);
    }
    constexpr void push(T &&v) {
        _c.push_back(std::move(v));
        pq_detail::sift_up(_c, _c.size() - 1, _cmp);
    }

    template <typename... Args>
    constexpr void emplace(Args &&...args) {
        _c.push_back(T(std::forward<Args>(args)...));
        pq_detail::sift_up(_c, _c.size() - 1, _cmp);
    }

    constexpr void pop() {
        size_type n = _c.size();
        if (n <= 1) {
            _c.pop_back();
            return;
        }
        auto tmp = std::move(_c[0]);
        _c[0] = std::move(_c[n - 1]);
        _c[n - 1] = std::move(tmp);
        _c.pop_back();
        pq_detail::sift_down(_c, 0, _c.size(), _cmp);
    }

    constexpr container_type       &container()       noexcept { return _c; }
    constexpr const container_type &container() const noexcept { return _c; }
};

} // namespace ycetl
