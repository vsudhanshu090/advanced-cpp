# Level 6 — Templates & Generic Programming

> Templates are the mechanism that makes C++ "write once, works for every type." Everything in this level is about **compile-time code generation**: the compiler reads your blueprint and stamps out a real function/class per type you actually use.

---

## 📑 Topics in this level
1. [Function templates & argument deduction](#1-function-templates--template-argument-deduction)
2. [Class templates](#2-class-templates)
3. [Full vs partial specialization](#3-template-specialization-full-vs-partial)
4. [Non-type template parameters](#4-non-type-template-parameters)
5. [Multiple params & default template arguments](#5-multiple-template-parameters-default-template-arguments)
6. [Templates + operator overloading](#6-templates--operator-overloading-interplay)
7. [`typename` vs `class`](#7-typename-vs-class)
8. [Why templates live in headers](#8-where-template-code-lives-header-only)
9. [`static_assert` basics](#9-basic-constraints-via-static_assert)

---

## 1. Function templates & template argument deduction

A function template is a **pattern**, not a function. Nothing is compiled until you call it with a concrete type — that call triggers *instantiation*.

```cpp
template <typename T>
T add(T a, T b) {
    return a + b;
}

add(2, 3);          // T = int
add(2.6, 4.7);       // T = double
add(string("a"), string("b")); // T = string
```

Conceptually, the compiler generates separate overloads behind the scenes:
```cpp
int add(int, int);
double add(double, double);
string add(string, string);
```
That's the whole idea of "generic programming" — you write the logic once, the compiler writes the boilerplate.

### Deduction limits
Deduction only works when **all** template parameters can be inferred consistently from the arguments. It will **not** mix types for you:
```cpp
add(10, 2.5);            // ❌ ambiguous: is T int or double?
add<double>(10, 2.5);    // ✅ explicit — force T
```
Or use two independent parameters instead of forcing them to match:
```cpp
template <typename T, typename U>
auto add(T a, U b) { return a + b; }   // T and U can differ
```

### References preserve constness of the *variable*, not create it
```cpp
template <typename T>
void fun(T x) { ... }

int a = 5;
const int b = 10;

fun(a);   // T = int
fun(b);   // T = int   <-- const is DROPPED when passing by value
```
Pass-by-value copies the value, so the copy doesn't need to respect the original's constness — deduction strips `const` here on purpose.

```cpp
template <typename T>
void fun(T& x) { ... }

fun(a);   // T = int         -> x is int&
fun(b);   // T = const int   -> x is const int&
```
By-reference deduction keeps `const`, because `x` now refers to the *original* object, not a copy. This also matters for large objects — passing `const std::string& s` avoids an expensive copy on every call, exactly like non-template functions.

### Arrays: `T` vs `T&`
This is a classic gotcha. An array argument **decays to a pointer** when passed by value, but keeps its array-ness (and therefore its size) when passed by reference.

```cpp
template <typename T>
void byValue(T arr) {
    cout << sizeof(arr) << endl;   // size of a POINTER (e.g. 8), not the array
}

template <typename T>
void byRef(T& arr) {
    cout << sizeof(arr) << endl;   // size of the ACTUAL array
}

int nums[5] = {1,2,3,4,5};
byValue(nums);   // T = int*        -> sizeof = 8
byRef(nums);      // T = int(&)[5]   -> sizeof = 20 (5 * sizeof(int))
```
This is why generic array-size utilities (like `std::size`) take the array **by reference** — that's the only way the compiler still knows how big it is.

### Deduction never looks at the return type
```cpp
template <typename T>
T fun(int x) {
    return T{};
}

fun(10);              // ❌ compiler has nothing to deduce T from — no argument is type T
int x = fun(10);      // ❌ still fails — the assignment target isn't used for deduction either
int x = fun<int>(10); // ✅ you must specify T explicitly
```
📌 **Rule of thumb:** template argument deduction only ever looks at *argument types*, never at how you use the result.

---

## 2. Class templates

Same blueprint idea, applied to classes. `vector`, `pair`, `array`, `map` are *all* class templates under the hood — nothing magic, just templates the standard library ships for you.

```cpp
template <typename T>
class Box {
    T value;
public:
    Box(T val) : value(val) {}
    T getValue() { return value; }
};

Box<int> a(10);
Box<string> c("hello");
```

### Are `Box`, `vector`, etc. "blueprints of blueprints"?
Sort of, but stated more precisely: a plain `class` is a blueprint for *objects*. A class **template** is a blueprint for *classes* — `Box<int>` and `Box<string>` are two genuinely different, unrelated classes that both happen to be stamped from the same template. `Box` by itself isn't a type at all; `Box<int>` is. So the "blueprint of a blueprint" intuition is right: the template is one level of abstraction above the class it produces.

### What a real (simplified) `vector` looks like
It's the same `Box` idea, just holding a dynamic buffer instead of one value:
```cpp
template <typename T>
class MiniVector {
    T* data;
    size_t size_;
    size_t capacity_;
public:
    MiniVector() : data(nullptr), size_(0), capacity_(0) {}
    void push_back(const T& val) {
        if (size_ == capacity_) resize(capacity_ == 0 ? 1 : capacity_ * 2);
        data[size_++] = val;
    }
    T& operator[](size_t i) { return data[i]; }
    size_t size() const { return size_; }
private:
    void resize(size_t newCap) {
        T* newData = new T[newCap];
        for (size_t i = 0; i < size_; ++i) newData[i] = data[i];
        delete[] data;
        data = newData;
        capacity_ = newCap;
    }
};
```
`std::vector<int>` and `std::vector<string>` are literally different classes generated from something like this at compile time.

### Class Template Argument Deduction (CTAD, C++17+)
You no longer have to spell out the type if the compiler can infer it from the constructor call:
```cpp
Box b(10);   // deduces Box<int> — no <int> needed
```

### Static members are **per-instantiation**, not shared globally
```cpp
template <typename T>
class Counter {
public:
    static int count;
    Counter() { ++count; }
};
template <typename T>
int Counter<T>::count = 0;
```
`Counter<int>::count` and `Counter<double>::count` are two completely separate static variables — because `Counter<int>` and `Counter<double>` are two separate classes.

### Defining member functions outside the class
```cpp
template <typename T>
class Box {
    T value;
public:
    Box(T value);
};

template <typename T>
Box<T>::Box(T value) : value(value) {}   // note the Box<T>:: — the template parameter list repeats
```

### Combine with non-type parameters
```cpp
template <typename T, size_t N>
class Array {
    T data[N];
};
Array<int, 5> a;
```

---

## 3. Template specialization: full vs partial

| | Full specialization | Partial specialization |
|---|---|---|
| Applies to | classes **and** functions | **classes only** |
| Customizes for | one exact type | a *pattern* of types (`T*`, `T&`, `T,T`, ...) |
| Syntax | `template<> class Foo<bool> {...}` | `template<typename T> class Foo<T*> {...}` |

### Full specialization
Provide a completely custom implementation for one specific type:
```cpp
template <typename T>
class Printer {
public:
    void print(T val) { cout << val << endl; }
};

template <>
class Printer<bool> {
public:
    void print(bool val) { cout << (val ? "true" : "false") << endl; }
};
```
For **functions**, full specialization looks like:
```cpp
template <typename T>
void print(T value) { cout << value << endl; }

template <>
void print<bool>(bool value) { cout << (value ? "true" : "false") << endl; }
```
⚠️ **Functions cannot be partially specialized** — only fully. If you need pattern-based behavior for functions, use plain **overloading** instead (see Q3's solution for exactly this situation).

### Partial specialization (classes only)
Customizes behavior for a *family* of types rather than one exact type.

**Pointer specialization** — a very common real pattern (this is basically what Q5 asks you to build):
```cpp
template <typename T>
class Info {
public:
    void describe() { cout << "generic type" << endl; }
};

template <typename T>
class Info<T*> {
public:
    void describe() { cout << "pointer type" << endl; }
};
```

**Reference specialization** — where this actually matters: a generic "print value" utility that needs to treat references specially (e.g. show that no copy is being stored, or unwrap the reference before printing), or a serializer that must reject/handle reference members differently from value members:
```cpp
template <typename T>
class Info<T&> {
public:
    void describe() { cout << "reference type (refers to existing object)" << endl; }
};
```
In practice you'll see `T&` partial specializations most often inside **trait-like utilities** (e.g. a hand-rolled version of `std::remove_reference`), not everyday application code — but the pattern-matching mechanism is the same one `std::remove_reference`, `std::is_pointer`, etc. are built on internally.

**Matching two parameters against each other:**
```cpp
template <typename T, typename U>
class PairInfo {
public:
    void describe() { cout << "different types" << endl; }
};

template <typename T>
class PairInfo<T, T> {   // partial: pattern is "both the same"
public:
    void describe() { cout << "same type" << endl; }
};
```

📌 **Naming check:** `A<T*>` is *partial* (still has a free parameter `T`). `A<int>` is *full* (nothing left to fill in).

---

## 4. Non-type template parameters

A template parameter doesn't have to be a *type* — it can be a compile-time **value**. `std::array<T, N>` is built exactly this way.

```cpp
template <typename T, int N>
class Array {
    T data[N];
};

Array<int, 5> a;     // N must be known at COMPILE TIME
```
```cpp
int n;
cin >> n;
Array<int, n> a;      // ❌ n isn't known until runtime — won't compile
constexpr int n = 5;
Array<int, n> a;      // ✅ constexpr is a compile-time constant, this works
```

Non-type parameters work on functions too:
```cpp
template <int N>
int square() { return N * N; }

square<4>();   // 16, computed at compile time capable, N is baked into the instantiation
```

### `fun<10>` vs `fun(10)` — what's actually different
- `fun(10)` — `10` is a **runtime argument**. It's stored in a variable/register when the function runs; the compiler generates *one* version of `fun` and the value `10` flows through it like any other argument.
- `fun<10>()` — `10` is a **compile-time template parameter**. The compiler generates a *separate, distinct* function `fun<10>` (different from `fun<20>`) with `10` baked directly into the machine code as a constant — there's no "variable holding 10" at runtime at all. This is what makes compile-time recursion (below) possible, and it's also why heavy use of non-type parameters can bloat binary size (a new function per value used).

### Compile-time recursion — the classic example
```cpp
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {     // full specialization = the base case
    static constexpr int value = 1;
};

constexpr int x = Factorial<5>::value;   // = 120, computed entirely at compile time
```
This is template **metaprogramming**: `Factorial<5>`, `Factorial<4>`, ..., `Factorial<0>` are five distinct types, each computed by the compiler before your program even runs.

---

## 5. Multiple template parameters, default template arguments

You already know multiple parameters:
```cpp
template <typename T, typename U>
class Pair { T first; U second; };

template <typename T, size_t N>
class Array { T data[N]; };
```

### Default template arguments
```cpp
template <typename T, typename U = int>
class Pair { T first; U second; };

Pair<double> p;   // same as Pair<double, int>
```
⚠️ **Ordering rule, precisely stated:** it's not really "right to left" — the actual rule is *once a parameter has a default, every parameter after it must also have a default*. In practice that pushes defaults toward the end of the list, which is why it *looks* like a right-to-left thing:
```cpp
template <typename T = int, typename U>   // ❌ error: T has a default but U (after it) doesn't
class Bad { };
```

Non-type parameters can default too:
```cpp
template <typename T, size_t N = 10>
class Buffer { T data[N]; };
```

Function template defaults, and how *overriding* them works:
```cpp
template <typename T, typename U = int>
void foo(T a, U b = U{});

foo(10, 2.5);   // instantiates foo<int, double> — your explicit double argument
                // OVERRIDES the default U=int, it doesn't fight with it
```
The default is only a fallback for when deduction/the call site doesn't otherwise pin down that parameter — an explicit argument always wins.

### What `std::vector` actually looks like
```cpp
template <typename T, typename Allocator = std::allocator<T>>
class vector;
```
That's why `vector<int>` "just works" without you ever mentioning an allocator — `Allocator` quietly defaults to the standard heap allocator for `T`.

### Combining default arguments with specialization
Defaults apply to the *primary* template; a specialization can still trigger even when the caller relied on the default:
```cpp
template <typename T, typename Compare = std::less<T>>
class SortedBox {
public:
    void describe() { cout << "generic sorted box" << endl; }
};

// Specialize for when Compare is explicitly std::greater<T> (descending order)
template <typename T>
class SortedBox<T, std::greater<T>> {
public:
    void describe() { cout << "descending sorted box" << endl; }
};

SortedBox<int> a;                        // uses default Compare -> "generic sorted box"
SortedBox<int, std::greater<int>> b;     // matches the specialization -> "descending sorted box"
```
The default argument and the specialization pattern are independent mechanisms that happen to interact at the call site — the default just decides *which* arguments get filled in before the specialization matching even happens.

---

## 6. Templates + operator overloading interplay

Operator overloading and templates compose for free — a template function that uses `+` will work for *any* type where `operator+` is defined, whether that type is built-in or user-defined:

```cpp
struct Point { int x, y; };

Point operator+(const Point& a, const Point& b) {
    return {a.x + b.x, a.y + b.y};
}

template <typename T>
T add(T a, T b) { return a + b; }

add(Point{1,2}, Point{3,4});   // works — Point's own operator+ is what `a+b` calls
```
Without that free-standing `operator+`, `add<Point>` wouldn't compile — the template doesn't grant `+` to `Point`, it just *requires* `+` to already exist for whatever `T` you use it with.

Operators can themselves be templates, and can even mix two *different* template instantiations:
```cpp
template <typename T, typename U>
auto operator+(const Box<T>& a, const Box<U>& b) {
    return a.get() + b.get();
}

Box<int> a(10);
Box<double> b(2.5);
auto result = a + b;   // int + double -> double, via the templated operator+
```

### Modern C++: constraining with `requires` (preview — full concepts in Level 18)
```cpp
template <typename T>
requires requires(T a, T b) { a + b; }
T add(T a, T b) { return a + b; }
```
This says: "only instantiate `add<T>` for types where the expression `a + b` is actually valid." Calling `add` with a type that has no `operator+` now fails with a much clearer error at the call site, instead of a wall of template-internals errors. This is exactly what `static_assert` (Topic 9) does manually — `requires` is the compiler-native, purpose-built version of it.

---

## 7. `typename` vs `class`

**In a template parameter list, they are 100% interchangeable:**
```cpp
template <typename T> ...   // identical
template <class T>    ...   // identical
```
This is purely historical — templates originally only had `class`. `typename` was added later specifically because "class" was misleading when `T` gets substituted with `int`, `double`, etc. (not classes at all). `typename` is now the conventional choice since it reads correctly for any type.

### `typename`'s *other* job: disambiguating dependent names
This is where `typename` stops being cosmetic and becomes load-bearing. Inside a template, when you refer to a name that depends on `T` (like `T::something`), the compiler **cannot know**, until `T` is actually substituted, whether `something` is a **type** or a **value/static member**. By default it assumes *value* — you must say `typename` to tell it "this is a type":

```cpp
template <typename T>
void foo() {
    typename T::value_type x;   // "T::value_type is a TYPE — declare a variable x of that type"
    // T::value_type x;         // ❌ error: without `typename`, compiler assumes value_type is
                                 //    a static data member, and this line looks like nonsense
}
```

**Concrete example using `vector`:**
```cpp
template <typename T>
void printFirst(const std::vector<T>& v) {
    typename std::vector<T>::const_iterator it = v.begin();
    cout << *it << endl;
}
```
`std::vector<T>::const_iterator` is a *type* nested inside `vector<T>` — but since `vector<T>` depends on the template parameter `T`, the compiler needs `typename` to know that `const_iterator` names a type and not (say) a static member function or variable. Outside a template, with a concrete type like `std::vector<int>`, you never need `typename` — the compiler already knows `vector<int>::const_iterator` is a type because there's no `T` left to be ambiguous about. (In modern code you'd just write `auto it = v.begin();` and sidestep this entirely — but understanding *why* `typename` is sometimes mandatory matters once you write your own generic containers/traits.)

---

## 8. Where template code lives (header-only)

A template is **not compiled into machine code until it's instantiated with a real type**. This is the single fact that explains everything else in this section.

### Why this forces templates into headers
```cpp
// math_utils.h
template <typename T>
T triple(T x) { return x * 3; }
```
When `main.cpp` calls `triple(5)`, the compiler needs to see the *entire body* of `triple` **at that call site** in order to generate `triple<int>`. If the definition only lived in `math_utils.cpp`, and `main.cpp` only saw a declaration (`template <typename T> T triple(T x);`), the compiler compiling `main.cpp` would have no body to instantiate from — it would emit a *call* to a function that was never actually generated anywhere. At link time, the linker looks for `triple<int>`'s machine code and finds nothing → **unresolved external symbol / linker error**. This is different from a normal (non-template) function, where the `.cpp` file compiles the function body exactly once into object code, and other files just need the declaration to link against it.

### The fix, option A: header-only (by far the most common)
Put the full definition in the header, `#include` it wherever it's used. Every translation unit that calls `triple` sees the full body and can instantiate it itself. This is why almost every template-heavy library (including the STL) ships as headers.

### The fix, option B: explicit instantiation
Keep the definition in a `.cpp`, but force the compiler to generate specific versions of it *in that one .cpp*, so real object code exists for the linker to find:
```cpp
// math_utils.cpp
template <typename T>
T triple(T x) { return x * 3; }

template int triple<int>(int);       // explicit instantiation: "generate triple<int> HERE"
template double triple<double>(double);
```
Now other `.cpp` files can `#include "math_utils.h"` (declaration only) and call `triple<int>`/`triple<double>` — the linker finds the object code that was explicitly generated in `math_utils.cpp`. The catch: this only works for the specific types you listed. Anyone who calls `triple<string>` still gets a linker error, because you never explicitly instantiated that version.

### `extern template`, and the `.tpp`/`.hpp` convention
- **`extern template`** is the opposite of explicit instantiation — it tells the compiler "don't instantiate this here, trust that some *other* .cpp already did (via explicit instantiation)." It's a compile-time-speed optimization: without it, if 10 different .cpp files all `#include` a header and call `triple<int>`, the compiler redundantly instantiates and compiles `triple<int>` 10 times (the linker then throws away the duplicates). `extern template int triple<int>;` in each of those files skips that repeated work, on the promise that exactly one .cpp does the real explicit instantiation.
- **`.tpp` (or `.tcc`) files** are a header-only *organization* convention, not a language feature — the compiler doesn't know what `.tpp` means. The idea: put the class **declaration** in `Foo.hpp` (clean, readable, no implementation clutter), put the **member function definitions** in `Foo.tpp`, and `#include "Foo.tpp"` at the very bottom of `Foo.hpp`. End result is functionally identical to writing everything directly in the `.hpp` — it's purely to keep declaration and implementation visually separated while still guaranteeing the definitions are visible wherever the header is included.

📌 **In short:** header-only is the default you should reach for. Explicit instantiation is a niche tool for library authors who want to hide implementation details and control exactly which types are supported, while shortening compile times for consumers.

---

## 9. Basic constraints via `static_assert`

`static_assert` checks a condition **at compile time**, and if it's false, the build fails with your custom message right there — instead of failing later with a confusing runtime bug or a wall of template-internals errors.

```cpp
static_assert(condition, "message");
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");
```

```cpp
template <typename T>
void process(T value) {
    static_assert(sizeof(T) >= 4, "T must be at least 4 bytes");
}
```

| | `assert` | `static_assert` |
|---|---|---|
| Checked | at **runtime** | at **compile time** |
| Cost | can be compiled out (`NDEBUG`), still costs cycles when active | **zero** runtime cost — it's gone after compilation |
| Failure | program aborts while running | build simply doesn't produce a binary |

For type-checking specifically, `<type_traits>` gives you ready-made predicates instead of hand-rolling logic:
```cpp
template <typename T>
void requireArithmetic(T x) {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
}
```
(`is_arithmetic_v<T>` is shorthand for `is_arithmetic<T>::value` — the `_v` suffix convention is everywhere in `<type_traits>`.)

### Why a typo like `is_arithmetric` (missing "h") might *not* show a red squiggle immediately
Two separate things are going on, and it's worth knowing both:
1. **Two-phase lookup.** Inside an uninstantiated template, the compiler only fully checks *non-dependent* names (names that don't involve `T`) at the point the template is *defined*. Anything depending on `T` is deferred until the template is actually instantiated with a real type. `is_arithmetric<T>` unfortunately still looks like "some identifier applied to T" syntactically, so depending on the tooling it can get lumped in with the deferred checks rather than flagged instantly.
2. **IntelliSense ≠ the compiler.** VS Code's squiggly-line checking (via clangd or the MS C++ extension) runs a *separate*, lighter-weight parser for live feedback — it's good, but it doesn't always fully resolve every template code path the way a real build does, especially for templates that are never actually instantiated anywhere in the file with a concrete type. The authoritative answer is always "does it actually compile," not "does VS Code show red." In your `main()`, `requireArithmetic(string("helo"))` *does* instantiate the template — so a real build there would catch the typo immediately as an undeclared-identifier error (unrelated to the intended `static_assert` failure on `string` not being arithmetic, which is the separate, correct failure you *were* trying to trigger).

📌 **Rule of thumb:** if a build vs. IntelliSense ever disagree, trust the build.

---

*(concepts — the modern, first-class replacement for hand-rolled `static_assert` type constraints — arrive in Level 18)*