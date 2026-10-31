
#include <queue>
#include <random>
#include <vector>
#ifndef GREEEDY_DENSITY_HPP
#define GREEEDY_DENSITY_HPP
#include "../instance.hpp"

/*
    GREEDY DENSITY

    Builds an initial solution by repeatedly placing, on any cache with
    enough room left, the (video, cache) pair with the best gain-per-size
    density, until no placement remains beneficial. Meant to run before
    local_search (src/solvers/local_search.cpp), which then refines it.
    With probability 0.1 the runner-up pair is placed instead of the best.

    The pairs wait in a max-heap ordered by density. Placing a video can only
    lower its gains, and a cache's free space only shrinks, so an entry popped
    from the heap is either still exact or can be thrown away: it is stale
    (a newer entry with the lower gain was pushed), the video is already on
    that cache, or the video no longer fits there (and never will again).
    This picks the same pair as scanning the whole video x cache table after
    every placement, which cost V x C per placement.
*/

/// @brief Latency saved by each endpoint -> cache link: savings[e * C + c],
/// 0 if endpoint e is not linked to cache c. If an endpoint lists a cache
/// twice, the first link counts.
std::vector<int> buildSavings(InstanceData* instance) {
    int C = instance->ip.C;
    std::vector<int> savings((size_t)instance->ip.E * C, 0);

    for (int e = 0; e < instance->ip.E; e++) {
        const Endpoint& ep = instance->endpoints[e];

        // Backwards, so that the first link listed is the one written last
        for (int i = ep.K - 1; i >= 0; i--) {
            savings[(size_t)e * C + ep.endpoint_connections[i].idC] = ep.dc_latency - ep.endpoint_connections[i].cache_latency;
        }
    }

    return savings;
}

/// @brief Total latency saved (over all requests for video v) by adding v to cache c,
/// on top of the placements already made.
long long gainOf(InstanceData* instance, const std::vector<int>& savings, int v, int c) {
    long long total = 0;

    for (int id : instance->videos[v].associated_requests) {
        const Request& req = instance->requests[id];

        int saving = savings[(size_t)req.idE * instance->ip.C + c];
        if (saving > req.gain) {
            total += (long long)(saving - req.gain) * req.count;
        }
    }

    return total;
}

/// @brief Places video v on cache c: updates the association matrix, the free
/// space, the gain of each affected request, and the instance score (same
/// convention as local_search's compute_updated_score: sum of gain deltas
/// weighted by each request's count).
void place(InstanceData* instance, const std::vector<int>& savings, int v, int c) {
    instance->cache_affectation[v][c] = true;
    instance->caches[c].left_memory -= instance->videos[v].vsize;

    long long score_diff = 0;

    for (int id : instance->videos[v].associated_requests) {
        Request& req = instance->requests[id];

        int saving = savings[(size_t)req.idE * instance->ip.C + c];
        if (saving > req.gain) {
            score_diff += (long long)(saving - req.gain) * req.count;
            req.gain = saving;
        }
    }

    instance->score += score_diff;
}

/// @brief A (video, cache) pair waiting in the heap, with its gain when it was pushed
struct GreedyCandidate {
    double density;
    long long gain;
    int v;
    int c;
};

/// @brief Heap order: highest density on top; ties go to the smallest video,
/// then the smallest cache (the order the full table scan used to pick)
struct GreedyCandidateOrder {
    bool operator()(const GreedyCandidate& a, const GreedyCandidate& b) const {
        if (a.density != b.density) return a.density < b.density;
        if (a.v != b.v) return a.v > b.v;
        return a.c > b.c;
    }
};

typedef std::priority_queue<GreedyCandidate, std::vector<GreedyCandidate>, GreedyCandidateOrder> GreedyHeap;

/// @brief Pops entries until one is still a valid placement
/// @return false if the heap ran out
bool popValid(InstanceData* instance, const std::vector<std::vector<long long>>& gainTable, GreedyHeap& heap, GreedyCandidate& out) {
    while (!heap.empty()) {
        GreedyCandidate top = heap.top();
        heap.pop();

        if (top.gain != gainTable[top.v][top.c]) continue;
        if (instance->cache_affectation[top.v][top.c]) continue;
        if (instance->videos[top.v].vsize > instance->caches[top.c].left_memory) continue;

        out = top;
        return true;
    }
    return false;
}

/// @brief Greedily builds a solution by repeatedly placing the (video, cache)
/// pair with the highest gain-per-size density, until no cache has room left
/// for a beneficial video.
/// @param instance
/// @param p_best probability of placing the best pair; otherwise the runner-up is placed
/// (1 = plain deterministic greedy)
void greedy_density(InstanceData* instance, double p_best = 0.85) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> random_prob(0.0, 1.0);

    int V = instance->ip.V;
    int C = instance->ip.C;

    std::vector<int> savings = buildSavings(instance);

    // gainTable[v][c] = gain of placing video v on cache c, as last computed.
    // Only the row of the video we just placed can become out of date,
    // so that is the only row we refresh.
    std::vector<std::vector<long long>> gainTable(V, std::vector<long long>(C, 0));

    // Fill the whole table once, before any placement, and put every useful pair in the heap
    std::vector<GreedyCandidate> candidates;
    for (int v = 0; v < V; v++) {
        for (int c = 0; c < C; c++) {
            gainTable[v][c] = gainOf(instance, savings, v, c);
            if (gainTable[v][c] > 0) {
                candidates.push_back({(double)gainTable[v][c] / instance->videos[v].vsize, gainTable[v][c], v, c});
            }
        }
    }
    GreedyHeap heap(GreedyCandidateOrder(), std::move(candidates));

    while (true) {
        GreedyCandidate best, runnerUp;
        if (!popValid(instance, gainTable, heap, best)) break;
        bool hasRunnerUp = popValid(instance, gainTable, heap, runnerUp);

        double prob = random_prob(gen);
        bool takeRunnerUp = hasRunnerUp && prob >= p_best; // 2nd param, proba greedy
        GreedyCandidate selected = takeRunnerUp ? runnerUp : best;

        // The pair not selected is still valid: back in the heap
        if (hasRunnerUp) heap.push(takeRunnerUp ? best : runnerUp);

        place(instance, savings, selected.v, selected.c);

        // Refresh ONLY the row of the video we just placed. A gain that did not
        // change keeps its entry; a lower one gets a new entry, the old one is now stale.
        for (int c = 0; c < C; c++) {
            long long gain = gainOf(instance, savings, selected.v, c);
            if (gain == gainTable[selected.v][c]) continue;

            gainTable[selected.v][c] = gain;
            if (gain > 0) {
                heap.push({(double)gain / instance->videos[selected.v].vsize, gain, selected.v, c});
            }
        }
    }
}

#endif
