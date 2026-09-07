---
title: Thread-Safe Bounded BlockingQueue
topics: [Concurrency, Threading, Synchronization]
difficulty: hard
type: implement
---

## Task
Implement a thread-safe, fixed-capacity `BlockingQueue<T>` in `solution.h`
with the following public interface:

```cpp
template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(size_t capacity);
    void push(T value);   // blocks while the queue is full
    T pop();               // blocks while the queue is empty
    size_t size() const;
    bool empty() const;
};
```

## Requirements
1. `push` **blocks** (does not busy-wait, does not return early) if the
   queue already holds `capacity` items, until a consumer calls `pop` and
   frees a slot.
2. `pop` **blocks** if the queue is empty, until a producer calls `push`.
3. Items come out in the order they went in (FIFO) — capacity limiting
   should not reorder anything.
4. `size()` and `empty()` are safe to call concurrently with `push`/`pop`
   from other threads.
5. Multiple producer threads and multiple consumer threads must be able to
   use the same queue simultaneously without losing, duplicating, or
   corrupting items.

## Notes
- You'll need `std::mutex` and `std::condition_variable` (or equivalent).
  A single mutex protecting a `std::queue<T>` plus two condition variables
  (one for "not full", one for "not empty") is the standard approach.
- The test suite includes timing-based checks for the blocking behavior
  (requirements 1 and 2) with generous margins to avoid flakiness on a
  typical dev machine — but a genuinely busy-waiting or non-blocking
  implementation will still fail them.
