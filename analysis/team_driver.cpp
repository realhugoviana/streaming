// Runs one of the team's algorithms with its own code (parser.cpp + solvers, only main() is swapped)
// and prints scores/timings as JSON.
// usage: team_driver <solution_out> <algorithm> [seconds, default 600] < instance
//   greedy       greedy_density, always the best pair (deterministic)
//   greedy90     greedy_density, best pair with probability 0.9, otherwise the runner-up
//   rr           greedy90, then ruin & recreate until the time is up: each iteration empties C/3
//                randomly drawn caches (ruin probability 1) and refills them with greedy90
//   rr_knapsack  as parser.cpp's main: greedy90, then rounds of 30 ruin & recreate iterations
//                followed by knapsack_reopt, until the time is up
// The time counts from the start, parsing included, like the contest's timeout.
#define main parser_main
#include "../parser.cpp"
#undef main
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>

const double P_BEST = 0.9;      // greedy: probability of placing the best pair
const double P_RUIN = 1.0;      // ruin: probability of removing each video of a drawn cache
const int RR_PER_KNAPSACK = 30; // rr_knapsack: ruin & recreate iterations between two knapsack_reopt

static void write_solution(const InstanceData& instance, const char* path) {
    std::ofstream out(path);
    for (int c = 0; c < instance.ip.C; c++) {
        out << c;
        for (int v = 0; v < instance.ip.V; v++) if (instance.cache_affectation[v][c]) out << " " << v;
        out << "\n";
    }
}

static double fill(const InstanceData& instance) {
    long long used = 0;
    for (int c = 0; c < instance.ip.C; c++) used += instance.ip.X - instance.caches[c].left_memory;
    return (double)used / ((double)instance.ip.C * instance.ip.X);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { std::fprintf(stderr, "usage: team_driver <solution_out> <greedy|greedy90|rr|rr_knapsack> [seconds] < instance\n"); return 2; }
    std::string algo = argv[2];
    double budget = argc > 3 ? std::atof(argv[3]) : 600;
    if (algo != "greedy" && algo != "greedy90" && algo != "rr" && algo != "rr_knapsack") { std::fprintf(stderr, "unknown algorithm %s\n", argv[2]); return 2; }
    std::ios::sync_with_stdio(false);
    auto t0 = std::chrono::steady_clock::now();
    auto elapsed = [&]() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };

    InstanceData instance = parser();
    double t_parse = elapsed();
    greedy_density(&instance, algo == "greedy" ? 1.0 : P_BEST);
    double t_greedy = elapsed() - t_parse;
    long long greedy_contest = computeContestScore(&instance);

    // Score over time, one point per improvement (both moves only keep improvements)
    std::vector<std::pair<double, long long>> trace = {{elapsed(), greedy_contest}};
    auto record = [&]() { long long s = computeContestScore(&instance); if (s != trace.back().second) trace.push_back({elapsed(), s}); };
    int num_moves = std::max(1, instance.ip.C / 3);
    long long rr_iterations = 0, knapsack_rounds = 0;
    if (algo == "rr") {
        while (elapsed() < budget) { ruin_recreate(&instance, 1, num_moves, P_BEST, P_RUIN); rr_iterations++; record(); }
    } else if (algo == "rr_knapsack") {
        while (elapsed() < budget) {
            for (int i = 0; i < RR_PER_KNAPSACK && elapsed() < budget; i++) { ruin_recreate(&instance, 1, num_moves, P_BEST, P_RUIN); rr_iterations++; record(); }
            if (elapsed() >= budget) break;
            knapsack_reopt(&instance, 50, budget - elapsed()); knapsack_rounds++; record();
        }
    }

    std::printf("{\"algorithm\":\"%s\",\"budget_s\":%.0f,\"Rdedup\":%d,\"sum_count\":%lld,\"greedy_contest\":%lld,"
                "\"final_contest\":%lld,\"final_weighted\":%lld,\"check_weighted\":%lld,\"fill\":%.6f,"
                "\"rr_iterations\":%lld,\"knapsack_rounds\":%lld,\"t_parse_s\":%.3f,\"t_greedy_s\":%.3f,\"t_total_s\":%.2f,\"trace\":[",
                algo.c_str(), budget, instance.ip.R, instance.sum_request_count, greedy_contest,
                computeContestScore(&instance), instance.score, computeTotalGain(&instance), fill(instance),
                rr_iterations, knapsack_rounds, t_parse, t_greedy, elapsed());
    for (size_t i = 0; i < trace.size(); i++) std::printf("%s[%.2f,%lld]", i ? "," : "", trace[i].first, trace[i].second);
    std::printf("]}\n");
    write_solution(instance, argv[1]);
    return 0;
}
