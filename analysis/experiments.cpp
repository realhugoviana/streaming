// Algorithm-response experiments for the instance atlas:
//   greedy_density / greedy_gain : lazy (CELF) greedy, same marginal-gain rule as the team's greedy
//   kca                          : per-cache exact 0/1 knapsack re-optimisation ("rewrite one cache
//                                  optimally, others fixed"), swept until no cache improves
// Prints one JSON line. usage: experiments <instance> <mode> [init_solution|-] [out_solution|-] [max_sweeps]
#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

int V, E, R, C, X;
vector<int> sz, dc;
vector<vector<pair<int,int>>> econn;           // endpoint -> (cache, saving)
vector<vector<pair<int,int>>> cconn;           // cache -> (endpoint, saving)
vector<int> rv, re_; vector<ll> rn;
vector<vector<int>> vreq, ereq;                // video -> requests, endpoint -> requests
vector<vector<int>> Ssave;                     // E x C saving (0 = none), best link kept
vector<vector<char>> in;                       // V x C placement
vector<int> leftm;
vector<int> cur;                               // current best saving per request
vector<int> curc, cur2;                        // cache giving cur[], and 2nd best saving
ll total_count = 0;

void load(const char* path) {
    FILE* f = fopen(path, "r");
    auto rd = [&]() { int x; if (fscanf(f, "%d", &x) != 1) { fprintf(stderr, "parse error\n"); exit(1); } return x; };
    V = rd(); E = rd(); R = rd(); C = rd(); X = rd();
    sz.resize(V); for (auto& s : sz) s = rd();
    dc.resize(E); econn.assign(E, {}); cconn.assign(C, {}); Ssave.assign(E, vector<int>(C, 0));
    for (int e = 0; e < E; e++) {
        dc[e] = rd(); int k = rd();
        for (int i = 0; i < k; i++) { int c = rd(), l = rd(); Ssave[e][c] = max(Ssave[e][c], dc[e] - l); }
        for (int c = 0; c < C; c++) if (Ssave[e][c] > 0) { econn[e].push_back({c, Ssave[e][c]}); cconn[c].push_back({e, Ssave[e][c]}); }
    }
    unordered_map<ll, int> idx; idx.reserve(R * 2);
    for (int i = 0; i < R; i++) {
        int v = rd(), e = rd(), n = rd();
        ll key = (ll)v * E + e;
        auto it = idx.find(key);
        if (it == idx.end()) { idx[key] = rv.size(); rv.push_back(v); re_.push_back(e); rn.push_back(n); }
        else rn[it->second] += n;
        total_count += n;
    }
    fclose(f);
    R = rv.size();
    vreq.assign(V, {}); ereq.assign(E, {});
    for (int r = 0; r < R; r++) { vreq[rv[r]].push_back(r); ereq[re_[r]].push_back(r); }
    in.assign(V, vector<char>(C, 0)); leftm.assign(C, X); cur.assign(R, 0); curc.assign(R, -1); cur2.assign(R, 0);
}

ll score() { ll s = 0; for (int r = 0; r < R; r++) s += (ll)cur[r] * rn[r]; return s; }
ll contest(ll w) { return (ll)((__int128)w * 1000 / total_count); }

void refresh_video(int v) {                    // recompute top-2 savings for requests of v
    for (int r : vreq[v]) {
        int b = 0, bc = -1, b2 = 0;
        for (auto& [c, s] : econn[re_[r]]) if (in[v][c]) {
            if (s > b) { b2 = b; b = s; bc = c; } else if (s > b2) b2 = s;
        }
        cur[r] = b; curc[r] = bc; cur2[r] = b2;
    }
}

ll gain(int v, int c) {
    ll g = 0;
    for (int r : vreq[v]) { int s = Ssave[re_[r]][c]; if (s > cur[r]) g += (ll)(s - cur[r]) * rn[r]; }
    return g;
}

void greedy(bool density) {
    // max-heap on key; stale entries re-evaluated lazily (gains only decrease, space only shrinks)
    priority_queue<tuple<double, int, int>> pq;
    for (int v = 0; v < V; v++) {
        if (sz[v] > X) continue;
        for (int c = 0; c < C; c++) { ll g = gain(v, c); if (g > 0) pq.push({density ? (double)g / sz[v] : (double)g, v, c}); }
    }
    while (!pq.empty()) {
        auto [key, v, c] = pq.top(); pq.pop();
        if (in[v][c] || sz[v] > leftm[c]) continue;
        ll g = gain(v, c);
        if (g <= 0) continue;
        double k = density ? (double)g / sz[v] : (double)g;
        if (k < key - 1e-9 * max(1.0, key)) { pq.push({k, v, c}); continue; }
        in[v][c] = 1; leftm[c] -= sz[v];
        refresh_video(v);
    }
}

// exact re-optimisation of cache c given every other cache fixed
bool reopt_cache(int c) {
    // best saving for each request excluding cache c
    vector<ll> marg(V, 0);
    vector<char> touched(V, 0);
    for (auto& [e, s] : cconn[c])
        for (int r : ereq[e]) {
            int v = rv[r];
            int other = curc[r] == c ? cur2[r] : cur[r];
            if (s > other) { marg[v] += (ll)(s - other) * rn[r]; touched[v] = 1; }
        }
    vector<int> items;
    ll curval = 0;
    for (int v = 0; v < V; v++) {
        if (in[v][c]) curval += marg[v];
        if (touched[v] && marg[v] > 0 && sz[v] <= X) items.push_back(v);
    }
    int n = items.size();
    vector<ll> dp(X + 1, 0);
    vector<vector<uint64_t>> keep(n, vector<uint64_t>((X + 64) / 64, 0));
    for (int i = 0; i < n; i++) {
        int w = sz[items[i]]; ll val = marg[items[i]];
        for (int cap = X; cap >= w; cap--)
            if (dp[cap - w] + val > dp[cap]) { dp[cap] = dp[cap - w] + val; keep[i][cap >> 6] |= 1ULL << (cap & 63); }
    }
    if (dp[X] <= curval) return false;
    vector<char> take(V, 0);
    int cap = X;
    for (int i = n - 1; i >= 0; i--) if (keep[i][cap >> 6] >> (cap & 63) & 1) { take[items[i]] = 1; cap -= sz[items[i]]; }
    vector<int> changed;
    for (int v = 0; v < V; v++) if ((bool)in[v][c] != (bool)take[v]) { in[v][c] = take[v]; changed.push_back(v); }
    leftm[c] = X;
    for (int v = 0; v < V; v++) if (in[v][c]) leftm[c] -= sz[v];
    for (int v : changed) refresh_video(v);
    return true;
}

void read_sol(const char* path) {
    ifstream f(path); string line;
    while (getline(f, line)) {
        istringstream ss(line); int c, v;
        if (!(ss >> c)) continue;
        while (ss >> v) { in[v][c] = 1; leftm[c] -= sz[v]; }
    }
    for (int v = 0; v < V; v++) refresh_video(v);
}

void write_sol(const char* path) {
    ofstream f(path);
    for (int c = 0; c < C; c++) { f << c; for (int v = 0; v < V; v++) if (in[v][c]) f << " " << v; f << "\n"; }
}

int main(int argc, char** argv) {
    auto t0 = chrono::steady_clock::now();
    auto el = [&]() { return chrono::duration<double>(chrono::steady_clock::now() - t0).count(); };
    load(argv[1]);
    string mode = argv[2];
    double tl = el();
    if (argc > 3 && string(argv[3]) != "-") read_sol(argv[3]);
    ll start = score();
    printf("{\"mode\":\"%s\",\"load_s\":%.2f,\"start_contest\":%lld", mode.c_str(), tl, contest(start));
    if (mode == "greedy_density" || mode == "greedy_gain") {
        greedy(mode == "greedy_density");
    } else if (mode == "kca") {
        int max_sweeps = argc > 5 ? atoi(argv[5]) : 50;
        printf(",\"sweeps\":[");
        for (int sw = 0; sw < max_sweeps; sw++) {
            int improved = 0;
            for (int c = 0; c < C; c++) improved += reopt_cache(c);
            printf("%s[%d,%lld,%.1f]", sw ? "," : "", improved, contest(score()), el());
            fflush(stdout);
            if (!improved) break;
        }
        printf("]");
    }
    ll used = 0; for (int c = 0; c < C; c++) used += X - leftm[c];
    printf(",\"final_contest\":%lld,\"final_weighted\":%lld,\"fill\":%.6f,\"time_s\":%.2f}\n", contest(score()), score(), (double)used / ((double)C * X), el());
    if (argc > 4 && string(argv[4]) != "-") write_sol(argv[4]);
}
