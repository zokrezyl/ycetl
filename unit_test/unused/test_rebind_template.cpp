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

// --- FIX: Add missing specialization for type_set_init for
// template_arguments_t --- This explicit specialization is necessary because
// type_set_init's primary template is not defined for template_arguments_t
// directly, even though template_arguments_t inherits from type_set. The
// compiler sees template_arguments_t as a distinct type when matching the
// primary template.
namespace ycetl {
template <typename... Ts> struct type_set_init<template_arguments_t<Ts...>> {
  using type = typename type_set_init<type_set<Ts...>>::type;
};
} // namespace ycetl

// --- Dummy Types for Testing the Type System (as provided by user) ---
// These mimic the necessary interfaces without depending on actual container
// implementations.

namespace ycetl {

// Dummy Allocator for basic type resolution
struct DummyAllocator {
  template <typename T> constexpr T *allocate(std::size_t n) {
    return static_cast<T *>(::operator new(n * sizeof(T)));
  }
  template <typename T> constexpr void deallocate(T *p, std::size_t) {
    ::operator delete(p);
  }
};

// Dummy Container Template (e.g., for vector or set)
template <typename T_Element, typename T_Memory = DummyAllocator>
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
  "template_info_properties"_test = [] {
    constexpr auto test = [] {
      // Test with DummyRebindableContainer as a concrete example of a type
      // using template_info
      using MyDummyRebindable = DummyRebindableContainer<char>;

      // Check template_info::template_type (Accessed directly as template_info
      // is a public base)
      static_assert(
          std::is_same_v<MyDummyRebindable::template_type<
                             wchar_t, std::char_traits<wchar_t>,
                             ycetl::memory::dynamic_memory<wchar_t>>,
                         DummyRebindableContainer<
                             wchar_t, std::char_traits<wchar_t>,
                             ycetl::memory::dynamic_memory<wchar_t>>>);
      return true;
    };
    expect(test());

    constexpr auto test_args = [] {
      using MyDummyRebindable = DummyRebindableContainer<char>;
      // Check template_info::template_arguments (Accessed directly as
      // template_info is a public base)
      static_assert(
          std::is_same_v<MyDummyRebindable::template_arguments,
                         template_arguments_t<
                             char, std::char_traits<char>,
                             typename container_traits<char>::default_memory>>);
      return true;
    };
    expect(test_args());
  };

  "is_template_rebindable_v_trait"_test = [] {
    constexpr auto test = [] {
      // Test cases from user's static_asserts and original tests
      static_assert(is_template_rebindable_v<A<int, double>>);
      static_assert(!is_template_rebindable_v<B>);
      static_assert(!is_template_rebindable_v<int>);
      static_assert(
          !is_template_rebindable_v<char>); // Added char for completeness
      static_assert(!is_template_rebindable_v<double>);

      // Additional tests for DummyRebindableContainer
      static_assert(is_template_rebindable_v<DummyRebindableContainer<int>>);
      static_assert(!is_template_rebindable_v<NonTemplateInfoClass>);
      return true;
    };
    expect(test());
  };

  "rebind_template_t_action"_test = [] {
    constexpr auto test = [] {
      // Test case from user's static_assert (using Example)
      using original_example = Example<int, float>;
      using new_types_example = template_arguments_t<double, bool>;
      using rebound_example =
          rebind_template_t<original_example, new_types_example>;
      static_assert(std::is_same_v<rebound_example, Example<double, bool>>);

      // Original test with DummyRebindableContainer
      using OriginalDummy =
          DummyRebindableContainer<char, std::char_traits<char>,
                                   ycetl::memory::dynamic_memory<char>>;
      using NewArgs =
          template_arguments_t<wchar_t, std::char_traits<wchar_t>,
                               ycetl::memory::dynamic_memory<wchar_t>>;
      using ReboundType = rebind_template_t<OriginalDummy, NewArgs>;
      static_assert(
          std::is_same_v<ReboundType,
                         DummyRebindableContainer<
                             wchar_t, std::char_traits<wchar_t>,
                             ycetl::memory::dynamic_memory<wchar_t>>>);
      return true;
    };
    expect(test());
  };

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

  "container_traits_deduction_for_base_container"_test = [] {
    constexpr auto test = [] {
      // Test with a simple container::container of int
      using MyIntBaseContainer =
          ycetl::container::container<DummyContainerTemplate, int,
                                      ycetl::memory::dynamic_memory<int>>;

      // storage_unit should be int
      static_assert(std::is_same_v<MyIntBaseContainer::storage_unit, int>);

      // backend_type should be dynamic_array<int>
      static_assert(
          std::is_same_v<MyIntBaseContainer::backend_type, dynamic_array<int>>);

      // relevant_of for int should typically be type_set<int, int> (from
      // relevant_types_t<T, storage_unit>)
      static_assert(
          std::is_same_v<MyIntBaseContainer::relevant_of, type_set<int, int>>);

      // default_memory should be the one deduced by container_traits<int>
      static_assert(
          std::is_same_v<MyIntBaseContainer::default_memory,
                         typename container_traits<int>::default_memory>);
      return true;
    };
    expect(test());
  };
};
