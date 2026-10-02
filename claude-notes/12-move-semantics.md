# Level 12 — Move Semantics & Perfect Forwarding

> 🧠 **Legend used in this file** — boxes like this one (🧠 and ⚠️) are **memory hooks / gotchas / interview flags**. Everything else is explanation. Each section starts with a one-line "what you must be able to say" and ends with the traps.

**Standards cheat:** 🟦 C++11 · 🟩 C++14 · 🟨 C++17 · 🟧 C++20 · 🟥 C++23. I tag features with these so you learn *when* C++ grew each idea (a full timeline is in §10).

---

## 0. The whole level in 8 sentences (read this first, re-read after 6 months)

1. Copying a `string`/`vector` copies its **heap buffer** (expensive). Moving just **steals the pointer** (cheap).
2. The compiler only steals from things that are **about to die** — temporaries (rvalues).
3. `T&&` (rvalue reference) is the *type* that lets a function say "give me things that are about to die".
4. `std::move(x)` **does not move**. It is a cast that says "treat x as about-to-die". The *move constructor* does the stealing.
5. A **named** variable is always an lvalue — even if its type is `T&&`. So you must `std::move` / `std::forward` it again to pass it on as an rvalue.
6. `template<class T> void f(T&& x)` is a **forwarding reference** (not an rvalue ref). `T` remembers if the caller passed an lvalue (`T = U&`) or rvalue (`T = U`). `std::forward<T>(x)` uses that memory to re-create the original category.
7. Returning a local **by value**: write `return x;` — never `return std::move(x);` (it blocks elision and turns 0 moves into 1).
8. Declaring **any** of destructor / copy ctor / copy assign / move ctor / move assign **kills the implicit move operations** → your "move" silently becomes a **copy**. Fix: Rule of 0 (declare none) or Rule of 5 (declare all).

---

## 1. Value categories (lvalue / prvalue / xvalue)

**You must be able to say:** every expression has a *type* and a *value category*. The category answers two questions: **"does it have an identity (a place in memory I can point to)?"** and **"can I steal from it?"**

### 1.1 The tree 🟦 (C++11 introduced xvalue/prvalue/glvalue)

```
                    Expression
                   /          \
             glvalue          rvalue
            /       \        /      \
        lvalue      xvalue        prvalue
```

- **glvalue** = "generalized lvalue" = *has identity* = lvalue + xvalue
- **rvalue** = *can be moved from* = xvalue + prvalue
- **xvalue** sits in the overlap: **has identity AND can be moved from** (that is why it's both a glvalue and an rvalue)

### 1.2 The 2×2 that makes it stick

|                         | **Can be moved from? NO** | **Can be moved from? YES** |
| ----------------------- | ------------------------- | -------------------------- |
| **Has identity? YES**   | **lvalue**                | **xvalue**                 |
| **Has identity? NO**    | *(doesn't exist)*         | **prvalue**                |

| Category | One-line intuition | Example |
| -------- | ------------------ | ------- |
| **lvalue** | "an existing object, I know where it lives, keep using it" | `x`, `*p`, `arr[3]`, `obj.member`, `++x` |
| **prvalue** | "a pure value / brand-new temporary, no name" | `10`, `x + 5`, `string("hi")`, `foo()` (returns by value), `x++`, `&x` |
| **xvalue** | "an existing object, but its guts may be taken" | `std::move(s)`, `static_cast<string&&>(s)`, `foo()` returning `T&&` |

```cpp
int x = 10;          // x: lvalue.   10: prvalue
int* p = &x;         // &x: prvalue. p: lvalue. *p: lvalue
int y = x + 5;       // x + 5: prvalue
string s = "hello";
string t = std::move(s);   // s, t: lvalues.   std::move(s): xvalue
                           // "hello": see the gotcha below (it's an lvalue!)
```

### 1.3 How to classify any expression (decision rules)

| Expression | Category |
| ---------- | -------- |
| name of a variable / parameter / member (any type, even `T&&`) | **lvalue** |
| call to function returning `T&` | **lvalue** |
| call to function returning `T` (by value) | **prvalue** |
| call to function returning `T&&` (e.g. `std::move`) | **xvalue** |
| literal `10`, `3.14`, `true`, `nullptr` | **prvalue** |
| `++x`, `--x`, `x = 5`, `x += 1`, `*p`, `a[i]` | **lvalue** |
| `x++`, `x--`, `a + b`, `&x`, `this` | **prvalue** |
| `static_cast<T&&>(x)` | **xvalue** |
| `obj.member` when `obj` is an lvalue | lvalue |
| `std::move(obj).member` | xvalue |

### 1.4 Gotchas

> ⚠️ **"lvalue = can be on the left of `=`" is the OLD, WRONG definition.**
> `const int x = 10; x = 20;` → **error**, yet `x` is still an lvalue. Lvalue means *has identity*, not *assignable*. (Mnemonic: **l**ocator value.)

> ⚠️ **String literals are lvalues.** `"hello"` has type `const char[6]` and lives in static memory, so it has identity → lvalue. In `string s = "hello";`, what's a prvalue is the *temporary `std::string`* the compiler builds from it. (Your notes called `"hello"` a prvalue — that's true for `10`, false for `"hello"`.)

> ⚠️ **A named `T&&` is an lvalue.**
>
> ```cpp
> void foo(string&& s) { /* here `s` is an LVALUE expression. Its TYPE is string&&, its CATEGORY is lvalue. */ }
> ```
>
> Type ≠ category. This is the single most confusing thing in this level.

> ⚠️ **`std::move(x)` does not change `x`.** It changes the category of the *expression* `std::move(x)` from lvalue to xvalue. `x` itself is untouched.

> 🧠 **Return-type rule to memorise:** returns `T&` → lvalue, returns `T&&` → xvalue, returns `T` → prvalue.
> So `T&` ≈ lvalue, `T&&` ≈ xvalue (as *expression results*), `T` ≈ prvalue.

---

## 2. Rvalue references (`T&&`) vs lvalue references (`T&`)

**You must be able to say:** `T&` binds to lvalues, `T&&` binds to rvalues (prvalue/xvalue), `const T&` binds to anything — and this asymmetry is what lets overloads *detect* "this is a temporary".

### 2.1 What binds to what

| Reference type | lvalue | const lvalue | prvalue / xvalue | const rvalue |
| -------------- | :----: | :----------: | :--------------: | :----------: |
| `T&`           | ✅ | ❌ | ❌ | ❌ |
| `const T&`     | ✅ | ✅ | ✅ | ✅ |
| `T&&`          | ❌ | ❌ | ✅ | ❌ |
| `const T&&`    | ❌ | ❌ | ✅ | ✅ |

```cpp
int x = 10;
int&  a = x;            // OK
int&  b = 10;           // ERROR: 10 is a prvalue, T& can't bind
const int& c = 10;      // OK  <-- the famous exception (old C++98 rule)
int&& d = 10;           // OK: binds to the temporary
int&& e = x;            // ERROR: x is an lvalue
int&& f = std::move(x); // OK: std::move(x) is an xvalue
```

### 2.2 A reference is not a new object

`int& ref = x;` creates **no new int** — `ref` is just another name for `x`.

### 2.3 Lifetime extension

```cpp
string&& r = string("Hello");   // temporary would normally die at the `;`
                                // binding it to a reference EXTENDS its life to r's scope
```
`const T&` also extends lifetime. That's why `const string& s = makeName();` is safe.

### 2.4 Why `T&&` exists at all

Before C++11 you couldn't tell "temporary I can steal from" from "variable someone still uses" — both looked like `const T&`. `T&&` gives overload resolution a way to **distinguish them**:

```cpp
void handle(const string& s);   // "you give me something I must not damage"  -> copy
void handle(string&& s);        // "you give me something dying"              -> steal
```
When both exist: lvalue → `const&`; rvalue → `&&` (the more specialised one wins).

> 🧠 `T&&` is a **promise from the caller**: "this object is expendable, do what you like with its guts." It is not magic — it's just a reference type used to pick an overload.

---

## 3. `std::move` — just a cast 🟦 (C++11; `constexpr` since 🟩 C++14)

**You must be able to say:** `std::move` does **zero** moving. It's `static_cast<T&&>(x)`. The *move constructor/assignment* (selected because the argument is now an rvalue) does the moving.

```cpp
// roughly what the standard library does (header <utility>)
template<class T>
constexpr std::remove_reference_t<T>&& move(T&& t) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(t);
}
```

```cpp
string a = "hello";
string b = static_cast<string&&>(a);   // identical to std::move(a): calls the MOVE ctor
string c = std::move(a);               // same thing

auto&& r = std::move(a);               // NOTHING moves. r is just a reference to a (xvalue-bound)
string d = std::move(a);               // THIS moves (move ctor runs and steals a's buffer)
```

### 3.1 Traps

> ⚠️ **`std::move` on a `const` object → silently copies.**
>
> ```cpp
> const string s = "x";
> string t = std::move(s);   // std::move(s) is `const string&&`
>                            // move ctor wants `string&&` (non-const, since it modifies the source) -> can't bind
>                            // falls back to copy ctor (const string&)
> ```

> ⚠️ **`std::move` into a `const T&` parameter is pointless.**
>
> ```cpp
> void foo(const string& s);
> foo(std::move(x));   // foo only reads; nothing is stolen. Just misleading code.
> ```

> ⚠️ **Never use `x` (its value) after `std::move(x)`** unless you first reassign it. It's "valid but unspecified" (§4.3).

> 🧠 Two different `std::move`s exist: `<utility>` (this one, 1 argument) and `<algorithm>` `std::move(first, last, out)` (moves a **range**, like `std::copy`). Same name, different function.

---

## 4. Move constructor & move assignment (deeper)

**You must be able to say:** a move op *steals the resource, leaves the source in a safe empty state, and is `noexcept`*.

### 4.1 Full template (memorise the shape)

```cpp
class Buffer {
    int*   data_;
    size_t size_;
public:
    explicit Buffer(size_t n) : data_(new int[n]{}), size_(n) {}
    ~Buffer() { delete[] data_; }

    // --- COPY ctor: deep copy (allocate NEW memory) ---
    Buffer(const Buffer& o) : data_(new int[o.size_]), size_(o.size_) {
        std::copy(o.data_, o.data_ + size_, data_);
    }

    // --- MOVE ctor: steal, then empty the source ---
    Buffer(Buffer&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr;      // <-- WITHOUT THIS: both objects delete[] the same memory = double free
        o.size_ = 0;
    }

    // --- MOVE assignment: release mine, steal, empty the source ---
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {           // guards `x = std::move(x)`
            delete[] data_;         // 1. release what I own
            data_ = o.data_;        // 2. steal
            size_ = o.size_;
            o.data_ = nullptr;      // 3. empty the source
            o.size_ = 0;
        }
        return *this;               // 4. ALWAYS return *this (forgetting = UB)
    }
};
```

| Rule | Why |
| ---- | --- |
| Null/zero the source's pointer | otherwise double-`delete[]` when both destructors run |
| Mark both moves **`noexcept`** | `std::vector` only *moves* on reallocation if the move is `noexcept`, otherwise it **copies** (§9.3) |
| Move **ctor**: no self-check needed | an object can't be constructed from itself |
| Move **assignment**: self-check `this != &o` | `x = std::move(x)` must not delete the data it's about to "steal" |
| Move assign must `delete[]` the old resource first | otherwise leak |
| Move assign must `return *this` | falling off the end of a non-void function is UB |

> ⚠️ **A "move" that copies element by element is not a move.** If your move ctor loops and assigns values, you've written a slow copy. A move is *pointer steals + reset source*.

### 4.2 Members: move each member (`std::move` inside the init list)

```cpp
struct Person {
    std::string name;
    std::vector<int> scores;
    Person(Person&& o) noexcept : name(std::move(o.name)), scores(std::move(o.scores)) {}
};
```
Inside the ctor `o` is an lvalue (it has a name!). Without `std::move(o.name)` you'd **copy** `name`.

### 4.3 Moved-from state ("valid but unspecified")

After `std::move(x)` was consumed by a move op, `x` is still a **fully alive object** — its destructor *will* run normally.

| Safe on a moved-from object | Not safe |
| --- | --- |
| let it be destroyed | assuming it's empty / has old value |
| assign a new value to it (`s = "new"`) | reading its old contents |
| call functions with **no preconditions** (`size()`, `empty()`, `clear()`) | calling things that *require* a specific state (`front()`, `operator[]`) |

Specifics: `unique_ptr` and `shared_ptr` moved-from → **guaranteed null**. `std::string`/`vector` → **in practice empty** but only "valid but unspecified" by standard (e.g. a small string using SSO may just copy chars).

**Moved-from vs destroyed:**

| | **moved-from** | **destroyed** |
| - | - | - |
| Object alive? | ✅ yes | ❌ no, lifetime ended |
| Destructor ran? | not yet (will run later) | yes |
| Can I call methods? | yes (safe subset) | **UB** — anything |
| Can I assign to it? | yes, revives its value | **UB** |

---

## 5. When does the compiler generate move operations? 🟦 (C++11)

**You must be able to say:** the compiler only *implicitly declares* a move ctor / move assign if you have declared **none** of: destructor, copy ctor, copy assign, move ctor, move assign. Otherwise "moving" quietly falls back to **copying**.

### 5.1 The full table (Hinnant's table)

| You declare → | default ctor | destructor | copy ctor | copy assign | move ctor | move assign |
| --- | --- | --- | --- | --- | --- | --- |
| *nothing* | ✅ default | ✅ default | ✅ default | ✅ default | ✅ default | ✅ default |
| **destructor** | ✅ | user | default (deprecated) | default (deprecated) | ❌ *not declared* | ❌ *not declared* |
| **copy ctor** | ❌ | ✅ | user | default (deprecated) | ❌ *not declared* | ❌ *not declared* |
| **copy assign** | ✅ | ✅ | default (deprecated) | user | ❌ *not declared* | ❌ *not declared* |
| **move ctor** | ✅ | ✅ | 🚫 **deleted** | 🚫 **deleted** | user | ❌ *not declared* |
| **move assign** | ✅ | ✅ | 🚫 **deleted** | 🚫 **deleted** | ❌ *not declared* | user |

- ❌ *not declared* = doesn't exist, overload resolution ignores it → the copy op is used for rvalues (because `const T&` accepts rvalues).
- 🚫 *deleted* = you declared a move, so copying is disabled → copy attempts are errors (this is how you make a move-only type).
- "deprecated" = the compiler still generates it (C++11 kept old code working) but it's discouraged.

### 5.2 The gotchas

> ⚠️ **Even `~T() {}` (empty!) or `~T() = default;` kills implicit moves.** The compiler thinks "he manages resources by hand, I don't know how to steal for him" → no implicit move.
>
> ```cpp
> struct WithDtor { std::string s; ~WithDtor() {} };
> WithDtor a, b = std::move(a);   // COPIES s. No implicit move ctor exists.
> ```

> ⚠️ **`= default` counts as declaring.** `T(const T&) = default;` still suppresses implicit moves. If you default the copy ops, default the move ops too:
>
> ```cpp
> T(const T&) = default;  T& operator=(const T&) = default;
> T(T&&) noexcept = default;  T& operator=(T&&) noexcept = default;
> ```

> ⚠️ **A non-movable member ⇒ no move.** `std::mutex` is neither copyable nor movable.
>
> ```cpp
> struct A { std::mutex m; };
> A a; A b = std::move(a);   // COMPILE ERROR
> ```
>
> Why an error and not "falls back to copy"? The implicit move ctor exists but is defined as **deleted**; a defaulted move that is deleted is *ignored* by overload resolution (fixed by DR CWG 1402, 🟩 C++14), so it tries the copy ctor → which is **also** implicitly deleted (mutex not copyable) → error.

### 5.3 The checklist for "is `b = std::move(a)` really a move?"

1. `std::move(a)` gives an xvalue.
2. Overload resolution looks at ctors: `A(A&&)` preferred, `A(const A&)` fallback.
3. Does `A(A&&)` **exist**? (not suppressed by any user-declared special member)
4. If it exists, is it **deleted** (a member is non-movable)? Deleted-defaulted moves are ignored → fallback.
5. Is `a` non-`const`? (`const` → `A&&` can't bind → copy)
6. Only if all pass → real move.

### 5.4 The rules

- **Rule of 0** (best): declare none; hold members that manage themselves (`vector`, `string`, `unique_ptr`) → everything is generated correctly.
- **Rule of 5**: if you must write any one, write/`default`/`delete` all five.
- **Rule of 3 (old, C++98)**: destructor + copy ctor + copy assign. Move came with C++11 → became the Rule of 5.

---

## 6. Forwarding references (`T&&` in templates) vs rvalue references

**You must be able to say:** `T&&` is a forwarding reference **only if `T` is deduced for *this* call** and the parameter is written exactly `T&&`. Otherwise it's a plain rvalue reference.

### 6.1 Telling them apart (interview classic)

```cpp
void foo(string&& s);                 // plain rvalue reference (no deduction)

template<typename T> void foo(T&& s); // FORWARDING reference (T deduced)
auto&& x = something;                 // FORWARDING reference too (auto is deduced) 🟦
```

**NOT forwarding references** (even though they look similar):

```cpp
template<typename T> void f(std::vector<T>&& v);  // && attached to vector<T>, not bare T   -> rvalue ref
template<typename T> void f(const T&& x);         // const breaks it                         -> rvalue ref to const
template<typename T>
class A { public: void foo(T&& x); };             // T is fixed when the class is instantiated, NOT deduced by foo
                                                  // A<int> makes foo(int&&) -> plain rvalue ref
```

Quick test: *"Is the exact form `T&&` (nothing else attached) and does the compiler deduce `T` at this call?"* Both yes → forwarding ref.

### 6.2 Deduction: how `T` remembers what you passed

```cpp
template<typename T> void foo(T&& x);

int a = 10;
foo(a);          // a is an lvalue  -> special rule: T = int&
                 //                    T&& = int& && -> collapses to int&        (x is an lvalue ref)

foo(std::move(a)); // xvalue        -> T = int,  T&& = int&&
foo(10);           // prvalue       -> T = int,  T&& = int&&
```

### 6.3 Reference collapsing 🟦

You can't write a reference to a reference, but templates/typedefs can produce them. The rule: **if either is `&`, the result is `&`. Only `&& &&` gives `&&`.**

| Combination | Result |
| --- | --- |
| `T& &`   | `T&` |
| `T& &&`  | `T&` |
| `T&& &`  | `T&` |
| `T&& &&` | `T&&` |

> 🧠 This is the whole trick: an lvalue argument makes `T = U&`, then `T&&` = `U& &&` = `U&`. An rvalue argument makes `T = U`, then `T&&` = `U&&`.

---

## 7. `std::forward` and perfect forwarding

**You must be able to say:** inside a wrapper, the parameter is a *named* variable = always an lvalue. `std::forward<T>(x)` restores the caller's original category.

### 7.1 The problem

```cpp
void foo(int&)  { cout << "lvalue\n"; }
void foo(int&&) { cout << "rvalue\n"; }

template<typename T>
void wrapperBad(T&& x)  { foo(x); }                   // x has a name -> ALWAYS calls foo(int&)
template<typename T>
void wrapper(T&& x)     { foo(std::forward<T>(x)); }  // preserves the original category

int a = 10;
wrapper(a);    // T = int&  -> forward<int&>(x)  -> lvalue -> foo(int&)   "lvalue"
wrapper(10);   // T = int   -> forward<int>(x)   -> rvalue -> foo(int&&)  "rvalue"
```

### 7.2 What `forward` does (conceptually)

```cpp
// conceptually:  static_cast<T&&>(x)
//  T = int&  ->  int& &&  -> int&   (stays lvalue)
//  T = int   ->  int&&              (becomes xvalue)
```
Real one takes `remove_reference_t<T>&` so you **must** write `forward<T>(x)` explicitly — deduction from `x` would always give "lvalue" and lose the information.

| Call | `T` | `x` (expression) | `std::forward<T>(x)` |
| --- | --- | --- | --- |
| `wrapper(s)` | `string&` | lvalue | lvalue |
| `wrapper(std::move(s))` | `string` | lvalue | xvalue |
| `wrapper(string("hi"))` | `string` | lvalue | xvalue |
| `wrapper(const_s)` | `const string&` | lvalue | const lvalue |
| `wrapper(std::move(const_s))` | `const string` | lvalue | const xvalue |

### 7.3 `forward` vs `move` — the difference

| | `std::move(x)` | `std::forward<T>(x)` |
| --- | --- | --- |
| Effect | **unconditionally** → xvalue | **conditionally** → xvalue only if caller passed an rvalue |
| Use when | *you* decide to give up the object | you're a generic pass-through and must not change the caller's intent |

```cpp
template<class T> void wrapperBug(T&& x) { foo(std::move(x)); }
// wrapperBug(s) with an lvalue s: silently STEALS from the caller's s!  <- the Q6 bug
```

> 🧠 **Forward exactly once per argument.**
>
> ```cpp
> foo(std::forward<T>(x));
> bar(std::forward<T>(x));   // BUG: if foo consumed an rvalue x, bar sees a hollow object
> ```

### 7.4 Perfect forwarding = forwarding reference + `std::forward`

"Passing an argument through a generic function while preserving its value category **and** `const`ness." Real-world users: `std::make_unique` 🟩 (C++14), `std::make_shared` 🟦, `emplace_back` 🟦, `std::thread`'s constructor, `std::function::operator()`.

### 7.5 Variadic templates — the syntax you asked about (`Args` and `...`) 🟦 (C++11)

Q5/Q6 use a **parameter pack**: "zero or more of these, of possibly different types".

```cpp
template<typename T, typename... Args>       // (1) Args is a *template parameter pack*: a list of types
unique_ptr<T> myMakeUnique(Args&&... args)   // (2) args is a *function parameter pack*: a list of values
{                                            //     `Args&&...` means: each one is its own forwarding reference
    return unique_ptr<T>(new T(std::forward<Args>(args)...));   // (3) pack EXPANSION
}
```

| Piece | Meaning |
| --- | --- |
| `typename... Args` | "a pack of types" (the `...` goes **before** the name when *declaring*) |
| `Args&&... args` | "a pack of parameters; the i-th has type `Args_i&&`" |
| `pattern...` (the `...` **after** an expression) | **expand**: repeat `pattern` once per element, separated by commas |
| `sizeof...(Args)` | number of elements in the pack |

**Expansion, worked out by hand.** Call: `myMakeUnique<Order>(id, name, 3)` where `int id; string name;`

```
Args = { int&,  string&,  int }         // lvalue -> U&, rvalue -> U
args = { id,    name,     3   }

pattern:   std::forward<Args>(args)...
expands to std::forward<int&>(id),  std::forward<string&>(name),  std::forward<int>(3)
              -> lvalue                  -> lvalue                     -> rvalue

so:  new Order( forward<int&>(id), forward<string&>(name), forward<int>(3) )
              = new Order( id (copied), name (copied), 3 (moved) )
```
The `...` applies to the **whole** pattern to its left: `f(args)...` → `f(a1), f(a2), f(a3)`; but `f(args...)` → `f(a1, a2, a3)`.

> 🧠 **Why forward and not move here?** Each `args[i]` is a forwarding reference. `forward` moves only the ones the caller passed as rvalues; caller's lvalues stay untouched. `move` would rob every lvalue the caller passed.

Related: 🟨 C++17 **fold expressions** (`(args + ...)`), 🟩 C++14 generic lambdas `[](auto&&... a){ return f(std::forward<decltype(a)>(a)...); }`, 🟧 C++20 templated lambdas `[]<class... A>(A&&... a){...}`.

### 7.6 `emplace_back` — the flagship example

```cpp
class Person { public: Person(std::string name, int age); };
std::vector<Person> v;

std::string name = "sudhanshu";
v.emplace_back(name, 23);              // name is lvalue -> copied into the ctor parameter
v.emplace_back(std::move(name), 23);   // name is xvalue -> moved
// conceptually: template<class... Args> void emplace_back(Args&&... args);
```

---

## 8. RVO / NRVO / copy elision

**You must be able to say:** when you return a local by value, write `return local;`. The compiler builds it directly in the caller's variable (0 copies, 0 moves); if it can't, it moves (never copies). `return std::move(local);` makes it *worse*.

### 8.1 Terminology

| Term | Meaning | Guaranteed? |
| --- | --- | --- |
| **Copy elision** | compiler omits a copy/move it would otherwise perform | umbrella term |
| **RVO** (Return Value Optimization) | `return string("x");` — returning a **prvalue** | 🟨 **C++17: mandatory** |
| **NRVO** (Named RVO) | `string s = ...; return s;` — returning a **named local** | ❌ **never guaranteed** (optional) |

```cpp
string makeName() { return string("sudhanshu"); }   // RVO: builds directly in the caller's `s`
string s = makeName();                              // 0 copies, 0 moves

string makeName2() { string s = "sudhanshu"; return s; }   // NRVO: `s` itself IS the caller's variable (if compiler does it)
```

```
Without NRVO:   local s ──move──> return slot ──> destination
With NRVO:      destination  <── local s is constructed directly here
```

### 8.2 Why `return std::move(local);` is a pessimization

```cpp
string bad()  { string s = "x"; return std::move(s); }   // ❌ 0 moves -> 1 move (blocks NRVO)
string good() { string s = "x"; return s; }              // ✅ NRVO (0 moves) or, at worst, an implicit move
```
- `std::move(s)` is an **xvalue expression of type `string&&`**, not the local variable's *name*. NRVO only applies to `return <name of local>;`. So the compiler is forced to construct `s` locally and then move it out.
- Even *without* NRVO, `return s;` is already treated as an rvalue first (🟦 C++11 rule: "the returned local is about to die, try moving first"). So `std::move` adds nothing when elision fails.
- GCC/Clang warn: `-Wpessimizing-move`, `-Wredundant-move` (in `-Wall`).

> 🧠 **Outcomes:** `return s;` → NRVO (0 moves) *or* implicit move. `return std::move(s);` → **always** 1 move. Never better.

### 8.3 Move ≠ Elision

```
Move : Widget b = std::move(a);   object A ──(move ctor)──> object B     (two objects existed)
RVO  : Widget b = Widget{};       there is only ever ONE object, built in place  (even better than a move)
```

### 8.4 When you SHOULD write `std::move` on a return

| Situation | `return ...;` |
| --- | --- |
| local variable / by-value parameter | `return x;` (implicit move already) |
| **member** of `*this` (`return member_;`) | `return std::move(member_);` if you mean to give it away (NRVO can't apply: it's not a local; otherwise it **copies**) |
| rvalue-reference parameter `T&& p` | 🟦–🟨 `return std::move(p);`; 🟧 C++20 `return p;` already works |
| ternary `return c ? a : b;` | no elision, no implicit move → copies; use if/else |

### 8.5 History of the return-value rules

| Standard | What changed |
| --- | --- |
| **C++98** | copy elision **allowed** (optional). The copy ctor must still be *accessible* even if elided. |
| 🟦 **C++11** | move semantics. `return local;` is tried as an rvalue first (implicit move). |
| 🟨 **C++17** | **guaranteed elision** for prvalues (P0135). `return T(...);` / `T x = T(...);` — the copy/move ctor **need not even exist** (you can return non-movable types like `std::mutex`-holding classes by prvalue). NRVO still optional. |
| 🟧 **C++20** | implicit move extended (P1825): also for `return` of rvalue-reference parameters and cases needing converting constructors. |
| 🟥 **C++23** | implicit move simplified further (P2266): `return x;` where `x` is an id-expression is always treated as an xvalue first. |

Only *members*, `cond ? a : b`, and returning by *reference-to-something-else* still need manual thought.

---

## 9. Move-only types in containers (STL specifics)

### 9.1 `unique_ptr` — the classic move-only type

```cpp
vector<unique_ptr<int>> v;
v.push_back(make_unique<int>(10));      // OK: prvalue -> push_back(T&&) moves it in

unique_ptr<int> p = make_unique<int>(10);
v.push_back(p);                         // ERROR: copy ctor is deleted
v.push_back(std::move(p));              // OK; p is now nullptr (guaranteed)
```

### 9.2 `push_back` vs `emplace_back`

```cpp
void push_back(const T& value);   // copy an existing object in
void push_back(T&& value);        // move a (temporary) object in
template<class... Args> reference emplace_back(Args&&... args);   // 🟨 returns T& since C++17 (void in C++11/14)
```

| | `push_back(Buffer(1,2,3))` | `emplace_back(1,2,3)` |
| --- | --- | --- |
| Step 1 | construct a **temporary** `Buffer` | forward args to `Buffer`'s ctor **inside vector's storage** (placement new) |
| Step 2 | **move** it into vector's slot (copy if no move ctor) | — |
| Step 3 | destroy the temporary | — |
| Cost | ctor + move + dtor | ctor only |

- For a trivially-copyable `struct` the move is just a copy of ints, so the difference is negligible. It matters for types with an expensive/nontrivial move.
- `emplace_back(Buffer(1,2,3))` (passing an existing temp) **doesn't save anything** — it just moves it in like `push_back`. The win comes from passing the **constructor arguments**.

**Extra emplace traps**
- Works with `explicit` constructors; `push_back({...})` doesn't. (Good and bad: you can accidentally do an explicit conversion.)
- Can't forward braced lists: `v.emplace_back({1,2})` fails to deduce. Write `v.emplace_back(vector<int>{1,2})`.
- `vector<unique_ptr<T>> v; v.emplace_back(new T);` leaks if reallocation throws before the ptr is owned. Prefer `push_back(make_unique<T>())`.

### 9.3 Vector growth & `noexcept` (important!)

When a `vector` reallocates it must move existing elements to the new buffer. It uses `std::move_if_noexcept` 🟦:

| Element type | vector on reallocation |
| --- | --- |
| move ctor is `noexcept` | **moves** |
| move ctor may throw **and** copy ctor exists | **copies** (to keep the "strong guarantee": if something throws mid-way, the old buffer is intact) |
| move may throw **and** no copy ctor (move-only) | **moves anyway** (no choice; if it throws, unspecified state) |

Verified experiment (3 `emplace_back`s forcing growth):
```
move ctor NOT noexcept: copy copy copy
move ctor noexcept:     move move move
```
This is why every move op you write should be `noexcept`.

```cpp
std::is_nothrow_move_constructible_v<T>   // 🟨 (_v suffix) asks: is T's move ctor noexcept?
std::move_if_noexcept(x)                  // returns T&& if nothrow-movable or non-copyable, else const T&
```

> 🧠 **Answer to your open question:** "copy deleted + move not `noexcept`" → vector moves anyway (only option). "copy available + move not `noexcept`" → vector **copies**.

---

## 10. C++ standards timeline for this level

| Standard | Feature (this level) |
| --- | --- |
| **C++98/03** | `const T&` binds temporaries; copy elision permitted but optional; `auto_ptr` (broken pseudo-move, removed in C++17) |
| 🟦 **C++11** | rvalue references `T&&`; `std::move`, `std::forward`, `std::move_if_noexcept`; move ctor/assign; implicit-move generation rules; value categories xvalue/prvalue/glvalue; reference collapsing; forwarding references (Scott Meyers' name "universal reference", 2012); `noexcept`; **variadic templates**; `std::unique_ptr`, `std::make_shared`; `emplace_back`; `<type_traits>` (`is_lvalue_reference<T>::value`); `= default` / `= delete`; implicit move on `return local;` |
| 🟩 **C++14** | `std::make_unique`; `_t` aliases (`remove_reference_t<T>`); `constexpr` `std::move`/`std::forward`; generic lambdas (`auto&&`); DR 1402 (deleted defaulted moves ignored) |
| 🟨 **C++17** | **guaranteed copy elision** (prvalues); `_v` variable templates (`is_lvalue_reference_v<T>`, `is_nothrow_move_constructible_v<T>`); fold expressions; `emplace_back` returns a reference; standard adopts the term "**forwarding reference**"; `auto_ptr` removed |
| 🟧 **C++20** | implicit move on more `return` forms (P1825); parenthesised aggregate init (so `make_unique`/`emplace_back` work for aggregates); templated lambdas `[]<class... A>` |
| 🟥 **C++23** | simplified implicit move (P2266); `std::forward_like`; `std::move_only_function` |

**Small vocabulary**
- `is_lvalue_reference<T>::value` (C++11) = `is_lvalue_reference_v<T>` (C++17). The `_v` = "**v**ariable template shortcut" for `::value`. Likewise `_t` (C++14) = shortcut for `::type`.

---

## 11. Common mistakes checklist

| ❌ Mistake | ✅ Fix |
| --- | --- |
| Thinking `std::move` moves | it's a cast; the move ctor moves |
| `return std::move(local);` | `return local;` |
| Move ctor that copies contents | steal the pointer + null the source |
| Move ops without `noexcept` | add `noexcept` (vector will otherwise copy on growth) |
| Forgetting to null the source pointer | double free |
| Move-assign without `return *this` | UB |
| Declared a destructor and expect implicit move | Rule of 0 or Rule of 5 |
| `std::move` on `const` object | it copies; moving needs a modifiable source |
| Using an object after moving from it | reassign first / don't touch |
| `std::move(args)...` in a forwarding wrapper | `std::forward<Args>(args)...` |
| Calling `std::forward` twice on the same arg | forward once |
| Assuming every `T&&` is a forwarding ref | needs deduced bare `T&&` (or `auto&&`) |
| `emplace_back(existingTemporary)` expecting a win | pass **constructor args**, not an object |

## 12. Interview one-liners

- **What does `std::move` do?** `static_cast<T&&>`; enables the move overload; moves nothing itself.
- **Forwarding vs rvalue reference?** Forwarding = deduced `T&&`; binds both categories; rvalue ref = fixed type; binds only rvalues.
- **Why is a named `T&&` parameter an lvalue?** It has a name/identity; use `move`/`forward` to pass it as an rvalue.
- **Why `noexcept` on move?** `vector` uses `move_if_noexcept`; without it, it copies to keep the strong guarantee.
- **Why doesn't my class move after I added a destructor?** Implicit move is suppressed; copy is used instead.
- **NRVO guaranteed?** No. Prvalue RVO is (C++17).
- **`push_back(T&&)` vs `emplace_back`?** push moves an already-built object; emplace builds it in place from arguments.