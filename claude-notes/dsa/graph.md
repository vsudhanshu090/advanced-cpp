# Graphs — Revision Sheet

`V` = vertices, `E` = edges. Adjacency list unless stated. Every DFS/BFS driver assumes the graph may be **disconnected**.

---

## 1. Basics + Traversal

### Representation

```cpp
vector<vector<int>> adj(n);                 // unweighted
vector<vector<pair<int,int>>> adj(n);       // weighted: {neighbour, weight}
struct Edge { int u, v, w; };               // edge list: Kruskal, Bellman-Ford
vector<vector<int>> mat(n, vector<int>(n)); // matrix: only if V <= ~500 (Floyd-Warshall)
```

Read 1-indexed input, store 0-indexed. Convert **once**, at input.

### 1. Recursive DFS

```cpp
void dfs(int node) {
    visited[node] = true;   // notice we mark true on top — the dfs call is the push operation

    for (int neigh : adj[node]) {
        if (!visited[neigh]) {
            dfs(neigh);
        }
    }
}
```

### 2. Iterative DFS

```cpp
st.push(start);

while (!st.empty()) {
    int node = st.top();
    st.pop();

    if (visited[node]) continue;
    visited[node] = true;       // mark on POP here, not on push

    for (int neigh : adj[node]) {
        if (!visited[neigh]) {
            st.push(neigh);
        }
    }
}
```

> Marking on push also works for plain reachability, but then a node is "visited" long before it's processed — the order won't match recursive DFS. Mark on pop and it does.
> Use iterative when depth can hit ~10⁶ (grids, path-like graphs) or recursion frames are fat.

### 3. Always use this for dfs as graph might be disconnected

```cpp
for (int i = 0; i < n; i++) {
    if (!visited[i]) {
        dfs(i);
    }
}
```

### 4. BFS

```cpp
queue<int> q;
q.push(start);
visited[start] = true;

while (!q.empty()) {
    int node = q.front();
    q.pop();

    for (int neigh : adj[node]) {
        if (!visited[neigh]) {
            visited[neigh] = true;      // in BFS you MUST mark on push
            q.push(neigh);
        }
    }
}
```

Marking on pop lets the same node enter the queue many times → broken levels.

### 5. BFS shortest distance

```cpp
dist[start] = 0;
q.push(start);

while (!q.empty()) {
    int node = q.front();
    q.pop();

    for (int neigh : adj[node]) {
        if (dist[neigh] == -1) {        // dist == -1 IS the visited array
            dist[neigh] = dist[node] + 1;
            q.push(neigh);
        }
    }
}
```

**Multi-source:** push all sources at dist 0 before the loop. Gives distance to the *nearest* source in one pass (rotting oranges, 01-matrix, nearest exit).

### 6. Connected components

```cpp
int components = 0;
for (int i = 0; i < n; i++) {
    if (!visited[i]) {
        components++;
        dfs(i);
    }
}
```

### 7. Cycle detection — Undirected

```
for every neighbor v:
    if v isn't visited:
        DFS(v, u)
    else if v != parent:
        cycle exists
```

```cpp
bool dfs(int node, int parent, vector<vector<int>>& adj, vector<bool>& visited) {
    visited[node] = true;

    for (int neigh : adj[node]) {

        if (!visited[neigh]) {
            if (dfs(neigh, node, adj, visited))
                return true;
        }
        else if (neigh != parent) {
            return true;
        }
    }
    return false;
}
```

> If parallel edges are allowed, `neigh != parent` wrongly skips the second copy. Fix: pass the **edge id** you came from and skip that instead.

### 8. Cycle detection — Directed

`visited` alone isn't enough: a cross edge into a finished node is not a cycle. The node must be **currently on the recursion stack**.

```cpp
bool dfs(int node, vector<vector<int>>& adj, vector<bool>& visited, vector<bool>& pathVisited) {
    visited[node] = true;
    pathVisited[node] = true;

    for (int neigh : adj[node]) {

        if (!visited[neigh]) {
            if (dfs(neigh, adj, visited, pathVisited))
                return true;
        }
        else if (pathVisited[neigh]) {
            return true;
        }
    }

    pathVisited[node] = false;      // must unmark on the way out
    return false;
}
```

Can also be done with 0/1/2 states (more memory efficient) — same code, one array. States: `0` unvisited, `1` on stack, `2` done.

### 9. Bipartite — can we 2-color so adjacent nodes differ?

```cpp
bool isBipartite(vector<vector<int>>& adj) {
    int n = adj.size();
    vector<int> color(n, -1);

    for (int i = 0; i < n; i++) {

        if (color[i] != -1)
            continue;

        queue<int> q;
        q.push(i);
        color[i] = 0;

        while (!q.empty()) {
            int node = q.front();
            q.pop();

            for (int neigh : adj[node]) {

                if (color[neigh] == -1) {
                    color[neigh] = 1 - color[node];     // 1 ^ color[node]
                    q.push(neigh);
                }
                else if (color[neigh] == color[node]) {
                    return false;
                }
            }
        }
    }

    return true;
}
```

**Bipartite ⟺ no odd-length cycle.** Trees are always bipartite; a triangle never is.

### 10. Alternative approaches

```
Cycle detection Undirected - BFS (add {u,parent} to queue)
Cycle detection Undirected - DSU (if find(u)==find(v) before uniting, that edge closes a cycle)
Cycle detection Directed   - BFS (Kahn's algorithm — failure to process all nodes = cycle)
Bipartite graph            - DFS (pass color to the dfs call)
```

### 11. Grids are graphs

Cell = node, adjacent cell = edge. `V = R*C`, `E ≈ 4*R*C`, so BFS/DFS is `O(R*C)`.

```cpp
int dr[] = {-1, 1, 0, 0};
int dc[] = { 0, 0,-1, 1};

for (int d = 0; d < 4; d++) {
    int nr = r + dr[d], nc = c + dc[d];
    if (nr < 0 || nr >= R || nc < 0 || nc >= C) continue;
    if (grid[nr][nc] == '#' || vis[nr][nc]) continue;
    // ...
}
```

Flatten to 1-D for DSU/dist arrays: `id = r * C + c`. Shortest path on a grid is **always BFS**, never DFS.

---

## 2. Topological sort

Ordering of vertices such that for every directed edge `u->v`, `u` appears before `v`.
Most important condition: **DAG** (Directed Acyclic Graph).
2 standard approaches — Kahn's algorithm and DFS.

> Orderings are **not unique**. For the lexicographically smallest one, use a min-heap instead of a queue in Kahn's: `priority_queue<int, vector<int>, greater<int>> pq;` → `O(V log V + E)`.
> The ordering is **unique** iff Kahn's queue never holds 2+ nodes at once.

### 1. Kahn's Algorithm (BFS + indegree)

```cpp
vector<int> topoSort(int n, vector<vector<int>>& adj) {
    vector<int> indegree(n, 0);

    for (int u = 0; u < n; u++) {
        for (int v : adj[u]) {
            indegree[v]++;
        }
    }

    queue<int> q;
    for (int i = 0; i < n; i++) {           // nodes with no prerequisites
        if (indegree[i] == 0)
            q.push(i);
    }

    vector<int> topo;
    while (!q.empty()) {
        int node = q.front();
        q.pop();

        topo.push_back(node);

        for (int neigh : adj[node]) {
            if (--indegree[neigh] == 0)
                q.push(neigh);
        }
    }

    if ((int)topo.size() != n)              // cycle exists
        return {};
    return topo;
}
```

### 2. DFS (postorder + reverse)

We want `u` before `v`. DFS naturally finishes `v` before `u`. So push nodes when DFS finishes and reverse.

Simply put — the node with no further dependency ends first, so it is the **last** element of the topological sorting. Collect finish order, then reverse.

```cpp
// state 0 -> not visited, 1 -> still visiting, 2 -> fully visited
// returns true if a cycle is found
bool dfs(int node, vector<vector<int>>& adj, vector<int>& state, vector<int>& topo) {

    state[node] = 1;

    for (int neigh : adj[node]) {

        if (state[neigh] == 0) {
            if (dfs(neigh, adj, state, topo))
                return true;
        }
        else if (state[neigh] == 1) {
            return true;                    // back edge => cycle
        }
    }

    state[node] = 2;
    topo.push_back(node);                   // postorder

    return false;
}

vector<int> topoSort(int n, vector<vector<int>>& adj) {
    vector<int> state(n, 0);
    vector<int> topo;

    for (int i = 0; i < n; i++) {
        if (state[i] == 0) {
            if (dfs(i, adj, state, topo))
                return {};
        }
    }
    reverse(topo.begin(), topo.end());
    return topo;
}
```

> 🐞 Your draft tested `state[node]` inside the neighbour loop instead of `state[neigh]`, and the driver looped on `visited[i]`.
> **Rule: inside the neighbour loop, every array access is indexed by `neigh`, never by `node`.**

### What topo order unlocks

| Ask | How |
|---|---|
| Course schedule / build order | Direct topo sort |
| Detect cycle in directed graph | `topo.size() != n` |
| Shortest **or longest** path in a DAG | Relax in topo order (§3.6) |
| Count paths in a DAG | DP over topo order: `ways[v] += ways[u]` |
| Alien dictionary | Build edges from adjacent word pairs, then topo sort |

---

## 3. Shortest Paths

Given a graph and source `s`, we want `dist[v]` = minimum cost from `s` to `v`.

**Choosing the algorithm:**

| Edge weights | Use | Time |
|---|---|---|
| All equal / unweighted | BFS | `O(V+E)` |
| Only 0 and 1 | 0-1 BFS (deque) | `O(V+E)` |
| Non-negative | Dijkstra | `O((V+E) log V)` |
| Negative allowed | Bellman-Ford | `O(V·E)` |
| Any, but graph is a DAG | Topo order + relax | `O(V+E)` |
| Any, **all pairs**, small V | Floyd-Warshall | `O(V³)` |

### 1. Unweighted graph — BFS

The first time we reach a node, we've found its shortest path.

```cpp
vector<int> shortestPath(int n, vector<vector<int>>& adj, int src) {
    vector<int> dist(n, -1);
    queue<int> q;

    dist[src] = 0;
    q.push(src);

    while (!q.empty()) {
        int node = q.front();
        q.pop();

        for (int neigh : adj[node]) {
            if (dist[neigh] == -1) {
                dist[neigh] = dist[node] + 1;
                q.push(neigh);
            }
        }
    }

    return dist;
}
```

**Path reconstruction** (works for every algorithm here): keep `parent[v] = u` wherever you set `dist[v]`, then walk back from the target and reverse.

### 2. Weighted graph — Dijkstra

```cpp
vector<long long> dijkstra(int n, vector<vector<pair<int,int>>>& adj, int src) {
    const long long INF = 1e18;

    vector<long long> dist(n, INF);

    priority_queue<pair<long long,int>, vector<pair<long long,int>>, greater<pair<long long,int>>> pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u])            // stale entry
            continue;

        for (auto [v, w] : adj[u]) {
            if (d + w < dist[v]) {
                dist[v] = d + w;
                pq.push({dist[v], v});
            }
        }
    }

    return dist;
}
```

**Why Dijkstra requires non-negative weights:** once it pops the smallest-distance node from the priority queue, that distance is treated as final. A negative edge could later reduce it, breaking the invariant.

Traps: `greater<>` or you get a max-heap. `long long` + `INF = 1e18` (not `LLONG_MAX`, it overflows on `dist[u] + w`).

Useful variants — small changes to the same template:

| Ask | Change |
|---|---|
| Count shortest paths | keep `ways[]`: on improvement `ways[v] = ways[u]`, on tie `ways[v] += ways[u]` |
| Minimise the **largest** edge on the path | relax with `max(d, w)` instead of `d + w` |
| Multi-source | push all sources with dist 0 |

### 3. 0-1 BFS

Special case where every edge is 0 or 1. A deque acts as a 2-bucket priority queue: 0-edges to the front, 1-edges to the back. `O(V+E)`.

```cpp
vector<int> zeroOneBFS(int n, vector<vector<pair<int,int>>>& adj, int src) {
    const int INF = 1e9;

    vector<int> dist(n, INF);
    deque<int> dq;

    dist[src] = 0;
    dq.push_front(src);

    while (!dq.empty()) {
        int u = dq.front();
        dq.pop_front();

        for (auto [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;

                if (w == 0)
                    dq.push_front(v);
                else
                    dq.push_back(v);
            }
        }
    }

    return dist;
}
```

Classic use: grids where some moves are free (breaking a wall costs 1, moving costs 0).

### 4. Negative weights — Bellman-Ford

Instead of greedily choosing the closest node, repeatedly relax every edge:
`dist[v] = min(dist[v], dist[u] + w)`, done `V-1` times — the max number of edges on a simple path.

```cpp
struct Edge { int u, v, w; };

vector<long long> bellmanFord(int n, vector<Edge>& edges, int src) {
    const long long INF = 1e18;

    vector<long long> dist(n, INF);
    dist[src] = 0;

    for (int i = 0; i < n - 1; i++) {
        bool changed = false;

        for (auto [u, v, w] : edges) {
            if (dist[u] == INF)
                continue;

            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                changed = true;
            }
        }

        if (!changed)
            break;
    }

    return dist;
}
```

**It can also detect negative cycles.** After `V-1` rounds, do one more round:

```cpp
for (auto [u, v, w] : edges)
    if (dist[u] != INF && dist[u] + w < dist[v])
        return true;        // reachable negative cycle
```

Why? Without a negative cycle the shortest path never needs more than `V-1` edges. If we can still improve, we're going around a cycle whose total weight is negative.

> This only finds cycles **reachable from src**. To find any negative cycle, initialise all `dist[] = 0`.
> Doesn't apply to undirected graphs with negative edges — one negative edge is already a negative cycle (`u→v→u`).

### 5. Floyd-Warshall — all pairs

Everything above is **single source** (SSSP). Sometimes we need every pair (APSP).
Ask "is the path `i→j` improved by going through `k`?": `dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])`, trying every intermediate `k`.

```cpp
void floydWarshall(int n, vector<Edge>& edges, vector<vector<int>>& dist) {
    const int INF = 1e9;
    dist.assign(n, vector<int>(n, INF));

    for (int i = 0; i < n; i++)
        dist[i][i] = 0;

    for (auto [u, v, w] : edges) {
        dist[u][v] = min(dist[u][v], w);        // min() handles parallel edges
        // dist[v][u] = min(dist[v][u], w);     // add if UNDIRECTED
    }

    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {

                if (dist[i][k] == INF || dist[k][j] == INF)
                    continue;

                dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);
            }
        }
    }
}
```

**Always use the k-i-j loop** — `k` is the DP dimension ("intermediates allowed = {0..k}"). Any other order is wrong.
Negative cycle check afterwards: `dist[i][i] < 0` for some `i`.
Practical up to `V ≈ 400-500`.

### 6. Shortest path in a DAG (topological order)

Topo sort → process nodes in that order → relax outgoing edges. No priority queue needed.

```cpp
vector<long long> dagShortestPath(int n, vector<vector<pair<int,int>>>& adj, int src) {
    const long long INF = 1e18;

    vector<int> topo = topoSort(n, adj);

    vector<long long> dist(n, INF);
    dist[src] = 0;

    for (int u : topo) {
        if (dist[u] == INF)
            continue;

        for (auto [v, w] : adj[u]) {
            dist[v] = min(dist[v], dist[u] + w);
        }
    }

    return dist;
}
```

Works even with **negative** weights — a DAG has no cycles, so it can't have a negative cycle.
Flip `min` to `max` and you get the **longest path**, which is NP-hard in general but trivial on a DAG. That's why "maximum score path" problems always force a DAG.

> All shortest path algos work on both directed and undirected graphs, except DAG shortest path (directed acyclic only).

### 7. When the node isn't enough — layered graphs

If the state needs more than "which node am I at", **put it in the node**, then run plain BFS/Dijkstra.

| Problem says | Node becomes |
|---|---|
| "at most k stops / k skips" | `(node, used)` |
| "you may break at most k walls" | `(cell, wallsBroken)` |
| "path length must be even" | `(node, parity)` |
| "collect keys" (small count) | `(cell, keyMask)` |

Complexity = states × transitions. Compute it before coding.

---

## 4. DSU

DSU (Disjoint Set Union), also called union-find. Maintains a collection of disjoint sets / components.

```
Initially:   {A} {B} {C} {D} {E}
union(A,B) : {A,B} {C} {D} {E}
union(C,D) : {A,B} {C,D} {E}
union(B,D) : {A,B,C,D} {E}
```

2 fundamental operations:
- `find(x)` — return the representative of x's component
- `unite(a,b)` — merge the components containing a and b

Naive `find` recurses up the parent chain, which can become a chain of length `n`. Two optimisations:

**1. Path compression** — instead of a long chain `A->B->C->D->E`, make everyone point to the root directly (`A->E, B->E, C->E, D->E`). One line: `parent[x] = find(parent[x])`.

**2. Union by size** — if A has 10 nodes and B has 2, don't point the large tree at the small one. Always **small -> large**.

```cpp
class DSU {
    vector<int> parent, sz;
    int comps;
public:
    DSU(int n) : parent(n), sz(n, 1), comps(n) {
        iota(parent.begin(), parent.end(), 0);
    }

    int find(int x) {
        if (parent[x] == x)
            return x;

        return parent[x] = find(parent[x]);
    }

    bool unite(int a, int b) {          // false if already in the same set
        a = find(a);
        b = find(b);

        if (a == b)
            return false;

        if (sz[a] < sz[b])
            swap(a, b);

        parent[b] = a;
        sz[a] += sz[b];
        comps--;

        return true;
    }

    bool same(int a, int b) { return find(a) == find(b); }
    int  size(int x)        { return sz[find(x)]; }
    int  components() const { return comps; }
};
```

With both optimisations: **`O(α(n))` ≈ O(1)** amortised per op, `O(n)` space. With only one, `O(log n)`.
`unite` must `find` both arguments first — writing `parent[b] = a` on raw inputs is a silent bug.

**Where DSU shows up:** Kruskal's MST · connected components · undirected cycle detection · accounts merge / friend circles / redundant connection · "number of islands as cells get added".

**One trick worth remembering:** DSU can only *add* edges. If a problem **removes** edges over time, process the queries in reverse and add them instead.

---

## 5. MST (Minimum Spanning Tree)

A subset of edges that connects all vertices with no cycle — exactly `V-1` edges — at minimum total cost.

- **Only for undirected graphs** (directed needs arborescences — ignore for now).
- The MST isn't necessarily unique, but the **total cost is**. Unique MST iff all weights are distinct.
- If the graph isn't connected you get a minimum spanning **forest** — one MST per component.

### 1. Kruskal's Algorithm — DSU

Sort edges, start from the minimum: if it doesn't make a cycle, take it, else ignore. DSU is how you detect the cycle in `O(α)`.

```cpp
struct Edge { int u, v, w; };

long long kruskal(int n, vector<Edge>& edges) {

    sort(edges.begin(), edges.end(),
         [](const Edge& a, const Edge& b) {
             return a.w < b.w;
         });

    DSU dsu(n);

    long long mstWeight = 0;
    int edgesUsed = 0;

    for (auto [u, v, w] : edges) {

        if (dsu.unite(u, v)) {
            mstWeight += w;
            edgesUsed++;

            if (edgesUsed == n - 1)
                break;
        }
    }

    if (edgesUsed != n - 1)
        return -1;      // graph disconnected

    return mstWeight;
}
```

`O(E log E)`, sort-dominated. **This is the default MST algorithm** — use it unless the graph is dense.

### 2. Prim's Algorithm — min heap

Kruskal thinks: *which edge should I add greedily?*
Prim thinks: *I already have a tree — what's the cheapest edge that expands it?*

```cpp
long long prim(int n, vector<vector<pair<int,int>>>& adj) {

    vector<bool> inMST(n, false);

    priority_queue<pair<long long,int>, vector<pair<long long,int>>, greater<pair<long long,int>>> pq;

    pq.push({0, 0});

    long long mstWeight = 0;
    int verticesUsed = 0;

    while (!pq.empty()) {

        auto [w, u] = pq.top();
        pq.pop();

        if (inMST[u])       // already reached more cheaply
            continue;

        inMST[u] = true;
        mstWeight += w;
        verticesUsed++;

        for (auto [v, weight] : adj[u]) {
            if (!inMST[v]) {
                pq.push({weight, v});
            }
        }
    }

    if (verticesUsed != n)
        return -1;          // disconnected

    return mstWeight;
}
```

`O(E log V)`. For **dense** graphs (complete graph, "connect all points on a plane", `V ≤ 1000`) use the `O(V²)` array version instead of a heap.

> Prim pushes the **edge weight**; Dijkstra pushes the **accumulated distance**. Mixing these up gives a wrong-but-plausible answer.

> **Don't confuse shortest path and MST — an MST does NOT give the minimum cost path between 2 vertices.**
> shortest path → minimise distance between specified vertices
> mst → minimise total cost of connecting all vertices

---

## 6. Worth knowing (overview only)

You don't need full templates for these yet — just recognise when a problem is asking for them.

| Topic | Idea in one line | Typical problem |
|---|---|---|
| **Bridges / articulation points** | DFS with `disc[]` and `low[]`; edge `u—v` is a bridge if `low[v] > disc[u]` | Critical Connections in a Network |
| **SCC (Kosaraju / Tarjan)** | Maximal mutually-reachable groups in a directed graph; collapsing them gives a DAG | "mutually reachable groups", longest path with cycles |
| **Tree diameter** | BFS from any node → farthest `a`; BFS from `a` → farthest `b` | Tree Diameter |
| **LCA / binary lifting** | Precompute `2^k`-th ancestors → `O(log n)` per query | distance between tree nodes, many queries |
| **Bipartite matching (Kuhn's)** | Augmenting paths; max matching = min vertex cover in bipartite graphs | task assignment |
| **Bitmask DP on graphs** | `n ≤ 20` → `O(2ⁿ·n²)` over visited-subsets | TSP, shortest path visiting all nodes |

---

## Complexity Summary

| Algorithm | Time | Space |
|---|---|---|
| DFS / BFS / components | `O(V+E)` | `O(V)` |
| Cycle detection (both kinds) | `O(V+E)` | `O(V)` |
| Bipartite check | `O(V+E)` | `O(V)` |
| Topological sort | `O(V+E)` | `O(V)` |
| BFS shortest path | `O(V+E)` | `O(V)` |
| 0-1 BFS | `O(V+E)` | `O(V)` |
| Dijkstra (heap) | `O((V+E) log V)` | `O(V+E)` |
| Dijkstra (dense array) | `O(V²)` | `O(V²)` |
| Bellman-Ford | `O(V·E)` | `O(V)` |
| Floyd-Warshall | `O(V³)` | `O(V²)` |
| DAG shortest / longest path | `O(V+E)` | `O(V)` |
| DSU (find / unite) | `O(α(n))` ≈ O(1) | `O(V)` |
| Kruskal | `O(E log E)` | `O(V+E)` |
| Prim (heap) | `O(E log V)` | `O(V+E)` |

## Graph-Type Applicability

| Algorithm | Undirected | Directed | Weighted | Negative weights |
|---|---|---|---|---|
| DFS / BFS traversal | ✅ | ✅ | ignored | ignored |
| BFS shortest path | ✅ | ✅ | ❌ | ❌ |
| 0-1 BFS | ✅ | ✅ | only 0/1 | ❌ |
| Dijkstra | ✅ | ✅ | ✅ | ❌ |
| Bellman-Ford | only non-negative | ✅ | ✅ | ✅ |
| Floyd-Warshall | ✅ | ✅ | ✅ | ✅ (no neg cycle) |
| Topo sort / DAG path | ❌ | ✅ DAG only | ✅ | ✅ |
| Cycle detect (parent) | ✅ | ❌ | – | – |
| Cycle detect (pathVisited) | ❌ | ✅ | – | – |
| Bipartite check | ✅ | underlying undirected | – | – |
| DSU | ✅ | ❌ | not stored | – |
| Kruskal / Prim | ✅ | ❌ | ✅ | ✅ |

---

## Problem → Algorithm

| The problem says… | Use |
|---|---|
| "how many islands / provinces / groups" | DFS/BFS components, or DSU |
| "minimum moves", all moves equal cost | BFS |
| "shortest path" with positive costs | Dijkstra |
| some moves free, others cost 1 | 0-1 BFS |
| negative costs / detect a profit loop | Bellman-Ford |
| distance between **every** pair, `n ≤ 400` | Floyd-Warshall |
| "prerequisites / build order / can you finish" | Topological sort |
| "longest path" and the graph is a DAG | topo order DP |
| "minimum cost to connect all X" | MST |
| "are these connected", many merges | DSU |
| edges get **removed** over time | reverse the queries + DSU |
| split into two groups with conflicts | Bipartite check |
| "at most k stops / k skips" | layered graph `(node, k)` |
| "minimise the largest edge on the path" | Dijkstra with `max(d, w)` |
| visit every node once, `n ≤ 20` | bitmask DP |

---

## Bug Checklist

- Converted 1-indexed input to 0-indexed **once**, at read time
- Added the reverse edge for undirected (and **not** for directed)
- Looped over all vertices as sources — the graph may be disconnected
- BFS marks visited on **push**; iterative DFS on **pop**
- Inside the neighbour loop, every index is `neigh`, not `node`
- `pathVisited[node] = false` on the way out of directed cycle DFS
- `greater<>` on the priority queue (min-heap) for Dijkstra and Prim
- `long long` distances, `INF = 1e18`, skip relaxing from `INF` nodes
- Floyd-Warshall has `k` as the outermost loop
- `unite` calls `find` on both arguments first
- Checked `edgesUsed == n-1` before reporting an MST weight
- Reset all global state between test cases

---

## Practice Set

| Topic | Problems |
|---|---|
| Traversal / components | Number of Islands · Number of Provinces · Clone Graph |
| Grid BFS | Rotting Oranges · 01 Matrix · Shortest Bridge |
| Cycle detection | Course Schedule · Redundant Connection |
| Bipartite | Is Graph Bipartite? · Possible Bipartition |
| Topological sort | Course Schedule II · Alien Dictionary |
| Dijkstra | Network Delay Time · Path With Minimum Effort · Swim in Rising Water |
| 0-1 BFS | Minimum Obstacle Removal to Reach Corner |
| Bellman-Ford / layered | Cheapest Flights Within K Stops |
| Floyd-Warshall | Find the City With the Smallest Number of Neighbors |
| DSU | Accounts Merge · Number of Operations to Make Network Connected |
| MST | Min Cost to Connect All Points · Critical & Pseudo-Critical Edges |
| Bridges | Critical Connections in a Network |