#include <chrono>
#include <cstring>
#include <random>
#include <string>

#include "../greedy.hpp"

/*
    MEMETIC GENETIC ALGORITHM

    Phase 5 showed that a move on one video is too weak on the large instances.
    The strength of the ruin and recreate is that it optimises a whole cache at
    the same time, with the knowledge of every candidate. A genetic algorithm
    must therefore keep that neighbourhood, not replace it.

    The crossover uses a property of the problem. A solution is one list of
    videos for each cache, and each cache respects its capacity alone. No
    constraint links two caches. Therefore a child that takes the content of
    each cache from one parent or the other is always feasible. The crossover
    needs no repair. This is not true of most assignment problems.

    One generation:

    1. Select two parents with a tournament.
    2. Cross them: each cache comes from one parent, drawn at random.
    3. Refill: the mixed caches can leave free space. The lazy greedy fills it.
    4. Mutate: some ruin and recreate iterations improve the child.
    5. Replace the worst individual, if the child is better and is new.
*/

/// @brief The candidate videos of each cache, in a compressed row layout
struct ByCache { std::vector<int> off, vid; };

/// @brief Group the pairs by cache. This gives the refill step its candidates.
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

/// @brief Build the pairs that can fill the free space of some caches
std::vector<Cand> refill(const Index& x, const State& st, const ByCache& b,
                         const std::vector<int>& targets, bool density,
                         const std::vector<double>* noise) {
    std::vector<Cand> out;
    for (int c : targets)
        for (int i = b.off[c]; i < b.off[c + 1]; i++) {
            int v = b.vid[i];
            if (x.size[v] > st.residual[c]) continue;
            long long g = gain(x, st, v, c);
            if (g > 0) {
                double k = keyOf(g, x.size[v], density);
                out.push_back({noise ? k * (*noise)[v] : k, v, c});
            }
        }
    return out;
}

/// @brief Rebuild a full state from a solution
/// @note  `place()` keeps the running total correct, thus the rebuilt state
///        carries its own total and needs no separate scoring pass.
State stateOf(const Index& x, const Solution& sol) {
    State st = makeState(x);
    for (int c = 0; c < (int)sol.size(); c++)
        for (int v : sol[c]) place(x, st, v, c);
    return st;
}

/// @brief One individual of the population
struct Individual {
    Solution sol;
    long long fit = 0;
};

/// @brief Some ruin and recreate iterations on a state. This is the local
///        improvement that makes the algorithm memetic.
/// @return The count of iterations that improved the total
int improve(const Index& x, State& st, const ByCache& b, std::mt19937& rng,
            int rounds, int ruin_count, double prob, bool density) {
    std::uniform_int_distribution<int> pick(0, x.C - 1);
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::vector<std::pair<int,int>> removed, added;
    std::vector<int> ruined;
    int improves = 0;

    for (int it = 0; it < rounds; it++) {
        long long before = st.total;
        removed.clear(); added.clear(); ruined.clear();

        // Ruin: empty a part of some caches
        for (int i = 0; i < ruin_count && (int)ruined.size() < x.C; i++) {
            // A duplicate must not consume a ruin attempt. The guard on
            // `ruined.size()` makes sure a free cache exists, thus this
            // loop always ends.
            int c = pick(rng);
            while (std::find(ruined.begin(), ruined.end(), c) != ruined.end()) c = pick(rng);
            ruined.push_back(c);
            std::vector<int> here = st.sol[c];
            for (int v : here) if (coin(rng) < prob) { unplace(x, st, v, c); removed.push_back({v, c}); }
        }

        // Recreate, then keep the result only if the total got better
        greedyLazy(x, st, refill(x, st, b, ruined, density, nullptr), density, &added, nullptr);
        if (st.total > before) { improves++; continue; }
        for (auto it2 = added.rbegin(); it2 != added.rend(); ++it2) unplace(x, st, it2->first, it2->second);
        for (auto it2 = removed.rbegin(); it2 != removed.rend(); ++it2) place(x, st, it2->first, it2->second);
    }
    return improves;
}

/*
    MAIN
*/
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in> [-o out.txt] [-t seconds]"
                  << " [-P population] [-M mutation_rounds] [-r ruin] [-p prob] [-a init_noise]"
                  << " [-T tournament] [-s seed] [-k gain|density]" << std::endl;
        return 1;
    }

    std::string input_path = argv[1], output_path, key = "density";
    double seconds = 60.0, prob = 1.0, amp = 0.6;
    int pop_size = 12, rounds = 40, ruin_count = 0, tour = 3;
    unsigned int seed = 42;
    try {
    for (int i = 2; i < argc; i += 2) {
        if (i + 1 >= argc) { std::cerr << "Option '" << argv[i] << "' needs a value" << std::endl; return 1; }
        if (std::strcmp(argv[i], "-o") == 0) output_path = argv[i + 1];
        else if (std::strcmp(argv[i], "-t") == 0) seconds = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-P") == 0) pop_size = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-M") == 0) rounds = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-r") == 0) ruin_count = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-p") == 0) prob = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-a") == 0) amp = std::stod(argv[i + 1]);
        else if (std::strcmp(argv[i], "-T") == 0) tour = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-s") == 0) seed = (unsigned int)std::stoul(argv[i + 1]);
        else if (std::strcmp(argv[i], "-k") == 0) key = argv[i + 1];
        else { std::cerr << "Unknown option '" << argv[i] << "'" << std::endl; return 1; }
    }
    } catch (const std::exception&) { std::cerr << "An option has a value that is not a number" << std::endl; return 1; }
    if (pop_size < 2) { std::cerr << "Option -P needs at least 2" << std::endl; return 1; }
    bool density = (key == "density");

    InstanceData raw = parseFile(input_path);
    InstanceData in = reduce(raw);

    Index x = buildIndex(in);
    State empty = makeState(x);

    long long weight = 0;
    for (const Request& r : raw.requests) weight += r.count;
    auto shown = [&](long long t) { return weight ? t * 1000 / weight : 0; };

    std::vector<Cand> cand = candidates(x, in, empty, density);
    ByCache b = groupByCache(cand, x.C);
    if (ruin_count <= 0) ruin_count = std::max(1, x.C / 20);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    auto start = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };

    /*
        INITIAL POPULATION

        Individual 0 is the plain greedy solution, thus the algorithm never
        returns less than phase 2. The others use a noise vector on the keys,
        which gives a different greedy order and thus a different basin. This
        is the "different start" that the phase 4 document asked for.
    */
    std::vector<Individual> pop;
    std::vector<double> noise(x.V, 1.0);
    for (int i = 0; i < pop_size; i++) {
        State st = makeState(x);
        if (i == 0) {
            greedyLazy(x, st, cand, density);
        } else {
            for (double& n : noise) n = std::max(0.01, 1.0 + amp * (coin(rng) * 2.0 - 1.0));
            std::vector<Cand> seeded = cand;
            for (Cand& p : seeded) p.key *= noise[p.v];
            greedyLazy(x, st, std::move(seeded), density, nullptr, &noise);
        }
        pop.push_back({st.sol, st.total});
    }

    int best_i = 0;
    for (int i = 1; i < pop_size; i++) if (pop[i].fit > pop[best_i].fit) best_i = i;
    std::cout << "[genetic] initial population: best " << shown(pop[best_i].fit)
              << ", worst " << shown(std::min_element(pop.begin(), pop.end(),
                     [](const Individual& a, const Individual& c){ return a.fit < c.fit; })->fit)
              << " (greedy " << shown(pop[0].fit) << ")" << std::endl;

    /// @brief Pick the best individual of `tour` random draws
    auto select = [&]() {
        std::uniform_int_distribution<int> pick(0, pop_size - 1);
        int best = pick(rng);
        for (int t = 1; t < tour; t++) { int c = pick(rng); if (pop[c].fit > pop[best].fit) best = c; }
        return best;
    };

    long long gens = 0, kept = 0;
    std::vector<int> holes;

    while (elapsed() < seconds) {
        gens++;
        int pa = select(), pb = select();

        // CROSSOVER. Each cache comes whole from one parent. Every cache of a
        // parent respects the capacity alone, thus the child is always feasible.
        Solution child(x.C);
        for (int c = 0; c < x.C; c++) child[c] = (coin(rng) < 0.5 ? pop[pa].sol[c] : pop[pb].sol[c]);

        State st = stateOf(x, child);

        // REFILL. A mixed solution can serve a request twice and leave free
        // space elsewhere. Only the caches that kept room need the greedy.
        holes.clear();
        for (int c = 0; c < x.C; c++) if (st.residual[c] > 0) holes.push_back(c);
        greedyLazy(x, st, refill(x, st, b, holes, density, nullptr), density, nullptr, nullptr);

        // MUTATION. Ruin and recreate improves the child in its own basin.
        improve(x, st, b, rng, rounds, ruin_count, prob, density);

        // REPLACEMENT. The child takes the place of the worst individual, if it
        // is better. A fitness that the population already holds is refused:
        // it would make every individual the same and stop the search.
        int worst = 0;
        bool clone = false;
        for (int i = 0; i < pop_size; i++) {
            if (pop[i].fit < pop[worst].fit) worst = i;
            if (pop[i].fit == st.total) clone = true;
        }
        if (!clone && st.total > pop[worst].fit) {
            pop[worst] = {st.sol, st.total};
            kept++;
            if (st.total > pop[best_i].fit) best_i = worst;
        }
        // `best_i` can point at a replaced slot, so find the best again
        for (int i = 0; i < pop_size; i++) if (pop[i].fit > pop[best_i].fit) best_i = i;
    }

    // Guard: the stored fitness must equal a full recomputation
    State check = stateOf(x, pop[best_i].sol);
    if (check.total != pop[best_i].fit) std::cerr << "[genetic] WARNING: the fitness drifted" << std::endl;

    std::cout << "[genetic] generations: " << gens << ", children kept: " << kept
              << ", population: " << pop_size << ", mutation rounds: " << rounds
              << ", ruin: " << ruin_count << ", noise: " << amp << std::endl;
    return report("genetic", raw, pop[best_i].sol, output_path) < 0 ? 1 : 0;
}
