#pragma once

namespace yectl {

class string_view {
  const char *data = nullptr;
  std::size_t size = 0;

public:
  constexpr string_view() = default;
  constexpr string_view(const char *data, std::size_t size)
      : data(data), size(size) {}
  constexpr const char *c_str() const { return data; }
  constexpr std::size_t length() const { return size; }
  constexpr std::size_t size_bytes() const { return size; }
  constexpr bool empty() const { return size == 0; }
  constexpr operator std::string_view() const {
    return std::string_view(data, size);
  }

  template <typename Allocator, std::size_t N>
  constexpr string_view(const string<Allocator, N> &str)
      : data(str.data()), size(str.size()) {}
};

} // namespace yectl
