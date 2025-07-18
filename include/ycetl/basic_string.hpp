#pragma once

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <utility>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>

namespace ycetl {

template <typename CharT, typename CharTraits, typename Memory,
          typename BackendMode>
class basic_string;

// clang-format off
template <typename CharT, typename CharTraits = std::char_traits<CharT>,
          typename Memory = typename container::container_traits<
                                basic_string, 
                                type_set<CharT, CharTraits>>::default_memory,
          typename BackendMode = container::by_value>
class basic_string;

// clang-format on

// clang-format off
template <typename CharT, 
          typename CharTraits,
          typename Memory,
          typename BackendMode >
// clang-format on
class basic_string
    : public container::container<basic_string, CharT, CharTraits, Memory,
                                  BackendMode>

{
public:
  using value_type = CharT;
  using traits_type = CharTraits;
  using base_type = container::container<basic_string, CharT, CharTraits,
                                         Memory, BackendMode>;
  using traits = typename base_type::traits;

  using storage_unit = typename traits::storage_unit;
  using backend_type = typename traits::backend_type;
  using size_type = typename traits::size_type;

  using reference = typename traits::reference;
  using const_reference = typename traits::const_reference;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;

  using iterator = storage_unit *;
  using const_iterator = const storage_unit *;

private:
  Memory _memory;
  backend_type _backend;

public:
  // Internal constructor
  constexpr basic_string(backend_type &backend, Memory &memory)
      : _memory(memory), _backend(&backend) {}

  /* constructors */
  constexpr basic_string() : _memory(), _backend() {}
  explicit constexpr basic_string(Memory &a) : _memory(&a), _backend() {}

  constexpr basic_string(const CharT *s)
      : _memory(), _backend(_memory, s, CharTraits::length(s)) {}

  constexpr basic_string(const CharT *s, Memory &a)
      : _memory(&a), _backend(_memory, s, CharTraits::length(s)) {}

  constexpr basic_string(std::initializer_list<CharT> il)
      : _memory(), _backend(_memory, il) {}

  constexpr basic_string(std::initializer_list<CharT> il, Memory &a)
      : _memory(&a), _backend(_memory, il) {}

  constexpr basic_string(const basic_string &o)
      : _memory(), _backend(_memory, o._backend) {}

  constexpr basic_string(const basic_string &o, Memory &a)
      : _memory(&a), _backend(_memory, o._backend) {}

  constexpr basic_string(basic_string &&o) noexcept
      : _memory(std::move(o._memory)), _backend(std::move(o._backend)) {}

  constexpr basic_string(basic_string &&o, Memory &a)
      : _memory(&a), _backend() {
    reserve(o.size());
    for (auto &e : *o._backend)
      _backend->push_back(_memory, std::move(e));
    o.clear();
  }

  constexpr ~basic_string() = default;

  /* capacity ----------------------------------------------------------- */
  constexpr bool empty() const noexcept { return size() == 0; }
  constexpr size_type size() const noexcept { return _backend.size(); }
  constexpr size_type capacity() const noexcept { return _backend.capacity(); }

  constexpr void reserve(size_type n) { _backend.reserve(_memory, n); }
  constexpr void resize(size_type n, CharT c = CharT()) {
    _backend.resize(_memory, n, c);
  }
  constexpr void clear() { _backend.clear(); }

  /* element access ----------------------------------------------------- */
  constexpr reference operator[](size_type i) noexcept { return (_backend)[i]; }
  constexpr const_reference operator[](size_type i) const noexcept {
    return (_backend)[i];
  }

  constexpr reference front() noexcept { return (_backend)[0]; }
  constexpr const_reference front() const noexcept { return (_backend)[0]; }

  constexpr const_reference back() noexcept { return (_backend)[size() - 1]; }
  constexpr const_reference back() const noexcept {
    return (_backend)[size() - 1];
  }

  /* operator+= overloads ----------------------------------------------- */

  // Append another basic_string
  constexpr reference operator+=(const basic_string &other) {
    reserve(size() + other.size());
    for (const auto &c : other)
      push_back(c);
    return *this;
  }

  // Append C-style string (const CharT*)
  constexpr reference operator+=(const CharT *s) {
    size_type len = traits_type::length(s);
    reserve(size() + len);
    for (size_type i = 0; i < len; ++i)
      push_back(s[i]);
    return *this;
  }

  constexpr const_pointer data() const noexcept { return _backend.data(); }

  constexpr std::basic_string_view<CharT, CharTraits> view() const noexcept {
    return {data(), size()};
  }

  /* modifiers ---------------------------------------------------------- */
  constexpr void push_back(CharT c) { _backend.push_back(_memory, c); }

  template <class... Args> constexpr reference emplace_back(Args &&...args) {
    return _backend.emplace_back(_memory, std::forward<Args>(args)...);
  }

  constexpr void pop_back() { _backend.pop_back(); }

  /* iterators ---------------------------------------------------------- */
  constexpr iterator begin() noexcept { return _backend.begin(); }
  constexpr iterator end() noexcept { return _backend.end(); }

  constexpr const_iterator begin() const noexcept { return _backend.begin(); }
  constexpr const_iterator end() const noexcept { return _backend.end(); }

  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
};

} // namespace ycetl
