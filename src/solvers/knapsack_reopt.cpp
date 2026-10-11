#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#ifndef KNAPSACK_REOPT_HPP
#define KNAPSACK_REOPT_HPP
#include "../instance.hpp"

/*
    KNAPSACK RE-OPTIMISATION

    Improves an existing solution (from greedy_density, ruin_recreate, ...) one cache at a time.

    Take one cache and freeze every other cache. For each video, the value of putting it in this
    cache is then known exactly: it is the latency saved, over all its requests, compared with the
    best option the request has elsewhere. These values simply add up, so choosing the cache's
    contents is a 0/1 knapsack: videos are items, their size is the weight, the cache size X is the
    capacity. Dynamic programming solves it exactly, so a rewrite never lowers the score.

    Sweeps go over all caches, again and again, until no cache changes, max_sweeps is reached or the
    time budget runs out. This is the "replace" move (take videos out of a cache, put others in)
    that add/remove/swap cannot express, applied optimally to one cache at a time.

    The instance keeps its usual invariants: cache_affectation, caches[c].left_memory,
    requests[r].gain and score. Gains follow compute_updated_score: the best link among the caches
    holding the video (if an endpoint lists a cache twice, its better latency counts).

    Large caches: the dynamic program costs (candidate videos x X) per cache. Above
    KNAPSACK_DP_BUDGET cells (X in the hundreds of thousands), the candidates are cut to the
    cache's current videos plus the best others by value per MB. The program stays exact over that
    pool, so the score still never goes down.

    COMPLEXITY

    Notation: V videos, E endpoints, C caches of size X, R distinct requests, K_e the number of
    caches linked to endpoint e, and P = sum over requests r of K_(endpoint of r): the number of
    (request, link) pairs. P_c is the part of P going through cache c (the requests of the
    endpoints linked to c); summed over all caches, P_c gives P. n_c is the number of candidate
    videos for cache c (videos worth something there), at most V.

    - Preparation (once):         O(P + sum_e K_e log K_e) time, O(P_links + R + V) memory, where
                                  P_links = sum_e K_e is the number of links.
    - One cache c:                O(P_c + n_c log n_c + n_c * X + V) time.
                                  The n_c * X term is the dynamic program; KNAPSACK_DP_BUDGET caps it
                                  at 4e8 cells, i.e. at most about 50 MB of choice bits and about
                                  one second per cache (measured: 57 s for 66 capped caches).
                                  The V term is the scan that applies a rewrite (only when it changes).
    - Applying a rewrite:         O(V + sum over changed videos of their (request, link) pairs).
    - One sweep (all caches):     O(P + sum_c n_c * X + C * V) time. Extra memory per cache: X values
                                  for the DP plus n_c * X bits of choices, freed after each cache.
    - Number of sweeps:           every rewrite raises the (integer) score, so the loop ends; on the
                                  instances so far 1 to 30 sweeps; max_sweeps / time_budget_s bound it.

    Which term dominates: on dense networks (kittens: about 70 million request-link pairs) the value
    computation O(P) per sweep; on large caches (X = 250000) the dynamic program n_c * X.
*/

/// @brief Largest dynamic program (candidate videos x cache size) solved for one cache
const long long KNAPSACK_DP_BUDGET = 400000000LL;

/// @brief A link seen from an endpoint: the cache and the latency it saves
struct KnapsackCacheLink {
    int id_cache;
    int gain;
};

/// @brief A link seen from a cache: the endpoint and the latency it saves
struct KnapsackEndpointLink {
    int id_endpoint;
    int gain;
};

/// @brief How a request is currently served: its best gain, the cache giving it, and the
/// second-best gain (what the request falls back to if that cache drops the video)
struct KnapsackServing {
    int best_gain = 0;
    int best_cache = -1;
    int second_gain = 0;
};

/// @brief Memory of the dynamic program, kept from one cache to the next. A fresh allocation of up
/// to 50 MB per cache would come straight from the system (zeroed pages, page faults) every time;
/// reusing it avoids that cost on large caches.
struct KnapsackBuffers {
    std::vector<long long> best_values;   // best value per capacity
    std::vector<uint64_t> choice_bits;    // one bit per (item, capacity): did the item improve it?
};

/// @brief Everything the re-optimisation needs besides the instance (built once per call)
struct KnapsackState {
    std::vector<std::vector<KnapsackCacheLink>> links_of_endpoint;   // one entry per linked cache
    std::vector<std::vector<KnapsackEndpointLink>> links_of_cache;   // reverse of links_of_endpoint
    std::vector<std::vector<int>> requests_of_endpoint;              // request ids
    std::vector<KnapsackServing> serving;                            // per request
    std::vector<long long> value_in_cache;                           // per video; scratch, all zeros between caches
    KnapsackBuffers buffers;                                         // dynamic program memory, reused for every cache
    long long caches_with_cut_pool = 0;                              // rewrites done on a reduced candidate pool
};

/*
    STATE: indexes and request bookkeeping
*/

/// @brief Builds the endpoint -> cache and cache -> endpoint links, keeping one link per pair
/// Cost: O(sum_e K_e log K_e) time (sorting each endpoint's links), O(number of links) memory.
/// @param instance
/// @param state
void knapsack_build_links(InstanceData* instance, KnapsackState& state) {
    state.links_of_endpoint.assign(instance->ip.E, {});
    state.links_of_cache.assign(instance->ip.C, {});

    for (int id_endpoint = 0; id_endpoint < instance->ip.E; id_endpoint++) {
        const Endpoint& endpoint = instance->endpoints[id_endpoint];
        std::vector<KnapsackCacheLink> links;
        for (int k = 0; k < endpoint.K; k++) {
            const EndpointCacheConnection& connection = endpoint.endpoint_connections[k];
            links.push_back({connection.idC, endpoint.dc_latency - connection.cache_latency});
        }

        // Sort by cache, best gain first, so a cache listed twice keeps only its better link
        std::sort(links.begin(), links.end(), [](const KnapsackCacheLink& a, const KnapsackCacheLink& b) {
            return a.id_cache != b.id_cache ? a.id_cache < b.id_cache : a.gain > b.gain;
        });

        for (size_t k = 0; k < links.size(); k++) {
            bool duplicate = k > 0 && links[k].id_cache == links[k - 1].id_cache;
            if (duplicate || links[k].gain <= 0) continue;
            state.links_of_endpoint[id_endpoint].push_back(links[k]);
            state.links_of_cache[links[k].id_cache].push_back({id_endpoint, links[k].gain});
        }
    }
}

/// @brief Lists the requests of each endpoint (only the requests registered on a video)
/// Cost: O(R + V) time and memory.
/// @param instance
/// @param state
void knapsack_build_requests(InstanceData* instance, KnapsackState& state) {
    state.requests_of_endpoint.assign(instance->ip.E, {});
    for (int id_video = 0; id_video < instance->ip.V; id_video++) {
        for (int id_request : instance->videos[id_video].associated_requests) {
            state.requests_of_endpoint[instance->requests[id_request].idE].push_back(id_request);
        }
    }
}

/// @brief Recomputes how every request for a video is served, and updates requests[r].gain and
/// the instance score accordingly (same result as compute_updated_score)
/// Cost: O(sum over the video's requests of K_e), the video's (request, link) pairs.
/// @param instance
/// @param state
/// @param id_video
void knapsack_update_video(InstanceData* instance, KnapsackState& state, int id_video) {
    long long score_diff = 0;

    for (int id_request : instance->videos[id_video].associated_requests) {
        Request& request = instance->requests[id_request];
        KnapsackServing serving;

        for (const KnapsackCacheLink& link : state.links_of_endpoint[request.idE]) {
            if (!instance->cache_affectation[id_video][link.id_cache]) continue;
            if (link.gain > serving.best_gain) {
                serving.second_gain = serving.best_gain;
                serving.best_gain = link.gain;
                serving.best_cache = link.id_cache;
            } else if (link.gain > serving.second_gain) {
                serving.second_gain = link.gain;
            }
        }

        state.serving[id_request] = serving;
        score_diff += (long long)(serving.best_gain - request.gain) * request.count;
        request.gain = serving.best_gain;
    }

    instance->score += score_diff;
}

/// @brief Builds the state and brings every request's gain (and the score) up to date
/// Cost: O(P + sum_e K_e log K_e) time, O(number of links + R + V) memory.
/// @param instance
/// @param state
void knapsack_prepare(InstanceData* instance, KnapsackState& state) {
    knapsack_build_links(instance, state);
    knapsack_build_requests(instance, state);
    state.serving.assign(instance->ip.R, KnapsackServing());
    state.value_in_cache.assign(instance->ip.V, 0);
    for (int id_video = 0; id_video < instance->ip.V; id_video++) {
        knapsack_update_video(instance, state, id_video);
    }
}

/*
    ONE CACHE: value the videos, choose the best contents, apply them
*/

/// @brief Fills state.value_in_cache for every video worth something in this cache, given that
/// all other caches stay as they are
/// Cost: O(P_c), one step per (request, link) pair through this cache.
/// @param instance
/// @param state
/// @param id_cache
/// @return the videos with a positive value (state.value_in_cache must be cleared for them afterwards)
std::vector<int> knapsack_value_videos(InstanceData* instance, KnapsackState& state, int id_cache) {
    std::vector<int> candidates;

    for (const KnapsackEndpointLink& link : state.links_of_cache[id_cache]) {
        for (int id_request : state.requests_of_endpoint[link.id_endpoint]) {
            const Request& request = instance->requests[id_request];
            const KnapsackServing& serving = state.serving[id_request];

            // Best gain this request gets without this cache
            int gain_elsewhere = serving.best_cache == id_cache ? serving.second_gain : serving.best_gain;
            if (link.gain <= gain_elsewhere) continue;

            long long& value = state.value_in_cache[request.idV];
            if (value == 0) candidates.push_back(request.idV);   // first time this video is worth something here
            value += (long long)(link.gain - gain_elsewhere) * request.count;
        }
    }

    return candidates;
}

/// @brief Keeps the dynamic program within KNAPSACK_DP_BUDGET cells: if the pool is too large,
/// keeps the cache's current videos plus the best other videos by value per MB
/// Cost: O(1) when the pool fits, otherwise O(n log n) for n candidates (two sorts).
/// @param instance
/// @param state
/// @param id_cache
/// @param pool candidate videos, sorted by id; reduced in place (and kept sorted)
/// @return false if even the cache's current videos exceed the budget (the cache is then left alone)
bool knapsack_limit_pool(InstanceData* instance, KnapsackState& state, int id_cache, std::vector<int>& pool) {
    long long max_items = KNAPSACK_DP_BUDGET / (instance->ip.X + 1);
    if ((long long)pool.size() <= max_items) return true;

    std::vector<int> kept, others;
    for (int id_video : pool) {
        (instance->cache_affectation[id_video][id_cache] ? kept : others).push_back(id_video);
    }
    if ((long long)kept.size() > max_items) return false;

    std::sort(others.begin(), others.end(), [&](int a, int b) {
        return (double)state.value_in_cache[a] / instance->videos[a].vsize >
               (double)state.value_in_cache[b] / instance->videos[b].vsize;
    });
    others.resize(max_items - kept.size());

    pool = kept;
    pool.insert(pool.end(), others.begin(), others.end());
    std::sort(pool.begin(), pool.end());
    state.caches_with_cut_pool++;
    return true;
}

/// @brief Records in an item's row of choice bits that the item improved this capacity.
/// The capacity is unsigned so that / 64 and % 64 compile to shifts in the hot loop.
void knapsack_mark(uint64_t* item_bits, unsigned capacity) { item_bits[capacity / 64] |= 1ULL << (capacity % 64); }

/// @brief Did the item improve this capacity?
bool knapsack_marked(const uint64_t* item_bits, unsigned capacity) { return item_bits[capacity / 64] >> (capacity % 64) & 1; }

/// @brief Result of a 0/1 knapsack
struct KnapsackSolution {
    long long value;
    std::vector<char> chosen;   // per item
};

/// @brief Exact 0/1 knapsack by dynamic programming over the capacity
/// Cost: O(items x capacity) time; O(capacity) for the values plus items x capacity bits for the
/// choices. Pseudo-polynomial: fine for HashCode cache sizes, capped by KNAPSACK_DP_BUDGET above.
/// @param sizes item sizes
/// @param values item values
/// @param capacity
/// @param buffers memory reused between calls
/// @return the best total value and which items reach it
KnapsackSolution knapsack_solve(const std::vector<int>& sizes, const std::vector<long long>& values, int capacity, KnapsackBuffers& buffers) {
    int items = sizes.size();
    size_t words_per_item = capacity / 64 + 1;
    buffers.best_values.assign(capacity + 1, 0);           // best[c] = best value using at most c units
    buffers.choice_bits.assign(items * words_per_item, 0);
    long long* best = buffers.best_values.data();

    for (int i = 0; i < items; i++) {
        const int size = sizes[i];
        const long long value = values[i];
        uint64_t* improved = buffers.choice_bits.data() + i * words_per_item;

        // Go down the capacities so each item is used at most once
        for (int c = capacity; c >= size; c--) {
            long long with_item = best[c - size] + value;
            if (with_item > best[c]) {
                best[c] = with_item;
                knapsack_mark(improved, c);
            }
        }
    }

    KnapsackSolution solution{best[capacity], std::vector<char>(items, 0)};
    int c = capacity;
    for (int i = items - 1; i >= 0; i--) {
        if (knapsack_marked(buffers.choice_bits.data() + i * words_per_item, c)) {
            solution.chosen[i] = 1;
            c -= sizes[i];
        }
    }
    return solution;
}

/// @brief Sets the contents of a cache, keeping the instance invariants (affectation, left
/// memory, request gains, score)
/// Cost: O(V) for the scan, plus knapsack_update_video for each video that moved.
/// @param instance
/// @param state
/// @param id_cache
/// @param wanted per video: should it be in the cache?
void knapsack_set_cache_contents(InstanceData* instance, KnapsackState& state, int id_cache, const std::vector<char>& wanted) {
    std::vector<int> changed;

    for (int id_video = 0; id_video < instance->ip.V; id_video++) {
        bool in_cache = instance->cache_affectation[id_video][id_cache];
        if (in_cache == (bool)wanted[id_video]) continue;

        instance->cache_affectation[id_video][id_cache] = !in_cache;
        instance->caches[id_cache].left_memory += in_cache ? instance->videos[id_video].vsize : -instance->videos[id_video].vsize;
        changed.push_back(id_video);
    }

    for (int id_video : changed) {
        knapsack_update_video(instance, state, id_video);
    }
}

/// @brief Rewrites a cache with the best contents given every other cache
/// Cost: O(P_c + n log n + n x X) for n candidates, plus O(V + updates) if the cache changes.
/// @param instance
/// @param state
/// @param id_cache
/// @return true if the cache changed (the score then went up)
bool knapsack_reoptimise_cache(InstanceData* instance, KnapsackState& state, int id_cache) {
    int capacity = instance->ip.X;
    std::vector<int> candidates = knapsack_value_videos(instance, state, id_cache);

    // Value of the current contents, and the videos that could fit
    long long current_value = 0;
    std::vector<int> pool;
    for (int id_video : candidates) {
        if (instance->cache_affectation[id_video][id_cache]) current_value += state.value_in_cache[id_video];
        if (instance->videos[id_video].vsize <= capacity) pool.push_back(id_video);
    }
    std::sort(pool.begin(), pool.end());

    bool solvable = knapsack_limit_pool(instance, state, id_cache, pool);
    KnapsackSolution solution{0, {}};
    if (solvable) {
        std::vector<int> sizes;
        std::vector<long long> values;
        for (int id_video : pool) {
            sizes.push_back(instance->videos[id_video].vsize);
            values.push_back(state.value_in_cache[id_video]);
        }
        solution = knapsack_solve(sizes, values, capacity, state.buffers);
    }

    for (int id_video : candidates) state.value_in_cache[id_video] = 0;   // scratch back to zeros
    if (!solvable || solution.value <= current_value) return false;

    std::vector<char> wanted(instance->ip.V, 0);
    for (size_t i = 0; i < pool.size(); i++) {
        if (solution.chosen[i]) wanted[pool[i]] = 1;
    }
    knapsack_set_cache_contents(instance, state, id_cache, wanted);
    return true;
}

/*
    ENTRY POINT
*/

/// @brief Re-optimises the caches of a solution one at a time, sweep after sweep, until no cache
/// improves, max_sweeps is reached or time_budget_s seconds have passed
/// Cost: preparation, then per sweep O(P + sum_c n_c x X + C x V) (see the header for details).
/// @param instance a solution (e.g. after greedy_density or ruin_recreate); improved in place
/// @param max_sweeps maximum number of passes over all caches
/// @param time_budget_s time limit in seconds, checked after each cache
/// @param verbose print the score after each sweep on std::cerr
/// @return the final score (weighted: sum of gain x count)
long long knapsack_reopt(InstanceData* instance, int max_sweeps = 50, double time_budget_s = 600.0, bool verbose = false) {
    auto start = std::chrono::steady_clock::now();
    auto elapsed = [&]() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };

    KnapsackState state;
    knapsack_prepare(instance, state);

    for (int sweep = 1; sweep <= max_sweeps; sweep++) {
        int rewritten = 0;
        bool out_of_time = false;
        for (int id_cache = 0; id_cache < instance->ip.C && !out_of_time; id_cache++) {
            rewritten += knapsack_reoptimise_cache(instance, state, id_cache);
            out_of_time = elapsed() > time_budget_s;
        }

        if (verbose) {
            std::cerr << "knapsack sweep " << sweep << ": " << rewritten << " caches rewritten, contest score "
                      << computeContestScore(instance) << ", " << elapsed() << " s"
                      << (state.caches_with_cut_pool ? " (candidate pool cut on some caches)" : "") << std::endl;
        }
        if (rewritten == 0 || out_of_time) break;
    }

    return instance->score;
}

#endif
