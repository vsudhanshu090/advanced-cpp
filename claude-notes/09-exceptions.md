# Level 9 — Exceptions & Error Handling

> C++ Course Notes | Reviewed & polished from self-notes
> Companion file: `09-exceptions-solutions.md` (question-by-question walkthrough)

---

## 📋 Table of Contents

1. [try / catch / throw mechanics](#1-trycatchthrow-mechanics)
2. [Standard exception hierarchy](#2-standard-exception-hierarchy)
3. [Writing custom exception classes](#3-writing-custom-exception-classes)
4. [Exception safety guarantees](#4-exception-safety-guarantees)
5. [RAII and exception safety](#5-raii-and-exception-safety)
6. [`noexcept`](#6-noexcept)
7. [Stack unwinding](#7-stack-unwinding)
8. [Multiple catches, `catch(...)`, rethrow](#8-multiple-catches-catch-rethrow)
9. [Error codes vs exceptions](#9-error-codes-vs-exceptions-quantlow-latency-context)

---

## 1. try/catch/throw mechanics

**Mental model:** an exception means *"something went wrong here, and I cannot/will not handle it at this level — hand control to code that can."*

```cpp
try {
    // code that might fail
}
catch (const SomeExceptionType& e) {
    // handle the failure
}
// throw SomeExceptionType(...);  <- reports the failure, from somewhere inside the try
```

### Example

```cpp
void divide(int a, int b) {
    if (b == 0)
        throw std::runtime_error("division by zero");
    std::cout << a / b;
}

int main() {
    try {
        divide(10, 0);
    }
    catch (const std::runtime_error& e) {
        std::cout << e.what();
    }
}
```

**What happens when `throw` runs:**
`divide()` throws → C++ immediately starts searching for a matching `catch` → the stack **unwinds** (local objects in `divide()` are destroyed) → control leaves `divide()`, leaves the `try` block → the first matching `catch` runs.

```cpp
try {
    foo();   // throws
    bar();   // never executes
    baz();   // never executes
}
catch (...) { }
```

> ⚠️ **Key point:** the instant an exception is thrown, stack unwinding begins and every local object on the unwound frames gets destroyed. This is *exactly* why RAII (§5) matters — destructors are your only guaranteed cleanup hook.

### Why catch by (const) reference?

```cpp
catch (const std::exception& e)   // ✅ the idiomatic form
```

| Reason | Explanation |
|---|---|
| **Avoid copying** | Catching by value copies the exception object every time it's caught. |
| **Preserve polymorphism** | If you catch by value, a derived exception gets **sliced** down to the base type — you lose the derived `what()` and any derived data members. Catching by reference binds to the *actual* thrown object, so virtual dispatch (e.g. `what()`) still resolves to the derived override. |
| **`const`** | You're not supposed to mutate an exception once it's caught — it's a read-only "here's what happened" record. |

**Slicing demonstrated:**

```cpp
class MyException : public std::exception {
public:
    const char* what() const noexcept override { return "MyException"; }
};

try {
    throw MyException();
}
catch (std::exception e) {           // ❌ catch by VALUE
    std::cout << e.what();           // prints std::exception's generic message,
}                                     // NOT "MyException" — sliced!

catch (const std::exception& e) {    // ✅ catch by REFERENCE
    std::cout << e.what();           // prints "MyException" correctly
}
```

*Why this happens:* `catch (std::exception e)` constructs a brand-new `std::exception` object by copying only the base-class portion of the thrown `MyException`. The derived part (and its `what()` override) simply isn't there anymore — same mechanism as assigning `Derived d; Base b = d;`.

### Catch order matters

```
specific  →  more general  →  most general
```
Put derived-exception handlers **before** base-exception handlers, or the base handler swallows everything and the derived one becomes unreachable (compilers usually warn about this).

### `catch(...)` — catch anything

```cpp
catch (...) {
    // catches literally any thrown type — usually the LAST catch block
}
```

### Rethrowing

```cpp
catch (const std::exception& e) {
    std::cout << "Logging error\n";
    throw;      // bare `throw;` = rethrow the CURRENTLY HANDLED exception, unchanged
}
```

If an exception escapes *every* enclosing `try`/`catch` all the way out of `main`, C++ calls `std::terminate()` and the program aborts.

### You can throw almost anything

```cpp
throw 42;
throw std::string("error");
```
caught with `catch (const std::string& s)`, etc. — but by convention, real C++ code throws types derived from `std::exception` so that generic handlers (`catch (const std::exception&)`) can catch anything meaningful.

---

## 2. Standard exception hierarchy

```
std::exception
├── std::logic_error            (bugs — theoretically detectable before running)
│   ├── std::invalid_argument
│   ├── std::out_of_range
│   ├── std::length_error
│   └── std::domain_error
├── std::runtime_error          (only detectable at runtime)
│   ├── std::range_error
│   ├── std::overflow_error
│   └── std::underflow_error
├── std::bad_alloc              (dynamic allocation failed)
├── std::bad_cast               (dynamic_cast<T&> failed)
├── std::bad_typeid
├── std::bad_optional_access
└── std::bad_variant_access
```

### `std::exception` — the root

```cpp
#include <exception>
virtual const char* what() const noexcept;
```

**Why `const noexcept`?**
- `const` → `what()` doesn't modify the exception object, so it can be called on a `const std::exception& e`.
- `noexcept` → `what()` promises never to throw. Throwing *while describing* another exception would be confusing (and can itself trigger `std::terminate()`).

### `logic_error` — programmer/precondition violations

`#include <stdexcept>`. Examples: `vec.at(n+100)`, calling a function with a value it explicitly forbids.

| Type | Meaning | Example |
|---|---|---|
| `invalid_argument` | Caller passed an invalid argument | `std::stoi("abc")`, `throw std::invalid_argument("age cannot be negative")` |
| `out_of_range` | A value/index is outside an allowed range | `vector<int>{1,2,3}.at(10)` |
| `length_error` | Operation would make an object larger than its max allowed size | `std::string(str.max_size()+1, 'a')` |
| `domain_error` | Value outside the valid *mathematical* domain | `sqrt(-1)` (if a checked math library throws) |

> **`vec[10]` vs `vec.at(10)`:** `operator[]` does **no bounds checking** — undefined behavior. `.at()` bounds-checks and throws `std::out_of_range`. Prefer `.at()` when you want safety over raw speed.

### `runtime_error` — only detectable while running

| Type | Meaning | Example |
|---|---|---|
| `runtime_error` (base) | Something went wrong during execution, not a logic bug per se | `5/0` on the domain you're modeling, a failed DB connection |
| `range_error` | A *computed result* can't be represented in the required range | a multiplication that overflows the intended output range |
| `overflow_error` | Arithmetic/computational overflow | (note: raw *signed integer overflow* itself is UB, not an exception — this type exists for library code that explicitly detects and reports it) |
| `underflow_error` | Arithmetic/computational underflow | similar to above |

**`out_of_range` vs `range_error` — the distinction that's easy to blur:**
- `out_of_range` → your **input/access** was outside a permitted range (e.g. an index).
- `range_error` → a **computed result** doesn't fit in the representable range.

### Allocation & cast failures

```cpp
#include <new>
int* p = new int[10000000000000000ULL];   // throws std::bad_alloc
```

```cpp
Base& b = derivedObject;
Derived& d = dynamic_cast<Derived&>(b);   // throws std::bad_cast if b isn't really a Derived
```

| Cast form | On failure |
|---|---|
| `dynamic_cast<T&>(ref)` | throws `std::bad_cast` |
| `dynamic_cast<T*>(ptr)` | returns `nullptr` (no exception) |
| `static_cast<T&>(ref)` | **no runtime check at all** — if the object isn't really a `T`, this is undefined behavior. `static_cast` trusts you; it never throws, because it never checks. |

---

## 3. Writing custom exception classes

```cpp
#include <exception>

class MyException : public std::exception {
public:
    const char* what() const noexcept override {   // `override` is important:
        return "Something went wrong";              // if you accidentally drop `const`
    }                                                // or misspell what(), `override`
};                                                    // makes the compiler catch it
```

```cpp
try {
    throw MyException();
}
catch (const std::exception& e) {
    std::cout << e.what();   // "Something went wrong"
}
```

If you *don't* derive from `std::exception`, generic `catch (const std::exception&)` code can't catch your type at all — deriving from it is what makes a class "an exception" in the eyes of the rest of the codebase.

### Carrying extra data

```cpp
#include <exception>
#include <string>

class InvalidAge : public std::exception {
    std::string message;
public:
    InvalidAge(int age) : message("Invalid age: " + std::to_string(age)) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};
```

> ⚠️ **UB trap:**
> ```cpp
> const char* what() const noexcept override {
>     std::string msg = "Something went wrong";
>     return msg.c_str();     // ❌ msg is destroyed when what() returns —
> }                            //    the returned pointer instantly dangles (UB)
> ```
> Always store the message as a **member** (so it lives as long as the exception object), never as a local variable inside `what()`.

### Prefer deriving from `runtime_error`/`logic_error` over `std::exception` directly

Most of the time you don't need to write `what()` at all — inherit it:

```cpp
class DatabaseError : public std::runtime_error {
public:
    DatabaseError(const std::string& msg) : std::runtime_error(msg) {}
};

throw DatabaseError("Connection failed");
```

Even shorter — **inherit the base class's constructors** with a `using` declaration:

```cpp
class PaymentError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;   // pulls in runtime_error(const string&)
};

PaymentError e("Payment failed");   // works directly, no constructor written by hand
```

> 🔑 **Important consequence of `using base::base;`:** you inherit the base's constructors *exactly as they are* — you do **not** get a default (zero-argument) constructor unless the base class had one. `std::runtime_error` has no default constructor (it always requires a message), so `PaymentError` doesn't either. `PaymentError e;` will fail to compile — see the worked example in the solutions file (Q2) for exactly this gotcha.

> ⚠️ If an exception escapes out of `what()` itself — even though it's marked `noexcept` — the program calls `std::terminate()`.

---

## 4. Exception safety guarantees

```cpp
void addUser(User user) {
    users.push_back(user);   // succeeds
    logUser(user);           // throws
}
```
Now one step succeeded and the next threw — the program hasn't crashed, but what state is it in? **Exception safety** is the answer to: *"If an operation throws partway through, what can I still guarantee about program state?"*

From strongest to weakest:

### No-throw guarantee
```cpp
void fn() noexcept { }
```
Callers know an exception will never *escape* `fn()`. (It doesn't mean nothing inside can throw — it means nothing escapes. If something does throw anyway, `std::terminate()` fires immediately.) This is why `noexcept` is standard on modern move constructors/move assignment — e.g. `std::vector` only uses *move* instead of *copy* during reallocation if the move is guaranteed `noexcept`; otherwise it falls back to copying for safety.

### Strong guarantee
Operation either **fully succeeds**, or the observable state is **exactly as it was before** — like a database transaction.

```cpp
void update(Account& account) {
    Account temp = account;
    temp.balance -= 100;
    temp.balance += calculateBonus();   // might throw
    account = temp;                     // "commit" — only reached on success
}
```

Common pattern: **do all the risky/throwing work on a temporary copy first, then commit with a non-throwing final step.**

### Basic guarantee
If an exception occurs, the object is still **valid** (no leaks, invariants intact) but its state **may have partially changed**.

*Strong* = nothing changed. *Basic* = something may have changed, but nothing is corrupted. E.g. an `Account` with an invariant `balance >= 0` — basic guarantee promises you'll never end up with `balance == -50`, even though *which* fields changed mid-failure isn't specified.

### No guarantee
No promise at all about the resulting state — partial modification, resource leaks, corrupted invariants are all possible. Avoid this.

```cpp
void f() {
    int* a = new int[100];
    doSomething();   // throws
    delete[] a;      // never reached → leak → "no guarantee" territory
}
```

> ⚠️ **Destructors are implicitly `noexcept`** in modern C++ unless you explicitly say otherwise. During stack unwinding, destructors run — if one of *those* threw too, you'd have two simultaneous exceptions in flight, and C++ resolves that ambiguity by calling `std::terminate()`. So destructors that throw are almost always a design mistake.

---

## 5. RAII and exception safety

**RAII = Resource Acquisition Is Initialization** — one of the most load-bearing ideas in modern C++. Tie a resource's *lifetime* to an *object's* lifetime.

```
object created   → resource acquired
object destroyed → resource released
```

Without RAII you'd need manual acquire/release pairing:
```cpp
acquire();
try {
    // work
} finally {   // (pseudocode — C++ has no `finally`)
    release();
}
```

With RAII, the destructor does it automatically, and — critically — **destructors still run during stack unwinding**:

```cpp
void f() {
    auto p = std::make_unique<int>(42);
    doSomething();   // throws
}   // p's destructor still runs here → memory released, no leak
```

- RAII gives you the **basic guarantee for free**: no leaks, cleanup happens automatically on every exit path (normal return *or* exception).
- It does **not automatically** give you the *strong* guarantee — that still requires the "build on a temporary, commit at the end" pattern from §4.
- RAII is everywhere in the STL: `vector`, `string`, `deque`, `unique_ptr`, `lock_guard`.
- RAII can even model a **transaction**: constructor = "begin transaction", destructor = "rollback if not explicitly committed".

> 🔑 Unlike garbage collection (Python/Java), RAII is **deterministic** — you know exactly *when* a resource is released (end of scope), and you can create an artificial scope with a bare `{ }` block just to control that timing.

---

## 6. `noexcept`

`noexcept` means: *this function does not allow an exception to escape it.* If it throws anyway, C++ calls `std::terminate()` (which calls `std::abort()`) — **no unwinding, no catching, program dies immediately.**

```cpp
void f() noexcept {
    try {
        throw std::runtime_error("oops");
    }
    catch (const std::exception& e) {
        std::cout << e.what();
    }
}   // fine — the exception was caught INSIDE f(), so nothing escaped
```

Common uses: destructors, cleanup/resource-release code, `swap()`, move constructors/move assignment.

```cpp
std::move_if_noexcept(x);   // move if it's safe (noexcept), otherwise copy
```

`noexcept` is also an **operator**, not just a specifier:

```cpp
void a() noexcept {}
void b() {}

std::cout << noexcept(a());  // true
std::cout << noexcept(b());  // false
static_assert(noexcept(a()));   // compiles — checked at compile time
```

```cpp
void f() noexcept(false) {   // equivalent to plain `void f() {}` but explicit
    throw std::runtime_error("error");
}
```

**Legacy note:** `void f() throw();` is the old (pre-C++11, now deprecated) spelling of `void f() noexcept;`. Always use `noexcept` in new code.

---

## 7. Stack unwinding

What happens to the *caught object itself*?

```cpp
throw MyException("error");
```
```cpp
catch (const MyException& e) {
    // e stays alive for the duration of this handler
}
```
The exception-handling machinery manages the thrown object's lifetime for you — it stays alive as long as it's "the currently handled exception," and you never need to (and never should) manually `delete` it.

What stack unwinding **does** clean up: every local object with automatic storage duration, on every stack frame between the `throw` and the matching `catch`, via its destructor — in reverse order of construction.

What stack unwinding does **NOT** clean up: anything not tied to an object's destructor — raw `new` without a matching RAII wrapper, open file descriptors obtained through non-RAII APIs, locks acquired without `lock_guard`/`unique_lock`, etc. This is *the* reason RAII (§5) exists.

---

## 8. Multiple catches, `catch(...)`, rethrow

When several `catch` blocks follow one `try`, C++ picks the **first one that matches**, runs it, and skips the rest — it does not fall through to later handlers.

**Ordering matters:** put specific handlers *before* general ones.
```cpp
catch (const std::out_of_range& e) { /* specific */ }
catch (const std::exception& e)    { /* general  */ }   // must come AFTER
```
If you swap the order, the general handler catches everything first and the specific one becomes dead code (most compilers will warn).

```cpp
catch (...) {
    // catches ANY thrown type
    // typical uses: cleanup / logging / rethrowing when you don't
    // care about (or can't know) the specific exception type
}
```

### Rethrowing

```cpp
try {
    doSomething();
}
catch (const std::exception& e) {
    std::cout << "Logging: " << e.what() << '\n';
    throw;          // rethrow the SAME exception object, unchanged
}
```

> 🔑 **`throw;` vs `throw e;` — not the same thing:**
> - `throw;` → the **original** exception (its full dynamic type) keeps propagating. E.g. a caught `std::out_of_range` stays a `std::out_of_range` for the next handler up.
> - `throw e;` → creates a **new** throw of whatever *static type `e` was declared as* — if `e` was caught as `const std::exception&`, you'd re-throw a plain `std::exception`, **silently losing the derived type** (a second slicing bug!).
>
> `throw;` is therefore almost always preferred, and it's **only valid inside a catch block**.

Catch-all + rethrow is a common, legitimate pattern — but prefer letting RAII handle cleanup rather than doing it by hand here:
```cpp
try {
    doSomething();
}
catch (...) {
    cleanup();
    throw;
}
```

If no `catch` matches at all, the exception keeps propagating up the call stack looking for one (and ultimately calls `std::terminate()` if it never finds one — see §1).

---

## 9. Error codes vs exceptions (quant/low-latency context)

Rule of thumb:
- **Expected / ordinary failure** → return a value representing failure (error code, `optional`, `expected`).
- **Exceptional / truly unexpected failure** → an exception is appropriate.

*"Expected failure" example:* looking up a price for a symbol that might legitimately not be quoted right now — this happens constantly and isn't a bug.
*"Truly exceptional" example:* a required config file is missing at startup, or a required piece of hardware/connection isn't available — rare, and arguably should stop the program.

**Why the hot path avoids exceptions:** the exceptional path is expensive —
```
throw → allocate/manage exception object → search for matching handler
      → unwind the stack → destroy locals along the way → transfer control
```
That's a lot of work with unpredictable latency, which is poison in a low-latency trading loop even if it's *rare*.

```cpp
auto [price, error] = getPrice(symbol);   // error as a bool/flag, not a throw

if (!error) {
    // use price
}
```

Or, with richer failure information:
```cpp
enum class PriceError { NotFound, Stale, Invalid };
std::expected<Price, PriceError> getPrice(Symbol symbol);
```

**But error codes aren't automatically better either** — chaining
```cpp
Error e1 = step1();
Error e2 = step2();
Error e3 = step3();
Error e4 = step4();
```
with manual `if`-checks after each one is verbose, easy to forget, and has its own (smaller, but nonzero) cost per check.

**Practical guidance:**
- Avoid exceptions on the hot path (per-tick/per-order logic).
- Exceptions are still fine — even good — for genuinely rare failures: startup/config loading, missing hardware, programmer-error assertions, order *validation at the edge* (before you're in the hot loop).
- Know the toolbox:
  - `std::optional<T>` → "success, or no value" (no error *reason* attached)
  - a plain error enum → when you need a specific *reason* for failure but not a value
  - `std::expected<T, E>` (C++23) → carries **either** the success value **or** a specific error, in one type

---

## 📌 Quick-reference cheat sheet

| Concept | One-liner |
|---|---|
| Catch by reference, not value | Avoids slicing + avoids unnecessary copies |
| `catch(...)` | Catches anything; no named object, so no `.what()` access — see solutions Q3 |
| `throw;` vs `throw e;` | `throw;` preserves the real dynamic type; `throw e;` re-slices to the static type |
| `noexcept` that throws | Immediate `std::terminate()` — no unwinding, no catching |
| RAII | Gives the **basic** guarantee automatically; **strong** guarantee still needs a "commit at the end" pattern |
| `at()` vs `[]` | `.at()` bounds-checks and throws `out_of_range`; `[]` is UB on out-of-bounds |
| Hot path in quant systems | Prefer error codes / `optional` / `expected`; save exceptions for genuinely rare failures |