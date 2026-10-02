# 📦 Level 11 — Memory Management

> **Goal of this level:** understand *where* memory lives, *who* is responsible for it at every step, and how to catch it when you get that responsibility wrong.

---

## 🗂️ Table of Contents
1. [Stack vs Heap](#1-stack-vs-heap)
2. [Process Memory Layout](#2-process-memory-layout)
3. [`new`/`delete` Internals](#3-newdelete-internals)
4. [Placement New](#4-placement-new)
5. [Alignment (`alignof`, `alignas`)](#5-alignment-alignof-alignas)
6. [Leaks, Dangling Pointers, Buffer Overflows](#6-leaks-dangling-pointers-buffer-overflows)
7. [Custom Allocators](#7-custom-allocators)
8. [RAII for Arbitrary Resources](#8-raii-for-arbitrary-resources)
9. [Tools: ASan / UBSan / Valgrind](#9-tools-asan--ubsan--valgrind)
10. [`std::byte` — Memory as Raw Storage](#10-stdbyte--memory-as-raw-storage)

---

## 1. Stack vs Heap

| | Stack | Heap |
|---|---|---|
| **Allocation cost** | ~free — just moves a stack pointer | goes through an allocator, has real bookkeeping cost |
| **Lifetime** | tied to scope — freed automatically | you decide (manually, or via RAII / smart pointers) |
| **Size** | small (typically a few **MB**, OS/thread/compiler dependent) | large, but still finite |
| **Fragmentation risk** | none | yes |

```cpp
int* p = new int(10);
```
Here `new` asks the dynamic allocator to find and hand back a free block, and that allocator has to manage *every* concurrent allocation/deallocation (thread-safety included). That bookkeeping is the expensive part — **not** the later dereference `*p`, which is just a normal memory access once you have the address.

> ⚠️ **Don't oversimplify to "stack fast, heap slow."** The *allocation/deallocation* step is what's expensive on the heap. Access speed for already-allocated memory is comparable — the real cost difference shows up in cache locality (see §10) and allocator overhead.

**Why the stack is small on purpose:**
```cpp
int arr[10'000'000]; // ⚠️ likely stack overflow
```
Stack size is fixed per-thread (set by the OS/runtime, typically a few MB) — going past it crashes the program (stack overflow), so anything large or size-unknown-at-compile-time belongs on the heap. But the heap isn't infinite either — a huge enough request can throw `std::bad_alloc`.

---

## 2. Process Memory Layout

```
High addresses
┌─────────────────────────┐
│          Stack           │  ← grows downward, function frames, automatic objects
│            ↓              │
│                          │
│            ↑              │
│           Heap            │  ← grows upward, dynamic (new/malloc) storage
├─────────────────────────┤
│           BSS              │  ← zero-initialized / uninitialized globals & statics
├─────────────────────────┤
│           Data             │  ← explicitly-initialized (non-zero) globals & statics
├─────────────────────────┤
│      Text / Code           │  ← compiled machine instructions (read-only, executable)
└─────────────────────────┘
Low addresses
```

| Segment | Holds | Example |
|---|---|---|
| **Text / Code** | compiled machine instructions | the compiled body of `int main(){...}` itself — read-only/executable, not "assembly you write," but the assembly the compiler *generated* from your C++ |
| **Data** | globals/statics with an explicit **non-zero** initializer, static storage duration | `int x = 10;` / `static int y = 20;` (anywhere: function, block, class) |
| **BSS** ("**B**lock **S**tarted by **S**ymbol") | globals/statics that are zero or have no initializer | `int x;` / `static int y;` / `static int z = 0;` |
| **Heap** | dynamic storage | `int* p = new int(10);`, `vector<int> v(100)`'s internal buffer, `make_unique<int>(10)`'s pointee |
| **Stack** | function-call frames, automatic (local) objects | any ordinary local variable |

**Why does BSS exist separately from Data at all?**
Because a zero-initialized global doesn't need its *value* stored anywhere in the compiled binary — the loader just needs to know "reserve N zeroed bytes here" at program start. BSS is metadata (a size), not actual stored bytes, which keeps the executable file smaller. Data-segment variables, by contrast, have real non-zero bytes that must be baked into the binary and copied in at load time.

> 🟣 **Memory-hook:** *"BSS = Big Silent Space — the compiler doesn't even bother writing the zeros to disk."*

**Important nuance — C++ doesn't actually mandate "stack variables":**
```cpp
int foo() {
    int x = 10;
    return x;
}
```
The standard only talks about **storage duration** (automatic / static / dynamic / thread), not "this variable lives on the stack." The compiler is free to keep `x` entirely in a CPU register and never touch the stack at all. "Local variable → stack" is a *useful mental model*, not a language guarantee.

**Where do string literals live?**
```cpp
const char* p = "hello";
p[0] = 'H';   // ❌ undefined behavior
```
The literal `"hello"` sits in a read-only region associated with the executable (conceptually part of Text/rodata) — `p` itself (the pointer variable) is an ordinary local, so it lives wherever the compiler puts locals. Writing through `p` into read-only memory is UB, and on many systems will actually crash (segfault) rather than silently corrupt anything.

---

## 3. `new`/`delete` Internals

`new`/`delete` are really **two responsibilities glued together**:
1. Memory allocation/deallocation
2. Object construction/destruction

```cpp
MyClass* m = new MyClass(10);
// → allocate raw memory → construct MyClass(10) in it → return MyClass*
```

### `new` vs `::operator new`
| Expression | What it does |
|---|---|
| `A* p = new A(32);` | **new-expression**: allocates storage **and** constructs the object |
| `void* p = ::operator new(sizeof(A));` | **allocation function only** — raw storage, nothing constructed |

To construct manually after a raw `operator new`, you need placement new (§4):
```cpp
void* memory = ::operator new(sizeof(A));
A* p = new (memory) A(42);
```

### `delete` vs `::operator delete`
| Expression | What it does |
|---|---|
| `delete p;` | **delete-expression**: runs the destructor, **then** frees memory |
| `::operator delete(memory);` | deallocation only — **no destructor call** |

If you allocated manually, you must clean up manually and in the right order:
```cpp
p->~A();
::operator delete(memory);
```

**If the constructor throws, who frees the just-allocated memory?**
The language handles this for you: if construction inside a `new`-expression throws, the runtime automatically calls the matching `operator delete` to release the raw storage (you never get a leaked block from a failed constructor when using plain `new T(...)`).

**How does `delete[]` know how many elements to destroy?**
The allocator stores bookkeeping (typically the element count) adjacent to the array's memory block when you use `new[]`; `delete[]` reads that hidden metadata to know how many destructors to run before freeing the block.

`delete nullptr;` is explicitly defined to do nothing — always safe.

You can also overload `operator new`/`operator delete` per-class (rarely needed, but it's just an operator):
```cpp
class A {
public:
    static void* operator new(size_t size) {
        std::cout << "Allocating " << size << '\n';
        return ::operator new(size);
    }
    static void operator delete(void* ptr) {
        std::cout << "Deallocating\n";
        ::operator delete(ptr);
    }
};
```

---

## 4. Placement New

| | Normal `new` | Placement `new` |
|---|---|---|
| Steps | allocate storage → construct → return pointer | (storage already exists) → construct only → return pointer |
| Syntax | `new T(...)` | `new (address) T(...)` |
| Cleanup | `delete p;` | manual: `p->~T();` then free storage separately |

```cpp
void* memory = ::operator new(sizeof(A));
A* p = new (memory) A(32);
p->print();          // used normally

p->~A();                       // manually destroy
::operator delete(memory);     // release raw storage — NOT `delete p;`
```

**Why does placement new even exist?** Because you often already own a block of memory (a memory pool, an arena, a slot inside a container) and don't want to pay for *another* general-purpose allocation just to construct an object into memory you already have:
```cpp
A* p = new (slot) A(12);   // slot is pre-existing storage
```

Also works with `malloc`:
```cpp
void* memory = malloc(sizeof(A));
A* p = new (memory) A(42);
// ...
p->~A();
free(memory);
```

**Canonical real-world example — `std::vector`:** when `capacity() > size()`, the extra slots are already-allocated-but-not-yet-constructed storage. Growing the vector (e.g. `push_back`) doesn't call `new` for a fresh block each time — it uses placement new to construct the new element directly into the next reserved slot.

> 🔑 **Does stack unwinding clean up a placement-new'd object for you?**
> **No.** Stack unwinding only runs destructors for *automatic (stack) objects* whose scope is ending. An object you built with placement new is not tracked by the compiler as a stack object — even if the raw storage it lives in *is* on the stack (e.g. a local `char buffer[N]`), the *object inside it* is entirely your responsibility to destroy. This is exactly why placement new is usually wrapped in an RAII class (§8) that calls `->~T()` in its destructor — so you *do* get automatic cleanup, but you built that guarantee yourself.

---

## 5. Alignment (`alignof`, `alignas`)

**Core idea:** many types must start at an address that's a multiple of some number (their *alignment requirement*) for correct/fast CPU access.

```cpp
int x;
// alignof(int) == 4  →  address of x must be divisible by 4
```

Typical (implementation-defined, but common on 64-bit systems):
```cpp
alignof(char)   // 1
alignof(int)    // 4
alignof(double) // 8
```

> 📌 **Clearing up a common confusion:** if `alignof(T) == 8`, valid addresses are `0x1000`, `0x1008`, `0x1010`, `0x1018`, `0x1020`, ... — **every one of these actually is divisible by 8** (`0x1010` = 4112 decimal = 8 × 514, `0x1018` = 4120 = 8 × 515). What's *invalid* is something like `0x1004` (4100, not divisible by 8). If a run of 8-aligned addresses ever looked "off" to you, double check the hex→decimal conversion, not the rule.

**Why does `alignof(SomeStruct)` usually equal its largest member's alignment?**
Because the struct as a whole must be safely placed inside *arrays* of itself too — every element of `SomeStruct arr[N]` needs each member correctly aligned, which forces the struct's own alignment to be at least as strict as its strictest member.

**Alignment vs size — padding:**
```cpp
struct A {
    char c;   // 1 byte
    int x;    // needs 4-byte alignment
};
```
You might expect `sizeof(A) == 5`, but it's typically **8**:
```
0  char c
1  padding
2  padding
3  padding
4  int x  (4 bytes)
```
The compiler inserts **padding** so `x` starts at an address divisible by 4, and so that in an array `A arr[N]`, every element's `x` stays aligned too.

**Member ordering changes struct size — real, measurable:**
```cpp
struct A { char c; int x; char d; };   // e.g. 12 bytes
struct B { int x; char c; char d; };   // e.g. 8 bytes  — same data, smaller!
```
Grouping same-size / same-alignment members together minimizes padding. This matters in performance-sensitive / memory-dense code (order books, tick data, etc.).

**`alignas` — requesting stricter alignment:**
```cpp
alignas(16) int x;                       // request 16-byte alignment
alignas(64) char buffer[1024];           // request 64-byte alignment
alignas(16) char buffer[sizeof(A)];      // storage sized/aligned correctly to build an A in-place
```

**Cache-line use case — avoiding false sharing:**
If two threads frequently write to *different* variables that happen to sit on the *same* 64-byte CPU cache line, each write invalidates the other core's cached copy of that line — this is **false sharing**, and it's a real, measurable multithreaded performance bug.
```cpp
struct alignas(64) Data {
    int value;
};
```
Padding `Data` out to a full cache line guarantees two instances of it never share a cache line.

> ⚠️ Don't sprinkle `alignas(64)` everywhere — it inflates memory usage. Reserve it for genuine cache/multithreading contention cases.

**Violating alignment (e.g. placement-new'ing at an unaligned address like `0x1003`) is undefined behavior** — it can silently work, silently be slow, or hard-fault, depending on architecture.

**You cannot *relax* alignment below a type's requirement:**
```cpp
struct A { double x; };
alignas(1) A x;   // ❌ not possible — alignas can only request >= the type's natural alignment
```

---

## 6. Leaks, Dangling Pointers, Buffer Overflows

These are the three "classic" memory bugs — **know the difference precisely**, they're not interchangeable:

| Bug | Definition | Danger |
|---|---|---|
| **Leak** | memory is still allocated, but you've lost every way to reach/free it | slow growth: 100MB → 200MB → ... → OOM kill / crash / `bad_alloc` |
| **Dangling pointer/reference** | points to an object whose lifetime has already ended | reading/writing it is UB — you might read garbage, or (worse) something that now belongs to a *different* live object |
| **Buffer overflow / OOB access** | you access outside an object's allocated bounds | can corrupt adjacent data, bookkeeping, or another object entirely |

### Leak
```cpp
void memoryLeak() {
    int* p = new int[1000];
    // never delete[] p — the block is now unreachable and un-freeable
}
```
Fix: let RAII own it.
```cpp
void memoryLeakFix() {
    std::vector<int> v(1000);            // owns + frees automatically
    auto p = std::make_unique<int[]>(1000);
}
```
A leak does **not** necessarily crash your program immediately — it degrades it (memory bloat → slowdown → eventual allocation failure or OS kill).

### Dangling pointer / reference
```cpp
int* p;
{
    int x = 10;
    p = &x;
}                 // x's lifetime ends here
std::cout << *p;  // ❌ UB — reading a dead stack slot
```
Use-after-free is the heap version, and it's especially dangerous because the allocator may have already handed that exact address to something else:
```cpp
int* p = new int(10);
delete p;
std::cout << *p;  // ❌ UB
```
**Not limited to pointers** — dangling *references* are just as real:
```cpp
int& foo() {
    int x = 10;
    return x;     // ❌ returns a reference to a destroyed local
}
```
And a subtler one — **iterator/pointer invalidation**:
```cpp
std::vector<int> v = {1, 2, 3};
int* p = &v[0];
v.push_back(4);     // may reallocate v's internal buffer
std::cout << *p;    // ❌ UB — p may point into freed memory
```
The same invalidation risk applies to iterators (`v.begin()`, etc.) after any operation that can resize/reallocate.

### Buffer overflow / OOB access
```cpp
int arr[5];
arr[5] = 30;   // ❌ UB — one past the end
```
Dangerous specifically because adjacent memory might hold something critical: another variable, a buffer, or allocator bookkeeping metadata.

Two more classic named bugs worth knowing: **use-after-free** and **double free** (both shown/discussed above and in §9).

---

## 7. Custom Allocators

An allocator answers exactly two questions:
1. "Give me raw storage for `n` objects of type `T`."
2. "I'm done with that storage — take it back."

```cpp
template<typename T>
struct MyAllocator {
    T* allocate(size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    void deallocate(T* p, size_t n) {
        ::operator delete(p);
    }
};
std::vector<int, MyAllocator<int>> v;   // vector now uses MyAllocator<int> for its storage
```

> ⚠️ **An allocator only provides storage — it does not construct objects.** Construction still happens via placement new, e.g. when `vector` inserts an element:
> ```cpp
> T* memory = allocator.allocate(100);
> new (memory + i) T(value);   // vector places each element itself
> ```

### A minimal arena / bump / pool allocator
```cpp
class Pool {
    void* memory;
    size_t next;
public:
    explicit Pool(size_t bytes) : memory(::operator new(bytes)), next(0) {}

    void* allocate(size_t bytes) {
        // memory is void* — void* has no defined size, so no pointer
        // arithmetic is allowed on it directly. Cast to char* (1-byte
        // element type) so "+ next" advances by exactly `next` BYTES,
        // giving the address of the next free slot.
        void* result = static_cast<char*>(memory) + next;
        next += bytes;
        return result;   // caller will placement-new an object here
    }

    ~Pool() { ::operator delete(memory); }
};

Pool pool(1024);
int*    a = new (pool.allocate(sizeof(int)))    int(10);
double* b = new (pool.allocate(sizeof(double))) double(20.34);
```
This trades general-purpose flexibility for raw speed: instead of asking the heap allocator for every small object individually, you carve out one big block up front and hand out slices — the allocation itself becomes just a pointer add. Extremely relevant to low-latency/quant systems that want zero `malloc` calls in the hot path.

**This deliberately has no per-object `free`.** Why is that an acceptable tradeoff for something like a per-tick scratch buffer?
- Objects in it are all "born" and "die" together, once per tick.
- Instead of freeing objects one by one, you simply **reset `next = 0`** at the start of the next tick and start overwriting from the top — O(1) "free everything."
- This is only safe when nothing needs to outlive the tick — which is exactly the scratch-buffer use case.

*(This works identically whether `memory` is heap-allocated as above, or you swap `memory`/`next` for a fixed-size `char buffer[N]` living on the stack — the bump-pointer logic doesn't change, only where the backing block itself lives.)*

### Modern C++: `std::pmr` (polymorphic memory resources)
```cpp
char buffer[1024];
std::pmr::monotonic_buffer_resource pool(buffer, sizeof(buffer));
std::pmr::vector<int> v(&pool);

v.push_back(10);
v.push_back(20);   // v pulls its storage from `pool`/`buffer` instead of the global heap
```
- `buffer` doesn't *have* to be `char[]` — any byte-sized array works (`unsigned char[]`, or `std::byte[]`, see §10) as long as it's raw, appropriately-sized storage.
- `buffer` living on the stack is completely fine *as long as `pool` doesn't outlive `buffer`* — here both are locals in the same scope, so that's satisfied. It's not "useless afterward"; it's used for exactly as long as it's in scope, same as any other local. If `pool`/`v` needed to escape this scope, `buffer` would need a longer lifetime too (e.g. a member variable, or heap-allocated).

---

## 8. RAII for Arbitrary Resources

**RAII is not "a smart pointer thing"** — it's a general pattern: *tie a resource's lifetime to an object's lifetime*, so the destructor guarantees cleanup no matter how the scope is exited (normal return, early return, or exception).

"Resource" can be *anything* that must eventually be released:
- heap memory (what smart pointers wrap)
- the arena/`Pool` from §7 (its destructor frees the whole block)
- file handles, mutex locks, sockets, database connections, GPU handles, ...

The through-line for this whole level: `unique_ptr`, `Pool`, and a `std::lock_guard` are all *the same idea* applied to different resource types.

---

## 9. Tools: ASan / UBSan / Valgrind

These catch the runtime bugs from §6 — ones the compiler generally **cannot** prove are wrong at compile time.

### AddressSanitizer (ASan)
```bash
g++ -std=c++20 -fsanitize=address -g main.cpp -o out
```
> ⚠️ **ASan is a *runtime* instrumentation tool, not a compile-time check.** It rebuilds your program with extra instrumentation that watches every memory access *while the program runs*, then reports a bug the moment the offending access actually happens — you still have to *run* the program (and hit the buggy code path) to see the report. Compiling with `-fsanitize=address` alone catches nothing by itself; it only takes effect once the instrumented binary executes.

Catches, at the moment they occur, with a precise report (including allocation/deallocation stack traces) instead of a silent corruption or generic segfault:
- heap/stack buffer overflow — `vector<int> v(10); v[9999] = 10;`
- use-after-free — `delete p; *p;`
- double free — `delete p; delete p;`
- some other lifetime-related errors

### UndefinedBehaviorSanitizer (UBSan)
```bash
g++ -fsanitize=undefined ...
```
Catches other UB categories: signed integer overflow, invalid shifts, invalid casts, alignment violations, etc.
```cpp
int x = INT_MAX;
x++;   // UBSan flags this
```

**Combine both** (standard real-world workflow):
```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g main.cpp
```

**`-g`** adds debugging symbols — without it you get `#0 0x29321 #1 0x30045`; with it you get `main.cpp:17`, dramatically more useful for any of the above tools.

### Valgrind (Linux only, no recompile needed — but slower)
```bash
g++ -std=c++20 -g main.cpp -o out
valgrind --leak-check=full ./out
```
Catches: invalid reads/writes, use-after-free, double free, memory leaks, uninitialized-memory reads.
```cpp
void foo() {
    int* p = new int(42);
}   // valgrind reports this as "definitely lost"
```

### Debugger (a different tool for a different job)
A debugger like `gdb` doesn't find bugs for you automatically — it lets you *inspect* what already happened:
```
gdb ./out
```
breakpoint → step → inspect variables → backtrace.

### Compiler warnings
```bash
-Wall -Wextra -Wpedantic
```
Cheapest first line of defense — catches a large class of mistakes before you ever run the program.

---

## 10. `std::byte` — Memory as Raw Storage

**Before C++17**, raw storage was usually represented as `char buffer[100];` — but `char` is semantically a *character/small-integer* type, which muddies the intent ("is this text? a number? just bytes?").

`std::byte` (C++17) exists purely to mean **"a byte of storage, nothing more"** — it's deliberately restrictive:
```cpp
std::byte b{5};
// b + 1;         ❌ not allowed — no arithmetic
// b * 2;         ❌ not allowed
int x = std::to_integer<int>(b);   // ✅ explicit, intentional conversion only
```
This is on purpose — it stops raw memory from being accidentally treated as a character or a number by mistake.

```cpp
std::byte buffer[100];   // 100 bytes of storage — no objects living here yet
```

To actually build an object in it correctly, you must respect both **size and alignment**:
```cpp
alignas(int) std::byte buffer[sizeof(int)];   // room for exactly one int, correctly aligned
int* p = new (buffer) int(100);
```
Forgetting `alignas` here is UB — sized-correctly isn't the same as *aligned*-correctly (see §5).

---

## ✅ Level 11 Recap

- Stack = fast, automatic, small. Heap = flexible, manual (or smart-pointer-managed), larger but not infinite.
- `new`/`delete` = allocate+construct / destruct+free, bundled. Placement new lets you split that bundle apart.
- Alignment isn't optional — it's why structs have padding, why member order changes `sizeof`, and why cache-line-aware code uses `alignas(64)`.
- Leak ≠ dangling pointer ≠ buffer overflow — three distinct failure modes, know which is which.
- Custom/arena allocators and `std::pmr` exist to sidestep general-purpose allocator overhead in hot paths — extremely relevant for quant/low-latency work.
- RAII is the general pattern behind smart pointers, pools, locks, and file handles alike.
- ASan/UBSan run at **runtime**; compiler warnings and `-g` are cheap and should always be on while learning this material.