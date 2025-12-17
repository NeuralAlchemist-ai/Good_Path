# Longest Induced Path in Undirected Graph  
### (Good Path Heuristic Solver)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![Topic: Graph Algorithms](https://img.shields.io/badge/Topic-Graph_Algorithms-green.svg)](https://github.com/topics/graph-algorithms)
[![Topic: NP-Hard](https://img.shields.io/badge/Topic-NP--Hard-red.svg)](https://github.com/topics/np-hard)
[![Topic: Heuristic](https://img.shields.io/badge/Topic-Heuristic-orange.svg)](https://github.com/topics/heuristic)

An advanced **heuristic solver** for finding a **maximum induced path** (also called a *good path*) in a large undirected graph.

This problem is **NP-hard**, so the solver focuses on producing **high-quality solutions within a fixed time limit**, rather than guaranteeing optimality.

---

## Problem Definition

Given an undirected graph \( G = (V, E) \), a **good path** (induced path) is a sequence of **distinct vertices**:

\[
(v_0, v_1, \dots, v_k)
\]

such that:
- \((v_i, v_{i+1}) \in E\) for all consecutive vertices, and
- \((v_i, v_j) \notin E\) for all \( |i - j| > 1 \)

The objective is to **maximize the number of vertices** in the path.

---

## Why This Is Hard

- The problem generalizes **Hamiltonian Path**, making it **NP-hard**.
- Exact algorithms scale poorly and are infeasible for large graphs.
- Practical solutions require **local search, diversification, and heuristic optimization**.

This solver targets graphs with up to:
- **10⁵ vertices**
- **10⁶ edges**

under a time limit of approximately **15 seconds**.

---

## Algorithm Overview

The implementation combines several complementary heuristics.

### 1. Graph Representation
- Standard adjacency lists for all vertices.
- **Heavy vertices** (degree ≥ 256) use **bitset adjacency** for fast edge-existence checks.

### 2. Induced Path Invariant
A vertex `v` can be added to the current path iff:
- `v` is not already in the path, and
- `v` has **exactly one neighbor** in the path (`cnt[v] == 1`)

This invariant guarantees the path remains **induced** at all times.

---

### 3. Initial Construction
- Multiple greedy DFS-like builds from **low-degree vertices**.
- Keeps the best path found during this seeding phase.

---

### 4. Two-End Greedy Extension
- Attempts to extend the path from **both ends**.
- Candidate vertices are ranked by:
  - Lower degree
  - More available (“free”) neighbors
  - Lower visit count (aging / diversification)

---

### 5. Local Search & Diversification
- **Tabu search** to avoid immediate cycling.
- **Shake**: remove a small number of vertices from one end.
- **Regrowth (3-improvement)**: greedily rebuild after shaking.
- **Restarts** when no improvement is observed for a long time.

---

### 6. Time-Bounded Optimization
- Search terminates when the time limit is reached.
- The **best valid induced path** found so far is returned.

---

## Input Format

N M
u1 v1
u2 v2
...
uM vM

- Undirected graph
- Vertex indexing may be **0-based or 1-based** (auto-detected)

---

## Output Format

K
v1 v2 ... vK

- `K` — number of vertices in the induced path
- Any valid solution is accepted; the **score equals `K`**

---

## Compilation

```bash
g++ -O2 -std=gnu++17 improved_induced_path_solver.cpp -o good_path_solver
./good_path_solver < input.txt > output.txt
