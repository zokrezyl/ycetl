#pragma once
#include <array>

namespace ycetl {
namespace simple {

template <typename Key, typename Value, std::size_t NumBuckets = 555,
          std::size_t BucketSize = 10>
struct simple_map {

  struct Entry {
    Key key;
    Value value;
    bool valid = false;
  };
  using Bucket = std::array<Entry, BucketSize>;
  std::array<Bucket, NumBuckets> table{};

  constexpr size_t get_count() const { return count; }

  constexpr std::size_t hash(const Key &key) const {
    // return ConstExprHash<Key>{}(key) % NumBuckets;
    return key % NumBuckets; // Simple modulus hash for demonstration
  }
  size_t count = 0;
  constexpr Value &get_or_create(const Key &key) {
    auto &bucket = table[hash(key)];
    for (auto &entry : bucket) {
      if (entry.valid && entry.key == key) {
        return entry.value; // already present
      }
    }
    for (auto &entry : bucket) {
      if (!entry.valid) {
        entry.key = key;
        entry.valid = true;
        count++;
        return entry.value;
      }
    }
    throw std::range_error("Bucket full");
  }
  struct ConstIterator {
    const std::array<std::array<Entry, BucketSize>, NumBuckets> *table;
    std::size_t bucket_index;
    std::size_t slot_index;

    constexpr ConstIterator(
        const std::array<std::array<Entry, BucketSize>, NumBuckets> *table,
        std::size_t bucket_index, std::size_t slot_index)
        : table(table), bucket_index(bucket_index), slot_index(slot_index) {
      // yinfo("simple_map ConstIterator created at bucket {}, slot {}",
      // bucket_index, slot_index);
      next_valid();
    }

    constexpr void next_valid() {
      // yinfo("simple_map ConstIterator next_valid called at bucket {}, slot
      // {}", bucket_index, slot_index);
      ++slot_index;
      while (bucket_index < NumBuckets) {
        while (slot_index < BucketSize) {
          if ((*table)[bucket_index][slot_index].valid)
            return;
          ++slot_index;
        }
        ++bucket_index;
        slot_index = 0;
      }
      slot_index = BucketSize;   // past last slot
      bucket_index = NumBuckets; // past last bucket
    }

    constexpr ConstIterator &operator++() {
      next_valid();
      return *this;
    }

    constexpr const Entry &operator*() const {
      if (bucket_index >= NumBuckets)
        throw std::out_of_range("Dereferencing end iterator");
      return (*table)[bucket_index][slot_index];
    }

    constexpr const Entry *operator->() const {
      if (bucket_index >= NumBuckets)
        throw std::out_of_range("Dereferencing end iterator");
      return &(*table)[bucket_index][slot_index];
    }

    constexpr bool operator==(const ConstIterator &other) const {
      return table == other.table && bucket_index == other.bucket_index &&
             slot_index == other.slot_index;
    }

    constexpr bool operator!=(const ConstIterator &other) const {
      return !(*this == other);
    }
  };

  constexpr ConstIterator begin() const {
    // yinfo("simple_map begin called");
    return ConstIterator(&table, 0, 0);
  }

  constexpr ConstIterator end() const {
    // yinfo("simple_map begin called");
    return ConstIterator(&table, NumBuckets,
                         BucketSize); // end = past last bucket
  }
};

} // namespace simple
} // namespace ycetl
