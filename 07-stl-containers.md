# Level 7 — STL Containers

> Every container is a different trade-off between **contiguous memory** (fast iteration, bad middle-insert), **linked structure** (fast middle-insert, bad iteration/cache behavior), and **hashing** (fast average lookup, no ordering). Picking a container is picking which operation you're willing to make slow.

---

## 📑 Topics in this level
1. [`std::vector`](#1-stdvector--dynamic-array-growth-strategy-capacity-vs-size)
2. [`std::deque`](#2-stddeque--double-ended-queue)
3. [`std::list` / `std::forward_list`](#3-stdlist--stdforward_list--doublysingly-linked-list)
4. [`std::map` / `std::unordered_map`](#4-stdmap--stdunordered_map--ordered-vs-hash-based-key-value)
5. [`std::set` / `std::unordered_set`](#5-stdset--stdunordered_set)
6. [`std::pair`, `std::tuple`, structured bindings](#6-stdpair-stdtuple-structured-bindings)
7. [`std::array`](#7-stdarray-fixed-size-stack-allocated)
8. [Container adapters: stack / queue / priority_queue](#8-stdstack--stdqueue--stdpriority_queue-container-adapters)
9. [Iterator invalidation rules](#9-iterator-invalidation-rules-per-container)
10. [Choosing the right container](#10-choosing-the-right-container--full-cheat-sheet)

---

## 1. `std::vector` — dynamic array, growth strategy, capacity vs size

`vector<T>` is a **contiguous**, dynamically resizable array. "Contiguous" is the whole story of why it behaves the way it does: all elements sit in one unbroken block of memory, so when that block runs out of room, the vector can't just "extend it" — it has to allocate a brand-new, bigger block, move every element over, and free the old block. This is **reallocation**.

- **`size()`** — how many elements are actually in the vector right now.
- **`capacity()`** — how many elements the *currently allocated block* can hold before the next reallocation is triggered.

📌 **Mental model:** size = occupied seats, capacity = total seats in the room. Room capacity doesn't shrink when a few seats empty out, but the room does need to *move to a bigger building* once every seat is full and one more person shows up.

```cpp
vector<int> v = {1,2,3};
v.size();       // 3
v.capacity();   // implementation-defined, commonly 3 or 4 — the standard doesn't mandate the growth factor
```
Growth factor is **implementation-defined**, not standardized: libstdc++ (GCC, what you're likely using) doubles capacity on reallocation; MSVC typically grows by ~1.5x. Both are "amortized O(1)" — see below.

### `reserve()` vs `resize()` — different tools, don't confuse them
```cpp
vector<int> vec;
vec.reserve(1000);   // capacity() >= 1000, size() STAYS 0 — no elements created
```
`reserve()` pre-allocates room so that up to 1000 `push_back`s won't trigger any further reallocation — pure optimization when you know the eventual size ahead of time.

```cpp
vec.resize(100);     // size() becomes 100 — 100 NEW elements are created (int -> 0-initialized)
```
`resize()` actually **changes the element count**, not just the backing storage.
- **Growing** an existing vector with `resize(n)`: new elements beyond the current size are value-initialized (`0` for `int`, `""` for `string`, default-constructed for a class). `resize(n, val)` fills the new elements with `val` instead of the default.
- **Shrinking** with `resize(n)` where `n < size()`: the extra elements past index `n-1` are destroyed (destructors run), `size()` becomes `n`, capacity is typically left unchanged (still available for future growth).
- Works identically regardless of `T` — for a class type, growing calls the default constructor for each new slot (so the type must be default-constructible, or you must pass a fill value: `resize(n, SomeType(args))`).

### Amortized O(1) — what "amortized" actually means
A single `push_back` that happens to trigger reallocation costs O(N) (every existing element gets copied/moved). But reallocation only happens O(log N) times total across N pushes (since capacity doubles each time), and the *total* work across all N pushes sums to O(N) — so the **average cost per push, spread across the whole sequence**, is O(1), even though occasional individual pushes are expensive.

📌 **"Amortized" = "expensive operations happen rarely enough that, averaged over many calls, each call still looks cheap."** It says nothing about any *one* call — it's a statement about a whole sequence of calls.

Other classic amortized-O(1) examples:
- **Hash table insertion** (`unordered_map`/`unordered_set`) — individual inserts are O(1), except the rare one that triggers a rehash (O(N)), averaged out over many inserts.
- **Dynamic array doubling in general** (this is really the same mechanism vector uses) — e.g. Python's `list.append`, Java's `ArrayList.add`.
- **Union-Find / Disjoint Set with path compression** — each `find` is amortized nearly O(1) (technically O(α(N)), the inverse Ackermann function) even though a single call can walk a long chain the first time.

### Reallocation invalidates *everything* pointing into the old storage
```cpp
vector<int> v = {10, 20, 30};
int* p = &v[0];
v.push_back(40);   // MAY reallocate -> p might now be a dangling pointer into freed memory
```
**Yes — this is exactly the same danger for `v.begin()`/`v.end()`.** Iterators from `vector` are typically implemented as (possibly wrapped) raw pointers into the buffer, so `v.begin()` taken before a reallocating `push_back` is just as invalid afterward as `p` above — same underlying reason, same fix (re-fetch `begin()`/`end()` after any operation that might reallocate, or `reserve()` up front so it never happens mid-loop).

### `push_back` vs `emplace_back`
`push_back` inserts an object you already constructed. `emplace_back` **constructs the object in-place** at the end of the vector's storage, forwarding whatever arguments you give it straight to `T`'s constructor — no separate object is built and then copied/moved in.
```cpp
people.push_back(Person("Alice", 25));   // builds a temporary Person, then moves/copies it in
people.emplace_back("Bob", 30);           // builds the Person DIRECTLY in the vector's storage
```
For simple types (`int`, `double`) there's no meaningful difference — nothing expensive to avoid copying. For user-defined types, `emplace_back` **can** be faster since it skips a move/copy — but only when you're constructing from raw arguments. If you already *have* an object, both end up doing the same move:
```cpp
Person p("sudhanshu", 20);
people.push_back(move(p));   // explicit move, no copy — same cost as emplace_back would've been here
```
📌 `emplace_back` isn't a strictly-better `push_back` — it shines specifically when you're constructing a new object from scratch.

---

## 2. `std::deque` — double-ended queue

`deque<T>` supports **efficient insertion/deletion at both ends** — `push_back`/`push_front`, `pop_back`/`pop_front`, all amortized O(1). It's tempting to picture it as "a vector that can also grow from the front," but its memory layout is fundamentally different, not just an extension of vector's.

### Why it isn't just a two-sided vector
A `deque` stores its elements across **multiple fixed-size memory blocks (chunks)**, not one contiguous allocation, plus a small internal map/index structure that tracks which chunk holds which range of elements.
- `vector` → one contiguous block.
- `deque` → several separately-allocated chunks, stitched together logically by an index.

This is *why* `push_front` can be O(1): there's room to grow a new chunk at the front (or use spare capacity already reserved there) without having to shift every existing element over, the way a hypothetical "front-growable vector" would need to.

### How does `dq[i]` stay O(1) if memory isn't contiguous?
Because the internal index structure lets the deque compute, from `i`, exactly *which chunk* and *which offset within that chunk* holds element `i` — a small constant amount of arithmetic, not a search. Still O(1), just with a slightly larger constant factor than `vector`'s "just add to a base pointer."

### `.data()` doesn't exist on deque
```cpp
v.data();    // vector: pointer to the one contiguous block — makes sense
dq.data();   // ❌ doesn't exist — there IS no single contiguous block to point to
```

### So why not always use `deque` instead of `vector`?
**Cache locality.** `vector`'s one unbroken block means walking through it sequentially is extremely CPU-cache-friendly (the whole point of Level 17's cache-locality discussion, foreshadowed here). `deque`'s chunks are separate allocations that aren't guaranteed to be near each other in memory — iterating a `deque` end-to-end typically means more cache misses than the equivalent `vector` walk. Default to `vector`; reach for `deque` specifically when you need front-insertion/removal that `vector` can't do efficiently.

---

## 3. `std::list` / `std::forward_list` — doubly/singly linked list

- `list<T>` — **doubly** linked list (each node points to both `next` and `prev`).
- `forward_list<T>` — **singly** linked list (each node points only to `next`) — more memory-lean, but you can only walk it forward.

| | Random access (`v[i]`) | Cache locality | Insert/erase *given a position* |
|---|---|---|---|
| `list` | ❌ not supported (must walk) | ❌ poor — nodes scattered in memory | ✅ O(1) |

`forward_list` only supports `insert_after(it, val)` / `erase_after(it)` — because with only a `next` pointer, you can't cheaply reach the node *before* a given iterator, so all its mutating operations are phrased in terms of "after this position" rather than "at this position."

### `splice` — the operation that makes linked lists genuinely special
`list::splice` moves elements from one list into another **by relinking nodes**, with **zero copying and zero iterator invalidation** for the moved elements — something no contiguous container can do, since `vector`/`deque` would have to physically copy the data and would invalidate iterators/pointers into the moved range.
```cpp
list<int> a = {1,2,3};
list<int> b = {10,20,30};
auto it = std::next(a.begin());   // points to "2"
a.splice(a.end(), b, b.begin());  // moves "10" from b into a, at a's end — O(1), no copying
// `it` is STILL valid and still points to "2" in `a` — splice never invalidates existing iterators
```
This is the single biggest practical reason to reach for `list` over `vector` when you're doing heavy element *reordering* (not just insertion) across the list — merge/splice-heavy algorithms (some LRU cache implementations, intrusive scheduling queues) lean on this directly.

---

## 4. `std::map` / `std::unordered_map` — ordered vs hash-based key-value

`map<K,V>` is (in every mainstream implementation) a **red-black tree** — a self-balancing binary search tree that keeps keys in sorted order at all times, guaranteeing O(log n) for lookup/insert/erase in the *worst* case, not just on average. Every insert/delete may trigger rotations to keep the tree balanced, but those rotations are themselves O(log n), so the guarantee holds.

`unordered_map<K,V>` is a **hash table**: keys are hashed to a bucket index, and elements landing in the same bucket are chained together (commonly via a linked list per bucket). Average-case O(1) for lookup/insert/erase, because with a good hash function elements spread evenly across buckets — but **worst-case is O(n)** (a pathological hash function or adversarial input could dump everything into one bucket).

📌 **In quant/low-latency code specifically:** the O(1)-average of `unordered_map` usually wins unless you specifically need sorted iteration (e.g. walking an order book by price) or need to guard against worst-case blowup (e.g. hostile/adversarial input where hash collisions could be weaponized) — `map`'s O(log n) worst-case guarantee is what you reach for there instead.

### `mp[key]` silently inserts
```cpp
cout << mp[10];   // if 10 doesn't already exist, this INSERTS it with a default value (0, "", etc.) first!
```
This is a very common accidental-mutation bug — checking a key's presence with `operator[]` actually *creates* the key. Use instead:
```cpp
mp.contains(10);        // C++20 — just a bool, no side effects
mp.find(10) != mp.end(); // pre-C++20 equivalent — returns an iterator, or mp.end() if absent
```

### Custom ordering / hashing
```cpp
map<int, string, greater<int>> mp;  // sorted DESCENDING instead of the default ascending
```
Both containers need to know how to compare/hash a custom key type:
- `map` needs an **ordering relation** — "is object A less than object B?" (either `operator<` on the type, or a custom comparator passed as the 3rd template argument).
- `unordered_map` needs both a **hash function** (how to turn the key into a bucket index) *and* an **equality comparison** (to resolve collisions within a bucket) — both must be supplied for a custom key type, since there's no automatic hash for arbitrary classes.

### Buckets, load factor, rehashing (unordered_map internals)
```cpp
mp.bucket_count();   // total number of buckets right now
mp.bucket(key);       // which bucket a given key currently lands in
mp.load_factor();     // elements / buckets — the average chain length per bucket
mp.reserve(n);         // pre-allocate enough buckets to comfortably hold n elements
```
When the load factor climbs too high (too many elements crammed into too few buckets, meaning long chains and degraded O(1)→O(n)-ish behavior), the table **rehashes**: it allocates a larger bucket array and reinserts every element into its new bucket. This one operation is O(n) and can momentarily spike a program's latency — but it happens rarely enough (buckets typically grow geometrically, same idea as vector's doubling) that normal operations stay efficient on average. `reserve()` up front is the standard way to dodge mid-run rehash spikes when you know your final size, exactly like `vector::reserve()`.

---

## 5. `std::set` / `std::unordered_set`

Key-only versions of `map`/`unordered_map` — same tree-vs-hash-table split, same complexity characteristics, just storing keys with no associated value.

```cpp
auto [it, inserted] = s.insert(10);   // `inserted` is true if 10 was newly added, false if it was already present
```

### `multiset` — duplicates allowed
```cpp
ms.erase(10);          // removes ALL elements equal to 10
ms.erase(ms.find(10)); // removes only ONE specific element (the one that iterator points to)
```
📌 The distinction matters: `erase(key)` is a *value-based, bulk* removal; `erase(iterator)` is a *position-based, single-element* removal. Same asymmetry exists for `multimap`.

---

## 6. `std::pair`, `std::tuple`, structured bindings

These group multiple values together **without writing a custom struct/class** — useful for quick, throwaway groupings; a named `struct` is still better once the values need a real meaning attached (see below).

```cpp
pair<int,string> p = {10, "sudhanshu"};
p.first;    // 10
p.second;   // "sudhanshu"
```
Every element of a `map<K,V>` is actually a `pair<const K, V>` — that `const` on the key exists because the map's internal ordering depends on the key never changing after insertion; mutating it in place would corrupt the tree/hash structure.

`tuple` generalizes `pair` to any fixed number of values:
```cpp
tuple<int, string, double> t = {10, "sudhanshu", 14.56};
get<0>(t);   // 10 — index must be a COMPILE-TIME constant
get<1>(t);   // "sudhanshu"
get<2>(t);   // 14.56
```
`get<>` also works on `pair` (`get<0>(p)` is equivalent to `p.first`) — `pair` is really just a `tuple` specialized for exactly two elements.

⚠️ **Readability trade-off:** `get<0>(t)`, `get<1>(t)`, `get<2>(t)` tell you *nothing* about what those fields mean — you have to remember the order. A named `struct` documents itself:
```cpp
struct Employee { string name; int age; int salary; };   // self-documenting
tuple<string, int, int> employee;                          // ...vs. this, where field meaning is implicit
```
Rule of thumb: `tuple`/`pair` for quick, local, throwaway grouping (especially return values); a real `struct` once the grouping has meaning that will be read/maintained later.

### Structured bindings — unpacking without `get<>`/`first`/`second`
```cpp
pair<int,string> p = {23, "sudhanshu"};
auto [age, name] = p;              // age=23, name="sudhanshu" — reads like real variable names

auto [a, b, c] = someTupleOrStruct; // works on tuples, structs, arrays too

for (const auto& [key, value] : mp) {   // avoids a copy of each pair — use `const auto&` when just reading
    cout << key << " : " << value << endl;
}
```
Structured bindings aren't limited to `pair`/`map` — they work on `tuple`, `array`, and any plain struct/class with public members, which is why they read so much more clearly than chained `get<N>()` calls once more than 2 fields are involved.

---

## 7. `std::array` (fixed-size, stack-allocated)

```cpp
array<int, 5> arr = {1,2,3,4,5};
```
A stack-based, fixed-size, C-style array wrapped in a safer, STL-friendly interface. **Size is part of the type** and fixed at compile time — `array<int,5>` and `array<int,10>` are genuinely different, unrelated types (same relationship as `Box<int>` vs `Box<string>` from Level 6).

Supports the familiar STL surface: `.size()`, `.empty()`, `.front()`, `.back()`, `.fill(val)`, `.at(i)`, plus algorithms like `sort`/`reverse` work directly on it.

### `array` is not *always* stack-allocated
```cpp
auto p = new array<int,100>;         // HEAP allocated — you can heap-allocate a C-style array too, same idea
static array<int, 100> arr;           // STATIC storage duration — lives until the program ends
array<int, 100> arr;                  // the common case: STACK (automatic storage)
```
"Stack-allocated" describes the *default/common* usage, not a guarantee baked into the type — `array` just avoids *dynamic (heap) allocation by default*, unlike `vector`, which always heap-allocates its buffer regardless of where the `vector` object itself lives.

### `.at()` vs `operator[]`
```cpp
arr[2];      // no bounds checking — out-of-range is UB
arr.at(2);   // bounds-checked — throws std::out_of_range on a bad index
```

### Partial initialization and `.fill()`
```cpp
array<int, 5> a = {1, 2};   // remaining 3 elements are value-initialized to 0
array<int, 5> a{};          // ALL elements are 0

array<int, 5> arr;
arr.fill(10);                // sets every element to 10
```

### Passing to functions — size is part of the type
```cpp
void print(const array<int, 5>& arr) { ... }   // ONLY accepts array<int,5> — array<int,10> is a different type entirely, won't compile
```
Fix with a template, exactly the pattern from Level 6:
```cpp
template<size_t N>
void print(const array<int, N>& arr) {   // now works for array<int,5>, array<int,10>, any N
    for (int x : arr) cout << x << ' ';
}
```

### The real payoff: `constexpr`-usable size
```cpp
constexpr int n = 100;
array<int, n> a1;
static_assert(a1.size() == 100);          // ✅ compiles — array's size() IS a compile-time constant

vector<int> vec(n);
// static_assert(vec.size() == 100);       // ❌ does NOT compile — vec.size() is a runtime function call,
                                            //    even though n itself was constexpr!
```
This is the actual point of the size-known-at-compile-time claim: it's not about whether you can *construct* the container with a constexpr count (both can — `vector<int> vec(n)` works fine at runtime with `n` as an ordinary value). It's about whether the **size is baked into the type itself**, usable anywhere the compiler needs a compile-time constant — another template's non-type parameter, a `static_assert`, an array bound. `array<int, N>`'s `N` qualifies; `vector`'s runtime-tracked `size()` never does, no matter how the vector was constructed.

---

## 8. `std::stack` / `std::queue` / `std::priority_queue` (container adapters)

These are **container adapters**, not standalone data structures — each wraps an *existing* container and exposes only a restricted subset of its interface, enforcing a specific access pattern.

| Adapter | Default underlying container | Interface exposed |
|---|---|---|
| `stack` | `deque` | `push`, `pop`, `top` (LIFO) |
| `queue` | `deque` | `push`, `pop`, `front`, `back` (FIFO) |
| `priority_queue` | `vector` | `push`, `pop`, `top` (heap order) |

You can swap the underlying container:
```cpp
stack<int, vector<int>> s;
stack<int, deque<int>> s;
stack<int, list<int>> s;
```

### Why bother with the adapter at all, instead of just using `deque` directly?
Because `deque` (or `vector`) exposes *everything* — including operations like `dq[10]` (random access) that break the intended discipline of "only touch the ends." The adapter's whole value is **enforcing the intended access pattern at the type level** — a `stack<int>` simply has no `operator[]` to misuse in the first place, which prevents a whole class of bugs by construction rather than by convention.

⚠️ Avoid `vector` as the underlying container for `queue` — `pop_front`-style removal from the front of a vector is O(n) (everything shifts left), defeating the point of a FIFO queue; `deque`'s O(1) front operations are why it's the default.

### `priority_queue` — it's a heap
```cpp
priority_queue<int, vector<int>, greater<int>> pq;   // MIN-heap (smallest on top)
```
```
parent(i) = (i-1)/2
left(i)   = 2*i + 1
right(i)  = 2*i + 2
```
None of these three adapters support random access — `stack`, `queue`, and `priority_queue` deliberately give you no way to peek at "the 3rd element from the top," and that's intentional: it's exactly what keeps the abstraction honest.

---

## 9. Iterator invalidation rules per container

```cpp
vector<int> v = {10, 20, 30};
auto it = v.begin();
int& ref = v[0];
int* ptr = &v[0];
v.push_back(100);   // MAY reallocate -> it, ref, AND ptr may ALL now be invalid
```

### The full picture, per container

| Container | Insertion invalidates... | Erasure invalidates... |
|---|---|---|
| `vector` | **ALL** iterators/pointers/references *if capacity is exceeded* (reallocation); otherwise only iterators from the insertion point onward | iterators/pointers/refs to the erased element **and everything after it** (elements shift left) |
| `deque` | all iterators (front/back push may or may not reallocate a chunk); references/pointers to existing elements usually stay valid | iterators/refs to erased element and everything "between" the erase point and whichever end got closer during the shuffle |
| `list` / `forward_list` | **nothing** is invalidated — existing iterators/pointers/refs stay valid | only the iterator/pointer/ref to the **erased element itself** — everything else stays valid |
| `map` / `set` (and multi- versions) | **nothing** is invalidated by insertion | only the iterator to the **erased element itself** — everything else stays valid |
| `unordered_map` / `unordered_set` | insertion may invalidate **all iterators** if it triggers a rehash (references/pointers to elements usually survive a rehash though) | only the iterator to the erased element |

📌 **The pattern to memorize:** contiguous containers (`vector`, and to a lesser extent `deque`) invalidate broadly because elements physically move. Node-based containers (`list`, `map`, `set`) barely invalidate anything, because a node's memory address never changes once allocated — only the *links between* nodes change.

### The correct "erase while iterating" pattern
`erase()` returns an iterator to the element *after* the one removed — use that return value instead of a manual increment:
```cpp
for (auto it = v.begin(); it != v.end(); ) {
    if (condition(*it))
        it = v.erase(it);   // erase() hands you back a still-valid iterator
    else
        ++it;                // only advance manually when you DIDN'T erase
}
```
This pattern is identical in spirit for `map`/`set`/`list` — always reassign from `erase()`'s return value rather than incrementing an iterator you just invalidated.

---

## 10. Choosing the right container — full cheat sheet

### Complexity by container (average case; worst case noted where it differs)

| Container | Access by index | Search | Insert (end) | Insert (front) | Insert (middle) | Erase (middle) |
|---|---|---|---|---|---|---|
| `vector` | O(1) | O(n) | O(1) amortized | O(n) | O(n) | O(n) |
| `deque` | O(1) | O(n) | O(1) amortized | O(1) amortized | O(n) | O(n) |
| `list` | O(n) (no `[]`) | O(n) | O(1) | O(1) | **O(1)** *(given the iterator)* | **O(1)** *(given the iterator)* |
| `array` | O(1) | O(n) | fixed size — N/A | N/A | N/A | N/A |
| `map` / `set` | — | O(log n) | O(log n) | — | O(log n) | O(log n) |
| `unordered_map` / `unordered_set` | — | O(1) avg / O(n) worst | O(1) avg / O(n) worst | — | O(1) avg / O(n) worst | O(1) avg / O(n) worst |

### Why `list`'s O(1) middle-insert often *loses* to `vector`'s O(n) in real benchmarks
This is a genuinely counter-intuitive, important result: raw Big-O hides **cache locality**. `vector`'s elements sit in one contiguous block, so shifting elements during a middle-insert is a tight, sequential, cache-friendly memory scan — modern CPUs are extremely fast at that, even for a few thousand elements. `list`'s O(1) insert avoids shifting entirely, but *reaching* the insertion point means chasing pointers through scattered heap nodes, each potentially a cache miss (a `list` walk essentially never benefits from CPU prefetching the way sequential `vector` access does). For small-to-medium N, `vector`'s "worse" asymptotic complexity frequently runs faster in wall-clock time than `list`'s "better" one — this is exactly what Level 17 (cache locality / memory hierarchy) will formalize.

📌 **Practical rule:** default to `vector`. Only reach for `list` when you're doing heavy splicing/reordering (see Topic 3's `splice`) or need guaranteed iterator stability across arbitrary insert/erase — not merely because Big-O says O(1) beats O(n) on paper.

### Quick decision guide

| You need... | Reach for |
|---|---|
| Default/general-purpose sequence | `vector` |
| Fast push/pop at **both** ends | `deque` |
| Frequent insert/erase in the middle **by iterator**, or splicing | `list` |
| Fixed-size, no heap allocation, compile-time-known size | `array` |
| Sorted key→value, worst-case guarantees, ordered iteration | `map` |
| Fastest average key→value lookup, ordering doesn't matter | `unordered_map` |
| Set semantics (uniqueness), sorted | `set` |
| Set semantics, fastest average lookup, ordering doesn't matter | `unordered_set` |
| LIFO discipline enforced at the type level | `stack` |
| FIFO discipline enforced at the type level | `queue` |
| Always-know-the-max/min-element discipline | `priority_queue` |