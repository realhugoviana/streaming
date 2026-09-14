#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "../greedy.hpp"

/*
    LAGRANGIAN BOUND — THE LP RELAXATION WITHOUT THE LP

    `src/tools/lp_bound.py` writes the program of section 9.1 and gives it to
    HiGHS. The program has one `w` variable and one row for each incidence, thus
    it does not fit: kittens needs 75 million variables, and the simplex does
    not even finish on videos_worth_spreading.

    This program computes the same bound by decomposition. Relax the constraint
    `sum_c w[r,c] <= 1` with a multiplier `mu_r >= 0`:

        L(mu) = sum_r mu_r
              + max_{x feasible} sum_{v,c} x[v,c] * A[v,c](mu)

        A[v,c](mu) = sum_{r on v, c reaches e_r} max(0, count_r * g(r,c) - mu_r)

    The `w` variables disappear: for a fixed `x`, the best `w` takes a pair as
    soon as its reduced value `count_r * g(r,c) - mu_r` is positive.

    No constraint links two caches, so the maximum splits into one knapsack for
    each cache. The program uses the fractional knapsack, thus `L(mu)` is the
    value of the Lagrangian of the LP and:

      - `L(mu)` is a valid upper bound for EVERY `mu >= 0`, thus the program can
        stop at any iteration and still return a true bound;
      - `min_mu L(mu)` is the value of the LP relaxation itself, by duality.

    A subgradient walk lowers `L(mu)`. The subgradient of `mu_r` is
    `1 - sum_c x*[v_r,c]` over the caches whose reduced value is positive: it is
    positive when no cache serves the request, and negative when several do.
*/

/// @brief An item of a cache knapsack
struct Item { double value; int size; int v; };

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <instance.in> [-n iterations] [-l lower_bound_score]"
                  << " [-a step] [-v]" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    int iters = 300;
    double lb_score = 0.0, alpha = 2.0;
    bool verbose = false;
    try {
    for (int i = 2; i < argc; i++) {
        if (std::strcmp(argv[i], "-v") == 0) { verbose = true; continue; }
        if (i + 1 >= argc) { std::cerr << "Option '" << argv[i] << "' needs a value" << std::endl; return 1; }
        if (std::strcmp(argv[i], "-n") == 0) iters = std::stoi(argv[++i]);
        else if (std::strcmp(argv[i], "-l") == 0) lb_score = std::stod(argv[++i]);
        else if (std::strcmp(argv[i], "-a") == 0) alpha = std::stod(argv[++i]);
        else { std::cerr << "Unknown option '" << argv[i] << "'" << std::endl; return 1; }
    }
    } catch (const std::exception&) { std::cerr << "An option has a value that is not a number" << std::endl; return 1; }

    InstanceData raw = parseFile(path);
    InstanceData in = reduce(raw);
    Index x = buildIndex(in);

    long long weight = 0;
    for (const Request& r : raw.requests) weight += r.count;
    auto shown = [&](double t) { return t * 1000.0 / (double)weight; };

    const int R = (int)x.count_of.size();

    // The connections of each request, flattened once: (cache, value) where the
    // value is count_r * g(r,c). Rebuilding it every iteration would dominate.
    std::vector<int> off(R + 1, 0);
    for (int r = 0; r < R; r++)
        off[r + 1] = off[r] + (int)in.endpoints[x.endpoint_of[r]].caches_connections.size();
    std::vector<int> conn_c(off[R]);
    std::vector<double> conn_val(off[R]);
    std::vector<int> video_of(R, 0);
    for (int v = 0; v < x.V; v++) for (int r : x.reqs[v]) video_of[r] = v;
    for (int r = 0; r < R; r++) {
        int k = off[r];
        for (const EndpointCacheConnection& e : in.endpoints[x.endpoint_of[r]].caches_connections) {
            conn_c[k] = e.id_cache;
            conn_val[k] = (double)x.count_of[r] * (double)(x.dc_of[r] - e.latency);
            k++;
        }
    }

    // A[v,c] and the fractional choice x*[v,c], both dense
    std::vector<double> A((size_t)x.V * x.C, 0.0);
    std::vector<float> xstar((size_t)x.V * x.C, 0.0f);
    std::vector<double> mu(R, 0.0), sub(R, 0.0);
    std::vector<std::vector<Item>> items(x.C);

    double best = 1e300;
    double lb = lb_score * (double)weight / 1000.0;   // the best known solution, in total units

    for (int it = 0; it < iters; it++) {
        // A(mu). Only the pairs that a request reaches become non zero.
        std::fill(A.begin(), A.end(), 0.0);
        for (int r = 0; r < R; r++) {
            int v = video_of[r];
            for (int k = off[r]; k < off[r + 1]; k++) {
                double red = conn_val[k] - mu[r];
                if (red > 0) A[(size_t)v * x.C + conn_c[k]] += red;
            }
        }

        // One fractional knapsack for each cache
        std::fill(xstar.begin(), xstar.end(), 0.0f);
        double inner = 0.0;
        for (int c = 0; c < x.C; c++) {
            items[c].clear();
            for (int v = 0; v < x.V; v++) {
                double a = A[(size_t)v * x.C + c];
                if (a > 0 && x.size[v] > 0 && x.size[v] <= x.X) items[c].push_back({a, x.size[v], v});
            }
            std::sort(items[c].begin(), items[c].end(), [](const Item& a, const Item& b) {
                return a.value * b.size > b.value * a.size;   // decreasing density
            });
            long long room = x.X;
            for (const Item& t : items[c]) {
                if (room <= 0) break;
                if (t.size <= room) {
                    inner += t.value; room -= t.size;
                    xstar[(size_t)t.v * x.C + c] = 1.0f;
                } else {
                    double f = (double)room / (double)t.size;
                    inner += t.value * f; room = 0;
                    xstar[(size_t)t.v * x.C + c] = (float)f;
                }
            }
        }

        double sum_mu = 0.0;
        for (int r = 0; r < R; r++) sum_mu += mu[r];
        double L = sum_mu + inner;
        if (L < best) best = L;

        // Subgradient: 1 minus the part of the request that the caches serve
        double norm = 0.0;
        for (int r = 0; r < R; r++) {
            int v = video_of[r];
            double served = 0.0;
            for (int k = off[r]; k < off[r + 1]; k++)
                if (conn_val[k] - mu[r] > 0) served += xstar[(size_t)v * x.C + conn_c[k]];
            sub[r] = 1.0 - served;
            norm += sub[r] * sub[r];
        }
        if (norm <= 0) break;   // the relaxation is already feasible: L is the optimum

        // Polyak step towards the best known solution
        double target = lb > 0 ? lb : best * 0.9;
        double step = alpha * (L - target) / norm;
        if (step <= 0) step = 1e-9;
        for (int r = 0; r < R; r++) mu[r] = std::max(0.0, mu[r] - step * sub[r]);

        if (verbose && (it % 10 == 0 || it == iters - 1))
            std::printf("  it %4d  L = %12.1f   best = %12.1f\n", it, shown(L), shown(best));
        if (it > 0 && it % 50 == 0) alpha *= 0.7;   // the step must go to zero
    }

    std::printf("[lagrangian] LP bound <= %.1f\n", shown(best));
    return 0;
}
