#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <ctime>
#include <chrono>
#include <vector>
#include <stack>
#include <algorithm>
#include <numeric>
#include <random>
#include <string>
#include <cstring>
#include <tuple>
#include <utility>
#include <queue>
#include <iostream>

using namespace std;
using ull = unsigned long long;
using ll = long long;
using chrono_clock = chrono::high_resolution_clock;

inline int fastReadInt() {
    int x = 0, c = getchar_unlocked();
    while (c <= ' ') c = getchar_unlocked();
    bool neg = false;
    if (c == '-') { neg = true; c = getchar_unlocked(); }
    while (c > ' ') { x = x * 10 + (c - '0'); c = getchar_unlocked(); }
    return neg ? -x : x;
}

const double TIME_LIMIT_SECONDS = 17.0;
const int HEAVY_THRESH = 1200;
const int TABU_TENURE_BASE = 20;
const int MAX_SHAKE = 16;
const int RESTART_NO_IMPROVE = 5000;
const int RNG_SEED_FUZZ = 0xA5B91E37u;

struct Graph {
    int N;
    vector<vector<int>> adj;
    vector<int> deg;
    vector<int> isHeavy;
    vector<int> heavyNodes;
    vector<vector<uint64_t>> heavyBits;
    int blocks;
    void buildHeavy() {
        blocks = (N + 63) >> 6;
        isHeavy.assign(N, -1);
        for (int i = 0; i < N; ++i)
            if ((int)adj[i].size() >= HEAVY_THRESH) {
                isHeavy[i] = (int)heavyNodes.size();
                heavyNodes.push_back(i);
            }
        heavyBits.assign((int)heavyNodes.size(), vector<uint64_t>(blocks, 0ULL));
        for (int idx = 0; idx < (int)heavyNodes.size(); ++idx) {
            int u = heavyNodes[idx];
            auto &bits = heavyBits[idx];
            for (int v : adj[u]) bits[v >> 6] |= (1ULL << (v & 63));
        }
    }
    inline bool hasEdge(int u, int v) const {
        if ((unsigned)u >= (unsigned)N || (unsigned)v >= (unsigned)N) return false;
        int hi = isHeavy[u];
        if (hi != -1) return ((heavyBits[hi][v >> 6] >> (v & 63)) & 1ULL) != 0;
        return binary_search(adj[u].begin(), adj[u].end(), v);
    }
};

struct Timer {
    chrono_clock::time_point start;
    Timer() { start = chrono_clock::now(); }
    bool time_exceeded() const {
        double elapsed = chrono::duration<double>(chrono_clock::now() - start).count();
        return elapsed > TIME_LIMIT_SECONDS;
    }
    double elapsed() const { return chrono::duration<double>(chrono_clock::now() - start).count(); }
};

struct Scorer {
    const Graph &G;
    const vector<char> &in_path;
    const vector<int> &cnt;
    const vector<int> &visCount;
    Scorer(const Graph &g, const vector<char> &ip, const vector<int> &c, const vector<int> &v)
        : G(g), in_path(ip), cnt(c), visCount(v) {}
    int freeNeighbors(int v) const {
        int free = 0;
        for (int w : G.adj[v]) if (!in_path[w] && cnt[w] == 0) ++free;
        return free;
    }
    tuple<int,int,int> metric(int v) const {
        int d = G.deg[v];
        int free = freeNeighbors(v);
        int age = visCount[v];
        return make_tuple(d, -free, age);
    }
};

int choose_best(const Graph& G, const vector<char>& in_path, const vector<int>& cnt,
                const vector<int>& cands, const vector<int>& visCount) {
    if (cands.empty()) return -1;
    Scorer sc(G, in_path, cnt, visCount);
    int best = cands[0];
    auto bestm = sc.metric(best);
    for (size_t i = 1; i < cands.size(); ++i) {
        int v = cands[i];
        auto m = sc.metric(v);
        if (m < bestm) { best = v; bestm = m; }
    }
    return best;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N_in = fastReadInt();
    int M = fastReadInt();
    vector<pair<int,int>> edges;
    edges.reserve(M);
    int minNode = INT_MAX, maxNode = INT_MIN;
    for (int i = 0; i < M; ++i) {
        int a = fastReadInt(), b = fastReadInt();
        edges.emplace_back(a,b);
        minNode = min(minNode, min(a,b));
        maxNode = max(maxNode, max(a,b));
    }
    int input_base = (minNode == 0 ? 0 : 1);
    int N = N_in;

    Graph G; G.N = N;
    G.adj.assign(N, {});
    for (auto &e : edges) {
        int a = e.first - input_base;
        int b = e.second - input_base;
        if ((unsigned)a < (unsigned)N && (unsigned)b < (unsigned)N && a != b) {
            G.adj[a].push_back(b);
            G.adj[b].push_back(a);
        }
    }

    G.deg.assign(N,0);
    for (int i = 0; i < N; ++i) {
        sort(G.adj[i].begin(), G.adj[i].end());
        G.adj[i].erase(unique(G.adj[i].begin(), G.adj[i].end()), G.adj[i].end());
        G.deg[i] = (int)G.adj[i].size();
    }
    G.buildHeavy();

    Timer timer;
    std::mt19937 rng((uint32_t)(chrono::high_resolution_clock::now().time_since_epoch().count() ^ RNG_SEED_FUZZ));

    vector<int> path, bestPath;
    vector<char> in_path(N, 0);
    vector<int> cnt(N, 0);
    vector<int> visitCount(N, 0);
    vector<int> tabu(N, 0);

    auto push_back_node = [&](int v) {
        path.push_back(v);
        in_path[v] = 1;
        for (int nb : G.adj[v]) ++cnt[nb];
        ++visitCount[v];
    };
    auto pop_back_node = [&]() {
        int v = path.back();
        path.pop_back();
        in_path[v] = 0;
        for (int nb : G.adj[v]) --cnt[nb];
    };
    auto push_front_node = [&](int v) {
        path.insert(path.begin(), v);
        in_path[v] = 1;
        for (int nb : G.adj[v]) ++cnt[nb];
        ++visitCount[v];
    };
    auto pop_front_node = [&]() {
        int v = path.front();
        path.erase(path.begin());
        in_path[v] = 0;
        for (int nb : G.adj[v]) --cnt[nb];
    };
    auto can_add = [&](int cand)->bool { return !in_path[cand] && cnt[cand] == 1; };

    vector<int> nodeOrder(N);
    iota(nodeOrder.begin(), nodeOrder.end(), 0);
    sort(nodeOrder.begin(), nodeOrder.end(), [&](int a,int b){ return G.deg[a] < G.deg[b]; });

    for (int idx = 0; idx < N && !timer.time_exceeded(); ++idx) {
        int start = nodeOrder[idx];
        vector<char> visited(N, 0);
        stack<pair<int,int>> st;
        st.push({start,1});
        visited[start] = 1;
        path.clear();
        fill(in_path.begin(), in_path.end(), 0);
        fill(cnt.begin(), cnt.end(), 0);
        while (!st.empty() && !timer.time_exceeded()) {
            auto [v,len] = st.top(); st.pop();
            while ((int)path.size() > len - 1) pop_back_node();
            push_back_node(v);
            if (path.size() > bestPath.size()) bestPath = path;
            for (int nb : G.adj[v]) {
                if (!visited[nb] && can_add(nb)) {
                    visited[nb] = 1;
                    st.push({nb, len+1});
                }
            }
        }
    }

    if (bestPath.empty()) {
        printf("1\n%d\n", 0 + input_base);
        return 0;
    }

    path = bestPath;
    fill(in_path.begin(), in_path.end(), 0);
    fill(cnt.begin(), cnt.end(), 0);
    for (int v : path) { in_path[v] = 1; for (int nb : G.adj[v]) ++cnt[nb]; ++visitCount[v]; }

    auto extend_both = [&]() {
        bool extended = true;
        while (extended && !timer.time_exceeded()) {
            extended= false;
            if (!path.empty()) {
                int end_v = path.back();
                vector<int> cands;
                for (int nb : G.adj[end_v]) if (can_add(nb)) cands.push_back(nb);
                if (!cands.empty()) {
                    int chosen = choose_best(G, in_path, cnt, cands, visitCount);
                    if (chosen >= 0) { push_back_node(chosen); extended = true; }
                }
            }
            if (!path.empty()) {
                int front_v = path.front();
                vector<int> cands;
                for (int nb : G.adj[front_v]) if (can_add(nb)) cands.push_back(nb);
                if (!cands.empty()) {
                    int chosen = choose_best(G, in_path, cnt, cands, visitCount);
                    if (chosen >= 0) { push_front_node(chosen); extended = true; }
                }
            }
            if (path.size() > bestPath.size()) bestPath = path;
        }
    };

    extend_both();

    int stepsSinceImprove = 0;
    int iter = 0;
    const int MAX_ITER = 1<<30;
    while (!timer.time_exceeded() && iter < MAX_ITER) {
        ++iter;
        bool moved = false;
        if (!path.empty()) {
            int endv = path.back();
            vector<int> cands;
            for (int nb : G.adj[endv]) if (can_add(nb) && iter > tabu[nb]) cands.push_back(nb);
            if (!cands.empty()) {
                int chosen = choose_best(G, in_path, cnt, cands, visitCount);
                push_back_node(chosen);
                if (path.size() > bestPath.size()) { bestPath = path; stepsSinceImprove = 0; }
                moved = true;
            }
        }
        if (!moved && !path.empty()) {
            int frontv = path.front();
            vector<int> cands;
            for (int nb : G.adj[frontv]) if (can_add(nb) && iter > tabu[nb]) cands.push_back(nb);
            if (!cands.empty()) {
                int chosen = choose_best(G, in_path, cnt, cands, visitCount);
                push_front_node(chosen);
                if (path.size() > bestPath.size()) { bestPath = path; stepsSinceImprove = 0; }
                moved = true;
            }
        }

        if (moved) { stepsSinceImprove = 0; continue; }

        ++stepsSinceImprove;
        int removeCount = 1 + (rng() % MAX_SHAKE);
        removeCount = min<int>(removeCount, max<int>(1, (int)path.size()-1));
        bool fromFront = (rng() & 1);
        vector<int> removed;
        for (int i = 0; i < removeCount; ++i) {
            if (path.size() <= 1) break;
            if (fromFront) { removed.push_back(path.front()); pop_front_node(); }
            else { removed.push_back(path.back()); pop_back_node(); }
        }
        int tabuTenure = TABU_TENURE_BASE + (rng() % 8);
        for (int rm : removed) tabu[rm] = iter + tabuTenure;

        bool regrew = false;
        for (int tries = 0; tries < 3 && !timer.time_exceeded(); ++tries) {
            extend_both();
            if (path.size() > bestPath.size()) { bestPath = path; stepsSinceImprove = 0; regrew = true; break; }
            if (!path.empty()) {
                int b = path.back();
                vector<int> cands;
                for (int nb : G.adj[b]) if (can_add(nb) && iter > tabu[nb]) cands.push_back(nb);
                if (!cands.empty()) {
                    sort(cands.begin(), cands.end(), [&](int x,int y){
                        return make_pair(G.deg[x], visitCount[x]) < make_pair(G.deg[y], visitCount[y]);});
                    int take = min<int>(3, (int)cands.size());
                    int chosen = cands[rng()%take];
                    push_back_node(chosen);
                    regrew = true;
                }
            }
            if (!path.empty()) {
                int f = path.front();
                vector<int> cands;
                for (int nb : G.adj[f]) if (can_add(nb) && iter > tabu[nb]) cands.push_back(nb);
                if (!cands.empty()) {
                    sort(cands.begin(), cands.end(), [&](int x,int y){
                        return make_pair(G.deg[x], visitCount[x]) < make_pair(G.deg[y], visitCount[y]);
                    });
                    int take = min<int>(3, (int)cands.size());
                    int chosen = cands[rng()%take];
                    push_front_node(chosen);
                    regrew = true;
                }
            }
            if (regrew) break;
        }

        if (regrew) { if (path.size() > bestPath.size()) bestPath = path; continue; }

        if (stepsSinceImprove > RESTART_NO_IMPROVE || (int)path.size() < max<int>(1, (int)bestPath.size()/2)) {
            stepsSinceImprove = 0;
            if ((rng() & 1) && !bestPath.empty()) {
                path = bestPath;
                fill(in_path.begin(), in_path.end(), 0);
                fill(cnt.begin(), cnt.end(), 0);
                for (int v : path) { in_path[v] = 1; for (int nb : G.adj[v]) ++cnt[nb]; }
            } else {
                int seed = nodeOrder[rng()%N];
                path.clear(); fill(in_path.begin(), in_path.end(), 0); fill(cnt.begin(), cnt.end(), 0);
                push_back_node(seed);
                extend_both();
            }
        }
    }

    if (bestPath.empty()) bestPath = {0};
    printf("%zu\n", bestPath.size());
    for (size_t i = 0; i < bestPath.size(); ++i) {
        if (i) putchar_unlocked(' ');
        printf("%d", bestPath[i] + input_base);
    }
    putchar_unlocked('\n');
    return 0;
}
