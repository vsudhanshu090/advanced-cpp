---
title: Rectangle
topics: [Classes, Constructors, Operator Overloading]
difficulty: basic
type: implement
---

## Task
Implement a `Rectangle` class in `solution.h` with the following public interface:

```cpp
class Rectangle {
public:
    Rectangle(double width, double height);
    double area() const;
    double perimeter() const;
    bool operator==(const Rectangle& other) const;
};
```

## Requirements
1. The constructor must throw `std::invalid_argument` if `width <= 0` or `height <= 0`.
2. `area()` returns `width * height`.
3. `perimeter()` returns `2 * (width + height)`.
4. `operator==` returns true if both rectangles have the same width and height
   (order matters — a 2x3 rectangle is NOT equal to a 3x2 rectangle).
5. All three methods (`area`, `perimeter`, `operator==`) must be usable on a
   `const Rectangle&`.

## Notes
- No default constructor is required — every `Rectangle` must be constructed
  with explicit width and height.
