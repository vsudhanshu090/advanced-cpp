# 🧩 LEVEL 8 — STL Algorithms & Iterators

> **Big picture:** algorithms don't know about containers. Containers don't know about algorithms. **Iterators are the glue.** Every algorithm is written once, generically, in terms of iterator operations (`++`, `*`, `==`, sometimes `+n`). This is why `std::sort` can work on a `vector` and a `deque` with zero duplicated code — but refuses to touch a `list`.

Keep that sentence in your head for this whole level. Almost every "why does X not compile / why is Y slow" question below traces back to it.

---

## 1. Iterator Categories

An **iterator** is an object that knows how to move to the "next" element and read/write "the current" element, without the algorithm needing to know *how* the container is laid out in memory.

**Category = a promise about which operations are legal.** Each category is a strict superset of the one before it (this answers your "are they intertwined?" question — yes, deliberately: a random-access iterator *is-a* bidirectional iterator *is-a* forward iterator *is-a* input/output iterator, in terms of capability, not C++ class inheritance. It's enforced via `iterator_category` tags, not `virtual`).

| Category | New capability added | Containers |
|---|---|---|
| **Input** | `*it` (read once), `++it` | `istream_iterator` |
| **Output** | `*it = val` (write once), `++it` | `ostream_iterator`, `back_insert_iterator` |
| **Forward** | can re-read the same element multiple times (multi-pass) | `forward_list` |
| **Bidirectional** | adds `--it` | `list`, `set`, `map`, `multiset`, `multimap` |
| **Random Access** | adds `it+n`, `it-n`, `it[n]`, `<`, `<=` — **O(1) jumps** | `vector`, `deque`, `array` |
| **Contiguous** *(C++20)* | guarantees elements are physically back-to-back in memory | `vector`, `array`, `string` |

### Input iterator, worked example
```cpp
#include <iterator>
#include <sstream>

std::istringstream ss("10 20 30");
std::istream_iterator<int> it(ss);
std::istream_iterator<int> end;   // default-constructed = "end of stream" sentinel

while (it != end) {
    std::cout << *it << " ";  // reads "10", then "20", then "30"
    ++it;                      // ++ actually pulls the NEXT token from the stream
}
```
Each `++it` performs a real read from the stream. That's the key mental shift from a `vector` iterator: dereferencing/advancing has a **side effect** (consumes input), and you can only go forward, once, per element — hence "input iterator", single-pass.

### Output iterator, worked example
```cpp
std::vector<int> v = {1, 2, 3};
std::ostream_iterator<int> out(std::cout, ", ");
std::copy(v.begin(), v.end(), out);   // prints: 1, 2, 3,
```
`*it = value` and `it++` look identical to an input iterator syntactically, but semantically it's write-only, single pass — you cannot read back what you wrote, and dereferencing twice without advancing is undefined behavior for a "pure" output iterator (in practice `ostream_iterator` doesn't crash, but don't rely on that).

### `const_iterator` — what it's actually for
```cpp
void printAll(const std::vector<int>& v) {
    for (auto it = v.begin(); it != v.end(); ++it) {
        // *it = 5;   // ❌ won't compile — *it is a const int&
        std::cout << *it << " ";
    }
}
```
Use case: any function that takes a container **by `const&`** automatically gets `const_iterator` from `.begin()`/`.end()` — this is the compiler *enforcing* that a read-only function can't secretly mutate the data. You can also request it explicitly with `.cbegin()`/`.cend()` even on a non-const container, to signal intent ("I promise not to modify this range").

### Why the category matters: complexity
```cpp
auto it = container.begin();
std::advance(it, 100000);
```
- `vector`/`deque` (random access): `it + 100000` directly → **O(1)**
- `list`/`map`/`set` (bidirectional): must do `++it` 100,000 times → **O(n)**

Same line of code, wildly different cost, depending only on the iterator category — this is *the* practical reason to care about this taxonomy.

### C++20 contiguous iterators — what actually changes
Before C++20, "contiguous in memory" was true for `vector`/`array` in practice, but there was no way to *prove* it to generic code at compile time. C++20 added a real category for it, which unlocks:
- `std::to_address(it)` — safely get a raw pointer from the iterator, even through fancy iterator wrappers.
- Algorithms (and library-internal code) can detect "this range is one contiguous memory block" and drop down to `memcpy`/SIMD-friendly loops instead of an element-by-element `++`/`*` loop.
- Cleaner interop with C APIs that want a raw `T*`.

You won't write code that depends on this often at your level, but recognize the term.

---

## 2. `std::sort`, `std::stable_sort`, Custom Comparators

- `std::sort` needs **random-access** iterators → works on `vector`, `deque`, `array` only. `list` has no `+n`, so `std::sort(list.begin(), list.end())` **fails to compile**, not fails at runtime.
- `list::sort()` is a **member function** instead — it's implemented as an in-place merge sort adapted to the linked-node structure (splicing nodes rather than needing random access), O(n log n), and it **is stable**.

| | Worst case | Extra memory | Stable? |
|---|---|---|---|
| `std::sort` | O(n log n) | O(log n) | ❌ No |
| `std::stable_sort` | O(n log n) *(may degrade to O(n log²n) if it can't allocate a buffer)* | up to O(n) | ✅ Yes |

### Comparator = "does a come before b?"
```cpp
sort(vec.begin(), vec.end(), [](const Person& a, const Person& b) {
    if (a.age != b.age) return a.age < b.age;
    return a.name < b.name;
});
```
> ⚠️ **Strict weak ordering — the #1 footgun.** Never write `a <= b`. The comparator must return `false` for `comp(a, a)`. If you use `<=`, then `comp(a, a)` is `true`, which breaks sort's internal invariants → **undefined behavior** (can manifest as a crash, an infinite loop, or silently wrong output, depending on STL implementation). Always use strict `<`.

Ascending = `std::less<int>{}` (the default). Descending = `std::greater<int>{}`, or since C++14 the transparent form `std::greater<>{}` (lets the compiler deduce the type, useful when comparing e.g. `int` against `long`).

### Why your Q2 confusion happened (important!)
`std::pair`'s **default** `operator<` compares `.first`, and only if those are equal, falls back to compare `.second`. Your `vector<pair<int,int>>` had `.first == 1` for every element but a **unique `.second`** (the original index). So the default comparison was *already* fully deterministic — it wasn't "unstable sort happening to look sorted", it was **not actually a tie at all** once you compare both fields. That's why `sort` and `stable_sort` produced identical output: there was nothing ambiguous for `sort` to reorder.

Your `q2Fix()` then used a comparator that looks at `.first` **only** — now there genuinely *are* ties (all 12 elements). Yet both `sort` and `stable_sort` still printed the same order. Why? Because **"not stable" doesn't mean "always shuffles equal elements"** — it means *no guarantee*. Real `sort` implementations (introsort) fall back to **insertion sort for small ranges** (typically under ~16 elements), and insertion sort *happens to be* stable. Your test vector had 12 elements — small enough to hit that fallback. Bump it to a few hundred random-ish elements with real duplicate keys and you'll see `sort` actually reorder equal elements while `stable_sort` doesn't. **Never rely on this** — "usually stable in practice for small n" is not a guarantee your code should depend on.

### `introsort`, briefly
`std::sort`'s typical real implementation is a hybrid:
1. Start with **quicksort** (fast average case).
2. If recursion depth exceeds roughly `2 × log₂(n)` (a sign of quicksort hitting its worst case, e.g. already-sorted or adversarial input), **switch to heapsort** for that partition — guarantees O(n log n) worst case instead of quicksort's O(n²).
3. Once a partition shrinks below a small threshold (commonly 16 elements), switch to **insertion sort**, which has lower constant-factor overhead than quicksort/heapsort for tiny ranges.

That's the "why" behind the small-n stability behavior above.

---

## 3. `std::find`, `find_if`, `count`, `count_if`

```cpp
auto it = find(v.begin(), v.end(), 20);          // first element == 20, or v.end()
auto it = find_if(v.begin(), v.end(), pred);      // first element where pred(x) is true
int n  = count(v.begin(), v.end(), 2);            // how many == 2
int n  = count_if(v.begin(), v.end(), pred);      // how many satisfy pred
```
`find_if_not` = same as `find_if` but looks for the first element where the predicate is **false**.

A **predicate** is just a callable returning `bool`, answering a yes/no question about one element:
```cpp
auto isEven = [](int x) { return x % 2 == 0; };
```

> ⚠️ **These are O(n) — linear scans — even on `set`/`unordered_set`.** If you already have a `set<int> s;`, don't write `find(s.begin(), s.end(), x)`. Use the container's **member** `.find()` instead:
> - `set`/`map`: `s.find(x)` → **O(log n)**, uses the tree structure.
> - `unordered_set`/`unordered_map`: `us.find(x)` → **O(1)** average, uses the hash table.
>
> The free-function `std::find` is generic and container-agnostic, so it can't know to use the tree/hash — it just walks element by element.

C++20 ranges shorthand (skips writing `.begin()/.end()` every time):
```cpp
std::ranges::find(v, 10);
std::ranges::count(v, 2);
```

---

## 4. `std::transform`, `std::accumulate`, `std::reduce`

Think of these three as **map / fold-left / fold-any-order**:

| | Role | Order guarantee | Notes |
|---|---|---|---|
| `transform` | apply a function to each element, write result to (possibly different) output range | preserves input order | like `map()` in Python |
| `accumulate` | combine elements **left-to-right** into one result | strictly sequential | like `reduce()`/`fold` in other languages |
| `reduce` | combine elements, order **unspecified** | not guaranteed sequential | can run in parallel — needs the combining op to be associative *and* commutative |

```cpp
// transform: USD -> cents
vector<double> usd = {2.43, 3.55};
vector<double> cents;
transform(usd.begin(), usd.end(), back_inserter(cents),
          [](double d) { return d * 100; });

// accumulate: strict left fold, order matters (e.g. for floating point!)
double total = accumulate(cents.begin(), cents.end(), 0.0);

// reduce: same idea, but allowed to reorder/parallelize
double total2 = reduce(std::execution::par, cents.begin(), cents.end(), 0.0);
```
**Why order matters for floats:** floating-point addition is *not* associative — `(a+b)+c` can differ in the last bits from `a+(b+c)`. `accumulate` gives you a reproducible result; `reduce` trades that determinism for potential speed via parallel execution policies (`std::execution::par`, `par_unseq`). Only reach for `reduce` when (a) you actually need the parallelism and (b) tiny floating-point drift is acceptable.

### Your Q4 floating-point question, answered
`3.542 * 100` doesn't cleanly become `354.2` because most decimal fractions (`0.1`, `0.542`, ...) **can't be represented exactly in binary floating point** — same reason `0.1 + 0.2 != 0.3` in almost every language. For money specifically, the standard fix is: **never store currency as `double`** — work in integer minor units (cents) from the start (parse the string `"3.542"` directly into cents, don't go through a `double` multiply), or use a decimal/fixed-point type. If you must convert an existing `double`, round explicitly: `long long c = std::llround(d * 100);` — `llround` rounds to nearest instead of silently truncating.

---

## 5. `std::copy`, `copy_if`, `remove`/`remove_if` + the erase-remove idiom

```cpp
vector<int> dest(src.size());          // ✅ pre-sized — copy writes INTO existing slots
copy(src.begin(), src.end(), dest.begin());

vector<int> dest2;                     // size 0 — dest2.begin() has nowhere valid to write
copy(src.begin(), src.end(), back_inserter(dest2));   // ✅ back_inserter grows it for you
```

**What `back_inserter` actually does under the hood:** it returns a `back_insert_iterator` — a tiny wrapper holding a reference to the container. Its `operator*()` and `operator++()` are essentially no-ops (just `return *this;`), and all the real work happens in `operator=`:
```cpp
// simplified concept
struct back_insert_iterator {
    Container& c;
    auto& operator*() { return *this; }
    auto& operator++() { return *this; }
    auto& operator=(const T& value) { c.push_back(value); return *this; }
};
```
So `*dest_it = x;` inside `std::copy`'s generic loop literally compiles down to `container.push_back(x);`. This is a great example of iterators being a *uniform interface* — the algorithm has no idea it's not writing into pre-existing memory.

`copy_if` = `copy` + a predicate:
```cpp
copy_if(v.begin(), v.end(), back_inserter(even), [](int x){ return x % 2 == 0; });
```

### The erase-remove idiom
```cpp
vector<int> v = {1, 2, 3, 2, 4, 2, 5};
auto new_end = remove(v.begin(), v.end(), 2);   // v.size() is STILL 7 here!
v.erase(new_end, v.end());                       // NOW it actually shrinks
```
**Why `remove` alone doesn't shrink the container:** algorithms only ever hold **iterators**, never a reference to the container itself — they have no idea whether they're operating on a `vector`, `deque`, or a raw array, and each of those has a completely different (or no) concept of "resize". So `remove` can't call `.erase()` even if it wanted to. What it *can* do is rearrange the range in-place: it walks through, copies every "kept" element progressively to the front, and returns an iterator to the new logical end.

**Your question — is the tail garbage, or leftover values?** Neither "uninitialized garbage" nor guaranteed-meaningful: it's specified as **valid but unspecified**. In practice, a typical implementation overwrites kept-elements-over-removed-ones via move-assignment, so the tail usually contains *duplicates of surviving values* (not garbage memory) — but the standard makes zero promises about what's there, so treat it as unreadable. That's exactly why `.erase(new_end, end())` is mandatory, not optional cleanup.

Shorthand forms (C++20):
```cpp
v.erase(remove(v.begin(), v.end(), 2), v.end());   // classic
erase(v, 2);                                        // C++20 free function, same effect
erase_if(v, [](int x){ return x % 2 == 0; });        // conditional version
```

---

## 6. `std::for_each`

"Do this to every element."
```cpp
for_each(v.begin(), v.end(), [](int& x) { x *= 2; });   // note: & to actually mutate
```
Range-`for` (`for (int x : v) x *= 2;` — needs `int&` there too) is usually **more idiomatic** for simple cases: fewer characters, no need to think about iterator ranges, and it's what most C++ codebases default to. Reach for `for_each` mainly when you're already deep in iterator-based/generic code, or passing an existing named function object. And **always prefer the named algorithm that states your actual intent**: summing → `accumulate`, transforming → `transform`, "just do something to each" → `for_each`. This is about readability, not performance — a reviewer scanning `accumulate(...)` instantly knows "this produces a sum" without reading the lambda body.

### Your Q11 capture question, answered
```cpp
int sum = 0;
for_each(v.begin(), v.end(), [sum](int x) { sum += x; });   // ❌ two separate problems
```
1. `[sum]` captures **by value** — the lambda gets its own private *copy* of `sum` at the moment the lambda object is created. Mutating that copy never touches the outer `sum`.
2. Even ignoring that: a lambda's `operator()` is `const` by default, so you can't reassign a captured-by-value variable at all *unless* you mark the lambda `mutable` — and even then you'd only be mutating the private copy, still not the outer one.

`[&sum]` captures **by reference** — the lambda holds a reference to the actual outer variable, so `sum += x` mutates the real thing. That's why only `&sum` "worked."

---

## 7. `std::unique`

Removes **adjacent** duplicates only — it does not know or care about duplicates elsewhere in the range.
```cpp
vector<int> v = {1, 1, 2, 2, 2, 3, 4, 4};
v.erase(unique(v.begin(), v.end()), v.end());   // {1, 2, 3, 4}
```
That's why it's almost always paired with `sort` first — sorting moves every duplicate to be adjacent, turning "remove all duplicates" into "remove adjacent duplicates":
```cpp
vector<int> v = {4, 2, 1, 2, 4, 3, 1};
sort(v.begin(), v.end());
v.erase(unique(v.begin(), v.end()), v.end());
```
Custom "equal enough" predicate (e.g. collapse near-duplicates within 1 of each other):
```cpp
auto it = unique(v.begin(), v.end(), [](int a, int b){ return abs(a - b) <= 1; });
```

---

## 8. `std::binary_search`, `lower_bound`, `upper_bound`

**Require a sorted range.** All run in **O(log n)**.
```cpp
bool found   = binary_search(v.begin(), v.end(), 5);   // just yes/no
auto lb = lower_bound(v.begin(), v.end(), 2);           // first element >= 2
auto ub = upper_bound(v.begin(), v.end(), 2);           // first element >  2
```
`[lower_bound, upper_bound)` is exactly the sub-range of elements equal to your target, so:
```cpp
int countOfX = upper_bound(v.begin(), v.end(), x) - lower_bound(v.begin(), v.end(), x);
```
Directly relevant to quant/order-book code: given sorted price levels, `lower_bound(target)` finds the first level `>= target` in O(log n) — the standard building block for "find the best price level at or above this one."

---

## 9. `std::min_element` / `std::max_element`

Scan a range, return an **iterator** to the min/max (not the value — dereference with `*it`, and check against `.end()` first if the range could be empty).
```cpp
auto minIt = min_element(v.begin(), v.end());
auto maxIt = max_element(v.begin(), v.end());
auto [mn, mx] = minmax_element(v.begin(), v.end());   // both in one pass — prefer this when you need both
```

> ⚠️ **Both use the *same* comparator semantics as `sort`** — `comp(a, b)` means "is a less than b?". Pass the **identical** `<`-style comparator to both `min_element` and `max_element`; don't flip the direction for "max". This is a very natural trap (Level 8's own Q9 fell into it — see the solution sheet).

### Your range question, answered
STL ranges are conventionally **half-open**: `[first, last)` means `last` is one-past-the-end and is **never touched**. If you want an algorithm to include the element `it2` points at, you pass `it2 + 1` as the "last" argument, not `it2` itself. This convention is *everywhere* — `sort`, `accumulate`, `find`, all of them — so it's worth internalizing once rather than re-deriving per algorithm.

### "Are `.begin()` etc. just pointers?"
Sometimes literally yes, sometimes no:
- `vector`, `array` (and often `deque`'s buffer-local case): the iterator is either literally a raw `T*`, or a paper-thin wrapper class around one with overloaded operators (implementation detail, but conceptually identical to a pointer).
- `list`, `map`, `set`: the iterator is a real class wrapping a **node pointer**, with `operator++` following the node's `next` link (or doing an in-order tree-walk step for `map`/`set`) — meaningfully more machinery than "just a pointer," which is exactly why it can't do `it + n` in O(1).

---

## 10. Const Iterators & Reverse Iterators

`const_iterator`: see §1 above — controls **mutability**, not direction.

`reverse_iterator`: controls **direction**. Walks backward, but you still write `++it` to advance (advancing = "moving toward the front" when reversed):
```cpp
for (auto it = v.rbegin(); it != v.rend(); ++it) {
    std::cout << *it << ' ';       // last element first
}
```
- `rbegin()` conceptually points at the **last** element.
- `rend()` conceptually points **one before the first** element.
- `crbegin()`/`crend()` = const + reverse combined.

### The `.base()` off-by-one — your Q12 confusion, resolved
A `reverse_iterator` is internally *implemented as a wrapper around a normal forward iterator* — call it its "base." The trick: `reverse_iterator(base).operator*()` doesn't dereference `base` directly, it dereferences `*(base - 1)`. This is what makes `rbegin()`, defined as `reverse_iterator(end())`, correctly point at the **last real element** even though its stored `base` is `end()` (one past the last element, in forward terms).

Consequence: **`.base()` always points one step "ahead"** (in forward-iterator terms) of what the reverse_iterator was actually dereferencing.
```
container:      [ a ][ b ][ c ]
                 ^begin           end^
rbegin() dereferences 'c', but rbegin().base() == end()   (points AFTER c, not at c)
```
So converting a reverse_iterator back to a normal one that points **at the same element**:
```cpp
auto rit = find(v.rbegin(), v.rend(), 5);   // rit dereferences the found '5'
auto it  = rit.base() - 1;                   // -1 needed to land on that same '5'
```

**Why your Q12 code "worked" without the `-1`/`if`:** by the time your loop finished, `it` had already become `v.rend()`. And `rend().base()` is defined to equal `v.begin()` exactly — that's the matching boundary case, same idea as `rbegin().base() == v.end()`. So `forwardItr` landed on `begin()` and your `if (forwardItr != v.begin())` was `false` — never fired. That's not "the -1 rule doesn't apply here," it's that you'd hit the **end-of-range boundary case**, where base() happens to already equal what you wanted. The rule (`base() - 1` to match the same element) is still true for every *non-boundary* reverse iterator position — try it with `rit` pointing at a real found element (like the `find` example above) and you'll see the `-1` is required there.

### Converting a *forward* iterator into a reverse one (your other question)
```cpp
std::vector<int>::iterator it = v.begin() + 2;
std::reverse_iterator<std::vector<int>::iterator> rit(it);
// or, cleaner:
auto rit2 = std::make_reverse_iterator(it);
```
Same `base() - 1` relationship applies in reverse: the resulting `rit` will dereference the element **before** `it`, not `*it` itself — worth testing on a small example if it doesn't click immediately.

---

## 📋 Reference Tables

### Algorithms
| Algorithm | Requires sorted? | Typical complexity | Example |
|---|---|---|---|
| `sort` | no | O(n log n) | `sort(v.begin(), v.end())` |
| `stable_sort` | no | O(n log n)* | `stable_sort(v.begin(), v.end(), cmp)` |
| `find` | no | O(n) | `find(v.begin(), v.end(), 5)` |
| `find_if` | no | O(n) | `find_if(v.begin(), v.end(), pred)` |
| `count` / `count_if` | no | O(n) | `count_if(v.begin(), v.end(), pred)` |
| `transform` | no | O(n) | `transform(v.begin(), v.end(), out, fn)` |
| `accumulate` | no | O(n), sequential | `accumulate(v.begin(), v.end(), 0)` |
| `reduce` | no | O(n), any order | `reduce(v.begin(), v.end(), 0)` |
| `copy` / `copy_if` | no | O(n) | `copy(v.begin(), v.end(), out)` |
| `remove` / `remove_if` | no | O(n) | `remove(v.begin(), v.end(), x)` |
| `unique` | **yes**, for full dedup | O(n) | `unique(v.begin(), v.end())` |
| `binary_search` | **yes** | O(log n) | `binary_search(v.begin(), v.end(), x)` |
| `lower_bound` / `upper_bound` | **yes** | O(log n) | `lower_bound(v.begin(), v.end(), x)` |
| `min_element` / `max_element` | no | O(n) | `max_element(v.begin(), v.end())` |

### Iterators
| Iterator | Category | Direction | Notes |
|---|---|---|---|
| `vector<T>::iterator` | random access | forward | often literally `T*` |
| `list<T>::iterator` | bidirectional | forward | no `+n` |
| `set/map::iterator` | bidirectional | forward | wraps a tree node |
| `vector<T>::reverse_iterator` | random access | **backward** | `.base()` is off by one |
| `vector<T>::const_iterator` | random access | forward | read-only |
| `istream_iterator<T>` | input | forward, single-pass | reads from stream |
| `ostream_iterator<T>` | output | forward, single-pass | writes to stream |
| `back_insert_iterator` | output | forward | `operator=` calls `push_back` |