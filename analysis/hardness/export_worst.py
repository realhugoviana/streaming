"""Export the worst instance per size limit (from hardness/adversary.json) as valid .in files.

Each file is re-checked: the gap against the exact optimum, and the team's compiled greedy
(bin/team_driver) must score what the Python greedy predicts.
usage (from analysis/out): python3 ../hardness/export_worst.py
Writes analysis/hardness/instances/*.in and a summary table on stdout.
"""
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from small import from_json, greedy, optimum, normalise  # noqa: E402
from adversary import make_valid  # noqa: E402
from traps import TRAPS  # noqa: E402

NAMES = {"any size (≤ X)": "worst_any_size", "≤ X/2": "worst_size_le_X_over_2", "≤ X/4": "worst_size_le_X_over_4",
         "≤ X/10": "worst_size_le_X_over_10", "unit size": "worst_unit_size"}
out = HERE / "instances"
out.mkdir(exist_ok=True)
adv = json.load(open("hardness/adversary.json"))
print(f"{'file':28s} {'V':>3} {'E':>2} {'C':>2} {'X':>4}  {'greedy':>8} {'optimum':>8}  gap     team binary")
for variant, name in NAMES.items():
    best = None
    # candidates: the search's instance and gadget made valid by scaling or by clipping, and the hand-built trap
    cands = [fix(from_json(adv[variant][k])) for k in ("found", "gadget") for fix in (normalise, make_valid)] + [TRAPS[variant]]
    for inst in cands:
        _, g = greedy(inst)
        _, o, _, st = optimum(inst, time_limit=60)
        assert st == "optimal"
        if best is None or 1 - g / o > best[1]:
            best = (inst, 1 - g / o, g, o)
    inst, gap, g, o = best
    path = out / f"{name}.in"
    path.write_text(inst.to_in())
    team = subprocess.run(["bin/team_driver", "/dev/null", "0"], stdin=open(path), capture_output=True, text=True, check=True).stdout
    team_contest = json.loads(team)["greedy_contest"]
    ok = "matches" if team_contest == inst.contest(g) else f"MISMATCH ({team_contest})"
    print(f"{path.name:28s} {inst.V:3d} {inst.E:2d} {inst.C:2d} {inst.X:4d}  {inst.contest(g):8,} {inst.contest(o):8,}  {gap:6.2%}  {ok}")
