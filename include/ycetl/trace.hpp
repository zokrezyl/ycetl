#pragma once

#include <array>
#include <bit>
#include <cstddef>

#include <algorithm>
#include <utility>

#include <stdexcept>

// #include <fmt/format.h>

#include <ycetl/cstring.hpp>

// simple constexpr execution tracer
// simplest is to attache to the allocator
namespace ycetl {
// clang-format off
//
//
namespace trace {


template <typename T, typename GrowthHandler = void>
class object_pool {
  T** _pool_vector = nullptr; // the dynamically array of array of slots (pool vector)
  T* _current_pool = nullptr;
  std::size_t _pool_vector_growth = 0; // how many pools to grow at once
  std::size_t _pool_vector_size = 0; // current size of the pool vector
  
  std::size_t _pool_size = 0;

  std::size_t _pools_allocated = 0; // how many pools are allocated, if zerro, nothing, if _pools_allocated greater _num_pools, we need to grow the pool vector array
  std::size_t _slots_taken = 0; // slots taken in the current pool, which is current free offset in the current pool, may be passed the last one, as we do lazy allocation
  //
  std::size_t _global_slots_taken = 0; // the total number of slots taken in all pools, this is used to report a global index

// Allocation algorithm:
// As the allocation is lazy, we do not allocate the pool array at the beginning, but only when we need it. Even the pool array is not allocated at the beginning
// If we want to add an object, we first check if there is a pool available, if not, we grow the pool array
// In order to allocate a new pool, first we need to make sure that the pool vector is allocated, if not, we allocate it
// If the pool vector is allocated, we check if there is a free slot in the current pool, if not, we allocate a new pool
// If the pool vector is fool, we grow it and copy all the existing pools to the new pool vector


public:
  constexpr object_pool(): _pool_vector(nullptr), _current_pool(nullptr), _pool_vector_growth(10), _pool_vector_size(0), _pool_size(10), _pools_allocated(0), _slots_taken(0) {

    // we do not do anything, all allocations are lazy
  }

  constexpr void make_sure_pool_vector() {
    if (_pool_vector && _pools_allocated <= _pool_vector_size) {
      return; // pool vector is already allocated
    }
    std::size_t new_pool_vector_size = _pool_vector_size + _pool_vector_growth;
    T** new_pool_vector = new T*[new_pool_vector_size ];
    if (!new_pool_vector) {
      throw std::runtime_error("Failed to allocate memory for new pool array");
      return; // TODO: throw exception
    }
    if (_pool_vector) {
      for (std::size_t i = 0; i < _pool_vector_size; ++i) {
        new_pool_vector[i] = _pool_vector[i]; // copy the existing pools to the new pool vector
      }
      delete[] _pool_vector; // deallocate the old pool vector
    }
    _pool_vector = new_pool_vector; // assign the new pool vector
    _pool_vector_size = new_pool_vector_size; // update the size of the pool vector
    _current_pool = nullptr; // reset the current pool
    // notify the growth handler if it exists
    if constexpr (!std::is_void_v<GrowthHandler>)
        GrowthHandler::vector_grew(static_cast<GrowthHandler*>(this),
                                   _pool_vector_size);    //
  }

  // enforces that there is a pool available, as we do lazy allocation
  constexpr void make_sure_pool() {
    make_sure_pool_vector(); // make sure the pool vector is allocated
    if(_current_pool && _slots_taken < _pool_size) {
      return; // we have a pool and there is a slot available
    }
    _current_pool = new T[_pool_size]; // allocate a new pool
    _pool_vector[_pools_allocated] = _current_pool; // add the new pool to the pool vector
    _pools_allocated++; // increment the number of pools allocated
    _slots_taken = 0; // reset the slots taken in the current pool
    //
    if constexpr (!std::is_void_v<GrowthHandler>)
        GrowthHandler::pool_grew(static_cast<GrowthHandler*>(this),
                                 _pools_allocated - 1);    //
    //
  }

  // enforces that there is a slot available in the pool, as we do lazy allocation
  constexpr void make_sure_slot() {
    make_sure_pool();
  }

  // adds an object to the pool
  constexpr std::size_t add(const T &t) {
    make_sure_slot();
    _current_pool[_slots_taken] = t; // add the object to the current pool
    _slots_taken++; // increment the slots taken in the current pool
    _global_slots_taken++; // increment the global slots taken
    return _global_slots_taken - 1; // return the global index of the object
  }

  // destructor
  constexpr ~object_pool() {
    if(_pool_vector) {
      for (std::size_t i = 0; i < _pool_vector_size; ++i) {
        if (_pool_vector[i]) {
          delete[] _pool_vector[i]; // deallocate the pool
        }
      }
      delete[] _pool_vector; // deallocate the pool vector
    }
  }
};


template<typename T>
class sparse_object_pool : public object_pool<T> {
    using base = object_pool<T>;

    std::uint8_t** _gap_vector = nullptr;      // bitmap array mirrors _pool_vector

    /* grow _gap_vector when base grows ----------------------------------- */
    constexpr void grow_gap_vector()
    {
        std::size_t new_sz = this->_pool_vector_size;           // same size
        auto gv = new std::uint8_t*[new_sz]();                  // zero-init
        for (std::size_t i = 0; i < this->_pools_allocated; ++i)
            gv[i] = _gap_vector ? _gap_vector[i] : nullptr;
        delete[] _gap_vector;
        _gap_vector = gv;
    }

    /* ensure bitmap for current pool exists ------------------------------ */
    constexpr void ensure_gap_pool()
    {
        if (this->_pools_allocated > this->_pool_vector_size) grow_gap_vector();
        if (!_gap_vector[this->_pools_allocated-1])
            _gap_vector[this->_pools_allocated-1] =
                new std::uint8_t[this->_pool_size]{};           // zero-filled
    }

public:
    constexpr sparse_object_pool() = default;

    /* add object and mark gap-bitmap -------------------------------------- */
    constexpr std::size_t add(const T& t)
    {
        std::size_t idx = base::add(t);         // use base allocator
        ensure_gap_pool();
        std::size_t off  = this->_slots_taken - 1;              // 0-based slot
        _gap_vector[this->_pools_allocated-1][off] = 1;         // mark used
        return idx;
    }

    /* read-only iterator that skips zero bytes --------------------------- */
    class const_iterator {
        const sparse_object_pool* sp_;
        std::size_t pool_ = 0, off_ = 0;

        void skip() {
            while (pool_ < sp_->_pools_allocated) {
                auto* bm = sp_->_gap_vector[pool_];
                while (off_ < sp_->_pool_size && bm && bm[off_] == 0) ++off_;
                if (off_ < sp_->_pool_size) return;
                ++pool_; off_ = 0;
            }
        }
        const T* ptr() const { return sp_->_pool_vector[pool_] + off_; }

    public:
        explicit constexpr const_iterator(const sparse_object_pool* p,
                                          std::size_t po = 0, std::size_t of = 0)
            : sp_(p), pool_(po), off_(of) { skip(); }

        constexpr const T& operator*()  const { return *ptr(); }
        constexpr const T* operator->() const { return  ptr(); }
        constexpr const_iterator& operator++() { ++off_; skip(); return *this; }
        friend constexpr bool operator==(const_iterator it, std::default_sentinel_t)
        { return it.pool_ >= it.sp_->_pools_allocated; }
        friend constexpr bool operator!=(const_iterator it, std::default_sentinel_t s)
        { return !(it == s); }
    };

    constexpr const_iterator begin() const { return const_iterator{this}; }
    constexpr std::default_sentinel_t end() const { return {}; }
};



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
constexpr void pack(object_pool<char *> &object_pool, const T &value) {
  //pack(object_pool->allocate(safe_sizeof<T>()), value);
}

struct Trace {
  char *_memory = nullptr;
  std::size_t _size = 0; // size for non-owned memory

  static const std::size_t _number_of_slots =
      1000; // maximum number of slots in the trace
  static const std::size_t _slot_size = 10000; // size of each slot in bytes
  //
  std::size_t _offset =
      0; // offset in memory which is in the current slot if owns memory
  std::array<std::pair<char *, std::size_t>, _number_of_slots>
      _slots; // the second element is the amount consumed in the slot
  bool _owns_memory = false;
  std::size_t _slots_allocated = 0; // not allocated;
  std::size_t _total_consumed = 0; // total amount of memory consumed in the trace

public:
  // lazy initialization
  constexpr Trace()
      : _memory(nullptr), _slots_allocated(0), _size(0), _owns_memory(true) {}
  constexpr Trace(char *memory, std::size_t size)
      : _memory(memory), _size(size) {
    if (_memory == nullptr || _size == 0) {
      // TODO: throw exception
    }
  }

  constexpr char *get_memory(std::size_t _size) {
    if (_size == 0) {
      throw std::runtime_error(
          "Cannot allocate zero size memory for trace");
      return nullptr; // TODO: throw exception
    }
    if (_memory == nullptr) {
      if (not _owns_memory) {
        throw std::runtime_error(
            "Cannot allocate memory for trace, no memory");
        // TODO assert someway
        return nullptr;
      }
    }
    if (_owns_memory && _memory && _offset + _size > _slot_size) {
      // we do not fit in current slot, allocate a new one
      _slots[_slots_allocated - 1].second =
          _offset;       // save the consumed size in the current slot
      _memory = nullptr; // make it null for the next step
    }
    if (_memory == nullptr && _owns_memory) {
      // assert that _slots_allocated is zero at this moment
      _memory = new char[_slot_size];
      _slots[_slots_allocated].first = _memory;
      _slots_allocated = +1;
    }
    if (not _owns_memory && _memory == nullptr) {
      throw std::runtime_error(
          "Cannot allocate memory for trace, no memory");
      // we do not own memory, so we cannot allocate
      return nullptr; // TODO: throw exception
    }

    char *ptr = _memory + _offset;
    _offset += _size;
    _total_consumed += _size;
    return ptr;
  }

  // destructor
  constexpr ~Trace() {
    if (_owns_memory) {
      for (std::size_t i = 0; i < _slots_allocated; ++i) {
        if (_slots[i].first != nullptr) {
          delete[] _slots[i].first;
        }
      }
    }
  }

  constexpr std::size_t length() const {
    return _total_consumed;
    ;
  }

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

};



template <typename T> constexpr void *construct_in_place(char *memory) {
  return new (memory) T();
}

template <typename T> constexpr std::size_t get_serialized_size(const T &v) {
  return safe_sizeof<T>() +
         safe_sizeof<std::size_t>(); /* for the constructor index */
}

template <typename... Args>
constexpr std::size_t get_serialized_size(const Args &...args) {
  return (get_serialized_size(args) + ...);
}

#if 0
// deleteme
class TmpMemory {
  char *_memory = nullptr;
  std::size_t _size = 0;
  std::size_t _offset = 0;

public:
  constexpr TmpMemory(char *memory, std::size_t size)
      : _memory(memory), _size(size), _offset(0) {}

  constexpr char *get_memory(std::size_t size) {
    if (_memory == nullptr || _size == 0 || _offset + size > _size) {
      throw std::runtime_error("Cannot allocate memory for trace");
      return nullptr; // TODO: throw exception
    }
    char *ptr = _memory + _offset;
    _offset += size;
    return ptr;
  }
};

#endif

using ConstructorFn = void *(*)(char *);

struct TraceMessageCollector {
  using ConstructorFn = void *(*)(char *);
  object_pool<ConstructorFn> &constructors;
  object_pool<char *> buffer; // memory pool for the objects
  //
  constexpr TraceMessageCollector(object_pool<ConstructorFn> &constructors)
      : constructors(constructors),
        buffer() {}
};


#if 0
// delme
class Constructors {
  std::array<ConstructorFn, 10000> _constructors;

public:
  constexpr Constructors() : _constructors() {}

  constexpr std::size_t add_constructor(ConstructorFn constructor) {
    // TODO .. make this a map
    for (std::size_t i = 0; i < _constructors.size(); ++i) {
      if (_constructors[i] == constructor) {
        return i; // already exists
      }
      if (_constructors[i] == nullptr) {
        _constructors[i] = constructor;
        return i;
      }
    }
    return _constructors.size(); // no space left
  }
};
#endif

template <typename T>
constexpr static void store_one(TraceMessageCollector &collector, const T &value) {
  using ConstructorFn = void *(*)(char *);

  ConstructorFn constructor = &construct_in_place<T>;

  std::size_t constructor_index = collector.constructors.add(constructor);

  /*
  pack(ctor_slot, constructor_index);

  char *ptr = memory.get_memory(safe_sizeof<T>());
  pack(ptr, value);
  */
}


struct TraceMessageHeader {
  std::size_t size;
  // to store the pointer is superfluous
};

struct TraceMessage {
  char *buffer = nullptr; // array of constructor indices
  size_t _size = 0;
  object_pool<ConstructorFn> &constructors; // object pool for constructors

public:
  constexpr TraceMessage(object_pool<ConstructorFn> &constructors): 
      constructors(constructors), buffer(nullptr), _size(0) {
  }

  template <typename ...Args>
  constexpr void pack(Args &&...args) {

    _size = get_serialized_size(args...) + sizeof(TraceMessageHeader) + sizeof(std::size_t);
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
  using ConstructorFn = void *(*)(char *);
  object_pool<ConstructorFn> _constructors;

public:
  constexpr Tracer()
      : _trace(new Trace()), _control_block(new _SharedPtrControlBlock(1)) {

    if(!_trace) {
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
    /*
    std::size_t total_size = get_serialized_size(args...);

    memory_size += sizeof(TraceMessageHeader) + sizeof(std::size_t);
    char *memory = new char[memory_size]
    */

    TraceMessage msg(_constructors);
    msg.pack(std::forward<Args>(args)...);

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

template <std::size_t N> class FrozenTrace {
  std::array<char, N> _memory;
  std::size_t _consumed = 0;

public:
  constexpr FrozenTrace(Trace *trace) : _memory(), _consumed(trace->length()) {
    auto to_copy = std::min(N, trace->length());
    std::ranges::copy_n(trace->begin(), 
                      to_copy,  
                      _memory.begin());
  }

  constexpr std::size_t length() const noexcept {
    return _consumed;
  }

  using const_iterator = const char *;

  [[nodiscard]] constexpr const_iterator begin() const noexcept
  { return _memory.data(); }

  [[nodiscard]] constexpr const_iterator end()   const noexcept
  { return _memory.data() + _consumed; }

  // (optionally) expose size helpers if you like
  [[nodiscard]] constexpr std::size_t size()  const noexcept { return _consumed; }
  [[nodiscard]] constexpr bool        empty() const noexcept { return _consumed == 0; }

};

// clang-format on
} // namespace trace
} // namespace ycetl
