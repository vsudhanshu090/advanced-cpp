# 🧠 Level 10 — Smart Pointers & RAII

> **Big idea:** tie a resource's lifetime to an object's lifetime, so the compiler
> — not you — guarantees cleanup on every exit path (normal return, early
> return, exception, `break`, whatever). Smart pointers are just the most common
> RAII wrapper, specialized for heap memory.

---

## 1. RAII — the principle everything else builds on

**RAII = Resource Acquisition Is Initialization.** Bad name for a great idea:
*acquire the resource in a constructor, release it in the matching destructor.*
Because C++ guarantees destructors run when an object leaves scope — **even
during stack unwinding from an exception** — the release code always runs.

A "resource" is anything you acquire and must eventually give back:

| Resource | Acquire | Release |
|---|---|---|
| Heap memory | `new` | `delete` |
| File | `fopen` | `fclose` |
| Mutex | `lock()` | `unlock()` |
| Thread | (join or detach) | `join`/`detach` |
| Socket | `open` | `close` |
| DB connection | `acquire` | `release` |

The hard part was never *acquiring* — it's **guaranteeing release on every
possible exit path**, including ones you didn't plan for:

```cpp
// ❌ manual — one throw between lock/unlock and you deadlock forever
mutex.lock();
doSomething();   // if this throws, unlock() below never runs
mutex.unlock();

// ✅ RAII — unlocks even if doSomething() throws, because ~lock_guard runs
{
    std::lock_guard<std::mutex> lock(mutex);
    doSomething();
} // destructor unlocks here, guaranteed
```

The general shape you're building toward for the rest of Level 10:

```cpp
class ResourceWrapper {
    Resource resource;
public:
    ResourceWrapper(/* args */) { /* acquire resource */ }
    ~ResourceWrapper()          { /* release resource */ }
};
```

`unique_ptr`, `shared_ptr`, and `lock_guard` (Level 14) are all just
pre-built versions of this pattern for specific resources.

---

## 2. `std::unique_ptr` — exclusive ownership, move-only

Exactly **one** `unique_ptr` owns a given heap object at any moment.
Move-only, never copyable — the type system enforces "single owner" for you.

```cpp
#include <memory>

auto p = std::make_unique<int>(42);   // 42 lives on the heap; p lives on the stack
```

```cpp
auto q = p;          // ❌ compile error — copy ctor is deleted
auto q = std::move(p); // ✅ ownership transfers: q now owns it, p is now empty/null
```

> **What `move` actually does:** `std::move` doesn't move anything by itself —
> it's just a `static_cast` to an rvalue reference. That cast is what makes the
> compiler pick the *move* constructor/assignment instead of the (deleted) copy
> one. All the "moving" happens inside `unique_ptr`'s move constructor, which
> just steals the raw pointer and nulls out the source.

**Prefer `make_unique` over `new`:**

```cpp
auto p1 = std::make_unique<Person>("sudhanshu", 23);   // ✅ modern
std::unique_ptr<Person> p2(new Person("sudhanshu", 23)); // ⚠️ works, but avoid — see Q8/exception-safety note below
```

**Polymorphism works exactly like with raw pointers** — just remember the
base class needs a **virtual destructor**, or destroying through the base
pointer only runs `~Base()` and leaks the derived part:

```cpp
class Animal { public: virtual ~Animal() = default; };
class Dog : public Animal { /* ... */ };

std::unique_ptr<Animal> animal = std::make_unique<Dog>(); // fine, ~Dog() runs correctly
```

**Borrowing vs. owning access** — three ways to touch the underlying pointer,
and they mean very different things:

```cpp
auto p = std::make_unique<int>(42);

int* raw = p.get();   // 👀 BORROW — p still owns it. Never `delete raw;`

p.reset();             // 💥 destroys the object now. `raw` above is now dangling.

int* raw2 = p.release(); // 🎁 GIVE UP ownership WITHOUT destroying — p becomes
                          //    empty, and YOU now own raw2 (you must delete it).
```

| Call | Destroys object? | Who owns the pointer afterward? |
|---|---|---|
| `p.get()` | No | Still `p` — you're just peeking |
| `p.reset()` | **Yes, immediately** | Nobody — `p` is now null |
| `p.release()` | No | You — `p` is now null, but the object is alive and un-managed |

`unique_ptr` has (essentially) zero overhead vs. a raw pointer —
`sizeof(unique_ptr<T>) == sizeof(T*)` (bigger only if you attach a
stateful custom deleter, see §6).

**In containers:** can't copy, so move or construct in place:

```cpp
std::vector<std::unique_ptr<Foo>> objects;
objects.push_back(p);            // ❌ copy — not allowed
objects.push_back(std::move(p)); // ✅ move
objects.emplace_back(std::make_unique<Foo>("sudhanshu", 23)); // ✅ construct in place
```

---

## 3. `std::shared_ptr` — shared ownership, reference counting

Multiple owners share one resource. A `shared_ptr` is actually **two
pointers** bundled together:

```
shared_ptr<T>
 ├── pointer to the managed object (T*)
 └── pointer to a control block
              ├── strong ref count   (how many shared_ptr own the object)
              ├── weak ref count     (how many weak_ptr observe it)
              ├── deleter
              └── allocator info
```

```cpp
auto p = std::make_shared<int>(46);   // strong count = 1
auto q = p;                            // strong count = 2 (copy)
auto r = std::move(p);                 // strong count STILL 2 — this is a MOVE,
                                        // not a copy. p is now null, r owns what
                                        // p owned. No new owner was added.
q.reset();                             // strong count = 1
```

The object is destroyed the instant the **strong** count hits 0 — it doesn't
matter which `shared_ptr` triggers that, or in what order they were created.

```cpp
std::cout << p.use_count(); // any shared_ptr sharing the object reports the
                             // same number — it's a property of the control
                             // block, not of "your particular" shared_ptr
```

**Always prefer `make_shared` over `shared_ptr<T>(new T(...))`.** Two
independent reasons:

1. **One allocation instead of two.** `make_shared` allocates the object *and*
   the control block together in a single block — better cache locality,
   fewer malloc calls.
2. **Exception safety.** `new T(...)` is a separate expression from
   constructing the `shared_ptr`; if something else in the same statement
   throws between the `new` and the `shared_ptr` taking ownership, you leak.
   `make_shared` does both atomically as one call.

**Polymorphism:** same rule as `unique_ptr` — virtual destructor in the base:

```cpp
class Base { public: virtual ~Base() = default; };
std::shared_ptr<Base> p = std::make_shared<Derived>(); // ~Derived() runs correctly
```

**Thread safety — the part that trips everyone up:**

> `shared_ptr`'s reference-count increments/decrements are atomic, so it's
> safe for *multiple threads to hold their own copies* of the same
> `shared_ptr` and let them go out of scope concurrently — the count itself
> won't get corrupted.
>
> **What is NOT thread-safe:** the object it points to. If two threads call
> methods on `*p` concurrently and those methods mutate shared state, that's a
> plain old data race — `shared_ptr` gives you zero protection there. It only
> protects its own bookkeeping (the count), never your object's contents.

**The double-control-block trap** — never do this:

```cpp
Foo* raw = new Foo;
std::shared_ptr<Foo> p(raw);
std::shared_ptr<Foo> q(raw);   // ❌ BAD — creates a SECOND, independent control block
                                //    pointing at the same object. Each one thinks
                                //    its count reaching 0 means "I should delete
                                //    raw" → double free.

std::shared_ptr<Foo> q = p;    // ✅ correct — shares the SAME control block
```

---

## 4. `std::weak_ptr` — non-owning observer

Ownership vocabulary, three levels:

- `unique_ptr` → "I own this, alone."
- `shared_ptr` → "We jointly own this."
- `weak_ptr` → "I want to look, but I refuse to keep it alive."

A `weak_ptr` always starts life *from* a `shared_ptr` (or another `weak_ptr`
that itself traces back to one) — you can't independently point a `weak_ptr`
at a raw address. A default-constructed `weak_ptr` is just empty.

It does **not** touch the strong count, so it can't prevent destruction:

```cpp
auto p = std::make_shared<int>(42);
std::weak_ptr<int> w = p;
std::cout << w.expired(); // false
p.reset();
std::cout << w.expired(); // true — object is gone, w just knows about it
```

You can never dereference a `weak_ptr` directly (`w->foo()` doesn't even
compile) — you must **promote** it back to a `shared_ptr` first:

```cpp
if (auto sp = w.lock()) {      // ✅ the correct pattern
    sp->foo();                  // sp keeps the object alive for this block
}
```

`lock()` returns a `shared_ptr` — non-null (and strong count bumped by 1) if
the object is still alive, or empty if it's gone.

> ⚠️ **Race condition trap — don't do this:**
> ```cpp
> if (!w.expired()) {
>     auto sp = w.lock();   // ❌ TOCTOU gap: another thread could destroy the
>                            //    object between the expired() check and lock()
> }
> ```
> `if (auto sp = w.lock())` does the check and the promotion **atomically**
> as one operation, so there's no window for another thread to sneak in.

**Real use case — a cache that doesn't force objects to live forever:**

```cpp
std::unordered_map<Key, std::weak_ptr<Object>> cache;

if (auto obj = cache[key].lock()) {
    // still alive somewhere else — reuse it
} else {
    // expired — need to recreate and re-cache it
}
```

### Breaking reference cycles (the main reason `weak_ptr` exists)

`shared_ptr` can only free an object once its **strong** count hits zero. A
cycle of `shared_ptr`s pointing at each other means that count can never
reach zero — a genuine memory leak.

```
   ┌───────────────┐        strong        ┌───────────────┐
   │   struct A     │ ───────────────────▶ │   struct B     │
   │ shared_ptr<B> b│                       │ shared_ptr<A> a│
   └───────────────┘ ◀─────────────────── └───────────────┘
                            strong
```

```cpp
struct A { std::shared_ptr<B> b; };
struct B { std::shared_ptr<A> a; };

auto a = std::make_shared<A>();   // A's strong count = 1 (owned by variable `a`)
auto b = std::make_shared<B>();   // B's strong count = 1 (owned by variable `b`)
a->b = b;                          // B's strong count = 2 (also owned by a->b)
b->a = a;                          // A's strong count = 2 (also owned by b->a)
```

Now walk through what happens when `a` and `b` (the *local variables*) go
out of scope at the end of the function:

1. Local `a` destructs → A's strong count drops from 2 → **1**. Not zero, so
   `~A()` does **not** run. The `A` object survives — kept alive by `b->a`.
2. Local `b` destructs → B's strong count drops from 2 → **1**. Not zero
   either — kept alive by `a->b`.

Both objects are now unreachable from anywhere in your program (no variable
names them anymore), yet both are still "alive" as far as `shared_ptr` is
concerned, because each one's last reference is held *by the other object*.
**Neither destructor will ever run.** That's the leak — memory that's
unreachable but never freed.

**The fix:** make one side of the cycle a `weak_ptr` (conventionally, the
"child → parent back-reference" side, since the parent already owns the
child):

```cpp
struct A { std::shared_ptr<B> b; };
struct B { std::weak_ptr<A> a; };   // 👈 no longer contributes to A's strong count

auto a = std::make_shared<A>();
auto b = std::make_shared<B>();
a->b = b;
b->a = a;   // weak — doesn't bump A's strong count
```

Now when local `a` goes out of scope, A's strong count drops to **0**
immediately (only `b->a` referenced it, and that's a `weak_ptr` which doesn't
count) → `~A()` runs → `a->b` is destroyed → B's strong count drops to
**0** → `~B()` runs. The cycle unwinds cleanly.

---

## 5. `make_unique` / `make_shared` — why prefer them over raw `new`

```cpp
// ❌ avoid
std::unique_ptr<Foo> p(new Foo(10));
std::shared_ptr<Foo> p(new Foo(10));

// ✅ prefer
auto p = std::make_unique<Foo>(10);
auto p = std::make_shared<Foo>(10);
```

The gain is bigger for `shared_ptr`, because `make_shared` folds the control
block into the **same allocation** as the object — better cache locality,
fewer allocator calls, and no window where you've allocated the object but
not yet handed it to a `shared_ptr` (exception safety).

**One real trade-off:** with the combined allocation, if a `weak_ptr` is
still alive after the last `shared_ptr` resets, the control block (and thus
the whole combined block, object included) **can't be freed yet** — the
memory for the now-destroyed object sits around unused until the last
`weak_ptr` also goes away. With separate allocations, the object's memory
would free immediately and only the (smaller) control block would linger.
Rarely matters in practice, but it's the one place `make_shared` isn't
strictly better.

---

## 6. Custom deleters

Not every resource is freed with `delete`. Files need `fclose`, sockets need
`close`, `malloc`'d memory needs `free`. A custom deleter tells the smart
pointer *how* to release what it owns.

```cpp
struct FileDeleter {
    void operator()(FILE* f) const {   // note: NOT `const FILE*` — see Level 10 solutions Q6
        if (f) fclose(f);
    }
};

std::unique_ptr<FILE, FileDeleter> file(fopen("data.txt", "r"), FileDeleter{});
```

A deleter can be a function object (above), a lambda, or a function pointer:

```cpp
auto deleter = [](FILE* f) { if (f) fclose(f); };
std::unique_ptr<FILE, decltype(deleter)> file(fopen("data.txt", "r"), deleter);
```

`std::unique_ptr<Foo>` on its own is really `std::unique_ptr<Foo,
std::default_delete<Foo>>` — you've been using the default deleter this
whole time without seeing it spelled out.

**Key difference in where the deleter lives:**

| | Deleter is part of... | Consequence |
|---|---|---|
| `unique_ptr` | **the type itself** (2nd template arg) | `unique_ptr<FILE, DeleterA>` and `unique_ptr<FILE, DeleterB>` are different, incompatible types |
| `shared_ptr` | **the control block** (runtime, type-erased) | `shared_ptr<FILE>` is always the same type regardless of deleter — set at construction |

```cpp
std::shared_ptr<FILE> p(
    fopen("data.txt", "r"),
    [](FILE* f) { if (f) fclose(f); }
);   // no second template arg needed — deleter is stored, not typed
```

`unique_ptr` is meant to be as cheap as a raw pointer, so a big stateful
deleter grows the object's actual size (e.g. `sizeof(unique_ptr<Foo,
BigStatefulDeleter>)` can be much bigger than `sizeof(Foo*)`). `shared_ptr`
doesn't have this issue since the deleter always lives in the (already
heap-allocated) control block.

> ⚠️ **`make_unique` cannot take a custom-deleter type.** `make_unique<T>(args...)`
> only ever produces a `unique_ptr<T, std::default_delete<T>>`. For a custom
> deleter you must use the `unique_ptr` constructor directly, as shown above.

---

## 7. Control block details: `use_count()` and the aliasing constructor

A `shared_ptr`-managed object actually has **two independent lifetimes**:

- **Object lifetime** — ends when the strong count hits 0.
- **Control block lifetime** — ends when *both* the strong count **and** the
  weak count hit 0.

So a `weak_ptr` can keep the (small) control block alive long after the
actual object has been destroyed — the block just remembers "the thing I
used to point to is gone now," which is exactly how `expired()` works.

```cpp
if (p.use_count() == 1) {
    // "I'm the only owner" — DANGEROUS in multithreaded code!
}
```

`use_count()` is a snapshot that can be stale the instant another thread
copies or destroys a `shared_ptr` concurrently. Treat it as a debugging /
diagnostic aid only — never branch production logic on it.

### The aliasing constructor

Lets a `shared_ptr` point at a **sub-object** of something, while sharing the
*owning* object's control block (and therefore its lifetime):

```cpp
auto person = std::make_shared<Person>();
std::shared_ptr<int> age_ptr(person, &person->age); // aliasing constructor:
                                                       // (owner, pointer-to-return)
person.reset();
```

- `age_ptr.get()` returns `&person->age` — a pointer *into* the `Person`.
- `age_ptr` shares `person`'s control block, so it counts as another strong
  owner **of the whole `Person` object**.
- After `person.reset()`, the strong count is still ≥1 (because of
  `age_ptr`), so **the entire `Person` object stays alive** — not just the
  `age` field. There's no such thing as "partially delete the Person except
  age" — an object's destructor either runs or it doesn't, for the whole
  object. `age_ptr` simply gives you a *view* into a still-alive `Person`.
- Only once `age_ptr.reset()` also happens does the strong count hit 0 and
  `~Person()` actually run.

Compare to a plain raw pointer, which does **not** extend anything's
lifetime:

```cpp
int* age = &person->age;
person.reset();          // Person is destroyed (if this was the last owner)
// `age` is now a dangling pointer — using it is undefined behavior
```

General signature: `shared_ptr<U> p(owner, ptr)` — `p` and `owner` end up
sharing the same control block, so `owner`'s object is kept alive as long as
either `p` or `owner` (or copies of either) exist.

---

## 8. Choosing function parameter types for smart pointers

**Rule of thumb: a parameter should express what the function actually
*needs* — ownership, or just access — not mirror whatever type the caller
happens to have lying around.**

| Function needs... | Parameter should be | NOT |
|---|---|---|
| Just to *use* the object, no ownership stake | `const Foo&` or `const Foo*` | `const shared_ptr<Foo>&` (see below) |
| To **take over** ownership permanently | `unique_ptr<Foo>` **by value** | `unique_ptr<Foo>&` unless you specifically need to reset/reassign the caller's pointer |
| To become a **co-owner** (keep the object alive past the call) | `shared_ptr<Foo>` **by value** | — |
| To *peek* at a shared_ptr's target without becoming an owner | `const Foo&` / `const Foo*` | `const shared_ptr<Foo>&` |

```cpp
// no ownership needed — just take a reference/pointer to the object itself
void print(const Foo& foo);
void print(const Foo* foo);

auto p = std::make_unique<Foo>();
print(*p);              // works regardless of whether the caller used unique_ptr,
                         // shared_ptr, or a plain stack Foo — print() doesn't care
```

```cpp
// transfer ownership — by value, moved in
void take(std::unique_ptr<Foo> p);

auto p = std::make_unique<Foo>();
take(std::move(p));     // p is now empty; ownership moved into take()
                         // when take() returns, the Foo is destroyed there
```

> `void process(std::unique_ptr<Foo>& p)` forces the *caller* to already own
> the object via a `unique_ptr` specifically — an unnecessary restriction if
> `process` doesn't actually need to reseat or release that particular
> `unique_ptr`. If it just needs to use/modify the `Foo`, `Foo&` is more
> general and works no matter how the caller manages the object's lifetime.

```cpp
// share ownership — by value, copied in (bumps the strong count)
void use(std::shared_ptr<Foo> p);   // `use` becomes a co-owner for its duration

auto p = std::make_shared<Foo>();
use(p);                              // strong count++ while inside use()
```

`void use(const shared_ptr<Foo>& p)` avoids the refcount bump, but ask
yourself: does `use` need to know it's a `shared_ptr` at all? If not,
`const Foo&` is simpler and doesn't leak an implementation detail (how the
object is owned) into the function's interface.

**Why this matters for performance:** every `shared_ptr` copy is an atomic
increment; every destruction, an atomic decrement. In multithreaded /
low-latency code that's real, measurable cost — passing `shared_ptr` by
value into something called millions of times per tick (see Q13) adds
synchronization overhead for zero benefit if the function was never going to
outlive the caller's ownership anyway.

---

## 9. Converting between `unique_ptr` and `shared_ptr`

**`unique_ptr` → `shared_ptr`: always safe, one direction only.**

```cpp
auto p = std::make_unique<Foo>();
std::shared_ptr<Foo> q = std::move(p);   // p had the only reference, so handing
                                          // it off is trivially safe. p is null now.
```

You can even skip the intermediate variable:

```cpp
std::shared_ptr<Foo> q = std::make_unique<Foo>();
```

This compiles because `shared_ptr` has a constructor that accepts a
`unique_ptr&&` and builds a fresh control block around it. **Why not just use
`make_shared` directly, then?** Because this path does a **separate**
allocation for the control block (the object was already allocated by
`make_unique`) — you lose the "one combined allocation" optimization that
`make_shared` gives you. Use this conversion only when you *started* with a
`unique_ptr` for some other reason (e.g. a factory function that returns
`unique_ptr` by convention) and later decide you need shared ownership.

**`shared_ptr` → `unique_ptr`: impossible, by design, not just "hard".**

```cpp
std::shared_ptr<Foo> p = std::make_shared<Foo>();
std::unique_ptr<Foo> q = std::move(p);   // ❌ compile error — no such conversion exists
```

This isn't really a thread-safety issue — it's about what a `shared_ptr`
*means*: it can have arbitrarily many owners, and there's no way to prove at
compile time that a given `shared_ptr` is (and will remain) the sole owner.
Even if `use_count() == 1` right now, that's a runtime fact that could
change the instant another thread — or even you, elsewhere in the same
function — copies it. `unique_ptr`'s entire contract is "exclusive
ownership, guaranteed by the type system," and there's no safe, general way
to *revoke* the possibility of other owners after the fact. So the standard
simply doesn't offer this conversion at all.

> ⚠️ **Never do this manually either:**
> ```cpp
> auto p = std::make_shared<Foo>();
> std::unique_ptr<Foo> q(p.get());   // compiles, but is WRONG
> ```
> Now `p` and `q` both believe they own the object independently. When `q`
> goes out of scope it calls `delete` on the raw pointer; later, when `p`'s
> strong count hits 0, it calls `delete` on the *same* pointer again —
> double free / undefined behavior.