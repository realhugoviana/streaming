#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "instance.hpp"

/// @brief A candidate solution: the list of video ids stored on each cache,
///        indexed by cache identifier.
using Solution = std::vector<std::vector<int>>;

/// @brief Build an empty solution sized for an instance
/// @param instance
/// @return One empty video list per cache server
inline Solution emptySolution(const InstanceData& instance) {
    return Solution(instance.gd.C);
}

/// @brief Total size used by a cache
/// @param videos Videos stored on the cache
/// @param instance
/// @return The occupied size, in Mo
inline long long usedSize(const std::vector<int>& videos, const InstanceData& instance) {
    long long used = 0;
    for (int id : videos) used += instance.size_of_video[id];
    return used;
}

/// @brief Check that a solution respects the capacity and identifier constraints
/// @param instance Must be the RAW instance (the filters drop videos and caches)
/// @param solution
/// @param error Filled with the reason on failure
/// @return true if the solution is submittable
inline bool validate(const InstanceData& instance, const Solution& solution, std::string& error) {
    // A solution cannot describe more caches than the instance declares
    if ((int)solution.size() > instance.gd.C) {
        error = "too many caches in the solution";
        return false;
    }

    for (int c = 0; c < (int)solution.size(); c++) {
        std::vector<int> seen = solution[c];
        std::sort(seen.begin(), seen.end());

        // A video cannot be stored twice on the same cache
        if (std::adjacent_find(seen.begin(), seen.end()) != seen.end()) {
            error = "cache " + std::to_string(c) + " stores a duplicated video";
            return false;
        }

        // Every identifier must exist
        for (int id : solution[c]) {
            if (id < 0 || id >= instance.gd.V) {
                error = "cache " + std::to_string(c) + " stores the unknown video " + std::to_string(id);
                return false;
            }
        }

        // And the cache must not overflow
        long long used = usedSize(solution[c], instance);
        if (used > instance.gd.X) {
            error = "cache " + std::to_string(c) + " overflows (" + std::to_string(used)
                  + "Mo > " + std::to_string(instance.gd.X) + "Mo)";
            return false;
        }
    }
    return true;
}

/// @brief Compute the official score of a solution
/// @param instance Must be the RAW instance: the score is averaged over every
///        request of the original problem, including the ones the filters drop
/// @param solution
/// @return The average time saved per request, in microseconds (floored)
inline long long score(const InstanceData& instance, const Solution& solution) {
    // Lookup table: does cache c hold video v ? Stored per cache as a flat
    // boolean row so the request loop stays a constant time membership test.
    std::vector<std::vector<char>> holds(solution.size());
    for (size_t c = 0; c < solution.size(); c++) {
        if (solution[c].empty()) continue;
        holds[c].assign(instance.gd.V, 0);
        for (int id : solution[c]) holds[c][id] = 1;
    }

    long long total_saved = 0;
    long long total_requests = 0;

    for (const Request& r : instance.requests) {
        total_requests += r.count;
        const Endpoint& e = instance.endpoints[r.id_endpoint];

        // Best latency available for this (video, endpoint) pair: the
        // datacenter by default, a connected cache holding the video if better
        int best = e.dc_latency;
        for (const EndpointCacheConnection& c : e.caches_connections) {
            if (c.id_cache < (int)holds.size() && !holds[c.id_cache].empty()
                && holds[c.id_cache][r.id_video] && c.latency < best) {
                best = c.latency;
            }
        }

        total_saved += (long long)r.count * (e.dc_latency - best);
    }

    if (total_requests == 0) return 0;

    // The statement asks for the average saving in microseconds, rounded down
    return (total_saved * 1000) / total_requests;
}

/// @brief Write a solution in the submission format
/// @param path Destination file
/// @param solution
inline void writeSolution(const std::string& path, const Solution& solution) {
    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "Error: could not write to '" << path << "'" << std::endl;
        std::exit(1);
    }

    // Only the caches that actually store something are described
    std::vector<int> used;
    for (int c = 0; c < (int)solution.size(); c++) {
        if (!solution[c].empty()) used.push_back(c);
    }

    out << used.size() << "\n";
    for (int c : used) {
        out << c;
        for (int id : solution[c]) out << " " << id;
        out << "\n";
    }
}

/// @brief Report a solution: validity, score and destination file
/// @param name Name of the solver
/// @param instance The RAW instance
/// @param solution
/// @param output_path Where to write the solution, empty to skip writing
/// @return The score, or -1 if the solution is invalid
inline long long report(const std::string& name, const InstanceData& instance,
                        const Solution& solution, const std::string& output_path) {
    std::string error;
    if (!validate(instance, solution, error)) {
        std::cerr << "[" << name << "] INVALID solution: " << error << std::endl;
        return -1;
    }

    long long s = score(instance, solution);
    std::cout << "[" << name << "] score: " << s << std::endl;

    if (!output_path.empty()) {
        writeSolution(output_path, solution);
        std::cout << "[" << name << "] solution written to " << output_path << std::endl;
    }
    return s;
}
