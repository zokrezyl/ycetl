#pragma once
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>

namespace ycetl {

/*───────────────────────────────────────────────────────────
  1.  Single data pool (lazy allocation on first add)
───────────────────────────────────────────────────────────*/
template <typename T> class object_pool {
  T *_data = nullptr;
  std::size_t _capacity = 0;
  std::size_t _used = 0;

public:
  constexpr explicit object_pool(std::size_t cap = 10) : _capacity(cap) {}
  constexpr ~object_pool() { delete[] _data; }

  [[nodiscard]] constexpr bool is_full() const noexcept {
    return _data && _used >= _capacity;
  }
  [[nodiscard]] constexpr std::size_t size() const noexcept { return _used; }

  constexpr std::size_t add(const T &v) {
    if (_data == nullptr)
      _data = new T[_capacity];
    _data[_used] = v;
    return _used++;
  }

  /* raw access */
  [[nodiscard]] constexpr T *data() noexcept { return _data; }
  [[nodiscard]] constexpr const T *data() const noexcept { return _data; }
};

/*───────────────────────────────────────────────────────────
  2.  Vector that owns many object_pool<T>*
───────────────────────────────────────────────────────────*/
template <typename T> class object_pool_vector {
  object_pool<T> **_vector = nullptr;
  std::size_t _growth = 8;
  std::size_t _pool_size = 10;
  std::size_t _capacity = 0;    // pointer slots
  std::size_t _pools_taken = 0; // valid pools

  constexpr void grow() {
    std::size_t new_cap = (_capacity ? _capacity : 0) + _growth;
    auto **nv = new object_pool<T> *[new_cap]();
    for (std::size_t i = 0; i < _pools_taken; ++i)
      nv[i] = _vector[i];
    delete[] _vector;
    _vector = nv;
    _capacity = new_cap;
  }

public:
  constexpr object_pool_vector(std::size_t pool_size = 10,
                               std::size_t growth = 8)
      : _growth(growth), _pool_size(pool_size) {}
  constexpr ~object_pool_vector() {
    for (std::size_t i = 0; i < _pools_taken; ++i)
      delete _vector[i];
    delete[] _vector;
  }

  constexpr object_pool<T> *get_new_pool() {
    if (_pools_taken == _capacity)
      grow();
    auto *p = new object_pool<T>(_pool_size);
    _vector[_pools_taken++] = p;
    return p;
  }

  /* tiny getters for read-only access */
  constexpr std::size_t pool_size() const noexcept { return _pool_size; }
  constexpr std::size_t pools_taken() const noexcept { return _pools_taken; }
  constexpr object_pool<T> *const *data() const noexcept { return _vector; }
};

/*───────────────────────────────────────────────────────────
  3.  Facade that hides the vector
───────────────────────────────────────────────────────────*/
template <typename T> class dynamic_object_pool {
protected:
  /* protected accessor for subclasses */
  constexpr object_pool_vector<T> *vec() noexcept { return _pool_vector; }
  constexpr object_pool_vector<T> const *vec() const noexcept {
    return _pool_vector;
  }

private:
  object_pool<T> *_current_pool = nullptr;
  object_pool_vector<T> *_pool_vector = nullptr;
  std::size_t _total_number_of_slots_taken = 0;

public:
  constexpr dynamic_object_pool() = default;

  constexpr ~dynamic_object_pool() { delete _pool_vector; }

  std::size_t add(const T &t) {
    if (_pool_vector == nullptr) {
      // lazy initialization of the pool vector
      _pool_vector = new object_pool_vector<T>();
    }
    if (_current_pool == nullptr) {
      // lazy initialization of the current pool
      _current_pool = _pool_vector->get_new_pool();
    }
    if (_current_pool->is_full()) {
      _current_pool = _pool_vector->get_new_pool();
    }
    _current_pool->add(t);
    return _total_number_of_slots_taken++;
  }
};

/*───────────────────────────────────────────────────────────
  4.  Sparse pool (bitmap of gaps)   — FIXED VERSION
───────────────────────────────────────────────────────────*/
template <typename T> class sparse_object_pool : public dynamic_object_pool<T> {
  using base = dynamic_object_pool<T>;

  std::uint8_t **_bitmaps = nullptr; // per-pool bitmaps
  std::size_t _bmp_cap = 0;          // slots in _bitmaps
  std::size_t _bmp_used = 0;         // bitmaps actually created

  /* ensure we have one bitmap for each data-pool */
  void sync() {
    std::size_t pools = base::vec() ? base::vec()->pools_taken() : 0;
    if (pools <= _bmp_used)
      return;

    /* grow pointer array if necessary */
    if (pools > _bmp_cap) {
      std::size_t new_cap = pools + 8;
      auto **nv = new std::uint8_t *[new_cap]();
      for (std::size_t i = 0; i < _bmp_used; ++i)
        nv[i] = _bitmaps[i];
      delete[] _bitmaps;
      _bitmaps = nv;
      _bmp_cap = new_cap;
    }

    /* allocate zero-filled bitmap for every new pool */
    std::size_t ps = base::vec()->pool_size();
    for (std::size_t i = _bmp_used; i < pools; ++i)
      _bitmaps[i] = new std::uint8_t[ps]();
    _bmp_used = pools;
  }

public:
  constexpr sparse_object_pool() = default;
  ~sparse_object_pool() {
    for (std::size_t i = 0; i < _bmp_used; ++i)
      delete[] _bitmaps[i];
    delete[] _bitmaps;
  }

  std::size_t add(const T &v) {
    std::size_t idx = base::add(v); // may allocate pools
    sync();
    std::size_t ps = base::vec()->pool_size();
    _bitmaps[idx / ps][idx % ps] = 1;
    return idx;
  }

  /* ─── read-only iterator skipping gaps ─────────────────── */
  class const_iterator {
    const sparse_object_pool *sp_{};
    std::size_t blk_{0}, off_{0};

    void skip() {
      while (blk_ < sp_->_bmp_used) {
        auto *bm = sp_->_bitmaps[blk_];
        std::size_t ps = sp_->base::vec()->pool_size();
        while (off_ < ps && bm[off_] == 0)
          ++off_;
        if (off_ < ps)
          return;
        ++blk_;
        off_ = 0;
      }
    }
    const T *ptr() const {
      return sp_->base::vec()->data()[blk_]->data() + off_;
    }

  public:
    /* iterator traits for <iterator>/<algorithm> */
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;

    /* default & value constructors */
    const_iterator() = default;
    explicit const_iterator(const sparse_object_pool *p, std::size_t b = 0,
                            std::size_t o = 0)
        : sp_(p), blk_(b), off_(o) {
      skip();
    }

    /* basic ops */
    reference operator*() const { return *ptr(); }
    pointer operator->() const { return ptr(); }

    const_iterator &operator++() {
      ++off_;
      skip();
      return *this;
    }
    const_iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const const_iterator &it, std::default_sentinel_t) {
      return it.blk_ >= it.sp_->_bmp_used;
    }
    friend bool operator!=(const const_iterator &it,
                           std::default_sentinel_t s) {
      return !(it == s);
    }
  };

  const_iterator begin() const { return const_iterator{this}; }
  std::default_sentinel_t end() const { return {}; }
};

} // namespace ycetl
