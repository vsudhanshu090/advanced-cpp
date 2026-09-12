# advanced-cpp Master Generation Prompt (self-contained, paste-anywhere)

Copy this ENTIRE file into a fresh Claude chat with code execution
available. It needs nothing else — no prior conversation, no other
file — everything required is below, including exactly what's been
covered so far. Send it as-is; edit only the "What I need this
iteration" line near the bottom if you want to override the default
(generate the next not-yet-covered topic in strict order).

When you get a response back, it will include an UPDATED copy of this
entire file (tracker checkboxes updated) — replace this file with that
one, so next time's paste is accurate.

---

## What this is

A generator for a local C++ practice judge platform (`advanced-cpp`).
Problems live at `levels/<difficulty>/<name>/{problem.md,solution.h,tests.cpp}`
and are graded by compiling `tests.cpp` (which includes `solution.h`)
against a hidden doctest suite. This file is the single source of truth
for: the full syllabus, what's covered so far, how problems must be
written, and what to generate next.

## Repo conventions

```
advanced-cpp/
├── framework/test_common.h   <- tests MUST include this, never doctest.h directly
└── levels/
    └── <difficulty>/<problem-name>/
        ├── problem.md
        ├── solution.h
        └── tests.cpp
```

`problem.md` frontmatter (exact schema):
```
---
title: <a few words>
topics: [Topic One, Topic Two]
difficulty: basic | easy | easy-medium | medium | hard | expert
type: implement | from-scratch | debug | refactor | performance
---
```

## THE critical pedagogical rule

**The specific mechanism/syntax being taught by a problem must NEVER be
pre-declared in `solution.h` — not just an empty body, the ENTIRE
declaration (return type, name, parameter list, qualifiers) must be
absent.** The learner must produce correct C++ syntax from memory, not
just fill logic into a shape you already gave them.

Example — a lesson on operator overloading:
- **Wrong** (still spoon-feeds the syntax):
  ```cpp
  bool operator==(const T& other) const {
      // TODO
  }
  ```
- **Right**: `problem.md` says in prose "supports equality comparison
  against another `T`" and `solution.h` simply doesn't mention
  `operator==` anywhere. The learner has to know to write
  `bool operator==(const T& other) const { ... }` themselves — the
  return type, the `const`, the reference parameter, all of it.

Supporting/incidental structure NOT central to the current lesson (e.g.
a constructor from an already-covered earlier level) may still be given,
so the learner isn't re-deriving things from three levels ago every
single time. Judgment call: if it's the thing THIS problem exists to
teach, it's omitted; if it's just scaffolding to reach that lesson, it's fine to include.

This rule applies at EVERY difficulty, including `basic` — "basic" means
one isolated concept, not "fully scaffolded."

## Testing safety rules (do not skip any of these)

1. Every `tests.cpp` includes the relative path to `framework/test_common.h`,
   never `doctest.h` directly (prevents a real Windows `windows.h`/GDI
   name collision with classes named e.g. `Rectangle`).
2. For `debug`/`refactor` problems: never destroy two objects that might
   be aliased due to a shallow-copy bug in the SAME test — use
   heap-allocated, deliberately-never-`delete`d objects to test
   independence via value comparison instead. This avoids a real
   double-free crashing the grader. At most ONE test near the end of the
   file may do the full realistic stack-allocated copy-then-destroy
   check, to verify the real fix end-to-end.
3. For `performance` problems: never guess a timing threshold. Write both
   a correct and a naive reference implementation, compile and time both
   empirically (`g++ -std=c++17 -O0 -g`, matching the judge), and pick N
   + budget with a clear (>5x) separation. Keep the naive case comfortably
   under 15 seconds (the judge's hard timeout) so it fails via a clean
   assertion, not a generic timeout.
4. Every problem needs a stub/starting `solution.h` that FAILS the tests,
   and you must verify (by compiling and running both) that a correct
   reference implementation you write yourself passes 100% before
   delivering anything. Report the pass/fail counts you actually observed.
5. For generic/templated problems, include at least one test using a
   move-only type (e.g. `std::unique_ptr<int>`) and, where relevant, a
   type with no default constructor, to catch overly-rigid solutions.
6. Judge compile flags already include `-pthread` — concurrency problems
   need nothing extra for that.

## Generation order: STRICT top-to-bottom

Work through the syllabus below in exact order — Level 1's topics in
listed order, then Level 2's, and so on. Find the first topic in the
Coverage Tracker that isn't checked off and generate for THAT one. Do
not skip ahead, reorder, or jump to a "more interesting" later topic.

## Difficulty is per-problem, not per-level

**Difficulty is NEVER derived from the level number.** A topic first
introduced at Level 2 can still have a genuinely `hard` variant if the
topic supports that much depth; a topic at Level 15 can have a legitimate
`basic`/`easy` first-exposure problem before ramping up. The level number
only controls WHEN a topic is introduced (prerequisite ordering) — it
says nothing about how difficult problems on that topic should be.

For the CURRENT topic, assign each generated problem's difficulty based
on how deep/complex THAT SPECIFIC problem is as an application of the
topic — judged independently every time, never inherited from the level
index. A rich topic (e.g. smart pointers, concurrency primitives) can
and should span basic all the way to hard within its own problem set,
built as a natural progression: start with a simple, isolated
first-exposure problem, then increase depth/complexity across further
problems until the topic's real range has been explored.

## How many problems per topic — no fixed count

Don't target a specific number. For the current topic, generate **as
many problems as are actually needed to build a thorough, lasting
understanding of it — decide this yourself, per topic, based on how much
depth the topic genuinely has**. A narrow, simple topic (e.g. "arrays vs
C-strings vs std::string basics") might only need 2 problems. A topic
with real range (e.g. smart pointers, or move semantics) might
legitimately need 6–8+ to actually cover it — first exposure, then
progressively deeper/harder applications, then at least one that
combines it with 1–2 previously-covered topics for reinforcement (this
is what makes it stick instead of fading after one exposure). Judge each
topic on its own merits rather than aiming for a round number.

Only check off a numbered topic in the tracker once you judge it's been
covered thoroughly enough by the problem(s) generated for it. A level
with 10 numbered topics will likely take several iterations to fully
clear — that's expected. Continue from wherever the tracker leaves off.

## Output format

1. A single bash script (works in Git Bash on Windows) using heredocs
   that creates each new problem folder and writes all three files
   directly. NOT a git patch — a fresh session's file contents may not
   match the real repo's baseline, and a diff can fail to apply for
   reasons unrelated to the new content; a script that writes files
   directly has no such failure mode.
2. This ENTIRE file, returned in full, with the Coverage Tracker section
   updated (newly-covered topics checked off). Paste that back in next
   time, replacing this file.

---

## Syllabus & Coverage Tracker

Full topic list (verbatim, from the user's own 20-level C++ sheet) with a
checkbox per topic. Update the checkboxes as topics are covered — this
IS the tracker, there's no separate list elsewhere in this file.

**Note:** 9 supplementary problems already exist in `levels/` from an
earlier, freeform-topic phase of this platform (Rectangle, Circle,
BankAccount, Vector2D, a DynamicArray debug problem, an LRU Cache, a
BlockingQueue, a ResourceHandle refactor, a RunningMedian performance
problem). They remain in the repo as bonus practice but are **not**
counted against this tracker, since they predate the syllabus below.
Start fresh at Level 1, Topic 1.

### Level 1: C++ Fundamentals Refresher — NOT STARTED
- [ ] 1. Compilation model (translation units, headers vs source)
- [ ] 2. Variables, types, type sizes, integer overflow, implicit conversions
- [ ] 3. Control flow (if/switch/loops) edge cases
- [ ] 4. Functions: pass-by-value / pass-by-reference / pass-by-pointer
- [ ] 5. Default arguments, function overloading, overload resolution
- [ ] 6. Arrays vs C-strings vs std::string basics
- [ ] 7. const correctness (basic level)
- [ ] 8. References vs pointers (first pass)
- [ ] 9. Scope, lifetime, storage duration (automatic/static/dynamic)
- [ ] 10. I/O: cin/cout, stringstream basics

### Level 2: Pointers Deep Dive — NOT STARTED
- [ ] 1. Pointer declaration/dereferencing, pointer to pointer
- [ ] 2. Pointer arithmetic and array-pointer duality
- [ ] 3. const with pointers (pointer to const, const pointer, both)
- [ ] 4. Null pointers, dangling pointers, wild pointers
- [ ] 5. Dynamic memory: new/delete, new[]/delete[]
- [ ] 6. Function pointers and typedef/using for function pointer types
- [ ] 7. void* and casting
- [ ] 8. Pointers to struct/class members (-> operator)
- [ ] 9. Common pointer bugs: double free, use-after-free, memory leaks

### Level 3: OOP Basics — NOT STARTED
- [ ] 1. Classes vs structs (default access), members, methods
- [ ] 2. Constructors (default, parameterized, delegating), destructors
- [ ] 3. Member initializer lists vs assignment in constructor body
- [ ] 4. `this` pointer
- [ ] 5. Static members and static methods (class-level state/behavior)
- [ ] 6. Access specifiers: public/private/protected
- [ ] 7. Const member functions, mutable
- [ ] 8. Friend functions/classes (brief)
- [ ] 9. In-class initialization of members

### Level 4: OOP Advanced — Inheritance & Polymorphism — NOT STARTED
- [ ] 1. Inheritance (public/protected/private), base/derived constructors
- [ ] 2. Virtual functions and dynamic dispatch, vtables (conceptual model)
- [ ] 3. Pure virtual functions and abstract classes/interfaces
- [ ] 4. Virtual destructors (why they matter)
- [ ] 5. Method overriding vs hiding, `override` and `final` keywords
- [ ] 6. Multiple inheritance, the diamond problem, virtual inheritance
- [ ] 7. Slicing (object slicing on copy)
- [ ] 8. Upcasting/downcasting, dynamic_cast, static_cast between classes
- [ ] 9. Abstract base class as interface pattern

### Level 5: Rule of 3 / Rule of 5 / Rule of 0 — NOT STARTED
- [ ] 1. Copy constructor (what triggers it, default shallow-copy behavior)
- [ ] 2. Copy assignment operator (self-assignment safety)
- [ ] 3. Destructor's role in managing owned resources
- [ ] 4. Move constructor / move assignment operator (mechanics only)
- [ ] 5. Rule of 3 (any one of copy ctor/copy assign/dtor -> need all three)
- [ ] 6. Rule of 5 (C++11 adds move ctor/move assign)
- [ ] 7. Rule of 0 (prefer members that manage their own resources)
- [ ] 8. Shallow copy vs deep copy, and the double-free bug shallow copy causes
- [ ] 9. `= default` and `= delete` for special member functions

### Level 6: Templates & Generic Programming — NOT STARTED
- [ ] 1. Function templates, template argument deduction
- [ ] 2. Class templates
- [ ] 3. Template specialization (full) and partial specialization
- [ ] 4. Non-type template parameters
- [ ] 5. Multiple template parameters, default template arguments
- [ ] 6. Templates + operator overloading interplay
- [ ] 7. `typename` vs `class` in template params (equivalent here)
- [ ] 8. Where template code lives (why templates are usually header-only)
- [ ] 9. Basic constraints via `static_assert`

### Level 7: STL Containers — NOT STARTED
- [ ] 1. std::vector — dynamic array, growth strategy, capacity vs size
- [ ] 2. std::deque — double-ended queue
- [ ] 3. std::list / std::forward_list — doubly/singly linked list
- [ ] 4. std::map / std::unordered_map — ordered vs hash-based key-value
- [ ] 5. std::set / std::unordered_set
- [ ] 6. std::pair, std::tuple, structured bindings
- [ ] 7. std::array (fixed-size, stack-allocated)
- [ ] 8. std::stack / std::queue / std::priority_queue (container adapters)
- [ ] 9. Iterator invalidation rules per container
- [ ] 10. Choosing the right container (complexity tradeoffs)

### Level 8: STL Algorithms & Iterators — NOT STARTED
- [ ] 1. Iterator categories (input, output, forward, bidirectional, random access)
- [ ] 2. std::sort, std::stable_sort, and custom comparators
- [ ] 3. std::find, std::find_if, std::count, std::count_if
- [ ] 4. std::transform, std::accumulate, std::reduce
- [ ] 5. std::copy, std::copy_if, std::remove/remove_if + erase-remove idiom
- [ ] 6. std::for_each
- [ ] 7. std::unique (and why it needs sorted input)
- [ ] 8. std::binary_search, std::lower_bound, std::upper_bound
- [ ] 9. std::min_element / std::max_element
- [ ] 10. Const iterators, reverse iterators

### Level 9: Exceptions & Error Handling — NOT STARTED
- [ ] 1. try/catch/throw mechanics, catching by reference
- [ ] 2. Standard exception hierarchy (std::exception, logic_error, runtime_error, out_of_range, invalid_argument, bad_alloc)
- [ ] 3. Writing custom exception classes (inheriting std::exception)
- [ ] 4. Exception safety guarantees: no-throw, strong, basic, no guarantee
- [ ] 5. RAII as the mechanism that makes exception safety possible
- [ ] 6. `noexcept` specifier and what happens if a noexcept function throws
- [ ] 7. Stack unwinding and what it does/doesn't clean up
- [ ] 8. Catching multiple exception types, catch(...), rethrow
- [ ] 9. Error codes vs exceptions: when each is preferable

### Level 10: Smart Pointers & RAII — NOT STARTED
- [ ] 1. RAII principle deep dive (resource lifetime == object lifetime)
- [ ] 2. std::unique_ptr — exclusive ownership, move-only
- [ ] 3. std::shared_ptr — shared ownership, reference counting
- [ ] 4. std::weak_ptr — non-owning observer, breaking cycles
- [ ] 5. make_unique / make_shared (why preferred over raw new)
- [ ] 6. Custom deleters
- [ ] 7. shared_ptr control block, use_count(), aliasing constructor (brief)
- [ ] 8. Passing smart pointers: by value vs by reference vs raw pointer parameter
- [ ] 9. Circular references with shared_ptr and how weak_ptr fixes them
- [ ] 10. Converting between unique_ptr and shared_ptr

### Level 11: Memory Management — NOT STARTED
- [ ] 1. Stack vs heap: allocation speed, lifetime, size limits
- [ ] 2. Memory layout of a process (text/data/bss/heap/stack, conceptual)
- [ ] 3. new/delete internals (constructor+allocation, destructor+dealloc)
- [ ] 4. Placement new (constructing an object in pre-allocated memory)
- [ ] 5. Alignment (alignof, alignas) and why it matters
- [ ] 6. Memory leaks, dangling pointers, buffer overflows (recap + tools)
- [ ] 7. Custom allocators (conceptual + a minimal example)
- [ ] 8. RAII wrappers for arbitrary resources (recap, applied)
- [ ] 9. Tools: AddressSanitizer / valgrind (conceptually)
- [ ] 10. std::byte, memory as raw storage

### Level 12: Move Semantics & Perfect Forwarding — NOT STARTED
- [ ] 1. Lvalues vs rvalues vs xvalues (value categories, intuition-level)
- [ ] 2. Rvalue references (T&&) vs lvalue references (T&)
- [ ] 3. std::move (what it actually does - just a cast, nothing more)
- [ ] 4. Move constructor / move assignment (revisited, deeper this time)
- [ ] 5. When the compiler implicitly generates move operations (and when it silently doesn't)
- [ ] 6. Universal/forwarding references (T&& in a template context) vs true rvalue references
- [ ] 7. std::forward and perfect forwarding
- [ ] 8. Return value optimization (RVO) / copy elision, and why std::move on a return is often counterproductive
- [ ] 9. Move-only types in containers (revisited with STL specifics)

### Level 13: Lambdas & Functional Tools — NOT STARTED
- [ ] 1. Lambda syntax: captures, parameters, return type, body
- [ ] 2. Capture by value [x] vs by reference [&x] vs default [=]/[&]
- [ ] 3. Mutable lambdas
- [ ] 4. Generic lambdas (auto parameters, C++14+)
- [ ] 5. std::function - type-erased callable wrapper
- [ ] 6. std::bind (and why lambdas usually replace it in modern code)
- [ ] 7. Function objects/functors (operator() overload) - what lambdas compile to
- [ ] 8. Storing/passing callables: raw function pointer vs template callable vs std::function
- [ ] 9. Recursive lambdas (brief, via std::function or a workaround)
- [ ] 10. Capturing `this` in a member function's lambda

### Level 14: Threading Basics — NOT STARTED
- [ ] 1. std::thread — creation, join vs detach, passing arguments
- [ ] 2. Race conditions (concretely demonstrated, not just described)
- [ ] 3. std::mutex, std::lock_guard, std::unique_lock
- [ ] 4. Deadlocks (how they happen, std::lock / std::scoped_lock to avoid them)
- [ ] 5. std::condition_variable — wait/notify_one/notify_all
- [ ] 6. Thread-safe initialization: std::call_once, static local variables
- [ ] 7. Data races vs benign races (why the former is UB even if "it works")
- [ ] 8. Passing data safely into threads (by value/copy vs shared state)
- [ ] 9. RAII for locks (why lock_guard exists instead of manual lock()/unlock())

### Level 15: Advanced Concurrency — NOT STARTED
- [ ] 1. std::atomic<T> — lock-free operations on simple types
- [ ] 2. Memory ordering (relaxed / acquire / release / seq_cst) — conceptual intuition
- [ ] 3. Compare-and-swap (compare_exchange_weak/strong) and its role in lock-free algorithms
- [ ] 4. std::future / std::promise / std::async — getting results back from async work
- [ ] 5. std::packaged_task
- [ ] 6. Thread pools (building a minimal one from std::thread + queue)
- [ ] 7. False sharing (cache-line contention between threads)
- [ ] 8. Lock-free vs lock-based tradeoffs (when each makes sense)
- [ ] 9. std::atomic_flag and simple spinlocks (educational, not production)

### Level 16: Design Patterns for Systems — NOT STARTED
- [ ] 1. Singleton (thread-safe implementation, revisited from L14)
- [ ] 2. Factory / Factory Method
- [ ] 3. Observer pattern (event/callback systems)
- [ ] 4. Strategy pattern (swappable algorithms)
- [ ] 5. Pimpl idiom (pointer to implementation)
- [ ] 6. CRTP (Curiously Recurring Template Pattern) — static polymorphism
- [ ] 7. RAII wrapper pattern (revisited as a formal "pattern")
- [ ] 8. Builder pattern (constructing complex objects step by step)
- [ ] 9. When to prefer composition over inheritance

### Level 17: Performance & Low-Latency Techniques — NOT STARTED
- [ ] 1. Cache locality (spatial/temporal), why contiguous data wins
- [ ] 2. Structure of Arrays (SoA) vs Array of Structures (AoS)
- [ ] 3. Branch prediction and branch misprediction cost
- [ ] 4. False sharing (revisited, applied)
- [ ] 5. Compiler optimization flags (-O0..-O3, -march=native, conceptually)
- [ ] 6. Inlining (inline keyword's real meaning vs compiler's own decision)
- [ ] 7. Avoiding unnecessary allocations in hot paths (reserve, avoiding std::function/shared_ptr where unneeded)
- [ ] 8. Passing by value vs const reference for performance (recap, applied)
- [ ] 9. Profiling mindset: measure before optimizing, avoid guessing
- [ ] 10. constexpr / compile-time computation

### Level 18: Advanced Templates & Metaprogramming — NOT STARTED
- [ ] 1. Variadic templates (parameter packs), fold expressions (C++17)
- [ ] 2. SFINAE (Substitution Failure Is Not An Error) — classic pre-concepts technique
- [ ] 3. Type traits (is_same, is_integral, enable_if, decltype)
- [ ] 4. C++20 concepts (the modern, readable replacement for most SFINAE)
- [ ] 5. constexpr if (compile-time branching inside a template, C++17)
- [ ] 6. decltype and trailing return types
- [ ] 7. Template template parameters (brief, conceptual)
- [ ] 8. Tag dispatch (an alternative to SFINAE for overload selection)
- [ ] 9. CRTP revisited briefly in the context of static polymorphism + templates together

### Level 19: Modern C++ (C++17 / C++20 Features) — NOT STARTED
- [ ] 1. Structured bindings (revisited, more contexts: maps, arrays, custom types)
- [ ] 2. std::optional — representing "maybe no value" without sentinels
- [ ] 3. std::variant — type-safe tagged union, std::visit
- [ ] 4. std::string_view — non-owning string reference (perf-relevant)
- [ ] 5. std::span (C++20) — non-owning view over contiguous data
- [ ] 6. Ranges basics (C++20) — composable views/pipelines
- [ ] 7. Coroutines (C++20) — conceptual overview only (co_await/co_yield)
- [ ] 8. std::any (brief, when it's appropriate vs variant)
- [ ] 9. Three-way comparison operator `<=>` (C++20, spaceship operator)

### Level 20: Capstone — Limit Order Book — NOT STARTED
(Breakdown pending — integrates Levels 1–19 into a small matching-engine/
order-book system. Revisit once Levels 1–19 are substantially covered.)

## What I need this iteration

Generate as many problems as you judge necessary for the first unchecked
topic above (see "How many problems per topic" — no fixed count), in
strict top-to-bottom order (should be Level 1, Topic 1 on the very first
run). Assign each problem's difficulty on its own merits, independent of
the level number. Then return the updated version of this whole file.
