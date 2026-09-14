#pragma once

#include <algorithm>
#include <cassert>
#include <queue>
#include <vector>

#include "instance.hpp"
#include "solution.hpp"

/*
    GREEDY (video, cache) KNAPSACK

    Rank the (video, cache) pairs by the latency that they save, then place them.
    The pairs interact: each placement lowers the value of every other cache that
    serves the same requests. Mode "lazy" refreshes a pair before it commits the
    pair (CELF). Mode "static" keeps the initial ranking.
*/

constexpr int INF = 1000000000;

/// @brief The fixed data that a gain computation needs. Built one time.
struct Index {
    int C, X, V;
    std::vector<int> size;                   // Video size, by video id
    std::vector<int> lat;                    // E*C latency matrix, INF if unconnected
    std::vector<int> endpoint_of, count_of;  // Endpoint and weight of each request
    std::vector<int> dc_of;                  // Datacenter latency of each request
    std::vector<std::vector<int>> reqs;      // Request ids, by video id
};

/// @brief The data that changes while a solver runs
struct State {
    Solution sol;                            // Videos on each cache
    std::vector<int> residual;               // Free space of each cache
    std::vector<int> best;                   // Current latency of each request
    std::vector<std::vector<int>> placed;    // Caches that hold each video
    long long total = 0;                     // Latency that the state saves
};

/// @brief A pair that waits for a placement
struct Cand {
    double key; int v, c;
    bool operator<(const Cand& o) const { return key < o.key; }
};

/// @brief Sort key of a pair: the raw gain, or the gain for each Mo
inline double keyOf(long long gain, int size, bool density) { return density && size > 0 ? (double)gain / size : (double)gain; }

/// @brief Latency from the endpoint of a request to a cache, INF if there is no connection
inline int latOf(const Index& x, int r, int c) { return x.lat[(size_t)x.endpoint_of[r] * x.C + c]; }

/// @brief Build the index from a reduced instance
inline Index buildIndex(const InstanceData& in) {
    Index x;
    x.C = in.gd.C; x.X = in.gd.X; x.V = in.gd.V; x.size = in.size_of_video;
    x.reqs.resize(in.gd.V);

    // The latency matrix keeps the gain loop free of any search
    x.lat.assign((size_t)in.gd.E * in.gd.C, INF);
    for (const Endpoint& e : in.endpoints)
        for (const EndpointCacheConnection& c : e.caches_connections)
            x.lat[(size_t)e.id_endpoint * in.gd.C + c.id_cache] = c.latency;

    // Bucket the requests by video. Each bucket holds R/V requests on average.
    for (int r = 0; r < (int)in.requests.size(); r++) {
        const Request& q = in.requests[r];
        x.endpoint_of.push_back(q.id_endpoint);
        x.count_of.push_back(q.count);
        x.dc_of.push_back(in.endpoints[q.id_endpoint].dc_latency);
        x.reqs[q.id_video].push_back(r);
    }
    return x;
}

/// @brief Build the start state: all the caches are empty
inline State makeState(const Index& x) {
    State st;
    st.sol = Solution(x.C);
    st.residual.assign(x.C, x.X);
    st.best = x.dc_of;
    st.placed.resize(x.V);
    return st;
}

/// @brief Latency that a pair saves now, against the latency that the requests already get
inline long long gain(const Index& x, const State& st, int v, int c) {
    long long g = 0;
    for (int r : x.reqs[v]) g += (long long)x.count_of[r] * std::max(0, st.best[r] - latOf(x, r, c));
    return g;
}

/// @brief Total latency that the state saves. Computed again from all the requests.
/// @note  `st.total` holds the same value. Use this function only to check `st.total`.
inline long long totalSaved(const Index& x, const State& st) {
    long long t = 0;
    for (size_t r = 0; r < st.best.size(); r++) t += (long long)x.count_of[r] * (x.dc_of[r] - st.best[r]);
    return t;
}

/// @brief Put a video on a cache and lower the latency of the related requests
inline void place(const Index& x, State& st, int v, int c) {
    st.sol[c].push_back(v);
    st.placed[v].push_back(c);
    st.residual[c] -= x.size[v];
    for (int r : x.reqs[v]) {
        int b = std::min(st.best[r], latOf(x, r, c));
        st.total += (long long)x.count_of[r] * (st.best[r] - b);
        st.best[r] = b;
    }
}

/// @brief Remove a video from a cache. The related requests lose their latency.
/// @note  The latency of a request comes from a minimum. Thus a removal needs
///        the other caches of the video again.
inline void unplace(const Index& x, State& st, int v, int c) {
    auto in_cache = std::find(st.sol[c].begin(), st.sol[c].end(), v);
    auto in_video = std::find(st.placed[v].begin(), st.placed[v].end(), c);
    assert(in_cache != st.sol[c].end() && in_video != st.placed[v].end());
    st.sol[c].erase(in_cache);
    st.placed[v].erase(in_video);
    st.residual[c] += x.size[v];
    for (int r : x.reqs[v]) {
        int b = x.dc_of[r];
        for (int cc : st.placed[v]) b = std::min(b, latOf(x, r, cc));
        st.total -= (long long)x.count_of[r] * (b - st.best[r]);
        st.best[r] = b;
    }
}

/// @brief Build every pair that saves latency, with its initial key
inline std::vector<Cand> candidates(const Index& x, const InstanceData& in, const State& st, bool density) {
    std::vector<Cand> out;
    std::vector<long long> acc(x.C, 0);
    std::vector<char> seen(x.C, 0);
    std::vector<int> touched;

    // One pass for each video accumulates the gain of all of its caches at the same time
    for (int v = 0; v < x.V; v++) {
        for (int r : x.reqs[v])
            for (const EndpointCacheConnection& c : in.endpoints[x.endpoint_of[r]].caches_connections) {
                if (!seen[c.id_cache]) { seen[c.id_cache] = 1; touched.push_back(c.id_cache); }
                acc[c.id_cache] += (long long)x.count_of[r] * std::max(0, st.best[r] - c.latency);
            }
        for (int c : touched) if (acc[c] > 0) out.push_back({keyOf(acc[c], x.size[v], density), v, c});
        for (int c : touched) { acc[c] = 0; seen[c] = 0; }
        touched.clear();
    }
    return out;
}

/// @brief Place the pairs in the initial order. The keys become too high as the caches fill.
inline void greedyStatic(const Index& x, State& st, std::vector<Cand> cand) {
    std::sort(cand.begin(), cand.end(), [](const Cand& a, const Cand& b) { return b < a; });
    for (const Cand& p : cand) if (x.size[p.v] <= st.residual[p.c]) place(x, st, p.v, p.c);
}

/// @brief Refresh the top pair before each placement. This gives the exact greedy order.
/// @param log If not null, it receives each placement, for an undo
/// @param noise If not null, one multiplier for each video changes the order. It must stay
///        constant while this function runs, or the lazy comparison becomes invalid.
inline long long greedyLazy(const Index& x, State& st, std::vector<Cand> cand, bool density,
                            std::vector<std::pair<int,int>>* log = nullptr,
                            const std::vector<double>* noise = nullptr) {
    std::priority_queue<Cand> pq(std::less<Cand>(), std::move(cand));
    long long pops = 0;

    while (!pq.empty()) {
        Cand p = pq.top(); pq.pop(); pops++;
        if (x.size[p.v] > st.residual[p.c]) continue;           // The cache has no more space for it
        p.key = keyOf(gain(x, st, p.v, p.c), x.size[p.v], density);
        if (noise) p.key *= (*noise)[p.v];
        if (p.key <= 0) continue;                               // Other caches took all of its value
        if (!pq.empty() && p.key < pq.top().key) { pq.push(p); continue; }  // Too high: try again later
        place(x, st, p.v, p.c);
        if (log) log->push_back({p.v, p.c});
    }
    return pops;
}
