// SPDX-License-Identifier: MIT

#include <boost/ut.hpp>
#include <functional> // For std::less (still needed for dummy_set's std::less, if used)
#include <initializer_list> // For std::initializer_list (for dummy_string constructor)
#include <string> // For std::char_traits (still needed for dummy_string's char_traits)
#include <tuple>       // For std::tuple in multitype_handler
#include <type_traits> // For std::is_same_v, std::is_class_v, std::is_union_v, std::void_t, std::enable_if_t
#include <utility>     // For std::declval

// --- YCETL Headers (as per your instructions) ---
// These are assumed to be in your project's include path
#include <ycetl/dynamic_array.hpp>
#include <ycetl/impl/container.hpp>
#include <ycetl/memory.hpp>
#include <ycetl/type_system.hpp>

// --- Dummy Types for Testing the Type System (as provided by user) ---
// These mimic the necessary interfaces without depending on actual container
// implementations.

namespace ycetl {

// Dummy Memory for basic type resolution
struct DummyMemory {
  template <typename T> constexpr T *allocate(std::size_t n) {
    return static_cast<T *>(::operator new(n * sizeof(T)));
  }
  template <typename T> constexpr void deallocate(T *p, std::size_t) {
    ::operator delete(p);
  }
};

// Dummy Container Template (e.g., for vector or set)
template <typename T_Element, typename T_Memory = DummyMemory>
class DummyContainerTemplate {
public:
  using value_type = T_Element;
  using memory_type = T_Memory;
};

// A dummy rebindable container that mimics ycetl::string or ycetl::set
template <typename T_Element, typename T_Traits = std::char_traits<T_Element>,
          typename T_Memory =
              typename container::container_traits<T_Element>::default_memory>
class DummyRebindableContainer
    : public ycetl::template_info<DummyRebindableContainer, T_Element, T_Traits,
                                  T_Memory> {
public:
  using value_type = T_Element;
  using memory_type = T_Memory;
  using traits_type = T_Traits;
  using relevant_of = ycetl::type_set<
      T_Element,
      typename ycetl::container::container_traits<T_Element>::storage_unit>;
  using backend_type = ycetl::dynamic_array<
      typename ycetl::container::container_traits<T_Element>::storage_unit>;

  constexpr DummyRebindableContainer(T_Memory &a) {}
  constexpr DummyRebindableContainer(std::initializer_list<T_Element> il,
                                     T_Memory &a) {}
};

// A simple class that does NOT inherit from template_info
struct NonTemplateInfoClass {};

// A simple class that exposes memory_type but does NOT inherit from
// template_info
struct HasMemoryTypeNoTemplateInfo {
  using memory_type = ycetl::memory::dynamic_memory<char>;
};

// A simple class that inherits from template_info but does NOT expose
// memory_type
template <typename T_Dummy>
class TemplateInfoNoMemoryType
    : public ycetl::template_info<TemplateInfoNoMemoryType, T_Dummy> {
public:
  // No memory_type alias here
};

// Example class for rebind_template test (similar to user's 'Example')
template <typename... Args> struct Example : template_info<Example, Args...> {};

// A simple class that inherits from template_info, similar to user's 'A'
template <typename... Args> class A : public template_info<A, Args...> {};

// A simple class not inheriting from template_info, similar to user's 'B'
class B {};

} // namespace ycetl

// --- Boost.UT Test Cases ---
using namespace boost::ut;
using namespace ycetl;
using namespace ycetl::container; // For container_traits and container (base
                                  // class)

suite type_system_and_rebinding_suite = [] {
  "container_value_type_deduction"_test = [] {
    constexpr auto test = [] {
      // Test the ycetl::container::container base class's value_type deduction
      // with a primitive element type (int)
      using MyIntBaseContainer =
          ycetl::container::container<DummyContainerTemplate, int,
                                      ycetl::memory::dynamic_memory<int>>;
      static_assert(std::is_same_v<MyIntBaseContainer::value_type, int>);

      // Test with a nested rebindable container (DummyRebindableContainer)
      using OuterContainerMemory = ycetl::memory::dynamic_memory<char>;
      using NestedDummy = DummyRebindableContainer<
          char, std::char_traits<char>,
          typename container_traits<char>::default_memory>;
      using MyBaseContainerOfDummy =
          ycetl::container::container<DummyContainerTemplate, NestedDummy,
                                      OuterContainerMemory>;

      // The value_type of MyBaseContainerOfDummy should be NestedDummy, but
      // with its memory rebound to OuterContainerMemory.
      static_assert(
          std::is_same_v<MyBaseContainerOfDummy::value_type,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  OuterContainerMemory>>);

      // Verify that the nested dummy's memory_type alias reflects the new,
      // rebound memory
      static_assert(std::is_same_v<
                    typename MyBaseContainerOfDummy::value_type::memory_type,
                    OuterContainerMemory>);
      return true;
    };
    expect(test());
  };
};

int main() {}
