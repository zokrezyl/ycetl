#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

// Example struct
struct TestStruct {
    int32_t a;
    float   b;
    // no matter how much padding the compiler would insert here,
    // we’ll never read it.
};

// Packs an unsigned integer (of any size) into buf at offset off, little-endian.
template <class Int, std::size_t N>
constexpr void pack_int(std::array<unsigned char, N>& buf, Int value, std::size_t off) {
  /*
    static_assert(std::is_integral_v<Int> && sizeof(Int) <= N - off,
                  "Integer too large or offset out of range");
                  */
    using U = std::make_unsigned_t<Int>;
    U u = static_cast<U>(value);
    for (std::size_t i = 0; i < sizeof(Int); ++i) {
        buf[off + i] = static_cast<unsigned char>((u >> (8 * i)) & 0xFF);
    }
}

// Main serializer: handles int32 and float via bit_cast→uint32
template <class T>
constexpr auto serialize_constexpr(const T& t) noexcept {
    static_assert(std::is_trivially_copyable_v<T>,
                  "serialize_constexpr only works on trivially copyable types");
    std::array<unsigned char, sizeof(T)> buf{};  // all bytes = 0

    // You must hand-code one line per member in declaration order:
    // 1) pack the int32_t member at offset 0
    pack_int(buf, t.a, /*offset=*/0);

    // 2) pack the float member by bit_cast to uint32_t at offset sizeof(a)
    auto fb = std::bit_cast<std::uint32_t>(t.b);
    pack_int(buf, fb, /*offset=*/sizeof(t.a));

    return buf;
}

// Test at compile time:
constexpr TestStruct ts{0x11223344, 3.14f};
constexpr auto data = serialize_constexpr(ts);
static_assert(data[0] == 0x44 && data[3] == 0x11, "int32 bits OK");
static_assert(data[4] == (std::bit_cast<std::uint32_t>(3.14f) & 0xFF),
              "float low byte OK");

int  main() {

}
