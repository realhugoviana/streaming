"""Statistics for one small instance: shape features, the team's greedy, the exact optimum and the gap.

usage (from anywhere):
  python3 analysis/hardness/stats.py <instance.in> [--time-limit SECONDS]
  python3 analysis/hardness/stats.py --random SEED [--save FILE]     generate one from small.random_instance

The greedy is greedy_density's exact rule (see small.py). The optimum is solved exactly with HiGHS, which
is quick up to a few dozen videos and caches; beyond that it may stop at the time limit, and the gap is
then reported as a range. Duplicate request lines are merged; a duplicated endpoint->cache link keeps
the first latency listed, as the team's latencyToCache does. If analysis/out/bin/team_driver exists
(built by run_all.sh), the team's compiled greedy is run as a cross-check.
"""
import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path
import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from small import Small, greedy, optimum, features, random_instance  # noqa: E402

TEAM = HERE.parent / "out" / "bin" / "team_driver"
LABELS = {
    "rho": "capacity pressure (C·X ÷ requested volume)",
    "items_per_cache": "videos per cache (X ÷ mean size)",
    "max_size_over_X": "largest video ÷ X",
    "frac_videos_too_big": "requested videos bigger than X",
    "size_cv": "size variability (CV)",
    "pop_gini": "popularity concentration (Gini)",
    "links_per_endpoint": "caches per endpoint",
    "cache_overlap": "cache overlap (Jaccard)",
    "second_over_best": "2nd-best ÷ best saving",
    "worst_over_best": "worst ÷ best saving",
    "density_cv": "gain-per-MB variability (CV)",
}


def read_in(path):
    tok = np.array(Path(path).read_bytes().split(), dtype=np.int64)
    V, E, R, C, X = (int(t) for t in tok[:5])
    p = 5
    size = tok[p:p + V].copy(); p += V
    dc = np.zeros(E, np.int64)
    S = np.zeros((E, C), np.int64)
    for e in range(E):
        dc[e], k = tok[p], int(tok[p + 1]); p += 2
        for c, lat in tok[p:p + 2 * k].reshape(k, 2):
            if S[e, c] == 0:                              # first link wins, like latencyToCache
                S[e, c] = dc[e] - lat
        p += 2 * k
    req = tok[p:p + 3 * R].reshape(R, 3)
    key, inv = np.unique(req[:, 0] * E + req[:, 1], return_inverse=True)
    rn = np.bincount(inv, weights=req[:, 2]).astype(np.int64)
    inst = Small(X, size, dc, S, key // E, key % E, rn)
    inst.meta = {"request_lines": R}
    return inst


def placement(placed):
    return {c: np.nonzero(placed[:, c])[0].tolist() for c in range(placed.shape[1])}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("instance", nargs="?", help=".in file")
    ap.add_argument("--random", type=int, metavar="SEED", help="generate a random small instance instead")
    ap.add_argument("--save", metavar="FILE", help="with --random: also write the instance to FILE")
    ap.add_argument("--time-limit", type=float, default=60.0)
    a = ap.parse_args()
    if a.random is not None:
        inst = random_instance(np.random.default_rng(a.random))
        inst.meta = {"request_lines": inst.R}
        if a.save:
            Path(a.save).write_text(inst.to_in())
        path = a.save
    elif a.instance:
        inst, path = read_in(a.instance), a.instance
    else:
        ap.error("give an instance file or --random SEED")

    total = int(inst.rn.sum())
    print(f"Instance: {inst.V} videos, {inst.E} endpoints, {inst.C} caches of {inst.X} MB, "
          f"{inst.meta['request_lines']} request lines -> {inst.R} distinct, {total:,} requests in total")
    if inst.V * inst.C > 5000:
        print(f"  note: {inst.V * inst.C:,} video-cache pairs; the exact optimum may hit the {a.time_limit:g} s limit")

    print("\nShape")
    for k, v in features(inst).items():
        if k in LABELS:
            print(f"  {LABELS[k]:44s} {v:.3f}")

    gp, gw, steps = greedy(inst, trace=True)
    print(f"\nGreedy (greedy_density): {inst.contest(gw):,} points, {len(steps)} placements")
    for i, (v, c, g, d) in enumerate(steps[:15], 1):
        print(f"  {i:3d}. video {v} ({inst.size[v]} MB) -> cache {c}   +{g * 1000 / total:,.1f} points, {d * 1000 / total:,.2f} per MB")
    if len(steps) > 15:
        print(f"  ... {len(steps) - 15} more")

    op, ow, bound, status = optimum(inst, a.time_limit)
    best = max(ow, gw)
    print(f"\nOptimum: {inst.contest(best):,} points ({status})")
    if status == "optimal":
        print(f"Gap: {1 - gw / best:.2%} of the optimum is lost by the greedy" if best > 0 else "Gap: 0 (nothing can be gained)")
    else:
        print(f"Gap: between {1 - gw / best:.2%} and {1 - gw / bound:.2%} (time limit reached; upper bound {inst.contest(bound):,})")

    gg = greedy(inst, key="gain")[1]
    print(f"Same greedy ranked by raw gain: {inst.contest(gg):,} points; best of both: {inst.contest(max(gw, gg)):,}")

    if ow > gw:
        print("\nWhere the optimum differs (per cache: only in the greedy / only in the optimum)")
        G, O = placement(gp), placement(op)
        for c in range(inst.C):
            only_g = sorted(set(G[c]) - set(O[c])); only_o = sorted(set(O[c]) - set(G[c]))
            if only_g or only_o:
                fmt = lambda vs: ", ".join(f"v{v} ({inst.size[v]} MB)" for v in vs) or "-"
                print(f"  cache {c}: greedy only {fmt(only_g)}  |  optimum only {fmt(only_o)}")

    if TEAM.exists():
        with tempfile.NamedTemporaryFile("w", suffix=".in", delete=False) as f:
            f.write(Path(path).read_text() if path else inst.to_in())
        out = subprocess.run([str(TEAM), "/dev/null", "0"], stdin=open(f.name), capture_output=True, text=True)
        Path(f.name).unlink()
        if out.returncode == 0:
            team = json.loads(out.stdout)["greedy_contest"]
            print(f"\nTeam's compiled greedy: {team:,} points ({'matches' if team == inst.contest(gw) else 'DIFFERS from the Python greedy'})")


if __name__ == "__main__":
    main()
