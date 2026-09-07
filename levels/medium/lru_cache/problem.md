---
title: LRU Cache
topics: [Design, Hash Maps, Data Structures]
difficulty: medium
type: from-scratch
---

## This is a "from-scratch" problem
Unlike other problems, `solution.h` does **not** give you a class skeleton
to fill in. You design the whole class yourself — member layout, private
helper methods, everything. Only the public contract below is fixed,
because the tests call it by name.

## Task
Design and implement a template class `LRUCache<K, V>` with a fixed
capacity, evicting the **least recently used** entry when a new insert
would exceed capacity.

Required public interface (exact names/signatures — everything else about
the class is entirely up to you):

```cpp
template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity);

    // Returns true and sets *out if key exists; false (leaves *out
    // untouched) otherwise. Accessing a key counts as "using" it.
    bool get(const K& key, V* out);

    // Inserts or updates. Counts as "using" the key. If inserting a new
    // key would exceed capacity, evict the least recently used entry first.
    void put(const K& key, const V& value);

    size_t size() const;
};
```

## Requirements
1. `get` and `put` are both average O(1) — a linear scan through everything
   on every call is not acceptable for this problem (a hash map + a
   doubly linked list, or equivalent, is the standard approach).
2. Both `get` and `put` count as "recently used" — order matters for which
   entry gets evicted next.
3. `put` on an **existing** key updates its value and refreshes its
   recency, without changing `size()`.
4. When capacity is exceeded, exactly one entry is evicted: the one that
   was least recently touched by either `get` or `put`.
5. `size()` never exceeds the capacity given to the constructor.

## Notes
- There's no single "correct" internal design here — that's the point.
  A `std::unordered_map` from key to an iterator/node in a
  `std::list`, updated on every access, is the classic approach, but you
  don't have to use exactly that.
