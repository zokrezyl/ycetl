#include <cstdint>
#include <array>
#include <iostream>

namespace rng {
    // Simple PRNG based on __TIME__
    class generator {
    private:
        std::uint64_t state;

    public:
        constexpr generator() : state(0) {
            const char* t = __TIME__;
            std::uint64_t h = (t[0] - '0') * 10 + (t[1] - '0');
            std::uint64_t m = (t[3] - '0') * 10 + (t[4] - '0');
            std::uint64_t s = (t[6] - '0') * 10 + (t[7] - '0');
            
            state = h * 3600 + m * 60 + s;
        }
        
        constexpr explicit generator(std::uint64_t seed) : state(seed) {}
        
        constexpr std::uint64_t next() {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            return state;
        }
    };
}

// First computation - generates a value that will determine the size of the second
constexpr std::uint64_t compute_size() {
    rng::generator gen;
      // Generate a number between 100-300 MB (in terms of uint32_t elements)
      // 100MB = ~25 million uint32_t elements (each 4 bytes)
      return 25'000'000 + (gen.next() % 50'000'000);
}

// Wrapper to access computation results
template<typename Computation>
struct ComputationWrapper {
    constexpr static auto value = Computation::result;
};

// Second computation - size determined by first computation
struct HugeComputation {
    constexpr static auto computation_size = compute_size();
    std::array<std::uint32_t, computation_size> data;
    
    constexpr HugeComputation() : data{} {
        rng::generator gen;
        for (std::uint64_t i = 0; i < computation_size; ++i) {
            //data[i] = static_cast<std::uint32_t>(gen.next());
            data[i] = i;
        }
    }

    constexpr std::uint64_t memory_size_mb() const {
        return (computation_size * sizeof(std::uint32_t)) / (1024 * 1024);
    }
};


// Create a huge array with size determined by the first computation
// This will be hundreds of megabytes in size
#if 0
constexpr HugeComputation huge_random_data;
#endif

int main() {

  #if 0
    // Access and use the huge computation result
    std::cout << "Date/Time: 2025-07-09 16:03:14\n";
    std::cout << "User: zokrezyl\n\n";
    
    std::cout << "Computation size: " << size_computation.value << " elements\n";
    std::cout << "Memory usage: " << huge_random_data.memory_size_mb() << " MB\n";
    
    // Print a few values from the huge array to show it works
    std::cout << "First few values:\n";
    for (int i = 0; i < 5; ++i) {
        std::cout << "data[" << i << "] = " << huge_random_data.data[i] << "\n";
    }
    
    // Verify the array size was actually allocated
    std::cout << "Array size matches computation: " 
              << (huge_random_data.data.size() == size_computation.value ? "Yes" : "No") << "\n";
    
#endif
    return 0;
}
