#include <chrono>
#include <cstring>
#include <random>
#include <string>

#include "../greedy.hpp"

/*
    LOCAL SEARCH — RUIN AND RECREATE

    The greedy solver commits to a video early. That video can become redundant
    when a later placement serves the same requests. This solver repairs that.

    Each iteration empties some caches, then fills them again with the lazy
    greedy. The rest of the solution stays. Thus the greedy sees a different
    context and it can make a different choice. The iteration keeps the result
    only if the total gets better.
*/

/// @brief The candidate videos of each cache, in a compressed row layout
struct ByCache { std::vector<int> off, vid; };

/// @brief Group the pairs by cache. This gives the recreate step its candidates.
ByCache groupByCache(const std::vector<Cand>& cand, int C) {
    ByCache b;
    b.off.assign(C + 1, 0);
    for (const Cand& p : cand) b.off[p.c + 1]++;
    for (int c = 0; c < C; c++) b.off[c + 1] += b.off[c];

    std::vector<int> at = b.off;
    b.vid.resize(cand.size());
    for (const Cand& p : cand) b.vid[at[p.c]++] = p.v;
    return b;
}

/// @brief Build the pairs that can fill the free space of the ruined caches
/// @note  The step computes a gain for every candidate video. A shorter list is
///        not possible. Section 6.2 of the document gives the measurements.
std::vector<Cand> refill(const Index& x, const State& st, const ByCache& b,
                         const std::vector<int>& ruined, bool density,
                         const std::vector<double>& noise) {
    std::vector<Cand> out;
    for (int c : ruined)
        for (int i = b.off[c]; i < b.off[c + 1]; i++) {
            int v = b.vid[i];
            if (x.size[v] > st.residual[c]) continue;
            long long g = gain(x, st, v, c);
            if (g > 0) out.push_back({keyOf(g, x.size[v], density) * noise[v], v, c});
        }
    return out;
}

/*
    MAIN
*/
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in> [-o out.txt] [-t seconds] [-r caches] [-p prob] [-a noise] [-e accept] [-c cap] [-s seed] [-k gain|density]" << std::endl;
        return 1;
    }

    std::string input_path = argv[1], output_path, key = "density";
    double seconds = 10.0, prob = 1.0, amp = 0.0, eps = 0.0;
    int ruin_count = 0;
    unsigned int seed = 42;
    try {
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 >= argc) { std::cerr << "Option '" << argv[i] << "' needs a value" << std::endl; return 1; }
        if (std::strcmp(argv[i], "-o") == 0) output_path = argv[i + 1];
        else if (std::strcmp(argv[i], "-t") == 0) seconds = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-r") == 0) ruin_count = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-p") == 0) prob = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-a") == 0) amp = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-e") == 0) eps = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-s") == 0) seed = (unsigned int)std::stoul(argv[i + 1]);
        else if (std::strcmp(argv[i], "-k") == 0) key = argv[i + 1];
        else { std::cerr << "Unknown option '" << argv[i] << "'" << std::endl; return 1; }
    }
    } catch (const std::exception&) { std::cerr << "An option has a value that is not a number" << std::endl; return 1; }
    bool density = (key == "density");

    InstanceData raw = parseFile(input_path);
    InstanceData in = reduce(raw);

    Index x = buildIndex(in);
    State st = makeState(x);

    // Start from the phase 2 solution
    std::vector<Cand> cand = candidates(x, in, st, density);
    ByCache b = groupByCache(cand, x.C);
    greedyLazy(x, st, std::move(cand), density);

    // The score divides by the request count of the raw instance
    long long weight = 0;
    for (const Request& r : raw.requests) weight += r.count;

    long long best = st.total;
    Solution best_sol = st.sol;
    std::cout << "[local] greedy start: " << (weight ? best * 1000 / weight : 0) << std::endl;

    // Ruin 5 % of the caches by default, and never less than one cache
    if (ruin_count <= 0) ruin_count = std::max(1, x.C / 20);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(0, x.C - 1);
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    auto start = std::chrono::steady_clock::now();
    long long iters = 0, improves = 0;
    std::vector<std::pair<int,int>> removed, added;
    std::vector<int> ruined;
    std::vector<double> noise(x.V, 1.0);

    double elapsed = 0.0;
    while (elapsed < seconds) {
        elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        iters++;
        long long before = st.total;

        // The limit goes down to zero at the end of the budget. The search thus
        // explores at the start and only climbs at the end.
        long long slack = (long long)(eps * (double)best * (1.0 - elapsed / seconds));
        removed.clear(); added.clear(); ruined.clear();

        // Ruin: take some caches and remove a part of their videos
        for (int i = 0; i < ruin_count && (int)ruined.size() < x.C; i++) {
            int c = pick(rng);
            while (std::find(ruined.begin(), ruined.end(), c) != ruined.end()) c = pick(rng);
            ruined.push_back(c);
            std::vector<int> here = st.sol[c];
            for (int v : here) if (coin(rng) < prob) { unplace(x, st, v, c); removed.push_back({v, c}); }
        }

        // Recreate: the lazy greedy fills the free space again. The noise makes
        // the same ruin give a different fill, thus the search does not stop.
        for (double& n : noise) n = std::max(0.01, 1.0 + amp * (coin(rng) * 2.0 - 1.0));
        greedyLazy(x, st, refill(x, st, b, ruined, density, noise), density, &added, &noise);

        // Keep the best solution apart, but only when the search can go down.
        // With no slack the current solution is always the best one.
        if (st.total > best) { best = st.total; if (eps > 0) best_sol = st.sol; improves++; }

        // Accept a result that is not worse than the limit. If not, undo.
        if (st.total >= before - slack) continue;
        for (auto it = added.rbegin(); it != added.rend(); ++it) unplace(x, st, it->first, it->second);
        for (auto it = removed.rbegin(); it != removed.rend(); ++it) place(x, st, it->first, it->second);
    }

    // Guard: the running total must equal a full recomputation
    if (st.total != totalSaved(x, st)) std::cerr << "[local] WARNING: the running total drifted" << std::endl;

    std::cout << "[local] iterations: " << iters << ", improvements: " << improves
              << ", ruin: " << ruin_count << " caches, prob: " << prob << ", noise: " << amp << ", accept: " << eps << std::endl;
    return report("local", raw, eps > 0 ? best_sol : st.sol, output_path) < 0 ? 1 : 0;
}
