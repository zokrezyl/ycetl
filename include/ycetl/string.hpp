#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <utility>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

/*─────────────────────────────── string ────────────────────────────────*/
// clang-format off
template <typename CharT, 
          typename Traits = std::char_traits<CharT>,
          typename Allocator = typename container::container<CharT>::default_allocator>
// clang-format on
class string : public container::container<CharT, Allocator> {
public:
  using base_type = container::container<CharT, Allocator>;
  using typename base_type::relevant_of;
  using typename base_type::storage_type;
  using typename base_type::storage_unit;
  using allocator_type = Allocator;

  using value_type = CharT;
  using traits_type = Traits;
  using size_type = std::size_t;
  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;

private:
  owned_pointer<Allocator> _alloc_ptr;
  owned_pointer<storage_type> _storage;

public:
  // Internal constructor
  constexpr string(storage_type &storage, Allocator &alloc)
      : _alloc_ptr(&alloc), _storage(&storage) {}

  constexpr Allocator &alloc() { return *_alloc_ptr; }
  constexpr const Allocator &alloc() const { return *_alloc_ptr; }

  /* constructors */
  constexpr string() : _alloc_ptr(), _storage() {}
  explicit constexpr string(Allocator &a) : _alloc_ptr(&a), _storage() {}

  constexpr string(const CharT *s)
      : _alloc_ptr(), _storage(alloc(), s, Traits::length(s)) {}

  constexpr string(const CharT *s, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), s, Traits::length(s)) {}

  constexpr string(std::initializer_list<CharT> il)
      : _alloc_ptr(), _storage(alloc(), il) {}

  constexpr string(std::initializer_list<CharT> il, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), il) {}

  constexpr string(const string &o)
      : _alloc_ptr(), _storage(alloc(), *o._storage) {}

  constexpr string(const string &o, Allocator &a)
      : _alloc_ptr(&a), _storage(alloc(), *o._storage) {}

  constexpr string(string &&o) noexcept
      : _alloc_ptr(std::move(o._alloc_ptr)), _storage(std::move(o._storage)) {}

  constexpr string(string &&o, Allocator &a) : _alloc_ptr(&a), _storage() {
    reserve(o.size());
    for (auto &e : *o._storage)
      _storage->push_back(alloc(), std::move(e));
    o.clear();
  }

  constexpr ~string() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _storage->size(); }
  constexpr size_type capacity() const noexcept { return _storage->capacity(); }

  constexpr void reserve(size_type n) { _storage->reserve(alloc(), n); }
  constexpr void resize(size_type n, CharT c = CharT()) {
    _storage->resize(alloc(), n, c);
  }
  constexpr void clear() { _storage->clear(); }

  /* element access ----------------------------------------------------- */
  constexpr CharT &operator[](size_type i) noexcept { return (*_storage)[i]; }
  constexpr const CharT &operator[](size_type i) const noexcept {
    return (*_storage)[i];
  }

  constexpr CharT &front() noexcept { return (*_storage)[0]; }
  constexpr const CharT &front() const noexcept { return (*_storage)[0]; }

  constexpr CharT &back() noexcept { return (*_storage)[size() - 1]; }
  constexpr const CharT &back() const noexcept {
    return (*_storage)[size() - 1];
  }

  constexpr const CharT *data() const noexcept { return _storage->data(); }

  constexpr std::basic_string_view<CharT, Traits> view() const noexcept {
    return {data(), size()};
  }

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(CharT c) { _storage->push_back(alloc(), c); }

  template <class... Args> constexpr CharT &emplace_back(Args &&...args) {
    return *_storage->emplace_back(alloc(), std::forward<Args>(args)...);
  }

  constexpr void pop_back() { _storage->pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _storage->begin(); }
  constexpr iterator end() noexcept { return _storage->end(); }

  constexpr const_iterator begin() const noexcept { return _storage->begin(); }
  constexpr const_iterator end() const noexcept { return _storage->end(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
};

} // namespace ycetl
