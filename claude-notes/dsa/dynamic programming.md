# Dynamic Programming — Revision Sheet

DP = recursion + reuse of overlapping subproblems. Two requirements to even consider it: **overlapping subproblems** and **optimal substructure** (the optimal answer is built from optimal answers to subproblems).

Every DP problem has a state. Find that first — the code is almost always secondary.

---

## 1. The Three Forms

Same problem, three ways to write it. Know all three; interviews expect memoization, CP expects tabulation for speed.

### 1. Recursion (brute force) — write this first, always

```cpp
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);      // recomputes the same n many times
}
```

`O(2^n)` — exponential, because subproblems overlap and get solved again and again.

### 2. Memoization (top-down) — add a cache to the recursion

```cpp
vector<int> memo(n + 1, -1);

int fib(int n) {
    if (n <= 1) return n;
    if (memo[n] != -1) return memo[n];

    return memo[n] = fib(n - 1) + fib(n - 2);
}
```

Same recursion, but each state is computed once. `O(n)` time, `O(n)` space (+ recursion stack).

### 3. Tabulation (bottom-up) — build the table iteratively, no recursion

```cpp
vector<int> dp(n + 1);
dp[0] = 0; dp[1] = 1;

for (int i = 2; i <= n; i++) {
    dp[i] = dp[i - 1] + dp[i - 2];
}
```

`O(n)` time, `O(n)` space, no recursion stack (no stack overflow risk on large `n`).

### 4. Space optimization — drop the array if only the last few states matter

```cpp
int prev2 = 0, prev1 = 1;
for (int i = 2; i <= n; i++) {
    int curr = prev1 + prev2;
    prev2 = prev1;
    prev1 = curr;
}
```

`O(1)` space. **Always the last step** — get it correct with a full array first, then compress.

> Converting memoization → tabulation: figure out the order dependencies (which states need which), loop in that order, and dp[] replaces memo[] with base cases filled in directly instead of via `if` conditions.

---

## 2. 1D DP

State = a single index. The recurrence usually looks at a small fixed window of previous states.

### 1. Climbing stairs / house robber pattern

```cpp
// house robber: dp[i] = max((i-2 answer) + take i, i-1 answer without taking i)
vector<int> dp(n);
dp[0] = nums[0];
dp[1] = max(nums[0], nums[1]);

for (int i = 2; i < n; i++) {
    dp[i] = max(dp[i - 1], dp[i - 2] + nums[i]);
}
```

`O(n)` time, `O(1)` space achievable (only need last two values).

### 2. Longest Increasing Subsequence (LIS)

**`O(n²)` DP:** `dp[i]` = length of the LIS ending exactly at `i`.

```cpp
vector<int> dp(n, 1);
for (int i = 1; i < n; i++) {
    for (int j = 0; j < i; j++) {
        if (nums[j] < nums[i]) {
            dp[i] = max(dp[i], dp[j] + 1);
        }
    }
}
int ans = *max_element(dp.begin(), dp.end());
```

**`O(n log n)` — patience sorting:** maintain `tails[]`, the smallest possible tail of an increasing subsequence of each length. Binary search for where `nums[i]` fits.

```cpp
vector<int> tails;
for (int x : nums) {
    auto it = lower_bound(tails.begin(), tails.end(), x);
    if (it == tails.end()) tails.push_back(x);
    else *it = x;
}
int ans = tails.size();
```

`tails` is **not** the actual LIS — only its length is guaranteed correct. Use `upper_bound` instead for the longest **non-decreasing** subsequence.

---

## 3. 2D DP — Grids

State = `(row, col)`. Movement is usually only right/down, which fixes the iteration order for you.

```cpp
// unique paths / min path sum shape
vector<vector<int>> dp(m, vector<int>(n));

for (int i = 0; i < m; i++)
    for (int j = 0; j < n; j++) {
        if (i == 0 && j == 0) { dp[i][j] = grid[i][j]; continue; }

        int up   = (i > 0) ? dp[i-1][j] : INT_MAX;
        int left = (j > 0) ? dp[i][j-1] : INT_MAX;

        dp[i][j] = grid[i][j] + min(up, left);
    }
```

`O(m*n)` time and space; can compress to `O(n)` (one row) since row `i` only needs row `i-1`.

**Variants:** obstacles → set `dp[i][j] = 0` or skip on a blocked cell; multiple paths merging (two robots) → 4D state `(r1, c1, r2, c2)` since both move together.

---

## 4. 0/1 Knapsack Family

State = `(index, remaining capacity)`. **The single most reused shape in all of DP** — recognise its disguises.

### 1. Base template

```cpp
// dp[i][w] = best value using first i items with capacity w
vector<vector<int>> dp(n + 1, vector<int>(cap + 1, 0));

for (int i = 1; i <= n; i++) {
    for (int w = 0; w <= cap; w++) {
        dp[i][w] = dp[i - 1][w];                        // don't take item i

        if (wt[i - 1] <= w) {
            dp[i][w] = max(dp[i][w], dp[i - 1][w - wt[i - 1]] + val[i - 1]);
        }
    }
}
```

`O(n * cap)` time and space. Space-optimize to a **1D array of size `cap+1`**, iterating `w` from **high to low** — this is the part everyone gets wrong:

```cpp
vector<int> dp(cap + 1, 0);
for (int i = 0; i < n; i++) {
    for (int w = cap; w >= wt[i]; w--) {              // MUST go right-to-left
        dp[w] = max(dp[w], dp[w - wt[i]] + val[i]);
    }
}
```

Right-to-left ensures each item is used **at most once** (you read `dp[w - wt[i]]` before it's updated for this item). Left-to-right would allow reusing the same item — which is exactly what **unbounded knapsack** wants:

```cpp
for (int i = 0; i < n; i++) {
    for (int w = wt[i]; w <= cap; w++) {              // left-to-right = unlimited reuse
        dp[w] = max(dp[w], dp[w - wt[i]] + val[i]);
    }
}
```

### 2. Disguises of 0/1 knapsack

| Problem | Mapping |
|---|---|
| Subset Sum | value = weight, ask "is `dp[cap] == cap` reachable" (boolean dp) |
| Equal Sum Partition | Subset sum with target = `totalSum / 2` |
| Target Sum (+/- signs) | Subset sum with target = `(totalSum + target) / 2` |
| Count subsets with a given sum | Same table, `dp[w] += dp[w - wt[i]]` instead of `max` |
| Minimum subset sum difference | Find the largest reachable sum `<= totalSum/2`, answer = `totalSum - 2*that` |

### 3. Unbounded knapsack disguises

| Problem | Mapping |
|---|---|
| Coin Change (min coins) | `dp[w] = min(dp[w], dp[w - coin] + 1)`, init `dp[0]=0`, rest `INF` |
| Coin Change II (count ways) | `dp[w] += dp[w - coin]`, loop **coins outer, amount inner** (order matters — outer coin loop stops permutations from being double-counted as different "ways") |
| Rod Cutting | Unbounded knapsack directly — cut lengths are the "items" |

---

## 5. String DP

State = `(i, j)` — a position in each of two strings, or `(i, j)` = a substring range in one.

### 1. Longest Common Subsequence (LCS)

```cpp
// dp[i][j] = LCS length of s1[0..i) and s2[0..j)
vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));

for (int i = 1; i <= m; i++) {
    for (int j = 1; j <= n; j++) {
        if (s1[i - 1] == s2[j - 1]) {
            dp[i][j] = 1 + dp[i - 1][j - 1];
        } else {
            dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
        }
    }
}
```

`O(m*n)` time and space; compress to two rows (`O(n)`) since row `i` only needs row `i-1`.

**Reused everywhere:**

| Problem | Change |
|---|---|
| Longest Common Substring | On mismatch, `dp[i][j] = 0` (not `max(...)`) — must stay contiguous. Track a global max instead of reading the corner. |
| Edit Distance | On mismatch, `dp[i][j] = 1 + min(insert, delete, replace) = 1 + min(dp[i][j-1], dp[i-1][j], dp[i-1][j-1])` |
| Shortest Common Supersequence | `LCS length` tells you how many characters can be shared; build the string by walking the LCS table back |
| Longest Palindromic Subsequence | LCS of the string with its own reverse |

### 2. Palindrome DP — range `(i, j)` on ONE string

```cpp
// isPalin[i][j] = true if s[i..j] is a palindrome
// MUST fill by increasing length — dp[i][j] depends on dp[i+1][j-1]
vector<vector<bool>> isPalin(n, vector<bool>(n, false));

for (int i = 0; i < n; i++) isPalin[i][i] = true;

for (int len = 2; len <= n; len++) {
    for (int i = 0; i + len - 1 < n; i++) {
        int j = i + len - 1;
        if (s[i] != s[j]) continue;
        isPalin[i][j] = (len == 2) || isPalin[i + 1][j - 1];
    }
}
```

`O(n²)` time and space. **Iteration order is the whole trick** — must go by increasing substring length, since `dp[i][j]` reads the strictly-smaller `dp[i+1][j-1]`. Row-by-row or column-by-column order (like the LCS table) does **not** work here.

Used for: Longest Palindromic Substring, Palindrome Partitioning (min cuts), Count Palindromic Substrings.

---

## 6. Interval DP

State = `(i, j)` = a range, but unlike palindrome DP the recurrence tries every **split point** `k` inside the range.

```cpp
// Matrix Chain Multiplication shape
// dp[i][j] = min cost to fully combine the range [i, j]
for (int len = 2; len <= n; len++) {
    for (int i = 0; i + len - 1 < n; i++) {
        int j = i + len - 1;
        dp[i][j] = INT_MAX;

        for (int k = i; k < j; k++) {                 // try every split point
            int cost = dp[i][k] + dp[k + 1][j] + mergeCost(i, k, j);
            dp[i][j] = min(dp[i][j], cost);
        }
    }
}
```

`O(n³)` time (`n²` ranges × `n` split points), `O(n²)` space. Same "increasing length" iteration order as palindrome DP.

**Recognise this pattern from:** "minimum cost to merge/multiply/burst in some order", Matrix Chain Multiplication, Burst Balloons, Minimum Cost to Cut a Stick, Boolean Parenthesization.

---

## 7. DP on Subsequences (pick / don't-pick)

The most common way a brand-new problem *feels* like DP: at each index you have exactly two choices.

```cpp
// generic shape — adapt the base case / combine step per problem
int solve(int i, /* other state */, vector<vector<int>>& memo) {
    if (i == n) return baseCase;
    if (memo[i][...] != -1) return memo[i][...];

    int notTake = solve(i + 1, /* same state */, memo);
    int take    = /* combine current element */ + solve(i + 1, /* updated state */, memo);

    return memo[i][...] = combine(take, notTake);      // max / min / sum / OR
}
```

This is the *derivation* path for LIS, knapsack, subset sum, partition problems — when stuck on a new problem, ask "at index `i`, what are my choices?" before anything else. The state is whatever information you need to carry forward besides the index.

---

## 8. Worth knowing (overview only)

Recognise these when they show up — full templates later.

| Topic | Idea in one line | Typical problem |
|---|---|---|
| **Bitmask DP** | State includes a bitmask of which items/cities are used, `n ≤ ~20` | TSP, assigning tasks to workers |
| **Digit DP** | State = `(position, tight bound, other constraint)`, built digit by digit | "count numbers ≤ N with property X" |
| **Tree DP** | State per node combines children's DP values on the way back up | max path sum in a tree, house robber III |
| **DP on DAGs / topo order** | Longest/shortest path in a DAG is just DP over topological order | see the graphs sheet, §3.6 |
| **DP with binary search (LIS-style)** | Replace an `O(n)` or `O(n²)` inner loop with binary search when monotonicity holds | LIS, box stacking |
| **Probability / Expected value DP** | Same recurrences, but combine with probabilities instead of max/min/sum | dice throw, egg drop |

---

## Complexity Summary

| Pattern | Time | Space (before optimization) | Space (optimized) |
|---|---|---|---|
| 1D DP (fib/stairs/robber) | `O(n)` | `O(n)` | `O(1)` |
| LIS — `O(n²)` DP | `O(n²)` | `O(n)` | `O(n)` |
| LIS — binary search | `O(n log n)` | `O(n)` | `O(n)` |
| 2D grid DP | `O(m*n)` | `O(m*n)` | `O(n)` |
| 0/1 Knapsack | `O(n * cap)` | `O(n * cap)` | `O(cap)` |
| Unbounded Knapsack | `O(n * cap)` | `O(cap)` | `O(cap)` |
| LCS / Edit Distance | `O(m*n)` | `O(m*n)` | `O(n)` |
| Palindrome DP | `O(n²)` | `O(n²)` | rarely compressible |
| Interval DP (MCM-style) | `O(n³)` | `O(n²)` | — |
| Bitmask DP | `O(2^n * n)` or `O(2^n * n²)` | `O(2^n * n)` | — |

---

## Problem → Approach

| The problem says… | Use |
|---|---|
| "count ways to reach / climb / tile" | 1D DP, `dp[i]` from a fixed window of previous states |
| "maximum sum, no two adjacent" | House robber pattern |
| "longest increasing subsequence" | LIS — `O(n²)` DP or `O(n log n)` binary search |
| "min/max path in a grid" | 2D DP, direction fixes iteration order |
| "choose items with a weight limit" | 0/1 Knapsack |
| "unlimited supply of each item" | Unbounded Knapsack |
| "can we partition into two equal groups" | Subset sum, target = total/2 |
| "count subsets/ways to reach a sum" | Knapsack shape, `+=` instead of `max` |
| "longest common ...(subsequence/substring)" | LCS shape, tweak mismatch handling |
| "min edits to transform one string to another" | Edit Distance |
| "is this string/substring a palindrome, in bulk" | Palindrome DP by increasing length |
| "min cost to fully merge/multiply/burst a sequence" | Interval DP, try every split point |
| "at every index, take it or skip it" | Pick/don't-pick recursion → memoize |
| "visit all n items", `n ≤ 20` | Bitmask DP |
| "count numbers up to N with a digit property" | Digit DP |
| "best value in a tree, considering children" | Tree DP |

---

## Bug Checklist

- Wrote the brute-force recursion **first** and confirmed it's correct before adding memoization — skipping this makes debugging a wrong DP much harder
- Memo array initialized to a value the answer could **actually** produce (e.g. `-1` is unsafe if answers can be `-1`) — use a sentinel outside the valid answer range, or a separate `visited[]` array
- 0/1 knapsack space-optimized loop goes **right to left**; forgetting this silently turns it into unbounded knapsack
- Coin Change II counts each combination once only because the **coin loop is outer** — swapping loop order double-counts permutations as different ways
- Palindrome / interval DP filled the table in row order instead of **by increasing length** — reads an unfilled cell and silently returns garbage/default values
- Off-by-one between `dp[i]` meaning "state after processing element `i`" vs "state at index `i`" — pick one convention and stay consistent through the recurrence and base cases
- Base cases for empty string / zero capacity / zero items set explicitly, not left as default-initialized zeros that happen to be right by coincidence
- Used `int` for a DP table where products/sums of large inputs overflow (common in "count the number of ways" problems — mod arithmetic needed, and mod every addition, not just the final answer)

---

## Practice Set

| Topic | Problems |
|---|---|
| 1D DP | Climbing Stairs, House Robber, House Robber II, Decode Ways |
| LIS | Longest Increasing Subsequence, Russian Doll Envelopes, Longest Chain of Pairs |
| Grid DP | Unique Paths, Minimum Path Sum, Dungeon Game |
| 0/1 Knapsack | Partition Equal Subset Sum, Target Sum, Last Stone Weight II |
| Unbounded Knapsack | Coin Change, Coin Change II, Rod Cutting |
| String DP | Longest Common Subsequence, Edit Distance, Distinct Subsequences |
| Palindrome DP | Longest Palindromic Substring, Palindrome Partitioning II |
| Interval DP | Matrix Chain Multiplication, Burst Balloons, Minimum Cost to Cut a Stick |
| Pick/don't-pick derivation | Combination Sum IV, Word Break |
| Bitmask DP | Traveling Salesman Problem, Partition to K Equal Sum Subsets |
| Tree DP | House Robber III, Binary Tree Maximum Path Sum |