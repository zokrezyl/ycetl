#pragma once

namespace yectl {

template <typename Allocator, yectl::size_t N = 0> class string {
  char *_data = nullptr;
  yectl::size_t _size = N + 1;
  yectl::size_t _capacity = N + 1;

public:
  constexpr string() { /* we delay allocation until we have a valid string */ }
  constexpr string(Allocator &allocator, const char *src, size_t size = 0) {
    constexpr auto len = _strlen(src);
    if (size == 0) {
      size = len; // If size is not provided, use the length of the string
    }
    if (N > 0) {
      _size = min(N, len); // Ensure we don't exceed N
    } else {
      _capacity = len + 1; // +1 for null terminator
    }
    _data = allocate<Allocator, char>(allocator, _capacity);
    memcpy(_data, src, _size);
    _data[_size - 1] = '\0'; // Null-terminate the string
  }

  constexpr string(Allocator allocator, const string_view &sv)
      : string<Allocator, N>(allocator, sv.c_str(), sv.length()) {};
};

} // namespace yectl
