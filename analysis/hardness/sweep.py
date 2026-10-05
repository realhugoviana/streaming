"""Random sweep: many small random instances, each solved by the team greedy and to optimality.

usage (from analysis/out): python3 ../hardness/sweep.py [n_instances] [workers]
Writes hardness/sweep.csv (one row per instance: shape parameters, features, scores, gap)
and hardness/sweep_worst/*.in (the 25 largest gaps).
"""
import sys
from multiprocessing import Pool
from pathlib import Path
import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
from small import random_instance, gap, features  # noqa: E402


def one(seed):
    rng = np.random.default_rng([2026, seed])
    inst = random_instance(rng)
    r = gap(inst)
    if r["opt"] <= 0:
        return None
    row = {"seed": seed, **{"p_" + k: v for k, v in inst.meta.items()}, **features(inst),
           "greedy": r["greedy"], "opt": r["opt"], "bound": r["bound"], "status": r["status"], "gap": r["gap"],
           "greedy_contest": inst.contest(r["greedy"]), "opt_contest": inst.contest(r["opt"])}
    return row


if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
    workers = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    with Pool(workers) as pool:
        rows = [r for r in pool.imap_unordered(one, range(n), chunksize=20) if r is not None]
    df = pd.DataFrame(rows).sort_values("seed")
    Path("hardness").mkdir(exist_ok=True)
    df.to_csv("hardness/sweep.csv", index=False)
    print(f"{len(df)} instances with a positive optimum ({(df.status == 'optimal').mean():.2%} proven optimal)")
    print(df.gap.describe(percentiles=[.5, .9, .99]).to_string())
    out = Path("hardness/sweep_worst"); out.mkdir(exist_ok=True)
    for _, r in df.nlargest(25, "gap").iterrows():
        inst = random_instance(np.random.default_rng([2026, int(r.seed)]))
        (out / f"seed{int(r.seed)}_gap{r.gap * 100:.1f}.in").write_text(inst.to_in())
