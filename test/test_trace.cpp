#include <ycetl/trace.hpp>
#include <iostream>

struct Memory {
  std::array<char, 1000> memory;
  std::size_t consumed;
  constexpr Memory(): memory(), consumed(0) {
  }
};

constexpr Memory test_trace(){
  Memory memory;
  ycetl::trace::TracerMemory tracer_memory(memory.memory.data(), memory.memory.size());
  ycetl::trace::Tracer tracer(&tracer_memory);
  tracer.trace("test_trace");
  memory.consumed = tracer_memory.consumed();
  return memory;
}


int main() {
  
  constexpr auto trace = test_trace();
  static_assert(trace.consumed > 0, "Trace should consume some memory");
  
  std::cout << "Trace memory consumed: " << trace.consumed << " bytes" << std::endl;

  std::cout.write(trace.memory.data(), trace.consumed);

}
