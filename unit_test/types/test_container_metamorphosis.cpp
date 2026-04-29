
#include <algorithm>
#include <boost/ut.hpp>
#include <memory>
#include <ycetl/memory.hpp>

using namespace ycetl;

using namespace boost::ut;

// Enhanced my_vector explicitly demonstrating memory metamorphosis
// between dynamic and static memory types.
template <typename T, typename Memory = static_memory<T>> class my_vector {
  T *_data;
  std::size_t _size;
  Memory _memory;

public:
  using value_type = T;

  constexpr auto data() const { return _data; }
  constexpr auto size() const { return _size; }

  constexpr explicit my_vector(Memory memory)
      : _data(nullptr), _size(0), _memory(memory) {}

  constexpr explicit my_vector() : _data(nullptr), _size(0), _memory() {}

  constexpr explicit my_vector(std::size_t size) : _size(0), _memory() {
    _data = construct_n<T>(_memory, size);
  }

  // Simple operation: populate vector with 'count' copies of 'value'
  constexpr void populate(std::size_t count, const T &value) {
    _data = construct_n<T>(_memory, count, value);
    _size = count;
  }

  constexpr ~my_vector() {
    if (_data) {
      destroy_n<T>(_memory, _data, _size);
    }
  }
  constexpr auto operator=(const my_vector &other) -> my_vector & {
    if (this != &other) {
      if (_data) {
        destroy_n<T>(_memory, _data, _size);
      }
      _size = other.size();
      _data = allocate<T>(_memory, _size);
      for (std::size_t i = 0; i < _size; ++i) {
        std::construct_at(_data + i, other.data()[i]);
      }
    }
    return *this;
  }

  constexpr const T &operator[](std::size_t index) const {
    return _data[index];
  }

  constexpr my_vector(const my_vector &other) : _memory(), _size(other.size()) {
    _data = allocate<T>(_memory, _size);
    for (std::size_t i = 0; i < _size; ++i) {
      std::construct_at(_data + i, other.data()[i]);
    }
  }
};

template <typename OtherVector>
constexpr my_vector<typename OtherVector::value_type>
create_from(const OtherVector &other) {
  my_vector<typename OtherVector::value_type> result(other.size());
  for (std::size_t i = 0; i < other.size(); ++i) {
    std::construct_at(result.data() + i, other.data()[i]);
  }
  return result;
}

suite memory_metamorphosis_test = [] {
  "dynamic_to_static_copy"_test = [] {
    constexpr auto test = [] {
      using dynamic_memory_t = dynamic_memory<int>;
      using static_memory_t = static_memory<int>;
      dynamic_memory_t dm;
      static_memory_t sm;
      my_vector<int, dynamic_memory_t> dynamic_vector(dm);
      dynamic_vector.populate(3, 42);

      auto static_vector = create_from(dynamic_vector);

      return static_vector;
    };
    constexpr auto result = test();
    static_assert(result.size() == 3);
    expect(result.size() == 3);
  };
};

int main() {}
