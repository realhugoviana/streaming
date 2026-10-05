"""Instance-specific statistics quoted in the atlas (parity pattern, dejaVu kernels, trending leftovers...).

Run from analysis/out after the team greedy solutions exist in sol/. Writes res/extra.json.
"""
import json
import numpy as np
import scipy.sparse as sp
from scipy.sparse.csgraph import connected_components
from inst import Instance, read_solution

ex = {}

# universalLambda: demand split by endpoint / video parity, and what the greedy places
I = Instance("custom_universallambda42")
pe, pv = I.re % 2, I.rv % 2
tab = {}
for a in (0, 1):
    for b in (0, 1):
        m = (pe == a) & (pv == b)
        tab[f"E{'even' if a == 0 else 'odd'}_V{'even' if b == 0 else 'odd'}"] = dict(
            requests=int(m.sum()), demand_share=float(I.rn[m].sum() / I.total_count), mean_count=float(I.rn[m].mean()))
s = I.size
hot = (I.re % 2 == 0) & (I.rv % 2 == 1)
ex["ul_parity"] = tab
ex["ul_size1_share_of_hot_demand"] = float(I.rn[hot & (s[I.rv] == 1)].sum() / I.rn[hot].sum())
ex["ul_size_by_id_bins"] = [(int(lo), float((s[lo:lo + 1000] == 1).mean()), float(s[lo:lo + 1000].mean())) for lo in range(0, 10000, 1000)]
sol = read_solution("sol/custom_universallambda42.greedy.txt")
pl = np.array([v for c in sol for v in sol[c]])
ex["ul_greedy_placed_odd_frac"] = float((pl % 2 == 1).mean())
ex["ul_greedy_placed_size1_frac"] = float((s[pl] == 1).mean())
ex["ul_odd_size1_videos"] = int(((np.arange(I.V) % 2 == 1) & (s == 1)).sum())
ex["ul_even_endpoints"] = int((np.arange(I.E) % 2 == 0).sum())

# dejaVu: per-endpoint kernels and duplicate-line multiplicity
D = Instance("custom_dejavu42")
dv = np.bincount(D.re, minlength=D.E)
raw = np.bincount(np.repeat(D.re, D.dup_multiplicity), minlength=D.E)
ex["dv_distinct_videos_per_endpoint_q"] = np.quantile(dv, [0, .5, 1]).tolist()
ex["dv_raw_requests_per_endpoint_q"] = np.quantile(raw, [0, .5, 1]).tolist()
ex["dv_multiplicity_q"] = np.quantile(D.dup_multiplicity, [0, .25, .5, .75, 1]).tolist()
ex["dv_merged_count_q"] = np.quantile(D.rn, [0, .25, .5, .75, 1]).tolist()
ex["dv_count_cv_raw_vs_merged"] = [float(D.raw_counts.std() / D.raw_counts.mean()), float(D.rn.std() / D.rn.mean())]
key = D.conn_e * D.C + D.conn_c
u, cnt = np.unique(key, return_counts=True)
ex["dv_duplicated_links"] = int((cnt > 1).sum())
ex["dv_endpoints_with_duplicated_link"] = int(len(np.unique(u[cnt > 1] // D.C)))

# trending_today: videos the greedy leaves out vs the free space left
T = Instance("trending_today")
sol = read_solution("sol/trending_today.greedy.txt")
placed = np.zeros(T.V, bool)
for c in sol:
    placed[sol[c]] = True
unp = np.nonzero(~placed)[0]
free = np.array([T.X - T.size[sol.get(c, [])].sum() for c in range(T.C)])
ex["tt_unplaced_sizes"] = T.size[unp].tolist()
ex["tt_unplaced_total"] = int(T.size[unp].sum())
ex["tt_free_total"] = int(free.sum())
ex["tt_free_max"] = int(free.max())
pop = np.bincount(T.rv, weights=T.rn, minlength=T.V)
ex["tt_points_per_unplaced_video"] = [int(pop[v] * 500 * 1000 // T.total_count) for v in unp]

# kittens: how interchangeable an endpoint's best caches are
K = Instance("kittens")
top2 = -np.sort(-K.S, 1)[:, :2]
ex["kittens_saving_spread_best_minus_2nd_q"] = np.quantile(top2[:, 0] - top2[:, 1], [0, .5, 1]).tolist()
ex["kittens_mean_best_saving"] = float(K.S.max(1).mean())

# videos_worth_spreading: connectivity and the popular head
W = Instance("videos_worth_spreading")
B = sp.csr_matrix((W.S > 0).astype(int))
n, lab = connected_components(sp.bmat([[None, B], [B.T, None]]), directed=False)
ex["vws_bipartite_components"] = int(n)
ex["vws_component_sizes_top"] = sorted(np.bincount(lab).tolist(), reverse=True)[:8]
popw = np.bincount(W.rv, weights=W.rn, minlength=W.V)
top = np.argsort(-popw)[:100]
ex["vws_top100_videos_demand_share"] = float(popw[top].sum() / W.total_count)
ex["vws_top100_videos_volume"] = int(W.size[top].sum())

json.dump(ex, open("res/extra.json", "w"), indent=1)
for k, v in ex.items():
    print(k, ":", v)
