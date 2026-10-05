"""Does running the greedy twice (by density and by raw gain) and keeping the better close the gap?

usage (from analysis/out, after sweep.py): python3 ../hardness/best_of_both.py [workers]
Adds a gain_greedy column to hardness/sweep.csv and prints the gap distribution of both options.
"""
import sys
from multiprocessing import Pool
from pathlib import Path
import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
from small import random_instance, greedy  # noqa: E402


def gain_greedy(seed):
    inst = random_instance(np.random.default_rng([2026, seed]))
    return greedy(inst, key="gain")[1]


if __name__ == "__main__":
    workers = int(sys.argv[1]) if len(sys.argv) > 1 else 8
    df = pd.read_csv("hardness/sweep.csv")
    with Pool(workers) as pool:
        df["gain_greedy"] = pool.map(gain_greedy, df.seed.tolist(), chunksize=50)
    df.to_csv("hardness/sweep.csv", index=False)
    d = df[df.status == "optimal"]
    both = np.maximum(d.greedy, d.gain_greedy)
    gaps = {"density greedy (team)": 1 - d.greedy / d.opt, "raw-gain greedy": 1 - d.gain_greedy / d.opt, "best of both": 1 - both / d.opt}
    for k, g in gaps.items():
        print(f"{k:22s} optimal {(g < 1e-9).mean():6.1%}  >5% {(g > .05).mean():6.2%}  >10% {(g > .10).mean():6.2%}  mean {g.mean():6.2%}  p99 {g.quantile(.99):6.2%}  max {g.max():6.2%}")
