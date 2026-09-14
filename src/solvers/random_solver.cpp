#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "../instance.hpp"
#include "../solution.hpp"

/*
    RANDOM BASELINE

    For every cache server, walk a fresh random permutation of the candidate
    videos and store each one that still fits. This gives a uniformly random,
    always full (first-fit saturated) assignment: the reference every smarter
    solver has to beat.
*/

/// @brief Build one random assignment
/// @param instance The reduced instance (its video_sizes are the candidates)
/// @param raw The raw instance, for the cache count and sizes
/// @param rng Random generator
/// @return The random solution
Solution randomSolution(const InstanceData& instance, const InstanceData& raw, std::mt19937& rng) {
    Solution solution = emptySolution(raw);

    // Candidate videos: the ones that fit in a cache at all
    std::vector<int> candidates;
    for (const Video& v : instance.video_sizes) candidates.push_back(v.id_video);

    // Only caches reachable by at least one endpoint are worth filling
    for (const auto& entry : instance.caches) {
        int id_cache = entry.first;

        std::shuffle(candidates.begin(), candidates.end(), rng);

        // First fit along the random order until the cache is saturated
        long long remaining = raw.gd.X;
        for (int id : candidates) {
            int size = raw.size_of_video[id];
            if (size <= remaining) {
                solution[id_cache].push_back(id);
                remaining -= size;
            }
        }
    }
    return solution;
}

/*
    MAIN
*/
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_input_file.in> [-o out.txt] [-n iterations] [-s seed]" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path;
    int iterations = 1;
    unsigned int seed = (unsigned int)std::chrono::steady_clock::now().time_since_epoch().count();

    // Parse the optional flags
    for (int i = 2; i + 1 < argc; i += 2) {
        if (std::strcmp(argv[i], "-o") == 0) output_path = argv[i + 1];
        else if (std::strcmp(argv[i], "-n") == 0) iterations = std::stoi(argv[i + 1]);
        else if (std::strcmp(argv[i], "-s") == 0) seed = (unsigned int)std::stoul(argv[i + 1]);
        else {
            std::cerr << "Unknown option '" << argv[i] << "'" << std::endl;
            return 1;
        }
    }

    // The score is always measured on the raw instance, the solver only works
    // on the reduced one
    InstanceData raw = parseFile(input_path);
    InstanceData instance = reduce(raw);

    std::mt19937 rng(seed);

    // Keep the best draw out of the requested number of iterations
    Solution best;
    long long best_score = -1;
    for (int i = 0; i < iterations; i++) {
        Solution candidate = randomSolution(instance, raw, rng);
        long long s = score(raw, candidate);
        if (s > best_score) {
            best_score = s;
            best = candidate;
        }
    }

    std::cout << "[random] seed: " << seed << ", iterations: " << iterations << std::endl;
    return report("random", raw, best, output_path) < 0 ? 1 : 0;
}
