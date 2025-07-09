#include <array>
#include <cstddef>
#include <iostream>
#include <utility>

// Forward declaration of storage
template <typename T, std::size_t Size>
struct static_storage {
    std::array<std::byte, Size> data;
};

// Primary wrapper template that determines the required size and stores the result
template <typename Computation>
struct ComputationWrapper {
private:
    // Step 1: Determine required size at compile time using the compute() method
    static constexpr std::size_t required_size = Computation::compute();
    
    // Step 2: Allocate exactly that much memory in .rodata
    static_storage<Computation, required_size> storage;

public:
    // Constructor that initializes the storage
    constexpr ComputationWrapper() {
        // Initialize the storage with the computation result
        Computation::serialize(storage.data.data(), required_size);
    }

    // Public interface
    static constexpr std::size_t size() { return required_size; }
    
    constexpr const std::byte* data() const { return storage.data.data(); }
    
    // Helper method to access a value of type T at the given offset
    template <typename T>
    constexpr T get(std::size_t offset) const {
        // Constexpr-safe way to "read" a T from bytes
        const std::byte* src = storage.data.data() + offset;
        T value{};
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            reinterpret_cast<std::byte*>(&value)[i] = src[i];
        }
        return value;
    }
};

// Fibonacci computation that determines how many numbers to store
struct FibonacciComputation {
    // Max value and how many numbers we'll store
    static constexpr int max_value = 1000;
    static constexpr std::size_t max_count = 20; // More than enough for Fibonacci < 1000
    
    // Calculate how many Fibonacci numbers are under max_value and their total storage size
    static constexpr std::size_t compute() {
        int a = 1, b = 1;
        std::size_t count = 2; // Start with 1, 1
        
        while (count < max_count) {
            int next = a + b;
            if (next > max_value) break;
            a = b;
            b = next;
            count++;
        }
        
        // Size needed: count (1 int) + all the Fibonacci numbers (count * sizeof(int))
        return sizeof(int) * (1 + count);
    }
    
    // Serialize the Fibonacci sequence to the buffer
    static constexpr void serialize(std::byte* buffer, std::size_t size) {
        // Generate the sequence
        int sequence[max_count];
        sequence[0] = 1;
        sequence[1] = 1;
        
        int a = 1, b = 1;
        std::size_t count = 2;
        
        while (count < max_count) {
            int next = a + b;
            if (next > max_value) break;
            sequence[count++] = next;
            a = b;
            b = next;
        }
        
        // Store the count first
        for (std::size_t i = 0; i < sizeof(int); ++i) {
            buffer[i] = reinterpret_cast<std::byte*>(&count)[i];
        }
        
        // Then store all the numbers
        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t j = 0; j < sizeof(int); ++j) {
                buffer[sizeof(int) + i * sizeof(int) + j] = 
                    reinterpret_cast<std::byte*>(&sequence[i])[j];
            }
        }
    }
};

// Test function to verify our implementation
constexpr bool test() {
    ComputationWrapper<FibonacciComputation> fib;
    
    // Get count of stored Fibonacci numbers
    int count = fib.get<int>(0);
    
    // Verify the 10th Fibonacci number (if we have enough)
    if (count >= 10) {
        int tenth = fib.get<int>(sizeof(int) * 10); // Skip count + 9 previous numbers
        return tenth == 55; // 10th Fibonacci number should be 55
    }
    return false;
}

// Global instance - stored in .rodata
inline constexpr ComputationWrapper<FibonacciComputation> fibonacci_results;

int main() {
    // Verify at compile time
    static_assert(test(), "Fibonacci computation failed");
    
    // Get count of stored numbers
    int count = fibonacci_results.get<int>(0);
    
    // Print the sequence from .rodata
    std::cout << "Fibonacci sequence up to 1000 (from .rodata):\n";
    for (int i = 0; i < count; ++i) {
        int value = fibonacci_results.get<int>(sizeof(int) * (i + 1));
        std::cout << value << " ";
    }
    std::cout << "\n\nNumber of Fibonacci numbers under 1000: " << count << std::endl;
    
    // Verify the 10th Fibonacci number
    if (count >= 10) {
        int tenth = fibonacci_results.get<int>(sizeof(int) * 10);
        std::cout << "The 10th Fibonacci number is: " << tenth << std::endl;
    }
    
    return 0;
}
