#pragma once

#include <ycetl/basic_string.hpp>

namespace ycetl {

// Convenience alias matching std::string. basic_string's Traits
// parameter is kept for API parity but unused — the canonical default
// is fine.
using string = basic_string<char>;

} // namespace ycetl
