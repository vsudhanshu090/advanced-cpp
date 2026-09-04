# 🧱 Level 5 — Rule of 3 / Rule of 5 / Rule of 0

> Special member functions, ownership, shallow vs deep copy, and how modern
> C++ tries to make you write as few of these by hand as possible.

---

## 📑 Topics Covered

1. Copy constructor
2. Copy assignment operator (and the self-assignment trap)
3. Destructor's role in owning resources
4. Move constructor / move assignment operator (mechanics only — rvalue
   reference *theory* comes properly in Level 12)
5. Rule of 3
6. Rule of 5
7. Rule of 0
8. Shallow copy vs deep copy, and the double-free bug
9. `= default` and `= delete`

---

## 1️⃣ Copy Constructor

Creates a **new object** as a copy of an existing object of the same type.

```cpp
Obj o1;
Obj o2 = o1;      // copy constructor: o2 is being CREATED as a copy of o1

void fun(Obj o) {          // pass-by-value parameter
    // ...
}
Obj o3;
fun(o3);          // copy constructor: fun's local `o` is a fresh copy of o3
```

**Syntax:**

```cpp
class A {
public:
    A(const A& other) {
        // copy other's data into *this
    }
};
```

- **Why `const`?** So the copy constructor can also accept a `const A&`
  argument. A non-const version (`A(A& other)`) would reject const objects —
  `const` is the more general, always-correct signature.
- **Why `&` (reference)?** If you passed by value (`A(A other)`), then to call
  the copy constructor the compiler would first need to *copy* `other` into
  the parameter — which requires calling the copy constructor — which
  requires copying its parameter — infinite recursion. Pass by reference
  breaks the loop.

### Creation vs. re-assignment — the rule that decides which function runs

```cpp
A a;
A b;
b = a;      // b ALREADY EXISTS -> this is copy ASSIGNMENT, not the copy constructor
A c = a;    // c is being CREATED right now -> this IS the copy constructor
```

| Situation | Function called |
|---|---|
| Object is being **created** as a copy | Copy constructor |
| Object **already exists** and receives a new value | Copy assignment operator |

---

## 2️⃣ Shallow Copy vs. Deep Copy

The **compiler-generated** copy constructor/assignment always does a
**member-wise (shallow) copy** — it copies each member's value exactly,
including raw pointers.

```cpp
class Box {
public:
    int* ptr;
    Box(int x) { ptr = new int(x); }
    ~Box()     { delete ptr; }
};

Box b1(10);
Box b2 = b1;     // shallow copy: b2.ptr == b1.ptr (SAME address!)
```

Now both `b1` and `b2` believe they own that `int`. When either one goes out
of scope first, its destructor frees the memory and the *other* object's
`ptr` becomes a **dangling pointer**. When the second one is destroyed too,
you get a **double free**.

> 🎯 **This is the single most common quant-interview / systems-interview C++
> bug.** If you remember one thing from this level, remember this pattern.

- **Memory leak** — you forget to release memory you own (nobody calls
  `delete`).
- **Double free** — the *same* heap block is released more than once
  (typically because two objects both think they own it, via shallow copy).

**When is shallow copy actually fine?** When the object being copied doesn't
*own* the resource — e.g. a non-owning pointer/view, like multiple widgets
just *looking at* one shared image without any of them being responsible for
freeing it.

### The fix — Deep copy

```cpp
class Box {
public:
    int* ptr;
    Box(int x) : ptr(new int(x)) {}
    Box(const Box& other) : ptr(new int(*other.ptr)) {}   // <-- allocate NEW memory, copy the VALUE
    ~Box() { delete ptr; }
};
```

Now `b2.ptr` and `b1.ptr` point at two **different** ints holding the same
value. Each object independently owns and frees its own memory.

> 📝 Note: RAII types (`std::string`, `std::vector`, `std::unique_ptr`,
> `std::shared_ptr`) already have a correctly-written copy constructor deep
> inside them. If a class only contains members like these, the
> **compiler-generated** copy constructor is automatically "deep" in effect,
> because it just calls each member's own (correct) copy constructor. This is
> exactly why Rule of 0 (see below) works.

---

## 3️⃣ Copy Assignment Operator

```cpp
class A {
public:
    A& operator=(const A& other) {
        this->x = other.x;
        return *this;
    }
};
```

Just like the copy constructor, the compiler-generated version is member-wise
assignment. For a raw-pointer-owning class you must fix it by hand — and
there are **two** things you must not forget:

### ⚠️ Trap 1 — must free the *old* memory before taking the new

```cpp
Box& operator=(const Box& other) {
    delete ptr;                    // release what THIS object currently owns
    ptr = new int(*other.ptr);     // allocate + deep-copy the new value
    return *this;
}
```

### ⚠️ Trap 2 — self-assignment (`a = a;`)

Walk through the code above if `this == &other`:

1. `delete ptr;` → frees the memory. Because `this == &other`, this is the
   *exact same* underlying pointer variable, so `other.ptr` is now a
   **dangling pointer** at this instant too (it's literally the same object).
2. `ptr = new int(*other.ptr);` → we dereference `other.ptr`, which we *just
   freed*. This is a **read of freed memory** → undefined behavior. It might
   crash, might silently produce garbage, might "work" by luck — that's what
   UB means.

**Fix — always guard against self-assignment first:**

```cpp
Box& operator=(const Box& other) {
    if (this == &other) return *this;   // guard
    delete ptr;
    ptr = new int(*other.ptr);
    return *this;
}
```

### Alternative — copy-and-swap idiom

```cpp
Box& operator=(Box other) {        // NOTE: by-VALUE, not const&
    std::swap(ptr, other.ptr);     // other (the local copy) now holds our old ptr
    return *this;                  // when `other` goes out of scope, it destroys OUR old memory
}
```

This is naturally self-assignment-safe and exception-safe, because the
"expensive"/risky copying work happens in the parameter-passing step (using
the already-correct copy constructor), and the assignment body itself can't
throw or leak.

> 🤔 **`a = b = c;` — how does chained assignment work?**
> `operator=` returns `A&` (a reference to `*this`). So `b = c` runs first,
> assigns `c`'s value into `b`, and evaluates to a reference to `b`. Then
> `a = (that reference to b)` runs. This is exactly why `operator=` should
> return `*this` by reference — so the result of one assignment can feed
> directly into the next.

---

## 4️⃣ Destructor

A special member function that runs **automatically** when an object's
lifetime ends, letting it release/clean up any resource it owns (heap
memory, file handles, sockets, locks, etc).

- Only `delete` something if **this object owns it**. If it's just observing
  a resource someone else owns, don't touch it.
- This automatic, deterministic cleanup **is RAII** (Resource Acquisition Is
  Initialization) — the idea that a resource's lifetime is tied to an
  object's lifetime, so it can never be forgotten or leaked as long as the
  object's destructor runs.

### ⚠️ Why destructors should not throw

If an exception is already propagating (i.e., the stack is being unwound
because of an earlier `throw`) and a destructor called *during that unwinding*
throws a second exception, C++ cannot have two simultaneous active exceptions
— it doesn't know which one to propagate. So the runtime calls
`std::terminate()` immediately, killing the program (no more graceful
handling, no `catch` block gets a chance).

```cpp
struct Bad {
    ~Bad() { throw std::runtime_error("boom"); }
};

void f() {
    Bad b;
    throw std::runtime_error("first error");   // stack unwinds -> b's destructor runs -> THROWS AGAIN -> std::terminate()
}
```

Even outside of that extreme scenario, a throwing destructor breaks
containers like `std::vector` (which destroy many elements in a loop —
if one throws mid-loop, the rest never get destroyed, and you leak). This is
why destructors are implicitly `noexcept` by default in C++11+, and you
should basically never override that.

---

## 5️⃣ Move Constructor & Move Assignment (C++11, `std::move`)

Copying a large owned resource is sometimes pure waste — e.g. when the source
object is a temporary that's about to be destroyed anyway. Instead of
duplicating the resource, you can just **transfer ownership** ("steal" the
pointer) and leave the source empty.

```cpp
Buffer b2 = std::move(temporaryBuffer);
// b2.ptr now points at temporaryBuffer's old memory
// temporaryBuffer.ptr is now nullptr (emptied, so its destructor is a safe no-op)
```

- Turns an O(n) deep-copy into an O(1) pointer swap.
- The moved-from object is left in a valid-but-unspecified (here:
  null/empty) state specifically so its destructor doesn't double-free the
  resource that was stolen from it.

### Syntax

```cpp
class Buffer {
public:
    Buffer(Buffer&& other) noexcept
        : str(other.str) {
        other.str = nullptr;          // empty the source
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] str;                 // release what WE currently own first
        str = other.str;              // steal
        other.str = nullptr;          // empty the source
        return *this;
    }
};
```

### Why mark moves `noexcept`?

Containers like `std::vector`, when they need to grow/reallocate, must move
(or copy) every existing element into the new buffer. They have to guarantee
**strong exception safety** — if something goes wrong mid-reallocation, the
original vector must be left untouched.

- If the move constructor is `noexcept`, the vector can safely move each
  element — nothing can throw mid-move, so there's no way to end up
  half-migrated.
- If the move constructor is **not** marked `noexcept` (even if it wouldn't
  actually throw), the vector can't take that risk, and **silently falls
  back to copying** instead — you lose all the performance benefit of having
  written a move constructor in the first place, with no compiler warning
  that this happened.

This is a very easy trap: writing a perfectly correct move constructor but
forgetting `noexcept` quietly disables its use inside `std::vector`.

---

## 6️⃣ Rule of 3 (pre-C++11)

Not a language rule — a **design guideline**. If your class needs to
hand-write *any one* of these three, it almost certainly needs all three,
because it means the class is directly managing a resource the compiler's
defaults don't know how to handle correctly:

| Function | Role |
|---|---|
| Destructor | Releases the owned resource |
| Copy constructor | Deep-copies the resource for a new object |
| Copy assignment | Releases old + deep-copies new, safely, including self-assign |

Applies mainly when a class directly owns a raw pointer, file handle, socket,
mutex, etc. Doesn't matter for plain `int`/`float`/etc members, and doesn't
matter if all your members are RAII types (`string`, `vector`, ...) — they
already manage themselves. Modern C++ leans hard on RAII members specifically
so you *rarely* have to invoke Rule of 3 by hand.

---

## 7️⃣ Rule of 5 (C++11+)

Move semantics added two more special member functions to the family, so the
rule of 3 became the rule of 5:

```cpp
class Buffer {
public:
    Buffer();                            // Constructor
    ~Buffer();                           // Destructor
    Buffer(const Buffer&);               // Copy constructor
    Buffer& operator=(const Buffer&);    // Copy assignment
    Buffer(Buffer&&);                    // Move constructor
    Buffer& operator=(Buffer&&);         // Move assignment
};
```

> ⚠️ **Important compiler-generation rule** (this connects directly to
> Level 12): if you declare **any** of destructor / copy ctor / copy assign
> **yourself**, the compiler will **not** implicitly generate the move
> constructor or move assignment for you. Any attempted "move" of your object
> then just silently falls back to using the copy constructor instead
> (assuming it exists and isn't deleted) — quietly losing the performance you
> expected, with no error or warning. See Q8 below for exactly where this
> bites.

---

## 8️⃣ Rule of 0 (modern C++, preferred)

The best fix to "I need to write 5 functions" is usually: **don't manage a
raw resource directly at all.**

```cpp
class Buffer {
    int* data;      // <-- you now owe the class Rule of 5
};

class Buffer {
    std::vector<int> data;   // <-- vector already IS Rule-of-5-correct internally
};                            //     Buffer itself needs NONE of the 5 functions
```

Classes should contain members that *already* correctly manage their
resources (`std::string`, `std::vector`, `std::unique_ptr`, `std::shared_ptr`
— all RAII types), rather than owning raw resources themselves. Then your
class needs zero custom special member functions — the compiler-generated
versions are correct by construction, because they just delegate to each
member's own correct implementation.

---

## 9️⃣ `= default` and `= delete`

Every class has a set of special member functions the compiler can
auto-generate for you (default constructor, copy ctor, copy assign, move
ctor, move assign, destructor) — but only under certain conditions, and
sometimes you want to be explicit about your intent.

### `= default` — "generate the normal implementation, and make that explicit"

```cpp
class Person {
public:
    Person(std::string name);      // you wrote a constructor with args...
    Person() = default;            // ...so the compiler won't give you a free
                                    // no-arg constructor anymore. Ask for it back explicitly.
};
```

Useful for documenting intent — "yes, I deliberately want the normal
behavior here," rather than leaving a reader wondering if you simply forgot
to write it.

**More real examples:**
```cpp
class Widget {
public:
    Widget() = default;                          // explicit no-op default ctor
    Widget(const Widget&) = default;              // "shallow copy is fine here, on purpose"
    Widget& operator=(const Widget&) = default;
    ~Widget() = default;                          // explicit trivial destructor
};
```

### `= delete` — "this operation must never happen, fail to compile if attempted"

```cpp
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
```

Before C++11, people faked this by declaring the copy constructor `private`
and never defining it — but that has real drawbacks:
- Member functions and friends of the class could still call it internally
  (compiles fine, fails only at *link time* with a cryptic error).
- Error messages were poor/indirect ("cannot access private member") instead
  of the modern, clear *"use of deleted function"* at the exact call site.

**More real examples of `= delete`:**
```cpp
class NoCopyAllowed {
public:
    NoCopyAllowed(const NoCopyAllowed&) = delete;
    NoCopyAllowed& operator=(const NoCopyAllowed&) = delete;
};

// deleting a specific overload to block an implicit conversion:
class Distance {
public:
    Distance(int meters);
    Distance(double) = delete;   // block accidental Distance d = 3.5;
};
```

---

## 🎯 Movable-but-not-copyable vs. Copyable-but-not-movable — real scenarios

**Movable, not copyable** (unique ownership — copying would mean two owners
of one resource, which is exactly the shallow-copy bug from earlier):

- `std::unique_ptr` itself
- `std::thread` (two `Thread` objects can't both represent/own the same OS
  thread)
- `std::lock_guard` / `std::unique_lock` (a lock guard represents ownership
  of holding a mutex locked — duplicating that ownership makes no sense)
- `std::ifstream` / `std::ofstream` (an open file handle is a unique
  resource)
- A `FileHandle`/`Socket`/`DatabaseConnection` wrapper class — exactly the
  Q11 exercise below

**Copyable, but not movable** (rarer, but happens when an object's *identity
or address* matters to something external):

- A class whose objects **register their own address** with a global
  registry/observer list on construction (e.g. some intrusive linked-list
  node, or an object pointed to by other objects via raw pointer) — moving it
  would invalidate those outstanding references, so the type deletes move
  and only allows copy (which creates an independent new registration).
- Some fixed-size, small "value-like" types that legacy APIs or ABI
  boundaries expect to always be copied by value, where making them movable
  provides no benefit and was deliberately left out for simplicity.

---

## 🧪 Quick mental checklist for any class you write

1. Does it own a raw resource (pointer, fd, socket, lock)? → you probably owe
   it Rule of 3/5, or better —
2. Can I instead wrap that resource in an existing RAII type
   (`unique_ptr`/`vector`/`string`)? → do that, get Rule of 0 for free.
3. If I *do* write copy operations, did I handle **self-assignment** and
   **freeing old memory before taking new**?
4. If I write move operations, did I mark them `noexcept`, and did I **empty
   the source** so its destructor is a safe no-op?
5. Did I use `delete[]` for anything allocated with `new[]`? (mismatched
   `new[]`/`delete` is undefined behavior — see Q2 in the solutions sheet.)