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

Ask Claude (in chat) for one with a spec — you don't need to specify a
level; problems can mix concepts from anywhere in your curriculum. Give:
- a **difficulty** (one of: `easy`, `easy-medium`, `medium`, `hard`, `expert`)
- optionally, topics you want covered (or leave it to Claude to pick a mix)

Claude hands you three files:
- `problem.md` — statement, with frontmatter:
  ```
  ---
  title: ...
  topics: [Smart Pointers, RAII]
  difficulty: easy
  ---
  ```
- `solution.h` — a stub with `// TODO`s for you to fill in
- `tests.cpp` — the hidden test suite (don't edit — it's the answer key)

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

## Known limitation: concurrency problems

Race conditions are non-deterministic — a subtly-wrong solution can
occasionally pass by luck. When we add concurrency-topic problems, their
`tests.cpp` files should run the critical assertions in a loop (e.g. 100-200
iterations) to make flaky failures surface reliably. Flag it when you're
ready for that topic and we'll build it into those problems specifically.
