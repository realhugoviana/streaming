"""Build a full-size HashCode instance from a hard gadget, with a certified optimum.

The instance is k independent copies of the gadget (own videos, endpoints and caches), each copy
randomly jittered and then solved exactly. Copies do not interact, so the greedy on the whole
instance makes the same choices as on each copy alone, and the optimum is the sum of the copies'
optima. Every copy is normalised to the official limits: video size <= 1000, 1 <= cache latency <= 500,
cache latency < data-center latency <= 4000, request count <= 10000.

usage (from analysis/out): python3 ../hardness/tile.py <variant> <copies> [jitter] [seed]
  variant: a key of hardness/adversary.json, e.g. "≤ X/4"; jitter: relative noise, default 0.15
Writes hardness/tiled/<slug>_x<copies>[_jitter<j>].in and a .json with the predicted greedy and optimal
scores. The traps rely on razor-thin density margins, so even small jitter defuses many copies.
"""
import json
import sys
from dataclasses import replace
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from small import Small, greedy, optimum, from_json, normalise  # noqa: E402

LIMITS = {"V": 10000, "E": 1000, "C": 1000, "R": 1_000_000}


def jitter(inst, rng, amount):
    if amount <= 0:
        return inst
    rn = np.maximum(1, np.round(inst.rn * np.exp(rng.normal(0, amount, inst.R)))).astype(np.int64)
    S = np.where(inst.S > 0, np.maximum(1, np.round(inst.S * np.exp(rng.normal(0, amount, inst.S.shape)))), 0).astype(np.int64)
    return replace(inst, rn=rn, S=S)


def tile(parts):
    X = parts[0].X
    assert all(p.X == X for p in parts)
    vo = eo = co = 0
    size, dc, S_blocks, rv, re, rn = [], [], [], [], [], []
    C = sum(p.C for p in parts)
    for p in parts:
        size.append(p.size); dc.append(p.dc)
        block = np.zeros((p.E, C), np.int64); block[:, co:co + p.C] = p.S; S_blocks.append(block)
        rv.append(p.rv + vo); re.append(p.re + eo); rn.append(p.rn)
        vo += p.V; eo += p.E; co += p.C
    return Small(X, np.concatenate(size), np.concatenate(dc), np.vstack(S_blocks), np.concatenate(rv), np.concatenate(re), np.concatenate(rn))


if __name__ == "__main__":
    variant, k = sys.argv[1], int(sys.argv[2])
    amount = float(sys.argv[3]) if len(sys.argv) > 3 else 0.15
    rng = np.random.default_rng(int(sys.argv[4]) if len(sys.argv) > 4 else 0)
    base = from_json(json.load(open("hardness/adversary.json"))[variant]["gadget"])
    k = min(k, LIMITS["V"] // base.V, LIMITS["E"] // base.E, LIMITS["C"] // base.C, LIMITS["R"] // max(1, base.R))
    parts, gsum, osum = [], 0.0, 0.0
    for _ in range(k):
        part = normalise(jitter(base, rng, amount))
        _, gw = greedy(part)
        _, ow, ob, st = optimum(part)
        assert st == "optimal"
        parts.append(part); gsum += gw; osum += max(ow, gw)
    big = tile(parts)
    total = int(big.rn.sum())
    slug = variant.replace(" ", "_").replace("≤", "le").replace("/", "_over_").replace("(", "").replace(")", "")
    out = Path("hardness/tiled"); out.mkdir(parents=True, exist_ok=True)
    path = out / (f"{slug}_x{k}.in" if amount <= 0 else f"{slug}_x{k}_jitter{amount:g}.in")
    path.write_text(big.to_in())
    info = {"variant": variant, "copies": k, "jitter": amount, "V": big.V, "E": big.E, "C": big.C, "R": big.R, "X": int(big.X),
            "greedy_weighted": gsum, "opt_weighted": osum, "greedy_contest": int(gsum * 1000 // total),
            "opt_contest": int(osum * 1000 // total), "gap": 1 - gsum / osum}
    json.dump(info, open(path.with_suffix(".json"), "w"), indent=1)
    print(json.dumps(info))
