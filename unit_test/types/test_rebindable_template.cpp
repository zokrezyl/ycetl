#include <boost/ut.hpp>
#include <functional> // For std::less (still needed for dummy_set's std::less)
#include <initializer_list> // For std::initializer_list (for dummy_string constructor)
#include <string> // For std::char_traits (still needed for dummy_string's char_traits)
#include <type_traits> // For std::is_same_v, std::is_class_v, std::is_union_v

// --- YCETL Headers (as per your instructions, no concrete container includes)
// --- These are assumed to be in your project's include path
#include <ycetl/dynamic_array.hpp>  // Contains ycetl::dynamic_array
#include <ycetl/impl/container.hpp> // Contains ycetl::container::container (the base class, not concrete containers)
#include <ycetl/memory.hpp> // Contains allocate, owned_pointer, multitype_dynamic_memory, default_memory, ycetl::memory::dynamic_memory, ycetl::memory::multitype_memory
#include <ycetl/type_system.hpp> // Contains type_set, template_arguments_t, template_info, is_template_rebindable_v, has_memory_type_v, has_rebindable_memory_v, rebind_template_t, rebind_memory_t, relevant_types_t, backend_type_of_t, has_relevant_of_v

// --- Dummy Types for Testing the Type System ---
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
// This will be used as the 'ContainerTemplate' argument for
// ycetl::container::container and also to create dummy rebindable types.
template <typename T_Element, typename T_Memory = DummyAllocator>
class DummyContainerTemplate {
  // This class doesn't need a full implementation, just its type signature
  // and potentially nested types if other traits depend on them.
public:
  using value_type = T_Element;
  using memory_type = T_Memory;
  // Add other types if needed by specific traits, e.g.,
  // using backend_type = ycetl::dynamic_array<T_Element>;
  // using relevant_of = ycetl::type_set<T_Element>;
};

// A dummy rebindable container that mimics ycetl::string or ycetl::set
// It inherits from template_info and exposes memory_type and relevant_of.
template <typename T_Element,
          typename T_Traits =
              std::char_traits<T_Element>, // For string-like behavior
          typename T_Memory =
              typename container::container_traits<T_Element>::default_memory>
class DummyRebindableContainer
    : public ycetl::template_info<DummyRebindableContainer, T_Element, T_Traits,
                                  T_Memory> {
public:
  using value_type = T_Element;
  using memory_type = T_Memory;
  using traits_type = T_Traits; // For string-like
  using relevant_of =
      ycetl::type_set<T_Element,
                      typename ycetl::container::container_traits<
                          T_Element>::storage_unit>; // Mimic relevant_of
  using backend_type =
      ycetl::dynamic_array<typename ycetl::container::container_traits<
          T_Element>::storage_unit>; // Mimic backend_type

  // Minimal constructor to allow instantiation in tests
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

      // Check template_info::template_type
      // It should be the original template 'DummyRebindableContainer'
      // instantiated with new arguments
      static_assert(
          std::is_same_v<MyDummyRebindable::template_info::template_type<
                             wchar_t, std::char_traits<wchar_t>,
                             ycetl::memory::dynamic_memory<wchar_t>>,
                         DummyRebindableContainer<
                             wchar_t, std::char_traits<wchar_t>,
                             ycetl::memory::dynamic_memory<wchar_t>>>);
      expect(std::is_same_v<
             MyDummyRebindable::template_info::template_type<
                 wchar_t, std::char_traits<wchar_t>,
                 ycetl::memory::dynamic_memory<wchar_t>>,
             DummyRebindableContainer<wchar_t, std::char_traits<wchar_t>,
                                      ycetl::memory::dynamic_memory<wchar_t>>>);

      // Check template_info::template_arguments
      // It should expose the original template arguments of MyDummyRebindable
      static_assert(
          std::is_same_v<MyDummyRebindable::template_info::template_arguments,
                         template_arguments_t<
                             char, std::char_traits<char>,
                             typename container_traits<char>::default_memory>>);
      expect(
          std::is_same_v<MyDummyRebindable::template_info::template_arguments,
                         template_arguments_t<
                             char, std::char_traits<char>,
                             typename container_traits<char>::default_memory>>);

      return true;
    };
    expect(test());
  };

  "is_template_rebindable_v_trait"_test = [] {
    constexpr auto test = [] {
      // Should be true for DummyRebindableContainer (as it inherits from
      // ycetl::template_info)
      static_assert(is_template_rebindable_v<DummyRebindableContainer<int>>);
      expect(is_template_rebindable_v<DummyRebindableContainer<int>>);

      // Should be false for primitive types (guarded by
      // std::is_class_v/std::is_union_v in its definition)
      static_assert(!is_template_rebindable_v<int>);
      expect(!is_template_rebindable_v<int>);

      static_assert(!is_template_rebindable_v<char>);
      expect(!is_template_rebindable_v<char>);

      // Should be false for other classes that do not inherit from
      // ycetl::template_info
      static_assert(!is_template_rebindable_v<NonTemplateInfoClass>);
      expect(!is_template_rebindable_v<NonTemplateInfoClass>);

      return true;
    };
    expect(test());
  };

  "has_memory_type_v_trait"_test = [] {
    constexpr auto test = [] {
      // Should be true for DummyRebindableContainer (as it exposes
      // 'memory_type')
      static_assert(has_memory_type_v<DummyRebindableContainer<int>>);
      expect(has_memory_type_v<DummyRebindableContainer<int>>);

      // Should be true for our dummy class that exposes memory_type
      static_assert(has_memory_type_v<HasMemoryTypeNoTemplateInfo>);
      expect(has_memory_type_v<HasMemoryTypeNoTemplateInfo>);

      // Should be false for primitive types (guarded by
      // std::is_class_v/std::is_union_v)
      static_assert(!has_memory_type_v<int>);
      expect(!has_memory_type_v<int>);

      // Should be false for classes that do not expose 'memory_type'
      static_assert(!has_memory_type_v<NonTemplateInfoClass>);
      expect(!has_memory_type_v<NonTemplateInfoClass>);

      static_assert(!has_memory_type_v<TemplateInfoNoMemoryType<int>>);
      expect(!has_memory_type_v<TemplateInfoNoMemoryType<int>>);

      return true;
    };
    expect(test());
  };

  "has_rebindable_memory_v_trait"_test = [] {
    constexpr auto test = [] {
      // Should be true for DummyRebindableContainer (is_template_rebindable_v
      // && has_memory_type_v)
      static_assert(has_rebindable_memory_v<DummyRebindableContainer<int>>);
      expect(has_rebindable_memory_v<DummyRebindableContainer<int>>);

      // Should be false if not template_rebindable (e.g., primitive)
      static_assert(!has_rebindable_memory_v<int>);
      expect(!has_rebindable_memory_v<int>);

      // Should be false if has memory_type but not template_rebindable
      static_assert(!has_rebindable_memory_v<HasMemoryTypeNoTemplateInfo>);
      expect(!has_rebindable_memory_v<HasMemoryTypeNoTemplateInfo>);

      // Should be false if template_rebindable but no memory_type
      static_assert(!has_rebindable_memory_v<TemplateInfoNoMemoryType<int>>);
      expect(!has_rebindable_memory_v<TemplateInfoNoMemoryType<int>>);

      return true;
    };
    expect(test());
  };

  "rebind_template_t_action"_test = [] {
    constexpr auto test = [] {
      using OriginalDummy =
          DummyRebindableContainer<char, std::char_traits<char>,
                                   ycetl::memory::dynamic_memory<char>>;
      // Define a new set of arguments for the DummyRebindableContainer template
      using NewArgs =
          template_arguments_t<wchar_t, std::char_traits<wchar_t>,
                               ycetl::memory::dynamic_memory<wchar_t>>;

      // Apply the rebind_template_t action
      using ReboundType = rebind_template_t<OriginalDummy, NewArgs>;

      // The result should be a DummyRebindableContainer with the new arguments
      static_assert(
          std::is_same_v<ReboundType,
                         DummyRebindableContainer<
                             wchar_t, std::char_traits<wchar_t>,
                             ycetl::memory::dynamic_memory<wchar_t>>>);
      expect(std::is_same_v<
             ReboundType,
             DummyRebindableContainer<wchar_t, std::char_traits<wchar_t>,
                                      ycetl::memory::dynamic_memory<wchar_t>>>);

      return true;
    };
    expect(test());
  };

  "rebind_memory_t_action"_test = [] {
    constexpr auto test = [] {
      using OriginalDummy =
          DummyRebindableContainer<char, std::char_traits<char>,
                                   ycetl::memory::dynamic_memory<char>>;
      using NewMemory =
          ycetl::memory::dynamic_memory<char>; // A different instance of the
                                               // same memory type

      // Rebind OriginalDummy's memory to NewMemory
      using ReboundMemoryDummy = rebind_memory_t<OriginalDummy, NewMemory>;

      // The result should be a DummyRebindableContainer with the original
      // element/traits, but NewMemory
      static_assert(
          std::is_same_v<ReboundMemoryDummy,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  NewMemory>>);
      expect(
          std::is_same_v<ReboundMemoryDummy,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  NewMemory>>);

      // Test rebinding to a different kind of memory (e.g., multitype_memory)
      using OriginalDummyWithDynamicMem =
          DummyRebindableContainer<char, std::char_traits<char>,
                                   ycetl::memory::dynamic_memory<char>>;
      // The multitype_memory needs to be able to handle 'char' for this dummy
      // type
      using NewMultiTypeMemory =
          ycetl::memory::multitype_memory<ycetl::memory::dynamic_memory,
                                          ycetl::type_set<char, int>>;
      using ReboundToMultiType =
          rebind_memory_t<OriginalDummyWithDynamicMem, NewMultiTypeMemory>;

      static_assert(
          std::is_same_v<ReboundToMultiType,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  NewMultiTypeMemory>>);
      expect(
          std::is_same_v<ReboundToMultiType,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  NewMultiTypeMemory>>);

      return true;
    };
    expect(test());
  };

  "container_value_type_deduction"_test = [] {
    constexpr auto test = [] {
      // Test the ycetl::container::container base class's value_type deduction
      // with a primitive element type (int)
      using MyIntBaseContainer =
          container::container<DummyContainerTemplate, int,
                               ycetl::memory::dynamic_memory<int>>;
      // value_type should just be int, as int is not memory-rebindable
      static_assert(std::is_same_v<MyIntBaseContainer::value_type, int>);
      expect(std::is_same_v<MyIntBaseContainer::value_type, int>);

      // Test with a nested rebindable container (DummyRebindableContainer)
      using OuterContainerMemory =
          ycetl::memory::dynamic_memory<char>; // The memory of the outer
                                               // container::container
      // Nested DummyRebindableContainer is initially using its default memory
      using NestedDummy = DummyRebindableContainer<
          char, std::char_traits<char>,
          typename container_traits<char>::default_memory>;
      // The outer container::container is instantiated with NestedDummy as its
      // element type and OuterContainerMemory
      using MyBaseContainerOfDummy =
          container::container<DummyContainerTemplate, NestedDummy,
                               OuterContainerMemory>;

      // The value_type of MyBaseContainerOfDummy should be NestedDummy, but
      // with its memory rebound to OuterContainerMemory.
      static_assert(
          std::is_same_v<MyBaseContainerOfDummy::value_type,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  OuterContainerMemory>>);
      expect(
          std::is_same_v<MyBaseContainerOfDummy::value_type,
                         DummyRebindableContainer<char, std::char_traits<char>,
                                                  OuterContainerMemory>>);

      // Verify that the nested dummy's memory_type alias reflects the new,
      // rebound memory
      static_assert(std::is_same_v<
                    typename MyBaseContainerOfDummy::value_type::memory_type,
                    OuterContainerMemory>);
      expect(std::is_same_v<
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
          container::container<DummyContainerTemplate, int,
                               ycetl::memory::dynamic_memory<int>>;

      // storage_unit should be int
      static_assert(std::is_same_v<MyIntBaseContainer::storage_unit, int>);
      expect(std::is_same_v<MyIntBaseContainer::storage_unit, int>);

      // backend_type should be dynamic_array<int>
      static_assert(
          std::is_same_v<MyIntBaseContainer::backend_type, dynamic_array<int>>);
      expect(
          std::is_same_v<MyIntBaseContainer::backend_type, dynamic_array<int>>);

      // relevant_of for int should typically be type_set<int, int> (from
      // relevant_types_t<T, storage_unit>)
      static_assert(
          std::is_same_v<MyIntBaseContainer::relevant_of, type_set<int, int>>);
      expect(
          std::is_same_v<MyIntBaseContainer::relevant_of, type_set<int, int>>);

      // default_memory should be the one deduced by container_traits<int>
      static_assert(
          std::is_same_v<MyIntBaseContainer::default_memory,
                         typename container_traits<int>::default_memory>);
      expect(std::is_same_v<MyIntBaseContainer::default_memory,
                            typename container_traits<int>::default_memory>);

      return true;
    };
    expect(test());
  };
};
