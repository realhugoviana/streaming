// Runs the team's own parser + greedy_density (+ optional local_search) unchanged and prints
// scores/timings as JSON; only main() is swapped out.
// usage: team_driver <solution_out> [local_search_iterations] < instance.in
#define main parser_main
#include "../parser.cpp"
#undef main
#include <chrono>
#include <fstream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // argv[1] = solution output path, argv[2] = local search iterations (0 = greedy only)
    int ls_iters = argc > 2 ? std::atoi(argv[2]) : 0;
    std::ios::sync_with_stdio(false);
    auto t0 = std::chrono::steady_clock::now();
    InstanceData instance = parser();
    auto t1 = std::chrono::steady_clock::now();
    greedy_density(&instance);
    auto t2 = std::chrono::steady_clock::now();
    long long g_score = instance.score, g_contest = computeContestScore(&instance);
    long long ls_ret = -1;
    if (ls_iters > 0) ls_ret = local_search(&instance, ls_iters);
    auto t3 = std::chrono::steady_clock::now();
    long long used = 0;
    for (int c = 0; c < instance.ip.C; c++) used += instance.ip.X - instance.caches[c].left_memory;
    auto ms = [](auto a, auto b){ return std::chrono::duration<double, std::milli>(b - a).count(); };
    std::printf("{\"Rdedup\":%d,\"sum_count\":%lld,\"greedy_weighted\":%lld,\"greedy_contest\":%lld,"
                "\"final_weighted\":%lld,\"final_contest\":%lld,\"check_weighted\":%lld,\"ls_ret\":%lld,"
                "\"fill\":%.6f,\"t_parse_ms\":%.1f,\"t_greedy_ms\":%.1f,\"t_ls_ms\":%.1f}\n",
                instance.ip.R, instance.sum_request_count, g_score, g_contest,
                instance.score, computeContestScore(&instance), computeTotalGain(&instance), ls_ret,
                (double)used / ((double)instance.ip.C * instance.ip.X), ms(t0,t1), ms(t1,t2), ms(t2,t3));
    if (argc > 1) {
        std::ofstream out(argv[1]);
        for (int c = 0; c < instance.ip.C; c++) {
            out << c;
            for (int v = 0; v < instance.ip.V; v++) if (instance.cache_affectation[v][c]) out << " " << v;
            out << "\n";
        }
    }
    return 0;
}
