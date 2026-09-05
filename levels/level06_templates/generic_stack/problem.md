---
title: Generic Stack<T>
topics: [Templates, Move Semantics, Exception Safety]
difficulty: medium
---

## Task
Implement a generic, type-safe stack class template `Stack<T>` in `solution.h` with the
following public interface:

```cpp
template <typename T>
class Stack {
public:
    void push(const T& value);
    void push(T&& value);        // move-enabled push
    T pop();                     // removes AND returns the top element
    T& top();                    // returns reference to top element (no removal)
    bool empty() const;
    size_t size() const;
};
```

## Requirements
1. Must work for any type `T` (int, std::string, custom structs, etc.)
2. `pop()` on an empty stack must throw `std::out_of_range`
3. `top()` on an empty stack must throw `std::out_of_range`
4. The move-enabled `push(T&& value)` must actually move, not copy, when given an rvalue
   (tests check this using a move-counting type)
5. `size()` and `empty()` must be O(1)

## Notes
- You may use `std::vector<T>` or a hand-rolled dynamic array internally — your choice.
- Do not change the public interface signatures above; the tests call them exactly as shown.
