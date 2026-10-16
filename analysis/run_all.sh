#!/usr/bin/env bash
# Reproduces the instance study on every instance in instances/ (not its subfolders): the team's
# greedy, features, upper bounds, algorithm experiments (including the knapsack re-optimisation),
# then both pages (out/atlas.html, out/replay.html). The replay page keeps its six instances.
# Everything is written under analysis/out/. Per-instance steps run in parallel (JOBS, default: all cores).
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(dirname "$HERE")"
OUT="$HERE/out"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
export HERE REPO
mkdir -p "$OUT"/{bin,res,res/exp,res/bounds,sol,traces}
cd "$OUT"

NAMES=$(cd "$REPO/instances" && ls -- *.in | sed 's/\.in$//')
REPLAY="me_at_the_zoo videos_worth_spreading trending_today kittens custom_dejavu42 custom_universallambda42"
echo "$(echo "$NAMES" | wc -l) instances in $REPO/instances, $JOBS jobs in parallel"
# each <parallel jobs> '<sh script using $1 as the instance name>'
each() { printf '%s\n' $NAMES | xargs -P "$1" -n 1 sh -c "$2" _; }
cxx() { g++ -std=c++17 "$@" -march=native 2> /dev/null || g++ -std=c++17 "$@"; }   # Apple clang may refuse -march=native

echo "== build"
g++ -std=c++17 -O2 -w -o bin/team_driver "$HERE/team_driver.cpp"
cxx -O3 -o bin/experiments "$HERE/experiments.cpp"
cxx -O3 -o bin/trace "$HERE/trace.cpp"

echo "== team greedy (the team's own code; the largest instances take several minutes)"
each "$JOBS" 'bin/team_driver "sol/$1.greedy.txt" 0 < "$REPO/instances/$1.in" > "res/$1.greedy.json" 2> /dev/null || { echo "FAILED team greedy: $1"; rm -f "res/$1.greedy.json"; }'

echo "== features and statistics"
python3 "$HERE/features.py" > /dev/null
python3 "$HERE/extra_stats.py" > /dev/null
python3 "$HERE/plots_data.py"

echo "== upper bounds (chosen by size; full LPs can take up to 15 min each)"
rm -f res/*.bounds.*.json
each 3 'python3 "$HERE/bounds.py" "$1" auto > "res/bounds/$1.log" 2>&1 || echo "FAILED bounds: $1"'

echo "== algorithm experiments (knapsack re-optimisation: at most 10 min per instance)"
rm -f res/exp/*.log
each "$JOBS" 'n=$1; f="$REPO/instances/$n.in"; [ -f "res/$n.greedy.json" ] || exit 0;
  { for m in greedy_density greedy_gain; do echo "$n $m $(bin/experiments "$f" $m - "sol/$n.$m.txt")"; done;
    echo "$n kca_from_team_greedy $(bin/experiments "$f" kca "sol/$n.greedy.txt" "sol/$n.kca.txt" 40 600)"; } > "res/exp/$n.log" 2>&1 || echo "FAILED experiments: $n"'
cat res/exp/*.log > res/exp.log

echo "== execution traces for the replay page"
printf '%s\n' $REPLAY | xargs -P "$JOBS" -n 1 sh -c 'fl=0; [ "$1" = custom_dejavu42 ] && fl=1;
  bin/trace "$REPO/instances/$1.in" "traces/$1.json" $fl 2> "res/trace_$1.log" || echo "FAILED trace: $1"' _

echo "== pages"
python3 "$HERE/report/build_atlas.py"
python3 "$HERE/report/build_replay.py"
echo "done: $OUT/atlas.html and $OUT/replay.html"
