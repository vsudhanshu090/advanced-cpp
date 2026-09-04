# 🎯 Level 2 — Pointers Deep Dive

> A pointer is just a variable whose value happens to be a memory address. Everything else in this level is consequences of that one fact.

---

## 1. Declaration & Dereferencing

```cpp
int  x  = 5;
int* p  = &x;   // p holds the address of x
int** q = &p;   // q holds the address of p (pointer to pointer)

// *p == x     (5)
// *q == p     (address of x)
// **q == x    (5)
```

> ⚠️ **Gotcha:** `*` binds to the variable, not the type.
> ```cpp
> int* p, q;   // p is int*, q is a plain int — NOT two pointers!
> int *p, *q;  // this is how you actually get two pointers
> ```

Dereferencing a `nullptr` → undefined behavior (crash on most systems, but not guaranteed).

---

## 2. `const` With Pointers — read right-to-left from the name

| Declaration | Meaning | Can reseat (`p++`)? | Can modify `*p`? |
|---|---|:---:|:---:|
| `int* p` | plain pointer to int | ✅ | ✅ |
| `const int* p` *(= `int const* p`)* | pointer to **const** int | ✅ | ❌ |
| `int* const p` | **const** pointer to int | ❌ | ✅ |
| `const int* const p` | const pointer to const int | ❌ | ❌ |

**Trick:** read the declaration right-to-left starting at the variable name. `const int* p` → "p is a pointer to an int that is const."

---

## 3. Pointer Arithmetic & Array-Pointer Duality

- `p + 1` moves forward by `sizeof(T)` **bytes**, not 1 byte.
- Only well-defined **inside a single array** (or exactly one-past-the-end as a *value* — dereferencing one-past-the-end is still UB).

> 🔴 **Bug pattern to remember:** incrementing a pointer to a *lone scalar* (not an array element) and then dereferencing it is undefined behavior — it will often silently "work" and print something plausible, which is exactly what makes it dangerous. `int x; int* p = &x; p++; *p;` ← UB, even though it "runs."

```cpp
int arr[5] = {10,20,30,40,50};
int* p = arr;

*(p+2)   == 30
p[2]     == 30
*(arr+2) == 30
2[arr]   == 30   // arr[i] desugars to *(arr+i), so i[arr] == *(i+arr) — addition commutes
```

`&arr` for `int arr[10]` has type `int (*)[10]` — pointer to an array of 10 ints (different from `int*`, which decays and loses the length).

`void f(int arr[])` and `void f(int* arr)` are **the same function** — the compiler always decays the array parameter to a pointer.

---

## 4. Null / Dangling / Wild Pointers

| Type | State | Danger |
|---|---|---|
| **Null** | explicitly points to nothing (`nullptr`) | safe to check, unsafe to dereference |
| **Dangling** | pointed to valid memory that has since been freed | UB on use — may "look fine" |
| **Wild** | never initialized at all | UB on use — literally random address |

```cpp
int* p;       // wild — could be ANY address
*p = 10;      // writes to a random address. very dangerous.
```

> ✅ **Rule of thumb:** always initialize a pointer to `nullptr` or a real address, never leave it wild.

**Always prefer `nullptr` over `NULL`** — `NULL` is often just `0`, which causes overload-resolution ambiguity:
```cpp
void foo(int);
void foo(int*);
foo(NULL);      // may resolve to foo(int)  — surprising!
foo(nullptr);   // unambiguously calls foo(int*)
```

`delete nullptr;` is explicitly safe — it does nothing.

```cpp
int* dangling_example() {
    int x = 5;
    return &x;   // x dies when the function returns → caller gets a dangling pointer
}
```

---

## 5. Dynamic Memory

- Every `new` ↔ exactly one `delete`. Every `new[]` ↔ exactly one `delete[]`. Mismatching is UB.
- `new int(42)` vs `new int{42}` — for a single scalar these produce the same value, but `{}` (list-init) rejects **narrowing conversions** at compile time:
  ```cpp
  new int(3.14);   // compiles — silently truncates to 3
  new int{3.14};    // ERROR — narrowing conversion double→int is rejected
  ```
  Prefer `{}` for exactly this reason: it catches mistakes `()` lets through.
- `delete[] arr;` — for class types, this calls the destructor on **every element** before freeing the block (conceptually "n times"). For built-in types like `int` there's no destructor to call, but the deallocation of the whole block still happens correctly only via `delete[]` (never plain `delete` on an array).

### Dynamic 2D array (`int**`)

```cpp
int** allocateMatrix(int rows, int cols) {
    int** m = new int*[rows];        // array of `rows` int* pointers
    for (int i = 0; i < rows; i++)
        m[i] = new int[cols];        // each row is its own heap block
    return m;
}

void freeMatrix(int** m, int rows) {
    for (int i = 0; i < rows; i++)
        delete[] m[i];               // free each row first
    delete[] m;                      // then free the array of pointers
}
```

**Memory diagram** (3×4 example):
```
m ──► [ptr0][ptr1][ptr2]         (array of 3 int* on the heap)
        │     │     │
        ▼     ▼     ▼
      [4 ints][4 ints][4 ints]   (3 separate heap blocks, one per row)
```
Note: rows are **not contiguous** with each other — this is different from a static `int grid[3][4]`, where all 12 ints sit in one contiguous block. That non-contiguity is the price of runtime-variable dimensions.

`new` can throw `std::bad_alloc` if the allocation fails (out of memory) — worth wrapping allocation-heavy code in `try/catch` in real systems, though this level doesn't require it yet.

---

## 6. Function Pointers

Functions live in memory too, so you can point to them:

```cpp
int add(int a, int b) { return a + b; }

int (*fp)(int, int) = add;     // fp = &add also works, identical

fp(2, 3);      // preferred — cleaner
(*fp)(2, 3);   // equivalent, older style
```

**Why bother?** Passing behavior as data:
```cpp
int compute(int a, int b, int (*operation)(int, int)) {
    return operation(a, b);
}
compute(2, 3, add);
compute(2, 3, multiply);
```

**`typedef` / `using` for readability** — raw function pointer syntax is ugly, so name the type:
```cpp
typedef double (*BinOp)(double, double);   // old style
using BinOp = double (*)(double, double);  // modern style, prefer this

double add(double a, double b) { return a + b; }
double multiply(double a, double b) { return a * b; }

BinOp ops[] = { add, multiply };
for (BinOp op : ops)
    cout << op(2.0, 3.0) << endl;
```

> This raw mechanism is what `std::function` and lambdas (Level 13) are built on top of, with a nicer interface.

---

## 7. `void*` and Casting

`void*` = "pointer to *something*, type unknown to the compiler." Can't be dereferenced directly — the compiler doesn't know how many bytes to read.

```cpp
int a = 5;
void* x = &a;

*x;                              // ERROR — compiler doesn't know the type
*(static_cast<int*>(x));         // OK — cast tells it what to read
```

Historically used in C (no templates) to write generic-ish functions. In modern C++, prefer:
- `template<typename T>` for compile-time generics
- `std::any` / `std::variant` for type-safe runtime polymorphism
- actual polymorphism (virtual functions) when it's an "is-a" relationship

---

## 8. `->` Operator (Pointers to struct/class)

```cpp
struct Order { string ticker; double price; int qty; };

Order* o = new Order();
o->ticker = "AAPL";     // equivalent to (*o).ticker
delete o;
```

`obj->member` is always exactly `(*obj).member` — just cleaner to write and read.

---

## 9. Common Pointer Bugs (the full rogues' gallery)

| Bug | Example | Why it's bad |
|---|---|---|
| **Memory leak** | `new` with no matching `delete` | memory never reclaimed until process exit |
| **Use-after-free** | `delete p; cout << *p;` | reads/writes freed memory — UB, may "look fine" |
| **Double free** | `delete p; delete p;` | corrupts heap allocator metadata — classic crash/security vector |
| **Multiple uncoordinated owners** | `int* a=p; int* b=p;` — who deletes? | ownership becomes ambiguous, easy to double-free or leak |
| **Returning heap memory** | `int* f(){ return new int(5); }` | caller MUST remember to delete — easy to forget |
| **Returning stack memory** | `int* f(){ int x=5; return &x; }` | `x` is destroyed on return → dangling pointer |
| **Losing the pointer** | `p = new int(5); p = new int(10);` | first allocation is now unreachable → leak |
| **Exception-unsafe cleanup** | `delete` skipped because an exception/early-return jumped past it | leak triggered by control flow, not obviously visible in a code read |

> 💡 Every single one of these disappears once you adopt smart pointers (Level 10) — but working through the pain here is what makes the smart-pointer motivation click later.

### Double free, specifically
Because `delete` is UB the *first* time it's misused, results after that are unpredictable — it might print old data, garbage, or crash immediately. The second `delete` on the same address can corrupt the heap allocator's internal bookkeeping (the metadata it uses to track free blocks), which is exactly why double-free bugs are a well-known **security vulnerability class**, not just a crash annoyance — corrupted heap metadata can sometimes be exploited to redirect program control flow.

---

## 10. Pointer Aliasing

Two pointers point to the same memory:
```cpp
int x = 5;
int* ptr1 = &x;
int* ptr2 = &x;      // ptr1 and ptr2 alias

*ptr1 = 10;
// *ptr2 is also 10 — same address

int*& ptr3 = ptr1;   // a reference to a pointer — also a form of aliasing
```

**Why it hurts optimization:** if the compiler can't *prove* two pointers don't alias, it can't safely cache a value it read through one pointer in a register and reuse it — a write through the *other* pointer might have changed it. This forces the compiler to reload from memory defensively, blocking optimizations like reordering, vectorization, and register caching.

C's `restrict` keyword (and GCC/Clang's `__restrict` extension in C++, since standard C++ has no `restrict`) is a promise from the programmer to the compiler: *"these pointers will never alias — optimize as if they're independent."* Lying about this is UB, but when it's true, it can unlock significant speedups.

---

## 🗂️ Quick Reference — Right-to-Left Const Reading Cheat Sheet

```
const int* p        → p is a pointer to (const int)        — pointee locked
int* const p         → p is a (const pointer) to int        — pointer locked
const int* const p   → both locked
```