"""Build the replay page (replay.html) from the execution traces.

Run from analysis/out after trace (traces/*.json), the bounds, and build_atlas.py (res/data.json).
"""
import json
import sys
from pathlib import Path
import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))          # analysis/, for the shared loader
from inst import Instance, INSTANCES, read_solution  # noqa: E402
REPLAYED = ["me_at_the_zoo", "videos_worth_spreading", "trending_today", "kittens", "custom_dejavu42", "custom_universallambda42"]


def full_instance(name):
    I = Instance(name)
    links = [[] for _ in range(I.E)]
    for e, c, l in zip(I.conn_e.tolist(), I.conn_c.tolist(), I.conn_lat.tolist()):
        links[e].append([c, l])
    return {"V": I.V, "E": I.E, "C": I.C, "X": I.X, "size": I.size.tolist(), "dc": I.dc.tolist(),
            "links": links, "req": [[int(v), int(e), int(n)] for v, e, n in zip(I.rv, I.re, I.rn)], "rawR": I.R_raw}


def best_bound(name, atlas):
    """Tightest proven upper bound, same rule as the atlas page."""
    f, b = atlas["features"][name], atlas["bounds"].get(name, {})
    cand = [(f["UB_uncapacitated"], "upper bound"), (f["UB_knapsack_sum"], "knapsack bound")]
    if b.get("UB_cov") is not None:
        cand.append((b["UB_cov"], "coverage bound"))
    for key in ("ALP", "RLP", "LP"):
        if isinstance(b.get(key), dict) and b[key].get("status") == 0:
            cand.append((int(b[key]["value"]), "LP bound"))
    mip = b.get("MIP")
    if isinstance(mip, dict) and mip.get("status") == 0 and mip.get("gap") == 0:
        cand.append((mip["verified_contest"], "proven optimum"))
    return min(cand, key=lambda x: x[0])      # min keeps the first entry on ties


atlas = json.load(open("res/data.json"))
out = {"instances": {}}
out["example"] = full_instance("example")
tok = INSTANCES["example"].read_bytes().split()
R_raw = int(tok[2])
out["example"]["rawReq"] = np.array(tok[len(tok) - 3 * R_raw:], dtype=int).reshape(R_raw, 3).tolist()
for n in REPLAYED:
    t = json.load(open(f"traces/{n}.json"))
    t["bound"], t["boundKind"] = best_bound(n, atlas)
    if n == "me_at_the_zoo":
        t["full"] = full_instance(n)
        t["optimum"] = {int(c): vs for c, vs in read_solution("sol/me_at_the_zoo.mip.txt").items()}
    out["instances"][n] = t
    print(f"{n:26s} {t['boundKind']} {t['bound']:,}  {len(t['greedy']) // 3} placements, {len(t['kca'])} rewrites")

data = json.dumps(out, separators=(",", ":")).replace("</", "<\\/")
body = (HERE / "replay_body.html").read_text()
assert body.count("/*DATA*/") == 1
Path("replay.html").write_text((HERE / "replay_head.html").read_text() + body.replace("/*DATA*/", data))
print(f"wrote replay.html ({len(data) / 1e6:.1f} MB of data)")
