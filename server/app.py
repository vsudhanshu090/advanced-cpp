#!/usr/bin/env python3
"""
Local web UI for the C++ practice judge.

Run:
    python3 server/app.py
Then open:
    http://localhost:5000

Scans ../levels/ for problems (any folder containing problem.md, solution.h,
tests.cpp), serves them to the frontend, runs the judge engine on demand,
and persists solved/attempt progress to server/data/progress.json so it
survives server restarts.
"""
import os
import sys
import re
import json
import datetime
from flask import Flask, jsonify, request, send_from_directory

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LEVELS_DIR = os.path.join(REPO_ROOT, "levels")
ENGINE_DIR = os.path.join(REPO_ROOT, "engine")
DATA_DIR = os.path.join(os.path.dirname(__file__), "data")
PROGRESS_FILE = os.path.join(DATA_DIR, "progress.json")

DIFFICULTIES = ["easy", "easy-medium", "medium", "hard", "expert"]

sys.path.insert(0, ENGINE_DIR)
from judge import run_judge  # noqa: E402

app = Flask(__name__, static_folder="static", template_folder="templates")

os.makedirs(DATA_DIR, exist_ok=True)


# ---------- progress persistence ----------

def load_progress():
    if not os.path.isfile(PROGRESS_FILE):
        return {}
    try:
        with open(PROGRESS_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return {}


def save_progress(data):
    tmp_path = PROGRESS_FILE + ".tmp"
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    os.replace(tmp_path, PROGRESS_FILE)


def record_run(problem_id, status):
    progress = load_progress()
    entry = progress.get(problem_id, {
        "solved": False, "attempts": 0, "first_solved_at": None,
    })
    entry["attempts"] += 1
    entry["last_status"] = status
    entry["last_run_at"] = datetime.datetime.utcnow().isoformat() + "Z"
    if status == "pass" and not entry["solved"]:
        entry["solved"] = True
        entry["first_solved_at"] = entry["last_run_at"]
    elif status == "pass":
        entry["solved"] = True
    progress[problem_id] = entry
    save_progress(progress)
    return entry


# ---------- problem discovery ----------

def parse_frontmatter(problem_md_path):
    """Parses the --- key: value --- header block at the top of problem.md."""
    with open(problem_md_path, "r", encoding="utf-8") as f:
        content = f.read()
    meta = {"title": "Untitled", "topics": [], "difficulty": "medium"}
    body = content
    m = re.match(r"^---\n(.*?)\n---\n(.*)$", content, re.DOTALL)
    if m:
        header, body = m.groups()
        for line in header.splitlines():
            if ":" not in line:
                continue
            key, val = line.split(":", 1)
            key, val = key.strip(), val.strip()
            if key == "topics":
                # supports [A, B, C] inline list syntax
                val = val.strip("[]")
                meta[key] = [t.strip() for t in val.split(",") if t.strip()]
            else:
                meta[key] = val
    if meta.get("difficulty") not in DIFFICULTIES:
        meta["difficulty"] = "medium"
    return meta, body.strip()


def find_problems():
    """Walks levels/ and returns metadata + progress for every valid problem folder."""
    progress = load_progress()
    problems = []
    if not os.path.isdir(LEVELS_DIR):
        return problems
    for root, dirs, files in os.walk(LEVELS_DIR):
        if {"problem.md", "solution.h", "tests.cpp"}.issubset(set(files)):
            rel_path = os.path.relpath(root, LEVELS_DIR).replace(os.sep, "/")
            meta, _ = parse_frontmatter(os.path.join(root, "problem.md"))
            entry = progress.get(rel_path, {})
            problems.append({
                "id": rel_path,
                "title": meta.get("title", rel_path),
                "topics": meta.get("topics", []),
                "difficulty": meta.get("difficulty", "medium"),
                "solved": entry.get("solved", False),
                "attempts": entry.get("attempts", 0),
            })
    diff_rank = {d: i for i, d in enumerate(DIFFICULTIES)}
    problems.sort(key=lambda p: (diff_rank.get(p["difficulty"], 2), p["title"]))
    return problems


def resolve_problem_dir(problem_id):
    safe = os.path.normpath(problem_id).replace("\\", "/")
    if safe.startswith("..") or os.path.isabs(safe):
        return None
    full = os.path.join(LEVELS_DIR, safe)
    if not os.path.isdir(full):
        return None
    return full


# ---------- routes ----------

@app.route("/")
def index():
    return send_from_directory(app.template_folder, "index.html")


@app.route("/api/problems")
def api_problems():
    return jsonify(find_problems())


@app.route("/api/stats")
def api_stats():
    problems = find_problems()
    total = len(problems)
    solved = sum(1 for p in problems if p["solved"])
    by_difficulty = {}
    for d in DIFFICULTIES:
        subset = [p for p in problems if p["difficulty"] == d]
        by_difficulty[d] = {
            "total": len(subset),
            "solved": sum(1 for p in subset if p["solved"]),
        }
    return jsonify({
        "total": total,
        "solved": solved,
        "percent": round((solved / total) * 100) if total else 0,
        "by_difficulty": by_difficulty,
    })


@app.route("/api/problem/<path:problem_id>")
def api_problem_detail(problem_id):
    pdir = resolve_problem_dir(problem_id)
    if not pdir:
        return jsonify({"error": "not found"}), 404
    meta, body = parse_frontmatter(os.path.join(pdir, "problem.md"))
    with open(os.path.join(pdir, "solution.h"), "r", encoding="utf-8") as f:
        solution = f.read()
    progress = load_progress().get(problem_id, {})
    return jsonify({"meta": meta, "problem_md": body, "solution": solution, "progress": progress})


@app.route("/api/problem/<path:problem_id>/solution", methods=["POST"])
def api_save_solution(problem_id):
    pdir = resolve_problem_dir(problem_id)
    if not pdir:
        return jsonify({"error": "not found"}), 404
    content = request.json.get("content", "")
    with open(os.path.join(pdir, "solution.h"), "w", encoding="utf-8") as f:
        f.write(content)
    return jsonify({"saved": True})


@app.route("/api/problem/<path:problem_id>/run", methods=["POST"])
def api_run(problem_id):
    pdir = resolve_problem_dir(problem_id)
    if not pdir:
        return jsonify({"error": "not found"}), 404
    content = request.json.get("content")
    if content is not None:
        with open(os.path.join(pdir, "solution.h"), "w", encoding="utf-8") as f:
            f.write(content)
    result = run_judge(pdir)
    progress_entry = record_run(problem_id, result.get("status", "unknown"))
    result["progress"] = progress_entry
    return jsonify(result)


if __name__ == "__main__":
    print(f"Scanning problems under: {LEVELS_DIR}")
    print(f"Found {len(find_problems())} problem(s)")
    print(f"Progress persisted to: {PROGRESS_FILE}")
    print("Open http://localhost:5000 in your browser")
    app.run(debug=True, port=5000)
