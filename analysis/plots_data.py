"""Distribution data for the atlas instance profiles (size histograms, popularity curves).

Run from analysis/out. Writes res/plots.json.
"""
import json
import numpy as np
from inst import Instance, INSTANCES

out = {}
for n in INSTANCES:
    if n == "example":
        continue
    I = Instance(n)
    edges = np.linspace(0, I.size.max(), 21)
    hist, _ = np.histogram(I.size, bins=edges)
    # share of total request count captured by the most-requested x% of videos
    pop = np.bincount(I.rv, weights=I.rn, minlength=I.V)
    pop = np.sort(pop[pop > 0])[::-1]
    cum = np.r_[0, np.cumsum(pop) / pop.sum()]
    lorenz = [float(cum[int(round(x * len(pop)))]) for x in np.linspace(0, 1, 51)]
    count_edges = np.logspace(0, np.log10(I.rn.max() + 1), 21)
    count_hist, _ = np.histogram(I.rn, bins=count_edges)
    out[n] = dict(size_edges=edges.tolist(), size_hist=hist.tolist(), X=I.X, lorenz=lorenz,
                  count_edges=count_edges.tolist(), count_hist=count_hist.tolist())
json.dump(out, open("res/plots.json", "w"))
print("wrote res/plots.json for", ", ".join(out))
