#include <type_traits>
#include <tuple>

// Type set for holding multiple types
template <typename... Ts>
struct type_set {
    template <typename NewT>
    static constexpr auto add() {
        return type_set<Ts..., NewT>{};
    }
    static constexpr std::size_t size = sizeof...(Ts);
};

// Simplified multitype_handler - HandlerImpl first for easier specialization
template <template <typename> class HandlerImpl, typename TypeSet>
class multitype_handler;

template <template <typename> class HandlerImpl, typename... Ts>
class multitype_handler<HandlerImpl, type_set<Ts...>> {
private:
    // Tuple of handlers for each type in the type_set
    std::tuple<HandlerImpl<Ts>...> handlers_;

public:
    // Default constructor
    constexpr multitype_handler() = default;
    // Access handler for a specific type
    template <typename T>
    constexpr HandlerImpl<T>& get_handler() {
        return std::get<HandlerImpl<T>>(handlers_);
    }
    template <typename T>
    constexpr const HandlerImpl<T>& get_handler() const {
        return std::get<HandlerImpl<T>>(handlers_);
    }
};

// Example usage:
// Storage handler that manages fixed allocations in a constexpr context
template <typename T, std::size_t Capacity = 1000>
struct storage {
    // Allocate a T object from the internal pool
    constexpr T* allocate() {
        if (count >= Capacity) return nullptr;
        T* ptr = &data[count];
        allocations[count] = ptr;
        count++;
        return ptr;
    }
    
    // Handle method to allocate and initialize an object
    constexpr void handle(const T& value) {
        if (T* ptr = allocate()) {
            *ptr = value;
        }
    }
    
    // Get count of allocated objects
    constexpr std::size_t allocated_count() const {
        return count;
    }
    
    // Get pointer to allocated object by index
    constexpr T* get(std::size_t index) const {
        if (index < count) return allocations[index];
        return nullptr;
    }
    
private:
    std::array<T, Capacity> data{};
    std::array<T*, Capacity> allocations{};
    std::size_t count = 0;
};

// Example constexpr function demonstrating usage
constexpr int test() {
    using my_types = type_set<int, double, char>;
    multitype_handler<storage, my_types> handler;
    
    // Store some values
    handler.get_handler<int>().handle(42);
    handler.get_handler<double>().handle(3.14);
    handler.get_handler<char>().handle('A');
    
    // Verify storage
    if (int* int_ptr = handler.get_handler<int>().get(0)) {
        return *int_ptr; // Return 42
    }
    return 0;
}

// Example handler that stores values
template <typename T>
struct storage_handler {
    constexpr void handle(const T& value) {
        stored_value = value;
    }
    
    constexpr T retrieve() const {
        return stored_value;
    }
    
private:
    T stored_value{};
};

;

// To use it:
int example() {
    using my_types = type_set<int, double>;
    multitype_handler<storage_handler, my_types> handler;
    
    // Access handlers directly
    handler.get_handler<int>().handle(42);
    handler.get_handler<double>().handle(3.14);
    //handler.get_handler<std::string>().handle("hello");
    
    // Retrieve values
    int i = handler.get_handler<int>().retrieve();  // i = 42
    double d = handler.get_handler<double>().retrieve();  // d = 3.14
    //std::string s = handler.get_handler<std::string>().retrieve();  // s = "Processed: hello"
    return 0;
  //
}

// If you want a handler with a more custom interface:
template <typename T>
struct custom_handler {
    constexpr void process(const T& value, int param) {
        // Custom processing with extra parameters
        result = value + param;
    }
    
    constexpr T get_result() const {
        return result;
    }
    
private:
    T result{};
};


int main() {
  auto res = example();

}
