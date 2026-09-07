---
title: Fix the DynamicArray bug
topics: [Debugging, RAII, Rule of Three]
difficulty: medium
type: debug
---

## This is a "debug" problem
`solution.h` already contains a **complete** implementation — it's not a
stub. It compiles, and the basic single-object tests pass. But it has
**one intentional bug**. Your job is to find it and fix it — not rewrite
the class.

## The class
`DynamicArray` is meant to be an owning wrapper around a heap-allocated
`int` array, with correct, independent copies:

```cpp
class DynamicArray {
public:
    explicit DynamicArray(size_t size);
    DynamicArray(const DynamicArray& other);   // should deep-copy
    DynamicArray& operator=(const DynamicArray& other);   // should deep-copy
    ~DynamicArray();

    int& at(size_t index);
    const int& at(size_t index) const;   // throws std::out_of_range if index >= size()
    size_t size() const;
};
```

## Symptom
Constructing a single `DynamicArray` and using it works fine, and bounds
checking works fine. But **copying** one — via the copy constructor or
`operator=` — does not produce an independent object: modifying the copy
also changes the original (and vice versa), as if they were secretly
sharing the same underlying storage. This is the single most classic
category of C++ bug there is.

## Requirements once fixed
1. Copying a `DynamicArray` (either via the copy constructor or
   `operator=`) must produce a fully independent copy — modifying one
   afterward must never affect the other.
2. All existing correct behavior (construction, bounds checking) must
   keep working.

## Note on how this is tested
To test copy independence, the test suite intentionally does not rely on
destroying both the original and the copy in the same test (a truly
unfixed shallow-copy bug would cause a real double-free when both are
destroyed — interesting to know, but not something we want crashing the
grader deterministically). Instead it checks independence directly by
modifying one and reading the other, which is a safe and equally valid
way to prove — or disprove — that the copy is real.
