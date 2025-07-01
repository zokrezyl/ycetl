#pragma once
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>

namespace ycetl {
/*───────────────────────────────────────────────────────────
  1.  A single data pool that allocates lazily on first add()
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
      _data = new T[_capacity]; // lazy allocate
    _data[_used] = v;
    return _used++; // old index
  }

  [[nodiscard]] constexpr T *data() noexcept { return _data; }
  [[nodiscard]] constexpr const T *data() const noexcept { return _data; }
};

/*───────────────────────────────────────────────────────────
  2.  Vector that holds pointers to many object_pools<>
───────────────────────────────────────────────────────────*/
template <typename T> class object_pool_vector {
  object_pool<T> **_vector = nullptr; // pointer array
  std::size_t _growth = 8;            // slots to grow at once
  std::size_t _pool_size = 10;        // elements per pool
  std::size_t _capacity = 0;          // current pointer slots
  std::size_t _pools_taken = 0;       // pools in use

  constexpr void grow_vector() {
    std::size_t new_cap = (_capacity ? _capacity : 0) + _growth;
    auto **nv = new object_pool<T> *[new_cap](); // zero-filled
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

  /* create and register a fresh data-pool */
  constexpr object_pool<T> *get_new_pool() {
    if (_pools_taken == _capacity)
      grow_vector();
    auto *p = new object_pool<T>(_pool_size);
    _vector[_pools_taken++] = p;
    return p;
  }

  /* 🔹 minimal getters for derived classes / tests */
  constexpr std::size_t pool_size() const noexcept { return _pool_size; }
  constexpr std::size_t pools_taken() const noexcept { return _pools_taken; }
  constexpr object_pool<T> *const *data() const noexcept { return _vector; }
};

/*───────────────────────────────────────────────────────────
  3.  Dynamic pool facade that hides the vector
───────────────────────────────────────────────────────────*/
template <typename T> class dynamic_object_pool {
protected: // expose pointer vector to subclasses (not to users)
  constexpr object_pool_vector<T> *vec() noexcept { return _pool_vector; }
  constexpr object_pool_vector<T> const *vec() const noexcept {
    return _pool_vector;
  }

private:
  object_pool<T> *_current_pool = nullptr;
  object_pool_vector<T> *_pool_vector = nullptr;

public:
  constexpr dynamic_object_pool() = default;
  constexpr ~dynamic_object_pool() { delete _pool_vector; }

  /* append value, return global index */
  constexpr std::size_t add(const T &v) {
    if (!_pool_vector)
      _pool_vector = new object_pool_vector<T>();
    if (!_current_pool || _current_pool->is_full())
      _current_pool = _pool_vector->get_new_pool();
    return _current_pool->add(v);
  }
};

/*───────────────────────────────────────────────────────────
  4.  Sparse pool that stores a bitmap for “gaps”
───────────────────────────────────────────────────────────*/
template <typename T> class sparse_object_pool : public dynamic_object_pool<T> {
  using base = dynamic_object_pool<T>;

  std::uint8_t **_gap_vector = nullptr; // bitmaps
  std::size_t _gap_cap = 0;             // bitmap pointer slots
  std::size_t _bitmap_pools = 0;        // bitmaps in use

  /* keep bitmaps in sync with data pools */
  void sync_with_base() {
    std::size_t pools = base::vec() ? base::vec()->pools_taken() : 0;
    if (pools <= _bitmap_pools)
      return;

    if (pools > _gap_cap) {
      std::size_t new_cap = pools + 8;
      auto **nv = new std::uint8_t *[new_cap]();
      for (std::size_t i = 0; i < _bitmap_pools; ++i)
        nv[i] = _gap_vector[i];
      delete[] _gap_vector;
      _gap_vector = nv;
      _gap_cap = new_cap;
    }
    /* allocate bitmap for each new data pool */
    for (std::size_t i = _bitmap_pools; i < pools; ++i) {
      _gap_vector[i] = new std::uint8_t[base::vec()->pool_size()]{};
    }
    _bitmap_pools = pools;
  }

public:
  constexpr sparse_object_pool() = default;
  ~sparse_object_pool() {
    for (std::size_t i = 0; i < _bitmap_pools; ++i)
      delete[] _gap_vector[i];
    delete[] _gap_vector;
  }

  /* store value + mark bitmap */
  std::size_t add(const T &v) {
    std::size_t idx = base::add(v); // may allocate pools lazily
    sync_with_base();
    std::size_t ps = base::vec()->pool_size();
    std::size_t blk = idx / ps;
    std::size_t off = idx % ps;
    _gap_vector[blk][off] = 1u;
    return idx;
  }

  /* read-only iterator that skips gaps */
  class const_iterator {
    const sparse_object_pool *sp_;
    std::size_t blk_{0}, off_{0};

    void skip() {
      while (blk_ < sp_->_bitmap_pools) {
        auto *bm = sp_->_gap_vector[blk_];
        auto ps = sp_->base::vec()->pool_size();
        while (off_ < ps && bm[off_] == 0)
          ++off_;
        if (off_ < ps)
          return;
        ++blk_;
        off_ = 0;
      }
    }
    const T *ptr() const { return sp_->base::vec()->data()[blk_] + off_; }

  public:
    /* iterator traits */
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = const T *;
    using reference = const T &;

    explicit const_iterator(const sparse_object_pool *p, std::size_t b = 0,
                            std::size_t o = 0)
        : sp_(p), blk_(b), off_(o) {
      skip();
    }

    reference operator*() const { return *ptr(); }
    const_iterator &operator++() {
      ++off_;
      skip();
      return *this;
    }
    friend bool operator==(const const_iterator &it, std::default_sentinel_t) {
      return it.blk_ >= it.sp_->_bitmap_pools;
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
