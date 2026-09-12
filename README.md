# advanced-cpp — Local C++ Practice Judge

A local, LeetCode-style practice platform for advanced C++ (OOP, templates,
lambdas, concurrency, memory management). Runs entirely on your machine —
only two small UI libraries (CodeMirror, marked.js, Google Fonts) load from
a CDN on first page load.

## Setup (one-time)

```bash
pip install -r requirements.txt
```

Requires: Python 3.8+, and a working `g++` (you already have this).

## Run it

```bash
python3 server/app.py
```

Then open **http://localhost:5000**.

- **Home** shows your dashboard: total problems, solved count, % complete,
  and a breakdown by difficulty. This is persisted to
  `server/data/progress.json` — closing and reopening the server (or your
  machine) does not lose it.
- **Sidebar**: filter by difficulty (click chips to toggle, multiple can be
  active at once), browse all problems with their topic tags.
- Click a problem → statement on the left, code editor on the right.
- **Run Tests** saves your code and runs it against the hidden test suite.
  A pass automatically marks the problem solved — no separate "submit"
  button, no manual save. Attempts are counted every run.

## Adding a new question

**Fastest path:** use `docs/MASTER_PROMPT.md` — one self-contained file
you paste into *any* fresh Claude chat (no history needed). It contains
the full 20-level syllabus, a coverage tracker of exactly what's been
generated so far, the schema/safety rules, and the pedagogical rule that
the mechanism being taught is never pre-declared for you (e.g. an
operator-overloading lesson never shows you the `operator==` signature —
you have to know to write it). It generates the next topic in strict
syllabus order and hands back an updated copy of the whole file — replace
your local copy with that one so the next paste stays accurate.

The rest of this section explains the same conventions in short form, for
reference or if you're doing it ad-hoc in this same chat:

Ask Claude (in chat) for one with a spec — you don't need to specify a
level; problems can mix concepts from anywhere in your curriculum. Give:
- a **difficulty** (one of: `basic`, `easy`, `easy-medium`, `medium`, `hard`, `expert`)
- optionally, a **type** (see below) — or leave it to Claude to pick one
  that fits the concept
- optionally, topics you want covered (or leave it to Claude to pick a mix)

### Question types

Not every problem has to be "fill in the stubbed class." Five types exist:

- **implement** — the classic shape: interface + stub given, fill in the
  logic. Best for basic/easy, isolating "can you get the logic right"
  without also testing design instinct.
- **from-scratch** — no class skeleton at all. Only a prose spec + minimal
  required interface; you design the whole class yourself.
- **debug** — `solution.h` ships a *complete*, plausible implementation
  with an intentional bug. Find and fix it.
- **refactor** — working-but-unsafe code (raw pointers, no Rule of Five)
  that you modernize to be safe, without changing its public interface.
- **performance** — correctness tests plus a wall-clock budget that a
  brute-force approach can't meet, forcing an efficient design.

Each problem folder has a `levels/<difficulty>/<name>/` example of every
type already in this repo — read one before generating a new one of the
same type, since they each have specific safe-testing conventions (e.g.
`debug`/`refactor` problems test copy/move independence using
heap-allocated-and-never-deleted objects to avoid a real double-free
crashing the grader — see the comments in
`levels/medium/dynamic_array_bug/tests.cpp` for the pattern).

Claude hands you three files:
- `problem.md` — statement, with frontmatter:
  ```
  ---
  title: ...
  topics: [Smart Pointers, RAII]
  difficulty: easy
  type: implement
  ---
  ```
- `solution.h` — for `implement`, a stub with `// TODO`s; for
  `from-scratch`, nearly blank; for `debug`/`refactor`, a complete
  (buggy/unsafe) implementation
- `tests.cpp` — the hidden test suite (don't edit — it's the answer key).
  Must `#include "../../../framework/test_common.h"` (adjusting `../../..`
  for folder depth), **not** `doctest.h` directly — see the note below on
  why.

Drop all three into a new folder anywhere under `levels/`, e.g.:
```
levels/smart_pointer_wrapper/
├── problem.md
├── solution.h
└── tests.cpp
```

Refresh the browser — it appears in the sidebar automatically, filterable
by its difficulty. No server restart needed. (The folder name and any
subfolder nesting under `levels/` is just organizational — the app doesn't
care about the path, only the three files inside.)

## How grading works

`engine/judge.py` compiles `tests.cpp` (which `#include`s your `solution.h`)
with g++, runs the resulting binary, and parses the
[doctest](https://github.com/doctest/doctest) framework's output into
pass/fail per test case. Standalone CLI usage still works too:

```bash
python3 engine/judge.py levels/level06_templates/generic_stack
```

## Structure

```
advanced-cpp/
├── engine/judge.py            <- compiles + runs + scores (CLI + importable)
├── framework/doctest.h        <- test framework (single header, don't touch)
├── server/
│   ├── app.py                  <- Flask backend: problems, stats, run, progress
│   ├── data/progress.json      <- your solved/attempt history (auto-created)
│   ├── templates/index.html
│   └── static/{style.css, app.js}
└── levels/
    └── level06_templates/generic_stack/   <- example problem, ready to solve
        ├── problem.md
        ├── solution.h
        └── tests.cpp
```

## Windows note: name collisions with windows.h

On Windows, doctest pulls in `<windows.h>` for its timer, which declares
GDI functions named `Rectangle`, `Arc`, `Polygon`, `Ellipse`, and others as
plain WinAPI symbols. If a problem's class happens to share one of these
names, you'll get a confusing compile error (e.g. "cannot convert 'double'
to 'HDC'"). `framework/test_common.h` defines `NOGDI` (and related macros)
before pulling in `doctest.h`, which excludes that section of windows.h
entirely — this is why every `tests.cpp` includes `test_common.h` instead
of `doctest.h` directly.

## Known limitation: concurrency problems

Race conditions are non-deterministic — a subtly-wrong solution can
occasionally pass by luck. When we add concurrency-topic problems, their
`tests.cpp` files should run the critical assertions in a loop (e.g. 100-200
iterations) to make flaky failures surface reliably. Flag it when you're
ready for that topic and we'll build it into those problems specifically.
