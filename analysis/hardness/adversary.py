"""Adversarial search: mutate small instances to maximise the greedy's gap to the optimum.

One simulated-annealing run per (size limit, restart); the best instance of each size limit is then
pruned to a minimal gadget (requests, links and videos removed while the gap holds).

usage (from analysis/out): python3 ../hardness/adversary.py [iterations] [restarts] [workers] [polish]
  polish: restart every variant from its best instance in hardness/adversary.json at a lower
          temperature, and keep a result only if it beats that instance.
Writes hardness/adversary.json and hardness/adversary/<variant>.in (+ _gadget.in). The .in files are
normalised to the official limits (counts <= 10000, cache latency <= 500); their gap is re-checked.
"""
import json
import sys
from dataclasses import replace
from multiprocessing import Pool
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from small import Small, greedy, optimum, random_instance, from_json, normalise  # noqa: E402

# size limit -> (largest video size, cache capacity X, number of videos). Videos are 1..20 MB and X is
# a multiple of 20, with enough videos that they can overflow the 3 caches; "unit" makes every video 1 MB.
VARIANTS = {
    "any size (≤ X)": (20, 20, 10),
    "≤ X/2": (20, 40, 10),
    "≤ X/4": (20, 80, 20),
    "≤ X/10": (20, 200, 40),
    "unit size": (1, 3, 10),
}
E, C = 5, 3


def evaluate(inst):
    _, gw = greedy(inst)
    _, ow, _, st = optimum(inst, time_limit=10)
    best = max(gw, ow)
    return (1 - gw / best) if best > 0 else 0.0


def make_valid(inst):
    """Official limits by construction: count <= 10000, cache latency (dc - saving) in [1, 500]."""
    lo = np.maximum(1, inst.dc - 500)[:, None]
    S = np.where(inst.S > 0, np.clip(inst.S, lo, inst.dc[:, None] - 1), 0)
    return replace(inst, S=S, rn=np.minimum(inst.rn, 10000))


def start(rng, smax, X, V):
    inst = random_instance(rng, V=V, E=E, C=C, rho=1.0)
    size = np.ones(V, np.int64) if smax == 1 else rng.integers(1, smax + 1, V)
    return make_valid(replace(inst, size=size, X=X, meta={}))


def mutate(inst, rng, smax):
    size, dc, S = inst.size.copy(), inst.dc.copy(), inst.S.copy()
    req = {(int(v), int(e)): int(n) for v, e, n in zip(inst.rv, inst.re, inst.rn)}
    V, E, C = inst.V, inst.E, inst.C
    op = rng.integers(0, 6 if smax > 1 else 5)
    if op == 0:                                        # request count
        k = list(req)[rng.integers(len(req))]
        req[k] = int(np.clip(round(req[k] * np.exp(rng.normal(0, 1))), 1, 10000))
    elif op == 1:                                      # add or remove a request
        v, e = int(rng.integers(V)), int(rng.integers(E))
        if (v, e) in req and len(req) > 1:
            del req[(v, e)]
        else:
            req[(v, e)] = int(rng.integers(1, 5000))
    elif op == 2:                                      # latency saved on a link
        e, c = int(rng.integers(E)), int(rng.integers(C))
        if S[e, c] > 0:
            S[e, c] = rng.integers(max(1, dc[e] - 500), dc[e])
    elif op == 3:                                      # toggle a link (every endpoint keeps one)
        e, c = int(rng.integers(E)), int(rng.integers(C))
        if S[e, c] > 0 and (S[e] > 0).sum() > 1:
            S[e, c] = 0
        elif S[e, c] == 0:
            S[e, c] = rng.integers(max(1, dc[e] - 500), dc[e])
    elif op == 4:                                      # data-center latency
        e = int(rng.integers(E))
        dc[e] = rng.integers(100, 1001)
        S[e] = np.where(S[e] > 0, np.clip(S[e], max(1, dc[e] - 500), dc[e] - 1), 0)
    else:                                              # video size
        size[rng.integers(V)] = rng.integers(1, smax + 1)
    keys = sorted(req)
    return replace(inst, size=size, dc=dc, S=S, rv=np.array([k[0] for k in keys]), re=np.array([k[1] for k in keys]),
                   rn=np.array([req[k] for k in keys], np.int64))


def anneal(job):
    variant, restart, iters, init, T0, T1 = job
    smax, X, V = VARIANTS[variant]
    rng = np.random.default_rng([7, restart, X, 0 if init is None else 1])
    cur = start(rng, smax, X, V) if init is None else make_valid(from_json(init))
    fc = evaluate(cur)
    best, fb = cur, fc
    for it in range(iters):
        T = T0 * (T1 / T0) ** (it / iters)
        cand = mutate(cur, rng, smax)
        f = evaluate(cand)
        if f >= fc or rng.random() < np.exp((f - fc) / T):
            cur, fc = cand, f
            if f > fb:
                best, fb = cand, f
    return variant, restart, fb, best


def prune(inst, target):
    """Drop requests, links and unused videos while the gap stays within 0.5 point of target."""
    changed = True
    while changed:
        changed = False
        for i in range(inst.R):
            if inst.R <= 1:
                break
            m = np.ones(inst.R, bool); m[i] = False
            cand = replace(inst, rv=inst.rv[m], re=inst.re[m], rn=inst.rn[m])
            if evaluate(cand) >= target - 0.005:
                inst, changed = cand, True
                break
        if changed:
            continue
        for e in range(inst.E):
            for c in range(inst.C):
                if inst.S[e, c] > 0 and (inst.S[e] > 0).sum() > 1:
                    S = inst.S.copy(); S[e, c] = 0
                    cand = replace(inst, S=S)
                    if evaluate(cand) >= target - 0.005:
                        inst, changed = cand, True
                        break
            if changed:
                break
    # renumber: keep requested videos and endpoints that make requests
    vids, ends = np.unique(inst.rv), np.unique(inst.re)
    vmap = {v: i for i, v in enumerate(vids)}; emap = {e: i for i, e in enumerate(ends)}
    S = inst.S[ends]
    used_c = np.nonzero((S > 0).any(0))[0]
    return replace(inst, size=inst.size[vids], dc=inst.dc[ends], S=S[:, used_c],
                   rv=np.array([vmap[v] for v in inst.rv]), re=np.array([emap[e] for e in inst.re]))


def describe(inst):
    gp, gw, steps = greedy(inst, trace=True)
    op, ow, ob, st = optimum(inst)
    return {
        "V": inst.V, "E": inst.E, "C": inst.C, "X": int(inst.X), "size": inst.size.tolist(), "dc": inst.dc.tolist(),
        "saving": inst.S.tolist(), "requests": [[int(v), int(e), int(n)] for v, e, n in zip(inst.rv, inst.re, inst.rn)],
        "greedy": gw, "opt": max(ow, gw), "status": st, "gap": 1 - gw / max(ow, gw),
        "greedy_contest": inst.contest(gw), "opt_contest": inst.contest(max(ow, gw)),
        "greedy_steps": [[int(v), int(c), float(g), float(d)] for v, c, g, d in steps],
        "greedy_placed": {int(c): np.nonzero(gp[:, c])[0].tolist() for c in range(inst.C)},
        "opt_placed": {int(c): np.nonzero(op[:, c])[0].tolist() for c in range(inst.C)},
    }


if __name__ == "__main__":
    iters = int(sys.argv[1]) if len(sys.argv) > 1 else 6000
    restarts = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    workers = int(sys.argv[3]) if len(sys.argv) > 3 else 8
    polish = len(sys.argv) > 4 and sys.argv[4] == "polish"
    prev = json.load(open("hardness/adversary.json")) if polish else {}
    jobs = []
    for v in VARIANTS:
        init = None
        if polish:   # start from whichever of the previous instance and its gadget has the larger gap
            init = max(prev[v]["found"], prev[v]["gadget"], key=lambda d: evaluate(make_valid(from_json(d))))
        jobs += [(v, r, iters, init, 0.01 if polish else 0.05, 0.0005 if polish else 0.002) for r in range(restarts)]
    with Pool(workers) as pool:
        results = pool.map(anneal, jobs, chunksize=1)
    out = {}
    Path("hardness/adversary").mkdir(parents=True, exist_ok=True)
    for variant in VARIANTS:
        runs = sorted((r for r in results if r[0] == variant), key=lambda r: -r[2])
        best, fbest = runs[0][3], runs[0][2]
        prev_valid = max((evaluate(make_valid(from_json(prev[variant][k]))), k) for k in ("found", "gadget")) if polish else (-1, None)
        if prev_valid[0] > fbest:       # nothing better found: keep the previous best
            print(f"{variant:16s} polish found {fbest:.2%}, keeping previous best {prev_valid[0]:.2%}")
            best, fbest = make_valid(from_json(prev[variant][prev_valid[1]])), prev_valid[0]
        gadget = prune(best, fbest)
        found, small_ = describe(best), describe(gadget)
        valid, valid_g = normalise(best), normalise(gadget)
        out[variant] = {"best_gap": max(found["gap"], small_["gap"]), "search_gap": fbest,
                        "gaps_per_restart": [r[2] for r in runs], "found": found, "gadget": small_,
                        "valid_gap": evaluate(valid), "valid_gadget_gap": evaluate(valid_g), "polished": polish}
        slug = variant.replace(" ", "_").replace("≤", "le").replace("/", "_over_").replace("(", "").replace(")", "")
        Path(f"hardness/adversary/{slug}.in").write_text(valid.to_in())
        Path(f"hardness/adversary/{slug}_gadget.in").write_text(valid_g.to_in())
        print(f"{variant:16s} search best {fbest:.2%}  (restarts: {', '.join(f'{r[2]:.1%}' for r in runs)})  gadget: "
              f"{gadget.V} videos, {gadget.E} endpoints, {gadget.C} caches, gap {small_['gap']:.2%}; "
              f"as valid .in: {out[variant]['valid_gap']:.2%} / gadget {out[variant]['valid_gadget_gap']:.2%}")
    json.dump(out, open("hardness/adversary.json", "w"), indent=1)
