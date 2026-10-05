"""Which instance features make the greedy fall short? Reads hardness/sweep.csv.

usage (from analysis/out): python3 ../hardness/sweep_report.py
Writes hardness/sweep_summary.json and prints the findings.
"""
import json
import numpy as np
import pandas as pd
from sklearn.tree import DecisionTreeRegressor, export_text

FEATS = ["rho", "items_per_cache", "max_size_over_X", "frac_videos_too_big", "size_cv", "pop_gini",
         "links_per_endpoint", "link_density", "cache_overlap", "second_over_best", "worst_over_best",
         "density_cv", "V", "E", "C"]

df = pd.read_csv("hardness/sweep.csv")
df = df[df.status == "optimal"].copy()
g = df.gap
summary = {
    "n": int(len(df)),
    "share_optimal": float((g < 1e-9).mean()),
    "share_gt_1pct": float((g > 0.01).mean()),
    "share_gt_5pct": float((g > 0.05).mean()),
    "share_gt_10pct": float((g > 0.10).mean()),
    "mean": float(g.mean()), "p90": float(g.quantile(.9)), "p99": float(g.quantile(.99)), "max": float(g.max()),
}
print(f"{summary['n']} instances solved to proven optimality")
print(f"greedy optimal on {summary['share_optimal']:.1%}; gap > 1% on {summary['share_gt_1pct']:.1%}, > 5% on {summary['share_gt_5pct']:.1%}, > 10% on {summary['share_gt_10pct']:.1%}")
print(f"mean gap {summary['mean']:.2%}, 90th pct {summary['p90']:.2%}, 99th pct {summary['p99']:.2%}, max {summary['max']:.2%}")

# rank correlation of each feature with the gap
corr = {f: float(df[f].corr(g, method="spearman")) for f in FEATS}
print("\nSpearman correlation with the gap:")
for f, c in sorted(corr.items(), key=lambda x: -abs(x[1])):
    print(f"  {f:22s} {c:+.3f}")
summary["spearman"] = corr

# binned gap per feature (deciles, or distinct values for integers)
bins = {}
for f in FEATS:
    x = df[f]
    if x.nunique() <= 8:
        grp = df.groupby(x)
        labels = [str(v) for v in grp.groups]
    else:
        q = pd.qcut(x, 10, duplicates="drop")
        grp = df.groupby(q, observed=True)
        labels = [f"{iv.left:.3g}–{iv.right:.3g}" for iv in grp.groups]
    st = grp.gap.agg(["mean", lambda s: s.quantile(.9), "max", "size"])
    st.columns = ["mean", "p90", "max", "n"]
    bins[f] = {"labels": labels, "mean": st["mean"].tolist(), "p90": st["p90"].tolist(), "max": st["max"].tolist(), "n": st["n"].astype(int).tolist()}
summary["bins"] = bins

# a shallow tree: readable rules for where the gap is large
tree = DecisionTreeRegressor(max_depth=3, min_samples_leaf=max(200, len(df) // 100), random_state=0)
tree.fit(df[FEATS], g)
rules = export_text(tree, feature_names=FEATS, decimals=3, show_weights=False)
print("\nRegression tree (leaf value = mean gap):\n" + rules)
summary["tree_text"] = rules
leaves = tree.apply(df[FEATS])
leaf_stats = df.assign(leaf=leaves).groupby("leaf").gap.agg(["mean", "size", lambda s: (s > 0.05).mean()])
leaf_stats.columns = ["mean", "n", "share_gt_5pct"]
summary["leaves"] = leaf_stats.sort_values("mean", ascending=False).reset_index().to_dict(orient="records")
imp = dict(zip(FEATS, tree.feature_importances_.round(4).tolist()))
summary["tree_importance"] = imp
print("tree feature importance:", {k: v for k, v in sorted(imp.items(), key=lambda x: -x[1]) if v > 0})

# the two strongest drivers together
rb = pd.cut(df.rho, [0, 0.25, 0.5, 0.75, 1.0, 1.25, 2.0])
ib = pd.cut(df.items_per_cache, [0, 1, 1.5, 2, 3, 5, 10, 1e9])
grid = df.groupby([rb, ib], observed=False).gap.agg(["mean", "size"])
summary["grid"] = {
    "rho_bins": [str(b) for b in rb.cat.categories], "ipc_bins": [str(b) for b in ib.cat.categories],
    "mean": grid["mean"].unstack().values.tolist(), "n": grid["size"].unstack().values.tolist(),
}
print("\nMean gap by capacity pressure (rows) and videos per cache (columns):")
print((grid["mean"].unstack() * 100).round(2).to_string())

# typical profile of the hardest 1% vs the rest
top = df.gap >= df.gap.quantile(0.99)
prof = pd.DataFrame({"hardest 1%": df[top][FEATS].median(), "all": df[FEATS].median()})
print("\nMedian features, hardest 1% vs all:\n" + prof.round(3).to_string())
summary["hardest_profile"] = {k: {"hardest": float(prof.loc[k, "hardest 1%"]), "all": float(prof.loc[k, "all"])} for k in FEATS}
json.dump(summary, open("hardness/sweep_summary.json", "w"), indent=1, default=float)
