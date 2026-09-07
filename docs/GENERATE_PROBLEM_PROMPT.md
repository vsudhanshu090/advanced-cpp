# Prompt: Generate new problems for advanced-cpp

Copy everything below this line into a fresh Claude chat (with code
execution / a bash tool available, so it can actually compile and verify
what it writes — this matters, don't skip it). Fill in the two `[...]`
placeholders before sending.

---

I'm building a local C++ practice judge platform called `advanced-cpp`.
You have no memory of it — everything you need is in this prompt. Please
read all of it before generating anything.

## Repo structure
```
advanced-cpp/
├── framework/test_common.h   <- tests must include THIS, not doctest.h directly
└── levels/
    └── <difficulty>/<problem-name>/
        ├── problem.md    <- statement + frontmatter
        ├── solution.h    <- what the user edits
        └── tests.cpp     <- hidden test suite (the answer key)
```

## problem.md frontmatter (exact schema)
```
---
title: <a few words>
topics: [Topic One, Topic Two]
difficulty: basic | easy | easy-medium | medium | hard | expert
type: implement | from-scratch | debug | refactor | performance
---
```

## Difficulty philosophy
- `basic`/`easy`: ONE concept, in isolation, `type: implement` only.
- `easy-medium`: concepts start lightly combining; first gently-hinted `debug` problems appear here.
- `medium`: heavier `from-scratch` designs; `debug` bugs are less hinted.
- `hard`: subtle/multi-concept `debug`, `refactor` introduced, `performance`-constrained problems introduced.
- `expert`: everything combined — e.g. a `from-scratch` design under a `performance` budget.

## Type definitions
- **implement**: `solution.h` has the full class skeleton with `// TODO` method bodies. Public interface is fixed and spelled out in problem.md.
- **from-scratch**: `solution.h` is nearly blank (just includes + a comment pointing at problem.md). Only a minimal required interface is specified in prose; the user designs everything else.
- **debug**: `solution.h` contains a COMPLETE, compiling implementation with exactly one intentional, realistic bug (not a stub). problem.md describes the symptom, not the fix.
- **refactor**: `solution.h` contains complete, working-for-simple-cases code that is unsafe (e.g. Rule of Five violation). Task is to modernize it without changing its public interface.
- **performance**: like `implement` or `from-scratch`, but the test suite adds a wall-clock budget on a large workload that only an efficient (specify the required Big-O) approach can meet.

## CRITICAL test-writing safety rules (learned the hard way — follow exactly)

1. **Every `tests.cpp` must `#include` the relative path to
   `framework/test_common.h`, never `doctest.h` directly.** On Windows,
   `doctest.h` pulls in `<windows.h>`, whose GDI header declares functions
   named `Rectangle`, `Arc`, `Polygon`, `Ellipse` as plain WinAPI symbols —
   these collide with any class using one of those names.
   `test_common.h` defines `NOGDI`/`WIN32_LEAN_AND_MEAN` first to prevent
   this. This is non-negotiable for every single problem.

2. **For `debug` and `refactor` problems**: NEVER write a test that
   creates two ALIASED objects (via a shallow-copy or shallow-move bug)
   and then lets BOTH reach their destructor in the same test. If the
   submission is still buggy, this causes a REAL double-free that can
   crash the whole test binary (a real risk, not theoretical — verified
   with an actual crash in development). Instead:
   - Test copy/move independence via **heap-allocated objects you
     deliberately never `delete`** (`auto* a = new Foo(...);`, no matching
     delete) — this proves independence via observable value comparison
     without ever risking a double-free.
   - Only stack-allocate and let destructors run naturally for objects
     you're CONFIDENT can't be aliased (e.g. a single object with no
     copying involved at all).
   - It's fine to have AT MOST one test near the end of the file that
     does the full realistic stack-allocated copy-then-destroy scenario
     (to genuinely verify the fix works end-to-end) — just keep it to one,
     and put it last, so a crash there doesn't prevent earlier tests from
     being reported.

3. **For `performance` problems**: NEVER guess a timing threshold.
   Actually write both a correct (efficient) and a naive (inefficient)
   reference implementation yourself, compile and time BOTH empirically
   (`g++ -std=c++17 -O0 -g`, matching the judge's own flags), and pick N
   and the budget so there's a clear separation (aim for >5x gap between
   naive and correct timing). Also make sure the naive implementation's
   time is comfortably under 15 seconds (the judge's hard timeout) so it
   fails via a clean assertion, not a generic timeout.

4. Every problem needs a stub/starting `solution.h` that FAILS the tests
   (compile error for `from-scratch`, wrong-answer failures for
   `implement`, the-actual-bug failures for `debug`/`refactor`) AND a
   correct reference implementation that you verify passes 100% before
   delivering anything. Show your work: compile both, run the judge
   logic (or just `g++ ... && ./a.out`) against both, and report the
   pass/fail counts you actually observed.

5. For `implement`/`from-scratch` problems on generic (templated) code,
   include at least one test using a **move-only type** (e.g.
   `std::unique_ptr<int>`) and, where relevant, a type with **no default
   constructor** — these catch overly-rigid implementations that
   secretly require copyability or default-constructibility they
   shouldn't.

6. Judge compile flags already include `-pthread`, so concurrency
   problems don't need anything extra for that.

7. Difficulty/type/topics are free text validated against the enums
   above — no other schema changes are needed on the receiving end.

## What NOT to duplicate

Here is my current curriculum coverage — do not repeat these topic
combinations at the same difficulty tier. Prioritize genuine gaps listed
in the roadmap section below unless I've asked for something specific.

```
[PASTE THE FULL CONTENTS OF docs/CURRICULUM.md HERE]
```

## What I need right now

Difficulty/type/topic (or "your choice from the roadmap gaps"): [DESCRIBE WHAT YOU WANT HERE — e.g. "one easy-medium problem on polymorphism/virtual functions" or "surprise me with 2 problems filling roadmap gaps, hard tier"]

## Deliverable format

Give me a single bash script (works in Git Bash on Windows) using heredocs
that creates each new problem's folder and writes all three files
directly — NOT a git diff/patch. I may not have the exact same baseline
files you do, and a patch can fail to apply for reasons that have nothing
to do with the new content; a script that just writes files directly has
no such failure mode.

Also give me a fully updated `docs/CURRICULUM.md` (the coverage table plus
the roadmap with your new addition(s) removed from the gap list) so I can
replace my copy and keep the manifest accurate for next time.
