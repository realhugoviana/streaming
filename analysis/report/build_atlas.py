"""Collect every result file into res/data.json and build the atlas page (atlas.html).

Run from the output folder after the team algorithms (res/<inst>.<algorithm>.json), features,
extra_stats, plots_data and bounds. Tables and charts are data-driven; the prose in atlas_prose.js and parts of
atlas_body.html were written for the 2026-09-27 results on the six study instances, and the page
only shows them when those instances are part of the run.
"""
import glob
import json
import math
import os
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
R = Path("res")

feat = json.load(open(R / "features.json"))
extra = json.load(open(R / "extra.json"))
plots = json.load(open(R / "plots.json"))
ALGORITHMS = ("greedy", "greedy90", "rr", "rr_knapsack")      # see team_driver.cpp
team = {n: {a: json.load(open(R / f"{n}.{a}.json")) for a in ALGORITHMS if (R / f"{n}.{a}.json").exists()} for n in feat}
src = Path(os.environ.get("STREAM_INSTANCES") or REPO / "instances" / "instances_contest").resolve()   # same default as inst.py
source = str(src.relative_to(REPO)) if src.is_relative_to(REPO) else src.name

bounds = {}
for f in glob.glob(str(R / "*.bounds.*.json")):
    d = json.load(open(f))
    bounds.setdefault(d["name"], {}).update({k: v for k, v in d.items() if k != "name"})

def finite(x):
    """NaN/inf (e.g. a correlation over constant sizes) are not valid JSON for the browser: use null."""
    if isinstance(x, float) and not math.isfinite(x):
        return None
    if isinstance(x, dict):
        return {k: finite(v) for k, v in x.items()}
    if isinstance(x, list):
        return [finite(v) for v in x]
    return x


data = finite({"source": source, "features": feat, "extra": extra, "plots": plots, "team": team, "bounds": bounds})
json.dump(data, open(R / "data.json", "w"), allow_nan=False)
for n in feat:
    print(f"{n:26s}", "  ".join(f"{a} {s:,}" for a, s in feat[n]["algo_contest"].items()),
          {k: (v if not isinstance(v, dict) else round(v.get("value") or 0)) for k, v in bounds.get(n, {}).items()})

# ---- page
prose = (HERE / "atlas_prose.js").read_text()
if feat.get("kittens", {}).get("algo_contest", {}).get("rr_knapsack"):
    k, g = feat["kittens"]["algo_contest"]["rr_knapsack"], feat["kittens"]["greedy_contest"]
    prose = prose.replace("/*KITTENS_KCA*/", f"Ruin & recreate with knapsack_reopt reaches {k:,} (+{(k / g - 1) * 100:.2f}%).")
if bounds.get("custom_universallambda42", {}).get("UB_cov"):
    ub, g = bounds["custom_universallambda42"]["UB_cov"], feat["custom_universallambda42"]["greedy_contest"]
    prose = prose.replace(" /*UL_BOUND*/", f" The coverage bound ({ub:,}) leaves at most {(1 - g / ub) * 100:.1f}%; the full LP was too large to finish in 25 minutes, so the real gap is probably smaller.")
body = (HERE / "atlas_body.html").read_text()
body = body.replace("/*DATA*/", json.dumps(data, separators=(",", ":")).replace("</", "<\\/"))
body = body.replace("<script>\n(function () {", "<script>\n" + prose + "\n</script>\n<script>\n(function () {", 1)
Path("atlas.html").write_text((HERE / "atlas_head.html").read_text() + body)
print("wrote atlas.html")
