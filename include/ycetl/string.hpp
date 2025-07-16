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

// clang-format on

// clang-format off
template <typename CharT, 
          typename Traits = std::char_traits<CharT>,
          typename Memory = typename container::container_traits<CharT>::default_memory>
// clang-format on
class string : public container::container<string, CharT, Memory> {
public:
  using base_type = container::container<string, CharT, Memory>;
  using typename base_type::backend_type;
  using typename base_type::relevant_of;
  using typename base_type::storage_unit;
  using memory_type = Memory;

  using value_type = CharT;
  using traits_type = Traits;
  using size_type = std::size_t;
  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;

private:
  owned_pointer<Memory> _memory_ptr;
  owned_pointer<backend_type> _backend;

public:
  // Internal constructor
  constexpr string(backend_type &backend, Memory &memory)
      : _memory_ptr(&memory), _backend(&backend) {}

  constexpr Memory &memory() { return *_memory_ptr; }
  constexpr const Memory &memory() const { return *_memory_ptr; }

  /* constructors */
  constexpr string() : _memory_ptr(), _backend() {}
  explicit constexpr string(Memory &a) : _memory_ptr(&a), _backend() {}

  constexpr string(const CharT *s)
      : _memory_ptr(), _backend(memory(), s, Traits::length(s)) {}

  constexpr string(const CharT *s, Memory &a)
      : _memory_ptr(&a), _backend(memory(), s, Traits::length(s)) {}

  constexpr string(std::initializer_list<CharT> il)
      : _memory_ptr(), _backend(memory(), il) {}

  constexpr string(std::initializer_list<CharT> il, Memory &a)
      : _memory_ptr(&a), _backend(memory(), il) {}

  constexpr string(const string &o)
      : _memory_ptr(), _backend(memory(), *o._backend) {}

  constexpr string(const string &o, Memory &a)
      : _memory_ptr(&a), _backend(memory(), *o._backend) {}

  constexpr string(string &&o) noexcept
      : _memory_ptr(std::move(o._memory_ptr)), _backend(std::move(o._backend)) {
  }

  constexpr string(string &&o, Memory &a) : _memory_ptr(&a), _backend() {
    reserve(o.size());
    for (auto &e : *o._backend)
      _backend->push_back(memory(), std::move(e));
    o.clear();
  }

  constexpr ~string() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _backend->size(); }
  constexpr size_type capacity() const noexcept { return _backend->capacity(); }

  constexpr void reserve(size_type n) { _backend->reserve(memory(), n); }
  constexpr void resize(size_type n, CharT c = CharT()) {
    _backend->resize(memory(), n, c);
  }
  constexpr void clear() { _backend->clear(); }

  /* element access ----------------------------------------------------- */
  constexpr CharT &operator[](size_type i) noexcept { return (*_backend)[i]; }
  constexpr const CharT &operator[](size_type i) const noexcept {
    return (*_backend)[i];
  }

  constexpr CharT &front() noexcept { return (*_backend)[0]; }
  constexpr const CharT &front() const noexcept { return (*_backend)[0]; }

  constexpr CharT &back() noexcept { return (*_backend)[size() - 1]; }
  constexpr const CharT &back() const noexcept {
    return (*_backend)[size() - 1];
  }

  constexpr const CharT *data() const noexcept { return _backend->data(); }

  constexpr std::basic_string_view<CharT, Traits> view() const noexcept {
    return {data(), size()};
  }

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(CharT c) { _backend->push_back(memory(), c); }

  template <class... Args> constexpr CharT &emplace_back(Args &&...args) {
    return *_backend->emplace_back(memory(), std::forward<Args>(args)...);
  }

  constexpr void pop_back() { _backend->pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _backend->begin(); }
  constexpr iterator end() noexcept { return _backend->end(); }

  constexpr const_iterator begin() const noexcept { return _backend->begin(); }
  constexpr const_iterator end() const noexcept { return _backend->end(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
};

} // namespace ycetl
