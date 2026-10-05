"""Hand-built worst cases for greedy_density, from the mechanisms the adversarial search found.

knapsack trap    one cache. A 1 MB video is marginally denser than a video that fills the whole
                 cache; the greedy takes the small one and the big one never fits. Gap -> 1 - 1/X.
assignment trap  endpoint A reaches caches 0 and 1 (cache 1 is 1 ms worse); endpoint B reaches
                 only cache 0. A's videos are marginally denser, so the greedy fills cache 0 with
                 them and B gets nothing; the optimum sends A's videos to cache 1. Gap -> 50%, for
                 any video size (with equal sizes the greedy is proven to reach at least half).
combined trap    sizes <= X/k. (k-1) videos of X/k MB that could also use cache 1 (1 ms worse) take
                 cache 0, then a 1 MB filler, just denser than the k videos of X/k MB that only
                 cache 0 can serve, takes 1 MB more: none of those k videos fits any more. The greedy
                 keeps about (k-1)/k of a cache's value, the optimum gets that plus a full cache, so
                 the gap -> k/(2k-1) (2/3, 4/7, 10/19 for k = 2, 4, 10). k = 1 is the knapsack trap.

All counts are <= 10000 and every latency is within the official limits.
usage (from analysis/out): python3 ../hardness/traps.py [seed]
  seed: also write the traps into hardness/adversary.json where they beat the search's best, so a
        polish run starts from them.
"""
import json
import sys
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from small import Small, greedy, optimum  # noqa: E402

S_BEST = 999          # latency saved through the better link (data center 1000 ms, cache 1 ms)


def build(X, size, links, requests):
    """links: per endpoint {cache: saving}; requests: (video, endpoint, count)."""
    C = 1 + max(c for l in links for c in l)
    S = np.zeros((len(links), C), np.int64)
    for e, l in enumerate(links):
        for c, s in l.items():
            S[e, c] = s
    dc = np.array([max(l.values()) + 1 for l in links])
    r = np.array(requests, np.int64)
    return Small(X, np.array(size, np.int64), dc, S, r[:, 0], r[:, 1], r[:, 2])


def knapsack_trap(X):
    n_big = 10000
    n_small = n_big // X + 1                          # small video: n_small per MB > n_big / X per MB
    return build(X, [X, 1], [{0: S_BEST}], [(0, 0, n_big), (1, 0, n_small)])


def assignment_trap(X, size):
    m = X // size                                     # videos per group: exactly fills cache 0
    links = [{0: S_BEST, 1: S_BEST - 1}, {0: S_BEST}]
    reqs = [(v, 0, 10000) for v in range(m)] + [(m + v, 1, 9999) for v in range(m)]
    return build(X, [size] * (2 * m), links, reqs)


def combined_trap(X, k):
    part = X // k                                     # every video but the filler is X/k MB
    n_b = 9000                                        # stranded videos: n_b / part per MB
    n_f = n_b // part + 1                             # 1 MB filler, just denser than them
    n_a = n_f * part + 1                              # contested videos, just denser than the filler
    a = list(range(k - 1)); f = k - 1; b = list(range(k, 2 * k))
    links = [{0: S_BEST, 1: S_BEST - 1}, {0: S_BEST}] if k > 1 else [{0: S_BEST}, {0: S_BEST}]
    reqs = [(v, 0, n_a) for v in a] + [(f, 1, n_f)] + [(v, 1, n_b) for v in b]
    return build(X, [part] * (k - 1) + [1] + [part] * k, links, reqs)


TRAPS = {   # variant of adversary.py -> trap at that variant's capacity and size limit
    "any size (≤ X)": combined_trap(20, 1),
    "≤ X/2": combined_trap(40, 2),
    "≤ X/4": combined_trap(80, 4),
    "≤ X/10": combined_trap(200, 10),
    "unit size": assignment_trap(3, 1),            # equal sizes: no room for a small filler
}

if __name__ == "__main__":
    from adversary import describe
    seed = len(sys.argv) > 1 and sys.argv[1] == "seed"
    adv = json.load(open("hardness/adversary.json")) if seed else {}
    out = Path("hardness/traps"); out.mkdir(parents=True, exist_ok=True)
    for variant, inst in TRAPS.items():
        d = describe(inst)
        slug = variant.replace(" ", "_").replace("≤", "le").replace("/", "_over_").replace("(", "").replace(")", "")
        (out / f"{slug}.in").write_text(inst.to_in())
        line = f"{variant:16s} {inst.V:2d} videos, {inst.E} endpoints, {inst.C} caches, X={inst.X}: gap {d['gap']:.2%} ({d['status']})"
        if seed:
            prev = adv[variant].get("valid_gap", adv[variant]["best_gap"])   # the gap that survives as a valid .in
            if d["gap"] > prev:
                adv[variant].update(best_gap=d["gap"], found=d, gadget=d, source="hand-built trap")
                line += f"  -> seeds the polish (search best was {prev:.2%})"
            else:
                line += f"  (search best {prev:.2%} is higher)"
        print(line)
    if seed:
        json.dump(adv, open("hardness/adversary.json", "w"), indent=1)
