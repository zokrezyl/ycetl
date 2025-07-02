#pragma once

#include <array>
#include <bit>
#include <cstddef>

#include <algorithm>
#include <utility>

#include <stdexcept>

// #include <fmt/format.h>

#include <ycetl/cstring.hpp>

#include <ycetl/object_pool.hpp>

// simple constexpr execution tracer
// simplest is to attache to the allocator
namespace ycetl {
// // clang-format off
//
//
namespace trace {

template <typename T> constexpr std::size_t safe_sizeof() {
  if constexpr (std::is_empty_v<T>) {
    return 0; // empty types have no size
  }
  return sizeof(T);
}

template <typename T> constexpr void pack(char *buffer, const T &value) {
  /*
  if constexpr (std::is_pointer_v<T>) {
    pack(buffer, *value);
    return;
  }
  else */
  if constexpr (std::is_empty_v<T>) {
    // do nothing for empty types
    return;
  }
  /*
  else if constexpr (std::is_array_v<T>) {
    for (std::size_t i = 0; i < std::extent_v<T>; ++i) {
      pack(buffer + i, value[i]);
    }
    return;
  }
  else if constexpr (std::is_enum_v<T>) {
    using UnderlyingType = std::underlying_type_t<T>;
    pack(buffer, static_cast<UnderlyingType>(value));
    return;
  }
  */
  auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(value);
  for (std::size_t i = 0; i < sizeof(T); ++i)
    buffer[i] = bytes[i];
}

template <std::size_t N>
constexpr void pack(char *buffer, const char (&value)[N]) {
  std::size_t len = ycetl::strlen(value);
  for (std::size_t i = 0; i < len; ++i) {
    buffer[i] = value[i];
  }
}

template <typename T>
constexpr void pack(object_pool<char *> &dynamic_arena, const T &value) {
  // pack(arena->allocate(safe_sizeof<T>()), value);
}

using ConstructorFn = void *(*)(char *);

template <typename T> constexpr std::size_t get_serialized_size(const T &v) {
  return safe_sizeof<T>() +
         safe_sizeof<std::size_t>(); /* for the constructor index */
}

template <typename... Args>
constexpr std::size_t get_serialized_size(const Args &...args) {
  return (get_serialized_size(args) + ...);
}

struct TraceMessageHeader {
  std::size_t size;
  // to store the pointer is superfluous
};

struct TraceMessageCollector {
  using ConstructorFn = void *(*)(char *);
  ycetl::object_pool<ConstructorFn> &constructors;
  ycetl::object_pool<char *> buffer; // memory pool for the objects
  //
  constexpr TraceMessageCollector(
      ycetl::object_pool<ConstructorFn> &constructors)
      : constructors(constructors), buffer() {}
};

struct TraceMessage {
  char *buffer = nullptr; // array of constructor indices
  size_t _size = 0;
  ycetl::object_pool<ConstructorFn>
      &constructors; // object pool for constructors

public:
  constexpr TraceMessage(ycetl::object_pool<ConstructorFn> &constructors)
      : constructors(constructors), buffer(nullptr), _size(0) {}

  template <typename... Args> constexpr void pack(Args &&...args) {

    _size = get_serialized_size(args...) + sizeof(TraceMessageHeader) +
            sizeof(std::size_t);
    if (_size == 0) {
      throw std::runtime_error("Cannot create TraceMessage with zero size");
      return; // nothing to do
    }

    if (!buffer) {
      throw std::runtime_error("Failed to allocate memory for TraceMessage");
    }

    TraceMessageCollector collector(constructors);
    (store_one(collector, std::forward<Args>(args)), ...);
  }

  // destructor
  constexpr ~TraceMessage() {
    if (buffer) {
      delete[] buffer; // deallocate the buffer
    }
  }
};

template <typename T> constexpr void *construct_in_place(char *memory) {
  return new (memory) T();
}

struct Trace {
private:
  ycetl::object_pool<TraceMessage> *_messages = nullptr;
  // we need to store the constructors separately as we cannot serialize
  // pointers in constexpr
  ycetl::object_pool<ConstructorFn *> *_constructors = nullptr;

public:
  // lazy initialization
  constexpr Trace() {}

  template <typename... Args> constexpr void add(Args &&...args) {}

  // destructor
  constexpr ~Trace() {
    if (_messages) {
      delete _messages; // deallocate the messages
    }
    if (_constructors) {
      delete _constructors; // deallocate the constructors
    }
  }
#if 0
  /* ───── read-only iterator (only Trace* in ctor) ───────── */
  class const_iterator {
    const Trace *t_ = nullptr;
    std::size_t s_ = 0;   // current slot index
    std::size_t off_ = 0; // offset inside that slot

    /* address of current element */
    constexpr const char *ptr() const {
      return t_->_owns_memory
                 ? t_->_slots[s_].first + off_ // owned, per-slot pointer
                 : t_->_memory + off_;         // external contiguous
    }

    /* bytes valid in current slot */
    constexpr std::size_t filled() const {
      return t_->_owns_memory ? t_->_slots[s_].second : t_->_size;
    }

    /* true if iterator already passed the very last byte */
    constexpr bool at_end() const {
      if (!t_->_owns_memory)
        return off_ >= t_->_size;

      if (s_ + 1 < t_->_slots_allocated)
        return false;                       // still before last slot
      return off_ >= t_->_slots[s_].second; // in last slot, past data
    }

  public:
    /* the *only* public constructor */
    explicit constexpr const_iterator(const Trace *t) noexcept : t_(t) {}

    /* iterator traits */
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using reference = const char &;
    using pointer = const char *;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::input_iterator_tag;

    /* dereference */
    constexpr reference operator*() const { return *ptr(); }
    constexpr pointer operator->() const { return ptr(); }

    /* pre-increment */
    constexpr const_iterator &operator++() {
      ++off_;
      if (t_->_owns_memory && off_ >= filled()) {
        ++s_;
        off_ = 0;
      }
      return *this;
    }

    /* post-increment */
    constexpr const_iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    /* equality / inequality with default sentinel */
    friend constexpr bool operator==(const const_iterator &it,
                                     std::default_sentinel_t) noexcept {
      return it.at_end();
    }

    friend constexpr bool operator==(std::default_sentinel_t s,
                                     const const_iterator &it) noexcept {
      return it == s;
    }

    friend constexpr bool operator!=(const const_iterator &it,
                                     std::default_sentinel_t s) noexcept {
      return !(it == s);
    }

    friend constexpr bool operator!=(std::default_sentinel_t s,
                                     const const_iterator &it) noexcept {
      return !(it == s);
    }
  };

  /* begin() / end() hand out only the iterator and the sentinel */
  constexpr const_iterator begin() const noexcept {
    return const_iterator{this};
  }
  constexpr std::default_sentinel_t end() const noexcept { return {}; }

#endif
};

template <typename T>
constexpr static void store_one(TraceMessageCollector &collector,
                                const T &value) {

  ConstructorFn constructor = &construct_in_place<T>;

  std::size_t constructor_index = collector.constructors.add(constructor);

  /*
  pack(ctor_slot, constructor_index);

  char *ptr = memory.get_memory(safe_sizeof<T>());
  pack(ptr, value);
  */
}

struct _SharedPtrControlBlock {
  std::size_t _ref_count;

public:
  constexpr _SharedPtrControlBlock(std::size_t ref_count)
      : _ref_count(ref_count) {}
  constexpr void increment() { ++_ref_count; }
  constexpr void decrement() { --_ref_count; }
  constexpr std::size_t ref_count() const { return _ref_count; }
};

class Tracer {
  Trace *_trace = nullptr;
  _SharedPtrControlBlock *_control_block;

public:
  constexpr Tracer()
      : _trace(new Trace()), _control_block(new _SharedPtrControlBlock(1)) {

    if (!_trace) {
      throw std::runtime_error("Failed to allocate Trace");
    }
  }
  // if a trace is passed, we do not own it
  constexpr Tracer(Trace *trace) : _trace(trace), _control_block(nullptr) {}

  // destructor
  constexpr ~Tracer() {
    if (_control_block) {
      _control_block->decrement();
      if (_control_block->ref_count() == 0) {
        delete _control_block; // deallocate the control block
        delete _trace;         // deallocate the trace
      }
    }
    // else .. we do not own the trace, so we do not delete it
  }

  template <typename... Args> constexpr void trace(Args &&...args) {

    _trace->add(std::forward<Args>(args)...);
    /*
    std::size_t total_size = get_serialized_size(args...);

    memory_size += sizeof(TraceMessageHeader) + sizeof(std::size_t);
    char *memory = new char[memory_size]
    */

    /*
    TraceMessage msg(_constructors);
    msg.pack(std::forward<Args>(args)...);
    */

    /*
    std::size_t total_size = get_serialized_size(args...) +
    get_serialized_size(msg); char *memory = _trace->get_memory(total_size);
    msg.memory = memory;
    msg.size = total_size;
    TmpMemory tmp_memory(memory, total_size);
    (store_one(_constructors, tmp_memory, std::forward<Args>(args)), ...);
    */

    /*
    std::size_t len = ycetl::strlen(value) + 1;
    char *buffer = _memory->get_memory(len);
    pack(buffer, value);
    buffer[len-1] = '\n'; // ensure null termination
    buffer[len] = '\0'; // ensure null termination
    */
  }
  constexpr Trace *trace() { return _trace; }
};

#if 0
template <std::size_t N> class FrozenTrace {
  std::array<char, N> _memory;
  std::size_t _consumed = 0;

public:
  constexpr FrozenTrace(Trace *trace) : _memory(), _consumed(trace->length()) {
    auto to_copy = std::min(N, trace->length());
    std::ranges::copy_n(trace->begin(), to_copy, _memory.begin());
  }

  constexpr std::size_t length() const noexcept { return _consumed; }

  using const_iterator = const char *;

  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return _memory.data();
  }

  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return _memory.data() + _consumed;
  }

  // (optionally) expose size helpers if you like
  [[nodiscard]] constexpr std::size_t size() const noexcept {
    return _consumed;
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return _consumed == 0; }
};
#endif

// clang-format on
} // namespace trace
} // namespace ycetl
