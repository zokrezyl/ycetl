#include <ycetl/trace.hpp>
#include <iostream>
//#include <cstring>
#include <fmt/core.h>

struct Memory {
  std::array<char, 1000> memory;
  std::size_t consumed;
  constexpr Memory(): memory(), consumed(0) {
  }
};

struct TestObject {


};

template <std::size_t Size>
constexpr ycetl::trace::FrozenTrace<Size> test_trace(){
  ycetl::trace::Tracer tracer;
  tracer.trace("test_trace");
  tracer.trace(TestObject{});
  tracer.trace("test_trace");
  tracer.trace("test_trace");
  tracer.trace("test_trace", "trace_test");
  ycetl::trace::Trace * trace = tracer.trace();

  ycetl::trace::FrozenTrace<Size> frozen_trace(trace);
  return frozen_trace;
}

#include <algorithm>

int main() {
  

  //constexpr auto trace = test_trace<10000>();

  //static_assert(trace.length() > 1, "Trace memory size should be 10000 bytes");
  //std::cout << "Trace memory consumed: " << trace.length() << " bytes" << std::endl;
  /*
  static_assert(trace.consumed > 0, "Trace should consume some memory");
  
  std::cout << "Trace memory consumed: " << trace.consumed << " bytes" << std::endl;
  std::cout.write(trace.memory.data(), trace.consumed);
  */

  //const ycetl::trace::TraceView trace_view(memory);


}
