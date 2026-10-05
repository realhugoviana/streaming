"""Extract structural features + bounds for every instance, and structure of the greedy solution."""
import json
import numpy as np
from inst import Instance, INSTANCES, read_solution


def gini(x):
    x = np.sort(np.asarray(x, dtype=np.float64))
    if x.sum() == 0:
        return 0.0
    n = len(x)
    return float((2 * np.arange(1, n + 1) - n - 1) @ x / (n * x.sum()))


def q(x, qs=(0, 0.25, 0.5, 0.75, 1)):
    return [float(v) for v in np.quantile(x, qs)] if len(x) else []


def top_share(w, frac):
    w = np.sort(w)[::-1]
    k = max(1, int(round(frac * len(w))))
    return float(w[:k].sum() / w.sum())


def frac_knapsack(values, sizes, cap):
    """Fractional knapsack optimum (upper bound on the 0/1 one)."""
    m = (values > 0)
    v, s = values[m], sizes[m].astype(np.float64)
    order = np.argsort(-v / s)
    v, s = v[order], s[order]
    cs = np.cumsum(s)
    full = cs <= cap
    tot = v[full].sum()
    k = full.sum()
    if k < len(v):
        rem = cap - (cs[k - 1] if k else 0)
        tot += v[k] * rem / s[k]
    return float(tot), int(k), float(s.sum())


def analyse(name):
    I = Instance(name)
    V, E, C, X = I.V, I.E, I.C, I.X
    f = {"name": name, "V": V, "E": E, "R_raw": I.R_raw, "R_dedup": I.R, "C": C, "X": X}
    f["dup_request_rate"] = 1 - I.R / I.R_raw
    f["total_count"] = I.total_count

    # ---------- videos
    s = I.size
    f["size_q"] = q(s)
    f["size_mean"] = float(s.mean()); f["size_cv"] = float(s.std() / s.mean())
    f["size_mean_over_X"] = float(s.mean() / X)
    f["frac_videos_bigger_than_X"] = float((s > X).mean())
    f["frac_videos_size1"] = float((s == 1).mean())
    pop = np.bincount(I.rv, weights=I.rn, minlength=V)          # total count per video
    requested = pop > 0
    f["frac_videos_requested"] = float(requested.mean())
    f["video_pop_gini"] = gini(pop[requested])
    f["top1pct_video_share"] = top_share(pop[requested], 0.01)
    f["top10pct_video_share"] = top_share(pop[requested], 0.10)
    ep_per_video = np.bincount(I.rv, minlength=V)
    f["endpoints_per_requested_video_q"] = q(ep_per_video[requested])
    f["corr_size_popularity"] = float(np.corrcoef(s[requested], pop[requested])[0, 1])

    # ---------- endpoints / links
    f["dc_q"] = q(I.dc)
    f["K_q"] = q(I.K)
    f["K_mean"] = float(I.K.mean())
    iso = I.K == 0
    f["frac_endpoints_K0"] = float(iso.mean())
    f["frac_demand_from_K0"] = float(I.rn[iso[I.re]].sum() / I.total_count)
    sav = I.dc[I.conn_e] - I.conn_lat
    f["lat_q"] = q(I.conn_lat)
    f["saving_q"] = q(sav)
    f["rel_saving_q"] = q(sav / I.dc[I.conn_e])
    # per-endpoint: best saving, and how much you lose using the WORST / 2nd best connected cache
    Smax = I.S.max(1)
    Spos = np.where(I.S > 0, I.S, np.nan)
    with np.errstate(all="ignore"):
        Smin = np.nanmin(Spos, 1)
        ssort = -np.sort(-I.S, 1)
        second = ssort[:, 1] if C > 1 else np.zeros(E)
    conn = ~iso
    dem_e = np.bincount(I.re, weights=I.rn, minlength=E)
    w = dem_e[conn] / dem_e[conn].sum() if dem_e[conn].sum() else None
    f["worst_over_best_saving_demandw"] = float(np.average(Smin[conn] / Smax[conn], weights=w))
    f["second_over_best_saving_demandw"] = float(np.average(second[conn] / Smax[conn], weights=w))
    f["requests_per_endpoint_q"] = q(np.bincount(I.re, minlength=E))
    f["demand_per_endpoint_gini"] = gini(dem_e)

    # ---------- caches
    deg = (I.S > 0).sum(0)
    f["cache_degree_q"] = q(deg)
    f["frac_caches_unconnected"] = float((deg == 0).mean())
    B = (I.S > 0).astype(np.float64)
    inter = B.T @ B
    union = deg[:, None] + deg[None, :] - inter
    iu = np.triu_indices(C, 1)
    with np.errstate(all="ignore"):
        jac = np.where(union[iu] > 0, inter[iu] / union[iu], 0)
    f["cache_pair_jaccard_mean"] = float(jac.mean()) if len(jac) else 0.0
    f["cache_pair_jaccard_max"] = float(jac.max()) if len(jac) else 0.0
    f["frac_cache_pairs_sharing_endpoint"] = float((inter[iu] > 0).mean()) if len(jac) else 0.0
    # demand-weighted number of caches reachable by a request (redundancy)
    f["caches_per_unit_demand"] = float(np.average(I.K[I.re], weights=I.rn))

    # ---------- capacity pressure
    reach = I.K[I.re] > 0                                  # requests that can be improved at all
    vid_cacheable = np.zeros(V, bool); vid_cacheable[I.rv[reach]] = True
    vid_cacheable &= s <= X
    f["catalogue_volume_cacheable"] = int(s[vid_cacheable].sum())
    f["total_capacity"] = C * X
    f["capacity_over_catalogue"] = C * X / max(1, s[vid_cacheable].sum())
    g = I.gain_matrix()                                    # V x C standalone gains
    g[s > X, :] = 0
    local_press = []
    ub_knap = 0.0
    items_fit = []
    for c in range(C):
        tot, k, vol = frac_knapsack(g[:, c], s, X)
        ub_knap += tot
        local_press.append(vol / X)
        items_fit.append(k)
    f["per_cache_candidate_volume_over_X_q"] = q(local_press)
    f["per_cache_items_in_frac_knapsack_q"] = q(items_fit)
    # "space-weighted" value: how much of the standalone value fits in X
    # ---------- bounds
    best_sav = I.S[I.re].max(1) if C else np.zeros(I.R)
    best_sav = np.where(s[I.rv] <= X, best_sav, 0)
    ub0 = float((best_sav * I.rn).sum())
    f["UB_uncapacitated"] = I.contest(ub0)
    f["UB_knapsack_sum"] = I.contest(ub_knap)
    f["UB_best_simple"] = min(f["UB_uncapacitated"], f["UB_knapsack_sum"])
    # knapsack bound over-counts duplicates: ratio shows how much cache overlap matters
    f["knap_over_uncap"] = ub_knap / ub0 if ub0 else 0.0

    # ---------- value density (what greedy_density sorts on)
    gpos = g[g > 0]
    dens = (g / s[:, None])[g > 0]
    f["pairs_with_positive_gain"] = int(len(gpos))
    f["density_cv"] = float(dens.std() / dens.mean())
    f["density_gini"] = gini(dens)

    # ---------- greedy solution structure
    sol = read_solution(f"sol/{name}.greedy.txt")
    wsc, used, best, Xm = I.score_solution(sol)
    f["greedy_contest"] = I.contest(wsc)
    f["greedy_gap_to_UB_pct"] = 100 * (1 - f["greedy_contest"] / f["UB_best_simple"]) if f["UB_best_simple"] else 0
    f["greedy_fill"] = float(used.sum() / (C * X))
    f["greedy_free_space_q"] = q(X - used)
    placed = Xm.sum()
    distinct = Xm.any(1).sum()
    f["greedy_placements"] = int(placed)
    f["greedy_distinct_videos"] = int(distinct)
    f["greedy_copies_per_video"] = float(placed / max(1, distinct))
    served = best > 0
    f["greedy_frac_demand_served"] = float(I.rn[served].sum() / I.total_count)
    f["greedy_frac_reachable_demand_served"] = float(I.rn[served].sum() / max(1, I.rn[reach].sum()))
    f["greedy_served_at_best_cache"] = float(I.rn[served & (best == best_sav)].sum() / max(1, I.rn[served].sum()))
    f["greedy_value_capture_of_served"] = float((best * I.rn)[served].sum() / max(1, (best_sav * I.rn)[served].sum()))
    f["greedy_frac_cacheable_catalogue_placed"] = float(Xm[vid_cacheable].any(1).mean()) if vid_cacheable.any() else 0
    # distributions for plotting
    dist = {
        "size": s.tolist() if V <= 20000 else None,
        "pop": pop[requested].tolist(),
        "counts": I.rn.tolist() if I.R <= 250000 else I.rn[np.random.default_rng(0).choice(I.R, 250000, replace=False)].tolist(),
        "K": I.K.tolist(),
        "dc": I.dc.tolist(),
        "lat": I.conn_lat.tolist() if len(I.conn_lat) <= 50000 else I.conn_lat[np.random.default_rng(0).choice(len(I.conn_lat), 50000, replace=False)].tolist(),
        "rel_saving": (sav / I.dc[I.conn_e]).tolist() if len(sav) <= 50000 else (sav / I.dc[I.conn_e])[np.random.default_rng(0).choice(len(sav), 50000, replace=False)].tolist(),
        "cache_degree": deg.tolist(),
        "free_space": (X - used).tolist(),
    }
    return f, dist


if __name__ == "__main__":
    import sys
    names = sys.argv[1:] or list(INSTANCES)
    out, dists = {}, {}
    for n in names:
        out[n], dists[n] = analyse(n)
        print(json.dumps(out[n]))
    json.dump(out, open("res/features.json", "w"), indent=1)
    json.dump(dists, open("res/dists.json", "w"))
