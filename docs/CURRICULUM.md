# Curriculum Coverage

This file is the source of truth for what's already been covered, so that
a *fresh* Claude chat (with no memory of past conversations) can generate
new problems that fill real gaps instead of guessing or repeating itself.

**How to use this file:** paste its full contents into the prompt in
`docs/GENERATE_PROBLEM_PROMPT.md` wherever it says
`[PASTE CURRICULUM.md HERE]`, before sending. When you get new problems
back, you'll also get an updated version of this file — replace this file
with that update.

## Taxonomy quick reference

**Difficulty** (`basic → easy → easy-medium → medium → hard → expert`):
teaches one concept in isolation at `basic`/`easy`; concepts combine and
tests get more adversarial as tier increases.

**Type**:
- `implement` — interface + stub given, fill in the logic
- `from-scratch` — no skeleton, you design the whole class
- `debug` — complete implementation with one intentional bug to find/fix
- `refactor` — unsafe/legacy code to modernize without changing its interface
- `performance` — correctness + a wall-clock budget that forces an efficient design

## Currently covered

| Difficulty | Type | Problem | Topics |
|---|---|---|---|
| basic | implement | Rectangle | Classes, Constructors, Operator Overloading |
| basic | implement | Circle | Classes, Constructors, Operator Overloading |
| easy | implement | BankAccount | Classes, Exception Safety, Encapsulation |
| easy | implement | Vector2D | Operator Overloading, Const Correctness, Math |
| medium | debug | Fix the DynamicArray bug | Debugging, RAII, Rule of Three |
| medium | from-scratch | LRU Cache | Design, Hash Maps, Data Structures |
| hard | implement | Thread-Safe Bounded BlockingQueue | Concurrency, Threading, Synchronization |
| hard | refactor | Refactor ResourceHandle | RAII, Rule of Five, Move Semantics, Refactoring |
| hard | performance | Running Median Tracker | Performance, Heaps, Algorithms |

**Notable gap:** no dedicated **templates/generic programming** problem
right now (one existed earlier and was removed) — worth prioritizing.

## Roadmap — topics not yet covered

Not a strict order, but a backlog to draw from so new problems fill real
gaps instead of repeating what already exists. Suggested tier is a
starting point, not a rule — the same topic can reappear at a harder tier
combined with something else later.

**Not yet touched at all:**
- Polymorphism / virtual functions / abstract base classes / virtual destructors (easy-medium)
- Templates & generic programming, standalone (easy-medium/medium — was covered before, needs replacing)
- Iterators — implementing a custom iterator for range-for support (medium)
- Lambdas, `std::function`, closures and captures (easy-medium)
- Smart pointers as a dedicated topic — `shared_ptr` ref-counting, `weak_ptr` cycle-breaking (medium)
- Perfect forwarding / universal references / `std::forward` (hard)
- Operator overloading beyond arithmetic — `operator<<`, `operator[]`, `operator()` (easy-medium)
- Multiple/virtual inheritance, the diamond problem (hard)
- `constexpr` / compile-time computation (medium)
- `std::optional` / `std::variant` (easy-medium)

**More advanced / expert-tier candidates:**
- CRTP (Curiously Recurring Template Pattern)
- Variadic templates / parameter packs
- SFINAE, type traits, `if constexpr`, C++20 concepts
- Lock-free structures / atomics beyond mutex-based concurrency
- Custom allocators / memory pools
- PIMPL idiom
- Expression templates
- Classic design patterns implemented idiomatically in C++ (Observer, Visitor, Strategy, Factory)

## Honesty note on how this works

This file is the *entire* mechanism for avoiding duplicate/random
coverage across stateless sessions — there is no other memory. It only
works if you keep pasting the current version in and saving the updated
version back. If you skip that step, a fresh chat has no way to know what
already exists and coverage decisions become guesswork again.
