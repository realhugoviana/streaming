"""Check that small.greedy reproduces the team's compiled greedy_density placement by placement.

usage (from analysis/out, after run_all.sh built bin/team_driver): python3 ../hardness/check_team.py [n]
"""
import subprocess
import sys
import time
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from small import random_instance, greedy, optimum  # noqa: E402

n = int(sys.argv[1]) if len(sys.argv) > 1 else 300
Path("hardness").mkdir(exist_ok=True)
rng = np.random.default_rng(12345)
same = 0
mip_t = []
for i in range(n):
    inst = random_instance(rng)
    path = Path("hardness/check.in")
    path.write_text(inst.to_in())
    subprocess.run(["bin/team_driver", "hardness/check_sol.txt", "0"], stdin=open(path), stdout=subprocess.DEVNULL, check=True)
    team = np.zeros((inst.V, inst.C), bool)
    for line in Path("hardness/check_sol.txt").read_text().splitlines():
        parts = list(map(int, line.split()))
        if parts:
            team[parts[1:], parts[0]] = True
    mine, w = greedy(inst)
    same += bool((team == mine).all())
    if (team != mine).any():
        print("MISMATCH on instance", i)
        Path(f"hardness/mismatch_{i}.in").write_text(inst.to_in())
    t = time.perf_counter(); optimum(inst); mip_t.append(time.perf_counter() - t)
print(f"{same}/{n} instances: identical placements to the team's compiled greedy")
print(f"MIP time per instance: median {np.median(mip_t)*1000:.0f} ms, 99th pct {np.quantile(mip_t, .99)*1000:.0f} ms, max {max(mip_t)*1000:.0f} ms")
