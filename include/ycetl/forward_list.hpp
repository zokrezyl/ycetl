#pragma once

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace ycetl {

// Singly-linked list. Like ycetl::list, but with only a forward pointer
// per node — half the per-node overhead, in exchange for: only forward
// iteration, no back()/pop_back(), and insert/erase work "after the
// given iterator" (std::forward_list semantics) since there's no way
// to walk backwards from a node.
template <typename T> class forward_list {
private:
    struct node {
        T     value;
        node *next;

        template <typename... Args>
        constexpr node(node *n, Args &&...args)
            : value(std::forward<Args>(args)...), next(n) {}
    };

    node       *_head = nullptr;
    std::size_t _size = 0;

    constexpr void clear_nodes() {
        node *cur = _head;
        while (cur) {
            node *nx = cur->next;
            delete cur;
            cur = nx;
        }
        _head = nullptr;
        _size = 0;
    }

    constexpr void push_back_internal(const T &v) {
        if (!_head) { _head = new node(nullptr, v); ++_size; return; }
        node *cur = _head;
        while (cur->next) cur = cur->next;
        cur->next = new node(nullptr, v);
        ++_size;
    }

public:
    using value_type = T;
    using size_type  = std::size_t;

    template <bool IsConst> class basic_iter {
        friend class forward_list;
        using node_pointer = std::conditional_t<IsConst, const node *, node *>;
        node_pointer _n = nullptr;

    public:
        using value_type      = T;
        using reference_type  = std::conditional_t<IsConst, const T &, T &>;
        using pointer_type    = std::conditional_t<IsConst, const T *, T *>;

        constexpr basic_iter() = default;
        constexpr basic_iter(node_pointer n) : _n(n) {}

        constexpr operator basic_iter<true>() const
            requires(!IsConst) { basic_iter<true> r; r._n = _n; return r; }

        constexpr reference_type operator*()  const { return _n->value; }
        constexpr pointer_type   operator->() const { return &_n->value; }

        constexpr basic_iter &operator++()    { _n = _n->next; return *this; }
        constexpr basic_iter  operator++(int) { auto t = *this; ++(*this); return t; }

        constexpr bool operator==(const basic_iter &o) const { return _n == o._n; }
        constexpr bool operator!=(const basic_iter &o) const { return _n != o._n; }
    };

    using iterator       = basic_iter<false>;
    using const_iterator = basic_iter<true>;

    constexpr forward_list() = default;

    constexpr forward_list(std::initializer_list<T> il) {
        // Init list is front-to-back; push_front would reverse — walk
        // backwards and prepend instead.
        for (auto it = il.end(); it != il.begin(); ) {
            --it;
            push_front(*it);
        }
    }

    constexpr forward_list(const forward_list &o) {
        node *tail = nullptr;
        for (auto it = o.begin(); it != o.end(); ++it) {
            node *n = new node(nullptr, *it);
            if (tail) tail->next = n; else _head = n;
            tail = n;
            ++_size;
        }
    }

    constexpr forward_list(forward_list &&o) noexcept
        : _head(o._head), _size(o._size) { o._head = nullptr; o._size = 0; }

    constexpr forward_list &operator=(const forward_list &o) {
        if (this != &o) {
            clear_nodes();
            for (auto it = o.begin(); it != o.end(); ++it) push_back_internal(*it);
        }
        return *this;
    }

    constexpr forward_list &operator=(forward_list &&o) noexcept {
        if (this != &o) {
            clear_nodes();
            _head = o._head; _size = o._size;
            o._head = nullptr; o._size = 0;
        }
        return *this;
    }

    constexpr ~forward_list() { clear_nodes(); }

    constexpr size_type size()  const noexcept { return _size; }
    constexpr bool      empty() const noexcept { return _size == 0; }
    constexpr void      clear()                 { clear_nodes(); }

    constexpr T       &front()       { return _head->value; }
    constexpr const T &front() const { return _head->value; }

    constexpr iterator       begin()       noexcept { return iterator(_head); }
    constexpr iterator       end()         noexcept { return iterator(nullptr); }
    constexpr const_iterator begin() const noexcept { return const_iterator(_head); }
    constexpr const_iterator end()   const noexcept { return const_iterator(nullptr); }

    constexpr void push_front(const T &v) { emplace_front(v); }
    constexpr void push_front(T &&v)      { emplace_front(std::move(v)); }

    template <typename... Args>
    constexpr T &emplace_front(Args &&...args) {
        _head = new node(_head, std::forward<Args>(args)...);
        ++_size;
        return _head->value;
    }

    constexpr void pop_front() {
        node *h = _head;
        _head = h->next;
        delete h;
        --_size;
    }

    // std::forward_list-style "insert AFTER pos" — can't splice in
    // front of a node without a back-pointer.
    constexpr iterator insert_after(iterator pos, const T &v) {
        node *fresh = new node(pos._n->next, v);
        pos._n->next = fresh;
        ++_size;
        return iterator(fresh);
    }

    constexpr iterator erase_after(iterator pos) {
        node *dead = pos._n->next;
        if (!dead) return end();
        pos._n->next = dead->next;
        delete dead;
        --_size;
        return iterator(pos._n->next);
    }
};

} // namespace ycetl
