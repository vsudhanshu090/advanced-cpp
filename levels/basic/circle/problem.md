---
title: Circle
topics: [Classes, Constructors, Operator Overloading]
difficulty: basic
type: implement
---

## Task
Implement a `Circle` class in `solution.h` with the following public interface:

```cpp
class Circle {
public:
    explicit Circle(double radius);
    double area() const;
    double circumference() const;
    bool operator==(const Circle& other) const;
};
```

## Requirements
1. The constructor must throw `std::invalid_argument` if `radius <= 0`.
2. `area()` returns `π * radius²`.
3. `circumference()` returns `2 * π * radius`.
4. `operator==` returns true if both circles have the same radius.
5. All three methods must be usable on a `const Circle&`.

## Notes
- Use whatever value of π you like (e.g. a `constexpr double` you define
  yourself) — the tests compare against expected values with a small
  tolerance, not exact bit-for-bit equality.
- `<cmath>`'s `M_PI` is not guaranteed to be defined on every compiler
  without extra flags, so defining your own constant is the safer choice.
