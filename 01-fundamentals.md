# 📘 LEVEL 1 — C++ Fundamentals Refresher

> Compile with: `g++ -std=c++20 -Wall -Wextra level01_fundamentals.cpp -o out`

**Topics:** compilation model · variables & types · integer overflow · control flow edge cases · pass-by-value/ref/pointer · default args & overloading · arrays/C-strings/`std::string` · const correctness · references vs pointers · scope/lifetime/storage duration · I/O streams

---

## 1. The Compilation Model

A C++ project is usually shaped like:

```
main.cpp
math.cpp   math.h
utils.cpp  utils.h
```

The compiler only ever looks at **one `.cpp` file at a time**. It has no idea other `.cpp` files exist.

### Pipeline

```
main.cpp → preprocessor → translation unit → compiler → assembler → main.o
                                                                        ↓
                                                      main.o + math.o + utils.o → linker → app.exe
```

- **Translation unit** = a `.cpp` file *after* the preprocessor has expanded all `#include`s and macros into one giant flat file. That's the actual thing the compiler compiles.
- **Why this matters:** because the compiler never sees other files, a function used in `main.cpp` but defined in `math.cpp` needs a **declaration** visible in `main.cpp` — that's what headers are for.
- **Why C++ scales:** if you change one `.cpp` file, only *that* file needs recompiling (assuming headers didn't change) — not the other 99. This is the whole reason large C++ codebases stay buildable.

### Declaration vs. Definition

| | Declaration | Definition |
|---|---|---|
| What | "this exists somewhere" | the actual implementation |
| Memory? | none | yes (allocates / generates code) |
| Lives in | `.h` files | `.cpp` files |

```cpp
// declaration (header)
int add(int, int);

// definition (source file)
int add(int a, int b) { return a + b; }
```

For **variables** shared across files, you need `extern`:
```cpp
extern int x; // "this is defined in some other .cpp file"
```

⚠️ Never `#include` a `.cpp` file — copy-pasting the definition into multiple translation units causes **duplicate-symbol linker errors**.

### Include Guards

```cpp
#ifndef MATH_H
#define MATH_H
// declarations...
#endif
```
or the modern equivalent: `#pragma once`. Without one, including the same header twice (directly or transitively) re-declares everything and breaks the build.

---

## 2. Variables & Types

A variable = **name + type + size + scope + value**. The *type* only matters to you and the compiler — at the hardware level everything is just bits. `int x = 65;` and `char x = 65;` are both `01000001` in memory; the type just tells the compiler how to *interpret* those bits (as `65` vs `'A'`).

### Sizes (typical)

| Type | Bytes | Notes |
|---|---|---|
| `short` | 2 | |
| `int` | 4 | |
| `long` | 4 / 8 | Windows vs Linux/Mac — prefer `int64_t` if you need a guarantee |
| `long long` | 8 | |
| `float` | 4 | suffix: `3.14f` |
| `double` | 8 | default for decimals |
| `long double` | often 16 | |
| `char` | 1 | |
| `bool` | 1 | |

Unsigned versions (`unsigned int`, etc.) are the same size, just no sign bit.

Fixed-width types (`<cstdint>`) when you need a *guarantee*, not "whatever this platform's `long` happens to be`:
```cpp
int8_t  int16_t  int32_t  int64_t
uint8_t uint16_t uint32_t uint64_t
```

### Integer Overflow — the interview classic

- **Signed overflow → undefined behavior.** The compiler is allowed to assume it never happens, which can produce surprising optimizer behavior.
- **Unsigned overflow → wraps modulo 2ⁿ.** Well-defined, predictable, but still usually a bug.
- **Integer promotion:** in an expression, small integer types (`char`, `short`, `unsigned char`, …) are promoted to `int` (or `unsigned int`) *before* any arithmetic happens. That's why `unsigned char(200) + unsigned char(100)` gives you `300`, not a wrapped char value — the addition itself happens at `int` width.

### Brace vs. `=` Initialization

```cpp
double x = 3.14;
int y = x;        // ✅ compiles (with a narrowing warning) — silently loses data
int y{x};          // ❌ compile error — brace-init forbids narrowing conversions
```
👉 Prefer `{}` initialization in modern C++ — it catches this class of bug at compile time instead of letting it silently corrupt data.

### Explicit Conversions

```cpp
double d = static_cast<double>(5);
int x    = static_cast<int>(3.45);   // truncates toward zero → 3
```

---

## 3. Functions: Passing Strategies

| Strategy | When |
|---|---|
| pass-by-value | small/cheap types, or you want the callee to have its own independent copy |
| pass-by-reference (`T&` / `const T&`) | large objects you don't want to copy; `const T&` when the callee shouldn't modify it |
| pass-by-pointer (`T*`) | the argument might legitimately be *absent* (`nullptr`), or you need to reseat what you're pointing at |

```cpp
int add(int x)        // copy
int add(int& x)       // mutable reference
int add(const int& x) // read-only reference, no copy
int add(int* x)       // pointer — receives a copy of the pointer itself
int add(int*& x)      // reference to a pointer
int add(int** x)      // pointer to a pointer
```

**References vs. pointers, precisely:**
- A reference **must** be initialized at declaration and can **never** be rebound to something else afterward.
- A pointer can be null, can be reseated, and can be left uninitialized (dangerous).
- **Prefer references** when a valid target is guaranteed to exist and you never need to "not point at anything" or reassign — cleaner call sites, no null-checks needed.
- **Prefer pointers** when: the value may be optional (`nullptr` = "no argument"), you need to point at different objects over the pointer's lifetime, you're interfacing with C APIs, or you're working with arrays/dynamic memory.
- Performance-wise: identical in modern compilers. Choose based on semantics, not speed.

⚠️ **Never return a pointer/reference to a local variable** — it becomes a **dangling pointer/reference** the moment the function returns and its stack frame is gone:
```cpp
int* foo() {
    int x = 5;
    return &x; // ❌ dangling — x is destroyed when foo() returns
}
```

---

## 4. Default Arguments & Overloading

**Default arguments** must be filled right-to-left:
```cpp
void foo(int a, int b = 1, int c = 2);   // ✅
void foo(int a = 3, int b);              // ❌ — foo(5) would be ambiguous: is b defaulted or is a?
```
They belong in the **declaration**, not the definition (when you have both).

**Overloading** = same name, different parameter list. **Return type alone cannot distinguish overloads** — the compiler has no way to know which one you meant from the call site alone:
```cpp
int foo();
double foo(); // ❌ invalid — ambiguous from a call site like `foo();`
```

**Deleting an overload** to forbid an implicit conversion path:
```cpp
void foo(int);
void foo(double) = delete;

foo(3.14); // ❌ would otherwise silently convert to int — now a compile error instead
```

### const overloading (member functions)

```cpp
class Person {
    string name;
public:
    string&       getName()       { return name; } // called on non-const objects
    const string& getName() const { return name; } // called on const objects
};
```
Mental model: `getName() const` is basically `getName(const Person* this)` — the `const` applies to the implicit `this`.

---

## 5. Arrays, C-strings, `std::string`

`arr[i]` is really `*(arr + i)` — array indexing is pointer arithmetic under the hood, because a plain array parameter **decays to a pointer**:
```cpp
void foo(int arr[])   // these two signatures
void foo(int* arr)    // are identical to the compiler
```
That's also why `sizeof(arr)` **inside a function that received an array parameter** gives you the size of a *pointer* (8 bytes on 64-bit), not the array's true byte size — the array already decayed before `sizeof` ever sees it. To recover element count from a real array (not a decayed pointer), use `sizeof(arr) / sizeof(arr[0])` — but only where `arr` hasn't decayed yet (e.g. in the same scope it was declared).

**C-strings** are just `char` arrays terminated by `'\0'` (ASCII 0):
```cpp
char str[] = "Hello"; // 'H' 'e' 'l' 'l' 'o' '\0'
```
Without the null terminator, functions like `strlen` would keep reading past the buffer until they *happen* to hit a zero byte — undefined behavior.

| | mutable? | owns memory? | safety |
|---|---|---|---|
| `char arr[]` | ✅ yes | ✅ own stack copy | must size the buffer yourself |
| `const char*` (literal) | ❌ no — UB if you try | ❌ points into read-only static memory | easy to misuse |
| `std::string` | ✅ yes | ✅ manages its own heap buffer | ✅ safest — bounds/resize handled for you |

```cpp
#include <cstring>
strlen(str);        // length excluding '\0'
strcpy(dest, src);  // copy
strcmp(a, b);       // compare
strcat(a, b);       // concatenate — dest must already have room!
```
⚠️ `strcat`/`strcpy` don't check destination capacity — if `dest` is a fixed-size buffer too small for the result, you get a **buffer overflow**. This is exactly why modern code reaches for `std::string` instead.

```cpp
char a[20] = "hello";     // note: size after the name, not before
const char* b = " world";
strcat(a, b);              // fine — a has room
```

---

## 6. `const` Correctness

```cpp
const int x = 5;   // same as:
int const x = 5;
x = 10;             // ❌ compile error

const int y;        // ❌ compile error — const must be initialized immediately
```

Reading pointer-const declarations right-to-left is the trick:
```cpp
const int* p;        // == int const* p  → *p cannot change, p itself can
int* const p;        // → p cannot be reseated, but *p can change
const int* const p;  // → neither p nor *p can change
```

```cpp
void fun(const int* p) {
    p++;   // ✅ allowed — the pointer itself isn't const
    *p;    // ❌ reading is fine, writing (*p = x) is not
}
```

---

## 7. Scope, Lifetime, Storage Duration

Three different questions:
- **Scope** — *where in the code* can I refer to this name?
- **Lifetime** — *when* does the object actually exist in memory?
- **Storage duration** — *how* is that memory managed?

| Storage duration | Where | Example |
|---|---|---|
| automatic | stack | ordinary local variables |
| static | static memory, lives for the whole program | `static int count` inside a function |
| dynamic | heap | `new` / `malloc` |

```cpp
void counter() {
    static int i = 0;
    i++;
    cout << i << endl;
}
```
Calling `counter()` three times prints `1 2 3` — `i` has **function (block) scope** but **static lifetime**: it's allocated once when the program starts and shared across every call, not re-created each time.

**Shadowing gotcha** — a variable's name enters scope *at its point of declaration*, not after the whole statement finishes:
```cpp
int x = 5;
{
    int x = x + 1; // the x on the RHS already refers to the new (uninitialized) inner x!
    cout << x;      // undefined behavior — NOT a reliable "6"
}
```

---

## 8. I/O Streams

Everything in C++ I/O is a **stream** — data flowing from one place to another.

```cpp
cin  // read from keyboard
cout // write to screen
cerr // error output, unbuffered
```
`<<` and `>>` are just operators and support chaining.

```cpp
string name;
cin >> name;          // stops at whitespace → only reads "John" from "John Smith"
getline(cin, line);    // reads the whole line up to '\n' → "John Smith"
```

### `stringstream`

Same interface as `cin`/`cout`, but the source/destination is a `string` instead of the keyboard/screen.

```cpp
stringstream ss("1234");
int x;
ss >> x;
ss.clear(); // reset stream error state before reusing
```

⚠️ `>>` only splits on **whitespace**, not arbitrary delimiters. Parsing `"AAPL,150.25,1000"` with `ss >> ticker >> price >> volume` will **not** work — there's no whitespace, so `ticker` swallows the entire string and the rest fails silently. Use `getline(ss, token, ',')` in a loop instead:
```cpp
stringstream ss("AAPL,150.25,1000");
string token;
getline(ss, token, ','); string ticker = token;
getline(ss, token, ','); double price = stod(token);
getline(ss, token, ','); long volume = stol(token);
```

```cpp
stringstream out;
out << 10 << " " << 20;
cout << out.str(); // "10 20"
```

---

## 9. Solution Review — key corrections

Full solutions are in `level01_fundamentals.cpp`. These are the ones where the reasoning needed a fix (see chat for the full explanation of each):

- **Q1** — output (300) was correct, but the mechanism is integer promotion, not overflow — both operands become `int` before `+` runs.
- **Q5** — compiles *and* runs; the `10` you saw is constant folding by the compiler exploiting `const`, while the actual undefined behavior is modifying `x` at all.
- **Q7** — fix `char[20] a` → `char a[20]`.
- **Q9** — does **not** reliably print `6` — classic shadowing/self-init UB.
- **Q11** — `>>` won't split on commas; needs `getline(ss, token, ',')`.
- **Q13** — value initialization (`int x{};`) zero-inits; your description matched copy-init instead.

Everything else (Q2–Q4, Q6, Q8, Q10, Q12) was conceptually right — wording tightened above.