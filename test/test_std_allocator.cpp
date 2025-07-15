#include <memory>
#include <cstddef>

template <typename T>
struct simple_allocator {
    using value_type = T;

    simple_allocator() noexcept = default;

    template <class U>
    simple_allocator(const simple_allocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) noexcept {
        ::operator delete(p);
    }

    template <typename U>
    struct rebind {
        using other = simple_allocator<U>;
    };
};

template <typename T, typename U>
bool operator==(const simple_allocator<T>&, const simple_allocator<U>&) noexcept {
    return true;
}

template <typename T, typename U>
bool operator!=(const simple_allocator<T>& a, const simple_allocator<U>& b) noexcept {
    return !(a == b);
}
#include <vector>

#include <cxxabi.h>
#include <iostream>
int main() {
    std::vector<std::vector<int>, simple_allocator<std::vector<int>>> v{1};
    auto name = typeid(decltype(v[0].get_allocator())).name();
    int status;
    char* demangled = abi::__cxa_demangle(name, 0, 0, &status);
    std::cout << demangled << '\n';
    std::free(demangled);
}

