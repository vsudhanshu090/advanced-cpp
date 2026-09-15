# Binary Trees & BSTs — Revision Sheet

`n` = number of nodes, `h` = height of the tree. Node definition used everywhere:

```cpp
struct Node {
    int val;
    Node *left, *right;
    Node(int v) : val(v), left(nullptr), right(nullptr) {}
};
```

---

## 1. Traversals

### 1. Recursive DFS (preorder / inorder / postorder)

```cpp
void preorder(Node* root) {           // root -> left -> right
    if (!root) return;
    visit(root);
    preorder(root->left);
    preorder(root->right);
}

void inorder(Node* root) {            // left -> root -> right
    if (!root) return;
    inorder(root->left);
    visit(root);
    inorder(root->right);
}

void postorder(Node* root) {          // left -> right -> root
    if (!root) return;
    postorder(root->left);
    postorder(root->right);
    visit(root);
}
```

**Inorder of a BST gives sorted order** — this single fact drives half of all BST problems.

### 2. Iterative traversals (stack)

```cpp
// Preorder — straightforward
stack<Node*> st;
st.push(root);
while (!st.empty()) {
    Node* node = st.top(); st.pop();
    visit(node);
    if (node->right) st.push(node->right);   // push right FIRST
    if (node->left)  st.push(node->left);    // so left is popped first
}
```

```cpp
// Inorder — go left as far as possible, then visit, then go right
stack<Node*> st;
Node* curr = root;
while (curr || !st.empty()) {
    while (curr) {
        st.push(curr);
        curr = curr->left;
    }
    curr = st.top(); st.pop();
    visit(curr);
    curr = curr->right;
}
```

Postorder iterative: do preorder but push left-then-right (reverse of above), collect into a list, then reverse it.

### 3. BFS / Level order

```cpp
queue<Node*> q;
q.push(root);

while (!q.empty()) {
    int sz = q.size();              // freeze the level size — THE key trick
    for (int i = 0; i < sz; i++) {
        Node* node = q.front(); q.pop();
        visit(node);
        if (node->left)  q.push(node->left);
        if (node->right) q.push(node->right);
    }
    // one level fully processed here
}
```

Almost every "level-by-level" problem (zigzag, right/left view, level averages, max width) is this loop with one line changed inside.

### 4. Morris traversal — O(1) space inorder

Uses **threading**: temporarily makes the inorder predecessor's right pointer point to the current node, then undoes it.

```cpp
void morrisInorder(Node* root) {
    Node* curr = root;
    while (curr) {
        if (!curr->left) {
            visit(curr);
            curr = curr->right;
        } else {
            Node* pred = curr->left;
            while (pred->right && pred->right != curr) pred = pred->right;

            if (!pred->right) {
                pred->right = curr;        // create thread
                curr = curr->left;
            } else {
                pred->right = nullptr;     // remove thread
                visit(curr);
                curr = curr->right;
            }
        }
    }
}
```

`O(n)` time, **`O(1)` space** — no stack, no recursion. Asked about mainly when a problem explicitly demands constant extra space.

---

## 2. Tree Properties

### 1. Height / Max depth

```cpp
int height(Node* root) {
    if (!root) return 0;
    return 1 + max(height(root->left), height(root->right));
}
```

### 2. Balanced check

A tree is balanced if for every node, `|height(left) - height(right)| <= 1`.

**Naive:** compute height at every node → `O(n²)` (height computed repeatedly).
**Better — bottom-up, one pass:** return `-1` as a "not balanced" signal instead of recomputing.

```cpp
int check(Node* root) {                // returns height, or -1 if unbalanced
    if (!root) return 0;

    int lh = check(root->left);
    if (lh == -1) return -1;

    int rh = check(root->right);
    if (rh == -1) return -1;

    if (abs(lh - rh) > 1) return -1;

    return 1 + max(lh, rh);
}
bool isBalanced(Node* root) { return check(root) != -1; }
```

`O(n)` time, `O(h)` space (recursion stack).

### 3. Diameter — longest path between any two nodes (not necessarily through root)

Same "return + update a global" pattern as balanced check.

```cpp
int best = 0;
int dfs(Node* root) {                  // returns height
    if (!root) return 0;
    int lh = dfs(root->left);
    int rh = dfs(root->right);
    best = max(best, lh + rh);         // path bending at this node
    return 1 + max(lh, rh);
}
```

`O(n)` time, `O(h)` space. This "compute height, update a global answer on the way back" shape reappears constantly — memorise the pattern, not just the problem.

### 4. Symmetric / same tree

```cpp
bool isSame(Node* a, Node* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->val == b->val && isSame(a->left, b->left) && isSame(a->right, b->right);
}

bool isMirror(Node* a, Node* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->val == b->val && isMirror(a->left, b->right) && isMirror(a->right, b->left);
}
bool isSymmetric(Node* root) { return isMirror(root, root); }
```

---

## 3. Views & Level-Based Problems

All variants of the BFS loop in §1.3, or a DFS carrying extra state.

### 1. Right view / Left view

```cpp
// BFS: last node processed in each level = right view; first = left view
for (int i = 0; i < sz; i++) {
    Node* node = q.front(); q.pop();
    if (i == sz - 1) result.push_back(node->val);   // right view
    ...
}
```

DFS alternative: recurse right-before-left, record the first node seen at each depth.

### 2. Zigzag / spiral level order

Same level-order BFS; reverse alternate levels before appending (or push into a `deque` and alternate front/back insertion).

### 3. Vertical order traversal

Assign each node a `(row, col)` — `left: col-1`, `right: col+1`. BFS/DFS while filling `map<col, vector<{row, val}>>`, then sort each column by row (and by value for ties at the same cell).

### 4. Top view / Bottom view

Same vertical-column idea: **top view** keeps the first node seen at each column (BFS order), **bottom view** keeps the last (overwrite as you go).

### 5. Maximum width of a tree

BFS with an index per node: root = 0, `left = 2*i`, `right = 2*i+1`. Width of a level = `lastIndex - firstIndex + 1`. Normalize indices per level (subtract the first index) to avoid overflow.

---

## 4. Construction & Serialization

### 1. Build tree from traversals

- **Preorder + Inorder** → unique tree. Preorder's first element is the root; find it in inorder to split left/right subtrees.
- **Postorder + Inorder** → unique tree. Postorder's last element is the root.
- **Preorder + Postorder alone** → tree is **not unique** unless every node has 0 or 2 children.
- Use a `hashmap<value, indexInInorder>` for `O(1)` root lookup → overall `O(n)`. Without it, `O(n²)`.

```cpp
unordered_map<int,int> inMap;   // value -> index in inorder
int preIdx = 0;

Node* build(vector<int>& pre, int inLo, int inHi) {
    if (inLo > inHi) return nullptr;

    int rootVal = pre[preIdx++];
    Node* root = new Node(rootVal);
    int mid = inMap[rootVal];

    root->left  = build(pre, inLo, mid - 1);
    root->right = build(pre, mid + 1, inHi);
    return root;
}
```

### 2. Serialize / Deserialize

Preorder with a sentinel for null works for **any** binary tree (doesn't need to be a BST):

```cpp
void serialize(Node* root, string& out) {
    if (!root) { out += "#,"; return; }
    out += to_string(root->val) + ",";
    serialize(root->left, out);
    serialize(root->right, out);
}

Node* deserialize(queue<string>& tokens) {
    string tok = tokens.front(); tokens.pop();
    if (tok == "#") return nullptr;

    Node* root = new Node(stoi(tok));
    root->left  = deserialize(tokens);
    root->right = deserialize(tokens);
    return root;
}
```

`O(n)` time and space for both.

---

## 5. Binary Search Tree — Basics

**Invariant:** for every node, everything in the left subtree is smaller, everything in the right subtree is larger. No duplicates assumed unless stated.

### 1. Search

```cpp
Node* search(Node* root, int key) {
    if (!root || root->val == key) return root;
    return key < root->val ? search(root->left, key) : search(root->right, key);
}
```

### 2. Insert

```cpp
Node* insert(Node* root, int key) {
    if (!root) return new Node(key);

    if (key < root->val) root->left  = insert(root->left, key);
    else                 root->right = insert(root->right, key);

    return root;
}
```

### 3. Delete — the one BST operation everyone forgets a case for

```cpp
Node* deleteNode(Node* root, int key) {
    if (!root) return nullptr;

    if (key < root->val)      { root->left  = deleteNode(root->left, key); }
    else if (key > root->val) { root->right = deleteNode(root->right, key); }
    else {
        // found the node to delete
        if (!root->left)  return root->right;     // 0 or 1 child
        if (!root->right) return root->left;

        // 2 children: replace with inorder successor (smallest in right subtree)
        Node* succ = root->right;
        while (succ->left) succ = succ->left;

        root->val = succ->val;
        root->right = deleteNode(root->right, succ->val);
    }
    return root;
}
```

Could also use the inorder **predecessor** (largest in left subtree) instead of successor — either is a valid BST after deletion.

### 4. Validate BST

**Wrong approach:** checking `left->val < node->val < right->val` only locally — misses violations from grandchildren.
**Correct:** carry a valid `(min, max)` range down the recursion.

```cpp
bool valid(Node* root, long long lo, long long hi) {
    if (!root) return true;
    if (root->val <= lo || root->val >= hi) return false;
    return valid(root->left, lo, root->val) && valid(root->right, root->val, hi);
}
bool isValidBST(Node* root) { return valid(root, LLONG_MIN, LLONG_MAX); }
```

Alternative: inorder traversal must come out strictly increasing.

### 5. Kth smallest / largest

Inorder traversal is sorted order → kth element = kth node visited (or `(n-k+1)`th for kth largest, or just do reverse-inorder for largest directly). Stop early once the count hits `k` — don't traverse the whole tree.

### 6. Floor / Ceil in a BST

```cpp
int floorBST(Node* root, int key) {         // largest value <= key
    int ans = -1;
    while (root) {
        if (root->val == key) return root->val;
        if (root->val < key) { ans = root->val; root = root->right; }
        else root = root->left;
    }
    return ans;
}
// ceil: mirror — go left when root->val >= key, track ans on the way
```

`O(h)` iterative, no recursion needed.

---

## 6. LCA (Lowest Common Ancestor)

### 1. Plain binary tree — no ordering to exploit

```cpp
Node* lca(Node* root, Node* p, Node* q) {
    if (!root || root == p || root == q) return root;

    Node* left  = lca(root->left, p, q);
    Node* right = lca(root->right, p, q);

    if (left && right) return root;      // p and q found in different subtrees
    return left ? left : right;
}
```

`O(n)` — visits every node once.

### 2. BST — use the ordering, don't do a blind search

```cpp
Node* lcaBST(Node* root, int p, int q) {
    while (root) {
        if (p < root->val && q < root->val)      root = root->left;
        else if (p > root->val && q > root->val) root = root->right;
        else return root;                        // split point = LCA
    }
    return nullptr;
}
```

`O(h)` — much faster than the general algorithm. **Always check "is this a BST?" before reaching for the general LCA.**

---

## 7. Worth knowing (overview only)

Recognise these when they show up — full templates later.

| Topic | Idea in one line | Typical problem |
|---|---|---|
| **Self-balancing BSTs (AVL, Red-Black)** | Rotations keep height `O(log n)` after insert/delete | usually just use `set`/`map` in C++ instead of implementing one |
| **Segment tree / Fenwick tree** | Different data structure entirely (range queries), not a binary *search* tree despite the name | range sum/min queries with updates |
| **Trie** | Tree over characters, not values — for prefix problems | word search, autocomplete |
| **Threaded binary tree** | Morris traversal's idea, made permanent | O(1) space traversal without recursion |
| **Binary Indexed / order-statistics tree** | Augment BST nodes with subtree size → kth-order queries in `O(log n)` | "kth smallest after k insertions/deletions" |

---

## Complexity Summary

| Operation | Time (BST, avg) | Time (BST, worst — skewed) | Space |
|---|---|---|---|
| Traversal (any) | `O(n)` | `O(n)` | `O(h)` recursion, `O(1)` Morris |
| Search / Insert / Delete | `O(log n)` | `O(n)` | `O(h)` |
| Height / Diameter / Balanced check | `O(n)` | `O(n)` | `O(h)` |
| LCA — plain binary tree | `O(n)` | `O(n)` | `O(h)` |
| LCA — BST | `O(log n)` | `O(n)` | `O(1)` iterative |
| Build from traversals | `O(n)` | `O(n)` | `O(n)` (hashmap + recursion) |
| Kth smallest via inorder | `O(h + k)` | `O(n)` | `O(h)` |

`h = O(log n)` for a balanced tree, `h = O(n)` for a skewed one (e.g. inserting sorted data into a BST with no rebalancing) — this is *why* AVL/Red-Black trees exist.

---

## Problem → Approach

| The problem says… | Use |
|---|---|
| "sorted order from a BST" | Inorder traversal |
| "level by level" / zigzag / right-left view | BFS with level-size freeze |
| "longest path between any two nodes" | Diameter pattern (height + global max) |
| "is this tree balanced/height-balanced" | Bottom-up height with -1 sentinel |
| "build tree from two traversals" | Preorder/postorder root + inorder split, hashmap for index |
| "serialize for storage/network, then rebuild" | Preorder + null sentinels |
| "kth smallest/largest in a BST" | Inorder, stop early at k |
| "closest value to X in a BST" | Floor/Ceil walk, or track closest while searching |
| "does this violate BST property anywhere" | min/max range check, not just local comparison |
| "common ancestor of two nodes" | General LCA, or `O(log n)` BST LCA if it's a BST |
| "convert sorted array/list to balanced BST" | Pick middle as root, recurse on both halves |
| "O(1) space traversal required" | Morris traversal |

---

## Bug Checklist

- Balanced-check / diameter computed height **inside** a separate call per node → accidentally `O(n²)`; use the single bottom-up pass instead
- BST validation compared only `node` vs immediate children — missed a grandchild violating range; pass `(lo, hi)` down
- Forgot the 2-children case in BST delete, or forgot both left and right can be null independently
- Preorder/postorder construction re-scanned inorder for the root index instead of using a hashmap → `O(n²)`
- Iterative preorder pushed left before right → came out in the wrong order
- BFS level-order forgot to freeze `q.size()` before the inner loop, so it looped over a moving target
- Used general `O(n)` LCA on a problem that specifies a BST, missing the `O(log n)` walk
- `long long` bounds for BST validation (`LLONG_MIN/MAX`, not `INT_MIN/MAX`) when values can hit int limits

---

## Practice Set

| Topic | Problems |
|---|---|
| Traversals | Binary Tree Inorder/Preorder/Postorder Traversal, Binary Tree Level Order Traversal |
| Views | Binary Tree Right Side View, Vertical Order Traversal, Top View / Bottom View |
| Properties | Maximum Depth of Binary Tree, Balanced Binary Tree, Diameter of Binary Tree, Symmetric Tree |
| Construction | Construct Binary Tree from Preorder and Inorder, Serialize and Deserialize Binary Tree |
| BST basics | Search in a BST, Insert into a BST, Delete Node in a BST, Validate Binary Search Tree |
| BST queries | Kth Smallest Element in a BST, Closest BST Value |
| LCA | Lowest Common Ancestor of a Binary Tree, Lowest Common Ancestor of a BST |
| Conversion | Convert Sorted Array to Binary Search Tree |