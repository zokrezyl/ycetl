#pragma once
#include <cstddef>
#include <new>

#include <ycetl/dynamic_storage.hpp>
#include <ycetl/static_storage.hpp>

#ifdef ENABLE_LOGGING
#define log(msg)                                                               \
  do {                                                                         \
    std::cerr << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl;      \
  } while (0)
#else
#define log(msg)                                                               \
  do {                                                                         \
  } while (0)
#endif

namespace ycetl {

namespace arena {

template <typename T> class block {
  T *_data = nullptr;
  std::size_t _capacity = 0;
  std::size_t _used = 0;

public:
  constexpr explicit block(std::size_t cap = 10) : _capacity(cap) {}
  constexpr ~block() { delete[] _data; }

  [[nodiscard]] constexpr bool is_full() const noexcept {
    return _data && _used >= _capacity;
  }
  constexpr bool fits(std::size_t size) const noexcept {
    return _data && (_used + size <= _capacity);
  }
  [[nodiscard]] constexpr std::size_t size() const noexcept { return _used; }

  // obsolote
  constexpr std::size_t add(const T &v) {
    if (_data == nullptr)
      _data = new T[_capacity];
    _data[_used] = v;
    return _used++;
  }

  constexpr T *allocate(std::size_t size) {
    if (_data == nullptr)
      _data = new T[_capacity];
    if (_used + size > _capacity) {
      throw std::bad_alloc(); // or handle it differently
    }
    T *ptr = _data + _used;
    _used += size;
    return ptr;
  }

  /* raw access */
  [[nodiscard]] constexpr T *data() noexcept { return _data; }
  [[nodiscard]] constexpr const T *data() const noexcept { return _data; }
};

template <typename T> class block_vector {
  block<T> **_vector = nullptr;
  // std::uint8_t **_gap_bitmaps = nullptr; // not yet implement, not even sure
  // if needed
  std::size_t _growth = 8;
  std::size_t _block_size = 10;
  std::size_t _capacity = 0;     // pointer slots
  std::size_t _blocks_taken = 0; // valid blocks

  constexpr void grow() {
    std::size_t new_cap = (_capacity ? _capacity : 0) + _growth;
    auto **nv = new block<T> *[new_cap]();
    for (std::size_t i = 0; i < _blocks_taken; ++i)
      nv[i] = _vector[i];
    delete[] _vector;
    _vector = nv;
    _capacity = new_cap;
  }

public:
  constexpr block_vector(std::size_t block_size = 10, std::size_t growth = 8)
      : _growth(growth), _block_size(block_size) {}
  constexpr ~block_vector() {
    for (std::size_t i = 0; i < _blocks_taken; ++i)
      delete _vector[i];
    delete[] _vector;

    /*
    for (std::size_t i = 0; i < _blocks_taken; ++i)
      delete[] _gap_bitmaps[i];
    delete[] _gap_bitmaps;
    */
  }

  constexpr block<T> *get_new_block() {
    if (_blocks_taken == _capacity)
      grow();
    auto *p = new block<T>(_block_size);
    _vector[_blocks_taken++] = p;
    return p;
  }

  /* tiny getters for read-only access */
  constexpr std::size_t block_size() const noexcept { return _block_size; }
  constexpr std::size_t blocks_taken() const noexcept { return _blocks_taken; }
  constexpr block<T> *const *data() const noexcept { return _vector; }
};

template <typename T> class arena {
protected:
  /* protected accessor for subclasses */
  constexpr block_vector<T> *vec() noexcept { return _block_vector; }
  constexpr block_vector<T> const *vec() const noexcept {
    return _block_vector;
  }

private:
  block<T> *_current_block = nullptr;
  block_vector<T> *_block_vector = nullptr;
  std::size_t _total_number_of_slots_taken = 0;

public:
  constexpr arena() = default;

  constexpr ~arena() { delete _block_vector; }

  // obsolete
  std::size_t add(const T &t) {
    if (_block_vector == nullptr) {
      // lazy initialization of the block vector
      _block_vector = new block_vector<T>();
    }
    if (_current_block == nullptr) {
      // lazy initialization of the current block
      _current_block = _block_vector->get_new_block();
    }
    if (_current_block->is_full()) {
      _current_block = _block_vector->get_new_block();
    }
    _current_block->add(t);
    return _total_number_of_slots_taken++;
  }

  //
  T *allocate(std::size_t size) {
    if (_block_vector == nullptr) {
      // lazy initialization of the block vector
      _block_vector = new block_vector<T>();
    }
    if (_current_block == nullptr) {
      // lazy initialization of the current block
      _current_block = _block_vector->get_new_block();
    }
    if (_current_block->fits(size)) {
      _current_block = _block_vector->get_new_block();
    }
    // TODO .. not sure what to do with the semantics of return
    // _total_number_of_slots_taken++; probably the template should be split for
    return _current_block->allocate(size);
  }
};
} // namespace arena

} // namespace ycetl
