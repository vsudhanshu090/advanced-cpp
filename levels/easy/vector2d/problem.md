---
title: Vector2D
topics: [Operator Overloading, Const Correctness, Math]
difficulty: easy
type: implement
---

## Task
Implement a `Vector2D` class in `solution.h` with the following public interface:

```cpp
class Vector2D {
public:
    Vector2D(double x, double y);
    double x() const;
    double y() const;

    Vector2D operator+(const Vector2D& other) const;
    Vector2D operator-(const Vector2D& other) const;
    Vector2D operator*(double scalar) const;   // scales both components

    double dot(const Vector2D& other) const;
    double magnitude() const;

    bool operator==(const Vector2D& other) const;
};
```

## Requirements
1. `operator+` / `operator-` combine components pairwise; neither modifies
   the operands (both return a new `Vector2D`).
2. `operator*(scalar)` returns a new vector with both components scaled.
3. `dot(other)` returns `x()*other.x() + y()*other.y()`.
4. `magnitude()` returns `sqrt(x()*x() + y()*y())`.
5. `operator==` compares both components. It's fine to compare exactly
   (no epsilon needed) since the tests only compare vectors built from the
   same literal inputs, never from independently-computed floating point
   results.
6. Every method above must be usable on a `const Vector2D&`.

## Notes
- No exceptions are required anywhere in this problem — any `(x, y)` pair
  is a valid vector.
