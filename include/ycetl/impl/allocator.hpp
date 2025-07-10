#pragma once

namespace ycetl {

namespace allocator {

template <typename T, typename MultitypeStorage> class allocator {
private:
  MultitypeStorage &_storage;

public:
  template <typename U, typename MS> friend class allocator;

  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_move_assignment = std::true_type;

  template <typename U> struct rebind {
    using other = allocator<U, MultitypeStorage>;
  };

  constexpr allocator(MultitypeStorage &storage) noexcept : _storage(storage) {}

  template <typename U>
  constexpr allocator(const allocator<U, MultitypeStorage> &other) noexcept
      : _storage(other._storage) {}

  [[nodiscard]] constexpr T *allocate(size_type n) {
    return _storage.template allocate<T>(n);
  }

  constexpr void deallocate(T *p, size_type n) noexcept {
    _storage.template deallocate<T>(p, n);
  }

  constexpr MultitypeStorage &get_storage() const noexcept { return _storage; }

  template <typename U>
  constexpr bool
  operator==(const allocator<U, MultitypeStorage> &other) const noexcept {
    return &_storage == &other._storage;
  }

  template <typename U>
  constexpr bool
  operator!=(const allocator<U, MultitypeStorage> &other) const noexcept {
    return !(*this == other);
  }
};

} // namespace allocator
} // namespace ycetl
