# 🧱 Level 3 — OOP Basics

> **Status:** ✅ Reviewed | 12 questions | Solution sheet walked through in chat
> **Feeds into:** Level 10 (RAII), Level 4+ (inheritance/polymorphism)

---

## 📋 Topics Covered

| # | Topic |
|---|-------|
| 1 | `class` vs `struct` (default access), members, methods |
| 2 | Constructors (default, parameterized, delegating), destructors |
| 3 | Member initializer lists vs. body assignment |
| 4 | `this` pointer |
| 5 | Static members / static methods |
| 6 | Access specifiers: `public` / `private` / `protected` |
| 7 | `const` member functions, `mutable` |
| 8 | `friend` functions/classes |
| 9 | In-class default member initializers |
| 10 | *(new — surfaced from your own bugs)* Most Vexing Parse |
| 11 | *(new — surfaced from Q9/Q12)* Pass-by-value copies & destructor counting |

---

## 🔑 Core Concepts

### 1. `class` vs `struct`
Identical in every way **except default access**.

```cpp
struct Foo { int x; };   // x is public
class  Bar { int x; };   // x is private
```
Convention, not the compiler, decides which one you reach for: `struct` for plain data bags, `class` when you have invariants to protect.

---

### 2. Constructors & Destructors

- **Default** — no args.
- **Parameterized** — takes args to set initial state.
- **Delegating** — one constructor calls another *of the same class* in its init list:
  ```cpp
  Point(double x, double y) : X(x), Y(y) {}
  Point() : Point(0, 0) {}   // delegates
  ```
- **Destructor** fires automatically when an object leaves scope (stack) or is `delete`d (heap). This automatic cleanup is the seed of **RAII** (Level 10).

> ⚠️ **Gotcha — Most Vexing Parse**
> `BankAccount a1();` does **not** create an object. The compiler reads it as a **function declaration**: "a function named `a1`, taking no args, returning `BankAccount`." No constructor runs.
> **Fix:** drop the parentheses → `BankAccount a1;`

---

### 3. Member Initializer Lists

```cpp
C3(int x, int y, int z) : x(x), y(y), z(z) {}
```

- Members are initialized **before** the constructor body runs.
- They are initialized in **declaration order** (top-to-bottom in the class), **regardless of the order you write them in the initializer list.**
- Prefer initializer lists over body assignment — assignment in the body means non-trivial types get **default-constructed first, then assigned**, which is wasted work.

> ⚠️ **Interview gotcha:** if member `B`'s initializer depends on member `A` already being set, and `B` is declared *before* `A` in the class, `B` will initialize using `A`'s garbage/default value — no matter what order they appear in the initializer list.

**Worked example:**
```cpp
class Bad {
    int* data;   // declared 1st
    int  size;   // declared 2nd
public:
    // Written to look like "size first, then data" — but that's not what happens.
    Bad(int n) : size(n), data(new int[size]) {}
};
```
Compilation order is dictated by **declaration order**, not by the order written after the `:`. So the compiler actually runs this, regardless of how you wrote the list:
```cpp
Bad(int n) {
    data = new int[size];   // (1) runs FIRST — size is still garbage/indeterminate here
    size = n;                // (2) runs SECOND
}
```
`data` is allocated using whatever garbage value `size` happens to hold at that point (`int size;` has no default value for a plain `int` member with no in-class initializer) — so you get an array of some random size, maybe 0, maybe huge, maybe a crash. Most compilers will even warn you here (`-Wreorder`), but the code still compiles and silently misbehaves if warnings are ignored.

**Fix:** either reorder the *declarations* so `size` comes before `data`, or don't let one member's init depend on another's at all.

---

### 4. `this` Pointer

Every non-static member function implicitly receives `this` — a pointer to the calling object. Mentally: `void setBalance(Account* this, double balance)`.

Use it to disambiguate a parameter that shadows a member:
```cpp
void setBalance(double balance) {
    this->balance = balance;   // this->member vs. local param
}
```

---

### 5. Static Members

- Belong to the **class**, not any instance — one shared copy.
- Must be defined once outside the class (pre-C++17) or declared `inline static` (C++17+).
```cpp
class BankAccount {
    static int accountCount;
    BankAccount()  { accountCount++; }
    ~BankAccount() { accountCount--; }
};
int BankAccount::accountCount = 0;   // required definition
```

**C++17+ alternative — `inline static`:** skips the separate out-of-class definition entirely.
```cpp
class BankAccount {
    inline static int accountCount = 0;   // defined right here, no line needed outside the class
    inline static string bankName = "Chase";  // you can have as many inline statics as you want
};
```
There's no limit of one — a class can have any number of `static` or `inline static` members, each tracking its own class-wide piece of state (a running count, a shared config value, a registry, etc.). `inline` here just means "it's fine for this definition to appear in multiple translation units without violating the One Definition Rule" — it doesn't mean "only one."

---

### 6. Access Specifiers

| Specifier | Visible to |
|---|---|
| `public` | everyone |
| `protected` | class + derived classes |
| `private` | class only (+ `friend`s) |

---

### 7. `const` Member Functions & `mutable`

- `void print() const` promises not to modify observable state.
- A `const` method can only call other `const` methods.
- `const Matrix& m = Matrix();` binds a `const` reference **directly to a temporary** — this is legal and extends the temporary's lifetime to match the reference's. Calling a non-`const` method (e.g. `m.set(...)`) on it fails to compile: *"object has type qualifiers that are not compatible with the member function."*
- `mutable` marks a member as bypassing constness — for things like log counters, caches, or mutexes that need to change even inside a logically-read-only call. You're not limited to one — a class can mark as many members `mutable` as it needs; each one individually opts out of the `const` promise while every other (non-mutable) member stays protected.

**Worked example — multiple mutable members:**
```cpp
class Logger {
    mutable int  logCount = 0;      // how many times log() was called
    mutable bool wasAccessed = false; // whether this object has ever been read from
    string prefix;                   // NOT mutable — stays truly read-only in const methods
public:
    void log(const string& msg) const {
        logCount++;         // OK — mutable
        wasAccessed = true; // OK — mutable
        cout << prefix << msg << endl;
        // prefix = "x";    // would NOT compile — prefix isn't mutable
    }
};
```
Each `mutable` member is independent — marking `logCount` mutable has no effect on whether `wasAccessed` or `prefix` can be modified in a `const` method.

---

### 8. `friend`

```cpp
class Account {
    int balance;
    friend void resetBalance(Account&);   // grants access
};
void resetBalance(Account& acc) { acc.balance = 0; }  // free function, NOT Account::resetBalance
```
**Tradeoff:** `friend` punches a hole in encapsulation — the class no longer fully controls who touches its private state. Use sparingly (operator overloads, tightly-coupled helper functions).

---

### 9. In-Class Default Member Initializers

```cpp
class Trade {
    int quantity = 0;   // default
};
```
A constructor can still override this default, either via the init list (`Trade(int q) : quantity(q) {}`) or in the body.

---

### 10. Pass-by-Value Copies & the "Extra Destructor" Mystery

Two things trip people up constantly, and both showed up in your own code:

**a) Passing a class by value silently copies it.**
```cpp
class Car {
    Engine e;
    Car(Engine _e) : e(_e) {}   // _e is a parameter COPY, then e(_e) COPIES AGAIN
};
```
Since C++17, `Car(Engine(5))` materializes the temporary directly into the parameter `_e` (no copy there), **but** `e(_e)` in the init list still copy-constructs `e` from `_e`. Two `Engine` objects end up existing (`_e` and `e`), each with its own destructor call — even though you only ever wrote one `Engine` constructor call. Pass by `const Engine&` to avoid the copy entirely.

**b) Brace-init lists create temporaries that outlive the statement.**
```cpp
std::vector<question12> vec = { question12(1), question12(2), question12(3) };
```
This constructs **3 temporaries** (3 ctor prints), which get copy-constructed into the vector's internal storage (3 more objects, silently — the implicit copy constructor doesn't print). The 3 *temporaries* are destroyed at the end of the full expression → 3 destructor prints. Then when `vec` goes out of scope, its 3 *internal* copies are destroyed → 3 more destructor prints. **6 destructors total, only 3 constructors printed** — the missing 3 "constructions" were silent copy-constructions.

---

## 🧠 One-Line Recap

> Default access differs (`struct`=public/`class`=private) → init lists run in **declaration order** before the ctor body → `this` disambiguates shadowed names → `static` = one copy per class → `const` methods need `mutable` to touch counters → `friend` trades encapsulation for access → **watch for the Most Vexing Parse and silent copy-constructor calls — they're the two bugs that bit you this level.**