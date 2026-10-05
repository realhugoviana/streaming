"""Collect every result file into res/data.json and build the atlas page (atlas.html).

Run from analysis/out after features, extra_stats, plots_data, bounds and experiments.
Tables and charts are data-driven; the prose in atlas_prose.js and parts of atlas_body.html
were written for the 2026-09-27 results and should be re-read if the solver changes.
"""
import glob
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
R = Path("res")

feat = json.load(open(R / "features.json"))
extra = json.load(open(R / "extra.json"))
plots = json.load(open(R / "plots.json"))
team = {n: json.load(open(R / f"{n}.greedy.json")) for n in feat}

exp = {}
for line in (R / "exp.log").read_text().splitlines():
    m = re.match(r"(\S+) (\S+) (\{.*\})$", line)
    if m:
        exp.setdefault(m.group(1), {})[m.group(2)] = json.loads(m.group(3))

bounds = {}
for f in glob.glob(str(R / "*.bounds.*.json")):
    d = json.load(open(f))
    bounds.setdefault(d["name"], {}).update({k: v for k, v in d.items() if k != "name"})

data = {"features": feat, "extra": extra, "plots": plots, "team": team, "exp": exp, "bounds": bounds}
json.dump(data, open(R / "data.json", "w"))
for n in feat:
    print(f"{n:26s} team {team[n]['greedy_contest']:>9,}",
          {k: (v if not isinstance(v, dict) else round(v.get("value") or 0)) for k, v in bounds.get(n, {}).items()},
          {k: v["final_contest"] for k, v in exp.get(n, {}).items()})

# ---- page
prose = (HERE / "atlas_prose.js").read_text()
k = exp["kittens"]["kca_from_team_greedy"]
g = feat["kittens"]["greedy_contest"]
prose = prose.replace("/*KITTENS_KCA*/", f"The per-cache knapsack adds +{(k['final_contest'] / g - 1) * 100:.2f}% ({k['final_contest']:,}) in {len(k['sweeps'])} sweeps.")
ub = bounds["custom_universallambda42"]["UB_cov"]
g = feat["custom_universallambda42"]["greedy_contest"]
prose = prose.replace(" /*UL_BOUND*/", f" The coverage bound ({ub:,}) leaves at most {(1 - g / ub) * 100:.1f}%; the full LP was too large to finish in 25 minutes, so the real gap is probably smaller.")
body = (HERE / "atlas_body.html").read_text()
body = body.replace("/*DATA*/", json.dumps(data, separators=(",", ":")).replace("</", "<\\/"))
body = body.replace("<script>\n(function () {", "<script>\n" + prose + "\n</script>\n<script>\n(function () {", 1)
Path("atlas.html").write_text((HERE / "atlas_head.html").read_text() + body)
print("wrote atlas.html")
