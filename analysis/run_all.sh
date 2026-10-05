#!/usr/bin/env bash
# Reproduces the whole instance study: team greedy runs, features, bounds, experiments,
# execution traces, then both pages (out/atlas.html, out/replay.html).
# Everything is written under analysis/out/. Full run: about 45 minutes on 8 cores.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(dirname "$HERE")"
GEN="$REPO/instances/new_instance_and_gen/instanceCreatorHashcode2016/instances"
OUT="$HERE/out"
mkdir -p "$OUT"/{bin,res,sol,traces}
cd "$OUT"

declare -A INST=(
  [example]="$REPO/instances/example.in"
  [me_at_the_zoo]="$REPO/instances/me_at_the_zoo.in"
  [videos_worth_spreading]="$REPO/instances/videos_worth_spreading.in"
  [trending_today]="$REPO/instances/trending_today.in"
  [kittens]="$REPO/instances/kittens.in"
  [custom_dejavu42]="$GEN/custom_dejavu42.in"
  [custom_universallambda42]="$GEN/custom_universallambda42.in"
)
ALL=(example me_at_the_zoo videos_worth_spreading trending_today kittens custom_dejavu42 custom_universallambda42)
REAL=("${ALL[@]:1}")

echo "== build"
g++ -std=c++17 -O2 -w -o bin/team_driver "$HERE/team_driver.cpp"
g++ -std=c++17 -O3 -march=native -o bin/experiments "$HERE/experiments.cpp"
g++ -std=c++17 -O3 -march=native -o bin/trace "$HERE/trace.cpp"

echo "== team greedy (the team's own code; kittens alone takes ~4 min)"
for n in "${ALL[@]}"; do
  bin/team_driver "sol/$n.greedy.txt" 0 < "${INST[$n]}" > "res/$n.greedy.json"
  echo "$n $(grep -o '"greedy_contest":[0-9]*' "res/$n.greedy.json")"
done

echo "== features and statistics"
python3 "$HERE/features.py" > /dev/null
python3 "$HERE/extra_stats.py" > /dev/null
python3 "$HERE/plots_data.py"

echo "== upper bounds (the two full LPs take ~2 and ~8 min)"
python3 "$HERE/bounds.py" example cov alp rlp lp mip
python3 "$HERE/bounds.py" me_at_the_zoo cov alp rlp lp mip
python3 "$HERE/bounds.py" trending_today cov
python3 "$HERE/bounds.py" kittens cov
python3 "$HERE/bounds.py" custom_dejavu42 cov alp lp
python3 "$HERE/bounds.py" custom_universallambda42 cov alp   # its LP does not finish in 25 min
python3 "$HERE/bounds.py" videos_worth_spreading cov alp lp

echo "== algorithm-response experiments (kittens re-optimisation ~6 min)"
: > res/exp.log
for n in "${REAL[@]}"; do
  for m in greedy_density greedy_gain; do
    echo "$n $m $(bin/experiments "${INST[$n]}" $m - "sol/$n.$m.txt")" >> res/exp.log
  done
  echo "$n kca_from_team_greedy $(bin/experiments "${INST[$n]}" kca "sol/$n.greedy.txt" "sol/$n.kca.txt" 40)" >> res/exp.log
  echo "$n done"
done

echo "== execution traces (kittens ~6 min)"
for n in "${REAL[@]}"; do
  first_link=0; [ "$n" = custom_dejavu42 ] && first_link=1   # read duplicated links like the team's code
  bin/trace "${INST[$n]}" "traces/$n.json" $first_link 2> "res/trace_$n.log"
  echo "$n $(head -1 "res/trace_$n.log")"
done

echo "== pages"
python3 "$HERE/report/build_atlas.py"
python3 "$HERE/report/build_replay.py"
echo "done: $OUT/atlas.html and $OUT/replay.html"
