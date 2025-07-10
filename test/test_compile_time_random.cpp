#include <array>
#include <cstdint>
#include <iostream>

// Convert __TIME__ string to a numeric seed
constexpr std::uint64_t time_to_seed(const char* time_str) {
    // __TIME__ format is "HH:MM:SS"
    std::uint64_t h = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    std::uint64_t m = (time_str[3] - '0') * 10 + (time_str[4] - '0');
    std::uint64_t s = (time_str[6] - '0') * 10 + (time_str[7] - '0');
    
    // Combine into a single value
    return h * 3600 + m * 60 + s;
}

// Simple constexpr PRNG
class CompileTimeRandom {
private:
    std::uint64_t state;
    
    // Parameters for a simple LCG
    static constexpr std::uint64_t a = 6364136223846793005ULL;
    static constexpr std::uint64_t c = 1442695040888963407ULL;

public:
    // Constructor with seed derived from compile time
    constexpr CompileTimeRandom() : state(time_to_seed(__TIME__)) {
        // Mix in the user name if needed for more variation
        // This is just a simple hash of "zokrezyl"
        state ^= 0x7a6f6b72657a796c; // ASCII values of "zokrezyl"
    }
    
    // Constructor with explicit seed
    constexpr CompileTimeRandom(std::uint64_t seed) : state(seed) {}
    
    // Generate next value
    constexpr std::uint64_t next() {
        state = state * a + c;
        return state >> 16; // Upper bits have better randomness
    }
    
    // Generate value in range [min, max]
    constexpr std::uint64_t next(std::uint64_t min, std::uint64_t max) {
        return min + next() % (max - min + 1);
    }
};

// Storage for compile-time generated random values
template <std::size_t Size>
struct RandomValues {
    std::array<std::uint64_t, Size> values;
    
    constexpr RandomValues() : values{} {
        CompileTimeRandom rng;
        for (std::size_t i = 0; i < Size; ++i) {
            values[i] = rng.next(1, 1000);
        }
    }
};

int main() {
    // Values computed at compile-time using the time of compilation as seed
    constexpr auto random_data = RandomValues<20>();
    
    std::cout << "Random values based on compilation time (" << __TIME__ << "):\n";
    for (auto v : random_data.values) {
        std::cout << v << " ";
    }
    std::cout << "\n";
    
    // Different compilations will produce different values!
    return 0;
}
