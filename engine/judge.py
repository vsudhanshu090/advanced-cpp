#!/usr/bin/env python3
"""
Local judge engine. Compiles a problem's tests.cpp (which includes the user's
solution.h) with g++, runs it, and parses doctest's output into a structured
result. Usable both as a CLI tool and as a library import from the Flask app.
"""
import subprocess
import re
import os
import tempfile


def run_judge(problem_dir: str) -> dict:
    """
    Compiles and runs the tests for a given problem directory.
    Returns a structured dict — never raises for compile/runtime failures,
    only for missing files.
    """
    problem_dir = os.path.abspath(problem_dir)
    tests_cpp = os.path.join(problem_dir, "tests.cpp")
    solution_h = os.path.join(problem_dir, "solution.h")

    if not os.path.isfile(tests_cpp):
        return {"status": "error", "message": f"No tests.cpp found in {problem_dir}"}
    if not os.path.isfile(solution_h):
        return {"status": "error", "message": f"No solution.h found in {problem_dir}"}

    with tempfile.TemporaryDirectory() as tmp:
        binary = os.path.join(tmp, "runner")
        compile_cmd = [
            "g++", "-std=c++17", "-O0", "-g",
            "-Wall", "-Wextra",
            tests_cpp, "-o", binary
        ]
        proc = subprocess.run(compile_cmd, capture_output=True, text=True, cwd=problem_dir)
        if proc.returncode != 0:
            return {
                "status": "compile_error",
                "compiler_output": proc.stderr,
            }

        try:
            run_proc = subprocess.run(
                [binary, "--reporters=console", "-s"],
                capture_output=True, text=True, timeout=15
            )
        except subprocess.TimeoutExpired:
            return {"status": "timeout", "message": "Test run exceeded 15s (possible infinite loop or deadlock)"}

        output = run_proc.stdout + run_proc.stderr
        return parse_doctest_output(output, proc.stderr)


def parse_doctest_output(output: str, compiler_warnings: str = "") -> dict:
    total_match = re.search(r"\[doctest\]\s+test cases:\s+(\d+)", output)
    failed_match = re.search(r"(\d+)\s+failed\s*\|\s*\d+\s+skipped", output)
    assertions_match = re.search(
        r"\[doctest\]\s+assertions:\s+(\d+)\s*\|\s*(\d+)\s+passed\s*\|\s*(\d+)\s+failed",
        output
    )

    if not total_match:
        return {"status": "unknown", "raw_output": output, "compiler_warnings": compiler_warnings}

    total_cases = int(total_match.group(1))
    failed_cases = int(failed_match.group(1)) if failed_match else 0
    passed_cases = total_cases - failed_cases

    total_a = passed_a = failed_a = 0
    if assertions_match:
        total_a, passed_a, failed_a = map(int, assertions_match.groups())

    # Split output into per-test-case blocks for detail view
    blocks = re.split(r"={10,}\n", output)
    cases = []
    for block in blocks:
        m = re.search(r"TEST CASE:\s*(.+)", block)
        if not m:
            continue
        name = m.group(1).strip()
        has_error = bool(re.search(r"ERROR:", block))
        cases.append({
            "name": name,
            "passed": not has_error,
            "detail": block.strip()
        })

    return {
        "status": "pass" if failed_cases == 0 else "fail",
        "total_cases": total_cases,
        "passed_cases": passed_cases,
        "failed_cases": failed_cases,
        "total_assertions": total_a,
        "passed_assertions": passed_a,
        "failed_assertions": failed_a,
        "cases": cases,
        "compiler_warnings": compiler_warnings,
        "raw_output": output,
    }


def _print_cli_report(result: dict):
    print("\n" + "=" * 60)
    print(" JUDGE RESULT")
    print("=" * 60)
    if result["status"] == "error":
        print(" ERROR:", result["message"])
    elif result["status"] == "compile_error":
        print(" COMPILE ERROR\n" + "-" * 50)
        print(result["compiler_output"])
    elif result["status"] == "timeout":
        print(" TIMEOUT:", result["message"])
    elif result["status"] in ("pass", "fail"):
        print(f" Assertions: {result['passed_assertions']}/{result['total_assertions']} passed")
        print(f" Test cases: {result['passed_cases']}/{result['total_cases']} passed")
        if result["status"] == "pass":
            print("\n ALL TEST CASES PASSED \u2705")
        else:
            print(f"\n {result['failed_cases']} TEST CASE(S) FAILED \u274c")
            print("\nDETAILS:\n" + "-" * 60)
            for case in result["cases"]:
                if not case["passed"]:
                    print(case["detail"] + "\n")
    else:
        print(result.get("raw_output", ""))
    print("=" * 60)


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python3 judge.py <problem_dir>")
        sys.exit(1)
    print(f"Compiling {sys.argv[1]}/tests.cpp ...")
    result = run_judge(sys.argv[1])
    _print_cli_report(result)
    sys.exit(0 if result.get("status") == "pass" else 1)
