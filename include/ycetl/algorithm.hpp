#pragma once

template <std::input_iterator I,          // “source” iterator
          std::sentinel_for<I> S,         // matching sentinel
          std::weakly_incrementable O>    // “destination” iterator
  requires std::indirectly_copyable<I, O> // assignment works
constexpr O copy(I first, S last, O result);
