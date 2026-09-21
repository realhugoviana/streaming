#include <vector>

#include "../instance.hpp"

/*
    GREEDY DENSITY

    Builds an initial solution by repeatedly placing, on any cache with
    enough room left, the (video, cache) pair with the best gain-per-size
    density, until no placement remains beneficial. Meant to run before
    local_search (src/solvers/local_search.cpp), which then refines it.
*/

/// @brief Latency from endpoint `ep` to cache `c`, or -1 if not connected.
int latencyToCache(const Endpoint& ep, int c) {
    for (int i = 0; i < ep.K; i++) {
        if (ep.endpoint_connections[i].idC == c) {
            return ep.endpoint_connections[i].cache_latency;
        }
    }
    return -1;
}

/// @brief Total latency saved (over all requests for video v) by adding v to cache c,
/// on top of the placements already made.
long long gainOf(InstanceData* instance, int v, int c) {
    long long total = 0;

    for (int id : instance->videos[v].associated_requests) {
        const Request& req = instance->requests[id];
        const Endpoint& ep = instance->endpoints[req.idE];

        int latency = latencyToCache(ep, c);
        if (latency == -1) continue;

        int saving = ep.dc_latency - latency;
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
void place(InstanceData* instance, int v, int c) {
    instance->cache_affectation[v][c] = true;
    instance->caches[c].left_memory -= instance->videos[v].vsize;

    long long score_diff = 0;

    for (int id : instance->videos[v].associated_requests) {
        Request& req = instance->requests[id];
        const Endpoint& ep = instance->endpoints[req.idE];

        int latency = latencyToCache(ep, c);
        if (latency == -1) continue;

        int saving = ep.dc_latency - latency;
        if (saving > req.gain) {
            score_diff += (long long)(saving - req.gain) * req.count;
            req.gain = saving;
        }
    }

    instance->score += score_diff;
}

/// @brief Greedily builds a solution by repeatedly placing the (video, cache)
/// pair with the highest gain-per-size density, until no cache has room left
/// for a beneficial video.
/// @param instance
void greedy_density(InstanceData* instance) {
    int V = instance->ip.V;
    int C = instance->ip.C;

    // gainTable[v][c] = gain of placing video v on cache c, as last computed.
    // Only the row of the video we just placed can become out of date,
    // so that is the only row we refresh.
    std::vector<std::vector<long long>> gainTable(V, std::vector<long long>(C, 0));

    // Fill the whole table once, before any placement
    for (int v = 0; v < V; v++) {
        for (int c = 0; c < C; c++) {
            gainTable[v][c] = gainOf(instance, v, c);
        }
    }

    while (true) {
        int bestVideo = -1;
        int bestCache = -1;
        double bestDensity = 0;

        // Scan the table: no gainOf call here, only reads
        for (int v = 0; v < V; v++) {
            for (int c = 0; c < C; c++) {
                if (instance->cache_affectation[v][c]) continue;
                if (instance->videos[v].vsize > instance->caches[c].left_memory) continue;

                long long gain = gainTable[v][c];
                if (gain <= 0) continue;

                double density = (double)gain / instance->videos[v].vsize;
                if (density > bestDensity) {
                    bestDensity = density;
                    bestVideo = v;
                    bestCache = c;
                }
            }
        }

        if (bestVideo == -1) break;

        place(instance, bestVideo, bestCache);

        // Refresh ONLY the row of the video we just placed
        for (int c = 0; c < C; c++) {
            gainTable[bestVideo][c] = gainOf(instance, bestVideo, c);
        }
    }
}
