---
title: Running Median Tracker (performance-constrained)
topics: [Performance, Heaps, Algorithms]
difficulty: hard
type: performance
---

## This is a "performance" problem
Correctness alone isn't enough here. The test suite includes a wall-clock
budget on a large workload — a correct but naive (e.g. re-sort-everything
or insertion-sort-style) implementation **will time out**, even though it
produces the right answer on small inputs.

## Task
Implement `RunningMedian` in `solution.h`:

```cpp
class RunningMedian {
public:
    void insert(int value);
    double median() const;   // undefined behavior if insert() was never called; not tested
};
```

`insert` adds one more value to the running data set. `median()` returns
the median of every value inserted so far: for an odd count, the exact
middle value; for an even count, the average of the two middle values.

## Requirements
1. `insert` must run in **O(log n)** — not O(n) and not O(n log n) — where
   `n` is the number of values inserted so far.
2. `median()` must run in **O(1)**.
3. Correctness must hold at every point, not just at the end — the tests
   call `median()` after every single `insert()` and check it against a
   naively-computed reference value.

## Notes
- The standard approach: keep two heaps — a max-heap of the smaller half
  of the values, and a min-heap of the larger half — kept balanced in size
  (differing by at most 1) after every insert. The median is then either
  the top of the larger heap, or the average of both tops.
- `std::priority_queue` gives you a max-heap directly; for a min-heap,
  either negate values or use `std::greater<int>` as the comparator.
- The performance test uses 500,000 insertions with a generous but real
  time budget (2 seconds). An O(n) or O(n log n) *per-insert* approach
  will not finish in time; an O(log n) *per-insert* approach finishes in
  well under half a second.
