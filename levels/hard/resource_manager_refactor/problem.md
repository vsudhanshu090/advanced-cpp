---
title: Refactor ResourceHandle to be copy/move-safe
topics: [RAII, Rule of Five, Move Semantics, Refactoring]
difficulty: hard
type: refactor
---

## This is a "refactor" problem
`solution.h` already contains **working code** for the single-object case
— it compiles, and using one `ResourceHandle` by itself is completely
fine. The problem is what happens when you copy or move it. Your job is
to refactor the class to be safe under copying and moving, without
changing its existing single-object behavior.

## The class
```cpp
class ResourceHandle {
public:
    explicit ResourceHandle(int value);
    int get() const;
    void set(int value);
    ~ResourceHandle();
};
```

The starting code only defines a constructor and a destructor — nothing
else. In modern C++ terms, it violates the Rule of Five: it manages a raw
owning resource but doesn't define copy/move behavior for it, so the
compiler quietly generates defaults that are wrong for this class.

## Requirements
1. **Copying** a `ResourceHandle` (via copy constructor or `operator=`)
   must produce a fully independent copy — modifying one afterward must
   never affect the other.
2. **Moving** a `ResourceHandle` (via `std::move`, move constructor, or
   move assignment) must transfer ownership cheaply (no unnecessary
   allocation) — the moved-to object ends up with the value, and the
   moved-from object is left in a valid, safely-destructible state.
3. Self-assignment (`a = a;`) must not corrupt or lose the object's data.
4. The existing single-object behavior (`get()`/`set()`) must keep working
   exactly as it does now — this is a refactor, not a rewrite of the
   public interface.

## Notes
- You can implement copy/move by hand (a deep-copying copy ctor/assign,
  plus a move ctor/assign that steals the pointer and nulls the source),
  or simplify the internals using `std::unique_ptr<int>` (which already
  gives you correct move semantics for free, though you'd still need to
  add your own copy constructor/assignment on top, since `unique_ptr`
  itself is move-only).
- The public interface above must stay exactly as it is — only the
  internals and the special member functions change.
