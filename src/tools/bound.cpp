#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "../greedy.hpp"

/*
    CAPACITY AWARE UPPER BOUND

    The bound of section 4.4 gives each request its best reachable cache and
    ignores the capacity. It is a true ceiling, but a loose one: it assumes that
    every video is on every cache.

    This program computes a bound that keeps the capacity. It uses the structure
    of the objective. `f(A)` is the latency that a set `A` of (video, cache)
    pairs saves. Each request contributes `count * max(0, max_{(v,c) in A} g(r,c))`,
    which is monotone and submodular in `A`, and a sum of such terms stays
    monotone and submodular.

    Let `S` be a known solution and `OPT` an optimal one. Then:

        f(OPT) <= f(OPT u S)                             monotonicity
                = f(S) + sum_{e in OPT\\S} f(e | S u ...) telescoping
               <= f(S) + sum_{e in OPT\\S} f(e | S)       submodularity

    `OPT\\S` is a subset of a feasible set, thus it is feasible. Therefore:

        f(OPT) <= f(S) + max { sum_{e in T} f(e|S) : T feasible }

    No constraint links two caches, so that maximum splits into one knapsack for
    each cache: items are the candidate videos, the value of a video is its
    marginal gain `f(e|S)`, which is exactly `gain()`, its weight is its size,
    and the capacity is `X`. The program uses the fractional knapsack, whose
    value is an upper bound of the integral one, thus the result stays valid.

    The bound depends on `S`. A better solution gives a tighter bound.
*/

/// @brief The uncapacitated bound of section 4.4, for reference
long long looseBound(const InstanceData& raw, const InstanceData& in) {
    long long t = 0;
    for (const Request& r : in.requests) {
        const Endpoint& e = in.endpoints[r.id_endpoint];
        int best = e.dc_latency;
        for (const EndpointCacheConnection& c : e.caches_connections) best = std::min(best, c.latency);
        t += (long long)r.count * (e.dc_latency - best);
    }
    (void)raw;
    return t;
}

/// @brief An item of a cache knapsack: a marginal gain and the size that it costs
struct Item {
    long long value;
    int weight;
    bool operator<(const Item& o) const {
        // Order by density. The fractional knapsack fills in this order.
        return (double)value * o.weight > (double)o.value * weight;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <instance.in> <solution.txt>" << std::endl;
        return 1;
    }

    InstanceData raw = parseFile(argv[1]);
    InstanceData in = reduce(raw);
    Index x = buildIndex(in);

    // Read the solution, in the submission format
    std::ifstream f(argv[2]);
    if (!f.is_open()) { std::cerr << "Error: could not open '" << argv[2] << "'" << std::endl; return 1; }
    int N; f >> N;
    Solution sol(x.C);
    for (int i = 0; i < N; i++) {
        int c; f >> c;
        if (c < 0 || c >= x.C) { std::cerr << "Error: unknown cache " << c << std::endl; return 1; }
        std::string line; std::getline(f, line);
        std::istringstream ss(line);
        int v;
        while (ss >> v) sol[c].push_back(v);
    }

    // Rebuild the state. `place()` keeps the running total, thus f(S) is st.total.
    State st = makeState(x);
    std::vector<char> in_S((size_t)x.C * x.V, 0);
    for (int c = 0; c < x.C; c++)
        for (int v : sol[c]) { place(x, st, v, c); in_S[(size_t)c * x.V + v] = 1; }

    long long weight = 0;
    for (const Request& r : raw.requests) weight += r.count;
    auto shown = [&](long long t) { return (double)t * 1000.0 / (double)weight; };

    // The candidate pairs. A pair outside this list has a gain of zero at the
    // empty state, and a gain only becomes smaller, thus it is zero here too.
    State empty = makeState(x);
    std::vector<Cand> cand = candidates(x, in, empty, false);

    std::vector<std::vector<Item>> items(x.C);
    for (const Cand& p : cand) {
        if (in_S[(size_t)p.c * x.V + p.v]) continue;         // T excludes S
        long long g = gain(x, st, p.v, p.c);                 // the marginal f(e|S)
        if (g > 0) items[p.c].push_back({g, x.size[p.v]});
    }

    // One fractional knapsack for each cache
    long long slack = 0;
    for (int c = 0; c < x.C; c++) {
        std::sort(items[c].begin(), items[c].end());
        long long room = x.X, add = 0;
        for (const Item& it : items[c]) {
            if (room <= 0) break;
            if (it.weight <= room) { add += it.value; room -= it.weight; }
            else { add += (long long)((double)it.value * (double)room / (double)it.weight); room = 0; }
        }
        slack += add;
    }

    long long loose = looseBound(raw, in);
    long long tight = st.total + slack;

    std::printf("solution      : %12.1f\n", shown(st.total));
    std::printf("capacity bound: %12.1f   (solution + %.1f)\n", shown(tight), shown(slack));
    std::printf("loose bound   : %12.1f   (section 4.4, ignores capacity)\n", shown(loose));
    std::printf("gap to capacity bound: %.2f %%\n", 100.0 * (1.0 - (double)st.total / (double)tight));
    return 0;
}
