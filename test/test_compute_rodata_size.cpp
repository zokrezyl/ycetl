#include <array>
#include <iostream>
#include <utility>

// A simple storage for constexpr computation results
template <typename T, std::size_t Capacity>
struct constexpr_storage {
    std::array<T, Capacity> data{};
    std::size_t size{0};
    
    constexpr bool add(T value) {
        if (size >= Capacity) return false;
        data[size++] = value;
        return true;
    }
    
    constexpr auto begin() const { return data.begin(); }
    constexpr auto end() const { return data.begin() + size; }
    constexpr std::size_t used_size() const { return size; }
};

// Example computation: Generate Fibonacci sequence up to max_value
template <typename T, std::size_t MaxSize>
constexpr auto fibonacci_sequence(T max_value) {
    constexpr_storage<T, MaxSize> result{};
    
    // Add initial values
    result.add(1);
    if (max_value < 1) return result;
    result.add(1);
    if (max_value < 2) return result;
    
    // Generate sequence
    T a = 1, b = 1;
    while (true) {
        T next = a + b;
        if (next > max_value) break;
        if (!result.add(next)) break; // Buffer full
        a = b;
        b = next;
    }
    
    return result;
}

// Our test function
constexpr auto test() {
    // Generate first 10 Fibonacci numbers under 1000
    auto fib = fibonacci_sequence<int, 20>(1000);
    
    // Verify the 10th Fibonacci number (should be 55)
    int tenth_fib = 0;
    if (fib.used_size() >= 10) {
        tenth_fib = fib.data[9];
    }
    
    return std::make_pair(fib, tenth_fib);
}

int main() {
    // Use the constexpr computation result
    constexpr auto result = test();
    constexpr auto& fib_sequence = result.first;
    constexpr int tenth_fib = result.second;
    
    // This static_assert verifies the computation happened at compile time
    static_assert(tenth_fib == 55, "10th Fibonacci number should be 55");
    
    // Print the results (runtime, but computation was at compile time)
    std::cout << "Fibonacci sequence up to 1000 (computed at compile time):\n";
    for (const auto& value : fib_sequence) {
        std::cout << value << " ";
    }
    std::cout << "\n\nThe 10th Fibonacci number is: " << tenth_fib << std::endl;
    
    return 0;
}
