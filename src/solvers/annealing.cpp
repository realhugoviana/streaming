#include <chrono>
#include <cmath>
#include <cstring>
#include <random>
#include <string>

#include "../greedy.hpp"

/*
    SIMULATED ANNEALING ON SINGLE VIDEO MOVES

    Phase 3 and phase 4 move the solution with a ruin and a recreate. The
    recreate calls the lazy greedy, thus one iteration costs a gain computation
    for every candidate of every ruined cache. kittens does 2,400 iterations in
    60 s.

    This solver uses a different neighbourhood. One move touches one video and
    one or two caches. Its cost is the cost of `place()` and `unplace()`, which
    is O(|reqs[v]|). kittens holds 20 requests for each video. One iteration is
    thus near 10,000 times cheaper than a ruin and a recreate.

    The move can make the total smaller. The Metropolis rule decides: a move
    that improves is always taken, a move that loses `d` is taken with the
    probability exp(-d/T). The temperature T goes down over the budget, thus the
    search explores at the start and climbs at the end.
*/

/// @brief A compressed row layout: `of[i]` is the slice `[off[i], off[i+1])` of `val`
struct Rows {
    std::vector<int> off, val;
    int begin(int i) const { return off[i]; }
    int end(int i) const { return off[i + 1]; }
    int count(int i) const { return off[i + 1] - off[i]; }
};

/// @brief Group the candidate pairs, by cache or by video
/// @param cand The pairs of phase 2
/// @param n Number of rows: the cache count, or the video count
/// @param by_cache true to group on `c`, false to group on `v`
Rows group(const std::vector<Cand>& cand, int n, bool by_cache) {
    Rows b;
    b.off.assign(n + 1, 0);
    for (const Cand& p : cand) b.off[(by_cache ? p.c : p.v) + 1]++;
    for (int i = 0; i < n; i++) b.off[i + 1] += b.off[i];

    std::vector<int> at = b.off;
    b.val.resize(cand.size());
    for (const Cand& p : cand) b.val[at[by_cache ? p.c : p.v]++] = (by_cache ? p.v : p.c);
    return b;
}

/// @brief Latency that a removal would lose, without changing the state
/// @note  The counterpart of `gain()`. `gain()` is zero for a video that a
///        cache already holds, thus it cannot rank a removal. The latency of a
///        request is a minimum over the caches of the video, so the loss needs
///        the other caches of the video again, exactly like `unplace()`.
inline long long lossOf(const Index& x, const State& st, int v, int c) {
    long long l = 0;
    for (int r : x.reqs[v]) {
        if (latOf(x, r, c) != st.best[r]) continue;   // Another cache already gives this latency
        int b = x.dc_of[r];
        for (int cc : st.placed[v]) if (cc != c) b = std::min(b, latOf(x, r, cc));
        l += (long long)x.count_of[r] * (b - st.best[r]);
    }
    return l;
}

/*
    THE MOVE

    A move is a list of placements and removals. The list is the undo: read it
    in the opposite order and apply the opposite operation. `holds` is a
    membership table that keeps the tests at a constant cost.
*/

/// @brief One operation of a move. `put` is true for a placement.
struct Op { int v, c; bool put; };

/// @brief The solver state: the greedy state, a membership table and the undo list
struct Move {
    State* st;
    const Index* x;
    std::vector<char>* holds;
    std::vector<Op> log;

    bool has(int v, int c) const { return (*holds)[(size_t)c * x->V + v] != 0; }

    void put(int v, int c) {
        place(*x, *st, v, c);
        (*holds)[(size_t)c * x->V + v] = 1;
        log.push_back({v, c, true});
    }

    void take(int v, int c) {
        unplace(*x, *st, v, c);
        (*holds)[(size_t)c * x->V + v] = 0;
        log.push_back({v, c, false});
    }

    /// @brief Undo every operation of the move, in the opposite order
    void undo() {
        for (auto it = log.rbegin(); it != log.rend(); ++it) {
            if (it->put) { unplace(*x, *st, it->v, it->c); (*holds)[(size_t)it->c * x->V + it->v] = 0; }
            else         { place(*x, *st, it->v, it->c);   (*holds)[(size_t)it->c * x->V + it->v] = 1; }
        }
        log.clear();
    }
};

/// @brief Free space on cache `c` for the video `v`, by removing other videos
/// @param evict Maximum count of removals. A move that empties a cache is rarely useful.
/// @param sample Count of videos to compare for each removal. The removal takes
///        the video of the sample that loses the least latency. A sample of 1 is
///        a random removal.
/// @return true if `v` now fits
bool makeRoom(Move& m, std::mt19937& rng, int v, int c, int evict, int sample) {
    const Index& x = *m.x;
    State& st = *m.st;
    for (int i = 0; i < evict && x.size[v] > st.residual[c]; i++) {
        if (st.sol[c].empty()) break;
        std::uniform_int_distribution<int> pick(0, (int)st.sol[c].size() - 1);

        int worst = st.sol[c][pick(rng)];
        long long least = sample > 1 ? lossOf(x, st, worst, c) : 0;
        for (int t = 1; t < sample; t++) {
            int u = st.sol[c][pick(rng)];
            long long l = lossOf(x, st, u, c);
            if (l < least) { least = l; worst = u; }
        }
        m.take(worst, c);
    }
    return x.size[v] <= st.residual[c];
}

/// @brief Draw `sample` candidate videos of a cache and keep the one with the best key
/// @note  A uniform draw is too weak. kittens holds 10,000 candidates for each
///        cache, and almost all of them already get their latency from another
///        cache, thus their gain is zero. A sample of a few candidates finds a
///        useful video far more often, and it stays much cheaper than the
///        greedy refill, which reads every candidate.
/// @return The chosen video, or -1 if the cache has no candidate
int bestCandidate(const Index& x, const State& st, const Rows& by_cache, std::mt19937& rng,
                  int c, int sample, bool density) {
    if (by_cache.count(c) == 0) return -1;
    std::uniform_int_distribution<int> pick(by_cache.begin(c), by_cache.end(c) - 1);

    int best_v = by_cache.val[pick(rng)];
    if (sample <= 1) return best_v;

    double best_key = keyOf(gain(x, st, best_v, c), x.size[best_v], density);
    for (int t = 1; t < sample; t++) {
        int v = by_cache.val[pick(rng)];
        double k = keyOf(gain(x, st, v, c), x.size[v], density);
        if (k > best_key) { best_key = k; best_v = v; }
    }
    return best_v;
}

/*
    MAIN
*/
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in> [-o out.txt] [-t seconds]"
                  << " [-s seed] [-k gain|density] [-x evictions] [-0 T0] [-1 T1] [-w start_accept]"
                  << " [-g 0|1] [-m transfer_share] [-i insert_sample] [-j evict_sample]" << std::endl;
        return 1;
    }

    std::string input_path = argv[1], output_path, key = "density";
    double seconds = 60.0, T0 = 0.0, T1 = 0.0, warm = 0.30, share = 0.5;
    int evict = 4, greedy_start = 1, insert_sample = 8, evict_sample = 8;
    unsigned int seed = 42;
    try {
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 >= argc) { std::cerr << "Option '" << argv[i] << "' needs a value" << std::endl; return 1; }
        if (std::strcmp(argv[i], "-o") == 0) output_path = argv[i + 1];
        else if (std::strcmp(argv[i], "-t") == 0) seconds = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-s") == 0) seed = (unsigned int)std::stoul(argv[i + 1]);
        else if (std::strcmp(argv[i], "-k") == 0) key = argv[i + 1];
        else if (std::strcmp(argv[i], "-x") == 0) evict = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-0") == 0) T0 = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-1") == 0) T1 = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-w") == 0) warm = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-g") == 0) greedy_start = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-m") == 0) share = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-i") == 0) insert_sample = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-j") == 0) evict_sample = std::stoi(argv[i + 1]);
        else { std::cerr << "Unknown option '" << argv[i] << "'" << std::endl; return 1; }
    }
    } catch (const std::exception&) { std::cerr << "An option has a value that is not a number" << std::endl; return 1; }
    if (warm <= 0.0 || warm >= 1.0) { std::cerr << "Option -w must stay in ]0,1[" << std::endl; return 1; }
    bool density = (key == "density");

    InstanceData raw = parseFile(input_path);
    InstanceData in = reduce(raw);

    Index x = buildIndex(in);
    State st = makeState(x);

    // The score divides by the request count of the raw instance
    long long weight = 0;
    for (const Request& r : raw.requests) weight += r.count;
    auto shown = [&](long long t) { return weight ? t * 1000 / weight : 0; };

    // The candidate pairs of phase 2. They give the move its choices: which
    // videos are worth a cache, and which caches are worth a video.
    std::vector<Cand> cand = candidates(x, in, st, density);
    Rows by_cache = group(cand, x.C, true);
    Rows by_video = group(cand, x.V, false);

    // The caches and the videos that appear in at least one pair. A move that
    // draws outside of these lists can never improve the total.
    std::vector<int> live_cache, live_video;
    for (int c = 0; c < x.C; c++) if (by_cache.count(c) > 0) live_cache.push_back(c);
    for (int v = 0; v < x.V; v++) if (by_video.count(v) > 0) live_video.push_back(v);
    if (live_cache.empty()) { std::cerr << "[anneal] no reachable cache" << std::endl; return report("anneal", raw, st.sol, output_path) < 0 ? 1 : 0; }

    if (greedy_start) greedyLazy(x, st, cand, density);
    std::cout << "[anneal] start: " << shown(st.total) << (greedy_start ? " (greedy)" : " (empty)") << std::endl;

    // Membership table. `place()` and `unplace()` keep `st.sol`, this table
    // makes "does cache c hold video v" a constant time test.
    std::vector<char> holds((size_t)x.C * x.V, 0);
    for (int c = 0; c < x.C; c++) for (int v : st.sol[c]) holds[(size_t)c * x.V + v] = 1;

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::uniform_int_distribution<int> pick_cache(0, (int)live_cache.size() - 1);

    Move m{&st, &x, &holds, {}};

    /// @brief Draw a neighbour and apply it. The undo list stays in `m.log`.
    /// @return true if the move changed the state
    auto propose = [&]() -> bool {
        m.log.clear();
        bool transfer = coin(rng) < share;

        if (transfer) {
            // TRANSFER: take a video off a cache and try it on another cache.
            // This is the move that the ruin and recreate never makes directly.
            int c = live_cache[pick_cache(rng)];
            if (st.sol[c].empty()) return false;
            std::uniform_int_distribution<int> pick_v(0, (int)st.sol[c].size() - 1);
            int v = st.sol[c][pick_v(rng)];
            if (by_video.count(v) == 0) return false;

            std::uniform_int_distribution<int> pick_c2(by_video.begin(v), by_video.end(v) - 1);
            int c2 = by_video.val[pick_c2(rng)];
            if (c2 == c || m.has(v, c2)) return false;

            m.take(v, c);
            if (!makeRoom(m, rng, v, c2, evict, evict_sample)) { m.undo(); return false; }
            m.put(v, c2);
            return true;
        }

        // INSERT: put a candidate video on a cache, and free the room that it
        // needs. With no eviction this is the greedy move; the eviction makes
        // the exchange that the greedy solver cannot undo.
        int c = live_cache[pick_cache(rng)];
        int v = bestCandidate(x, st, by_cache, rng, c, insert_sample, density);
        if (v < 0) return false;
        if (m.has(v, c)) {
            // The video is already there: the opposite move, a plain removal.
            // It always loses, thus only the temperature can accept it.
            m.take(v, c);
            return true;
        }
        if (!makeRoom(m, rng, v, c, evict, evict_sample)) { m.undo(); return false; }
        m.put(v, c);
        return true;
    };

    // Calibration of T0: sample the neighbourhood and read the average loss of
    // a move that loses. T0 then accepts such a move with the probability -w.
    if (T0 <= 0.0) {
        long long losses = 0, n = 0;
        for (int i = 0; i < 4000; i++) {
            long long before = st.total;
            if (!propose()) continue;
            if (st.total < before) { losses += before - st.total; n++; }
            m.undo();
        }
        double mean = n ? (double)losses / (double)n : 1.0;
        T0 = mean / std::log(1.0 / warm);
        if (T0 <= 0.0) T0 = 1.0;
    }
    if (T1 <= 0.0) T1 = T0 * 1e-3;

    long long best = st.total;
    Solution best_sol = st.sol;

    auto start = std::chrono::steady_clock::now();
    long long iters = 0, moved = 0, accepted = 0, worse = 0;
    double elapsed = 0.0, T = T0;
    const double ratio = T1 / T0;

    while (elapsed < seconds) {
        // The clock is read once for each block. A move is far too short to
        // pay a clock read for each iteration.
        for (int block = 0; block < 4096; block++) {
            iters++;
            long long before = st.total;
            if (!propose()) continue;
            moved++;

            long long d = st.total - before;
            if (d >= 0 || coin(rng) < std::exp((double)d / T)) {
                accepted++;
                if (d < 0) worse++;
                if (st.total > best) { best = st.total; best_sol = st.sol; }
            } else {
                m.undo();
            }
        }
        elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        T = T0 * std::pow(ratio, std::min(1.0, elapsed / seconds));
    }

    // Guard: the running total must equal a full recomputation
    if (st.total != totalSaved(x, st)) std::cerr << "[anneal] WARNING: the running total drifted" << std::endl;

    std::cout << "[anneal] iterations: " << iters << ", moves: " << moved
              << ", accepted: " << accepted << " (" << worse << " worse)"
              << ", T0: " << T0 << ", T1: " << T1 << ", evict: " << evict
              << ", transfer: " << share << ", samples: " << insert_sample << "/" << evict_sample << std::endl;
    return report("anneal", raw, best_sol, output_path) < 0 ? 1 : 0;
}
