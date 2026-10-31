#!/usr/bin/env bash
# Reproduces the instance study on every instance of the local contest set, instances/instances_contest/
# (not its subfolders): the team's four algorithms (team_driver.cpp, with parser.cpp's code), features,
# upper bounds, then the pages (atlas.html, and replay.html when its six instances are present).
# Everything is written under analysis/out/contest/. Per-instance steps run in parallel (JOBS, default: all cores).
#   INST_DIR=dir    study the *.in / *.txt files of another folder; the original study is
#                   INST_DIR=../instances OUT=out ./run_all.sh
#   OUT=dir         write there instead of analysis/out/contest
#   RR_SECONDS=s    time limit of the two ruin & recreate algorithms, per instance (default 600, as run_final.sh)
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(dirname "$HERE")"
INST_DIR="$(cd "${INST_DIR:-$REPO/instances/instances_contest}" && pwd)"
OUT="${OUT:-$HERE/out/contest}"
RR_SECONDS="${RR_SECONDS:-600}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
mkdir -p "$OUT"/{bin,res,res/bounds,sol,traces}
OUT="$(cd "$OUT" && pwd)"
export HERE REPO RR_SECONDS STREAM_INSTANCES="$INST_DIR"
cd "$OUT"

FILES=$(find "$INST_DIR" -maxdepth 1 -type f \( -name '*.in' -o -name '*.txt' \) | sort)
REPLAY="me_at_the_zoo videos_worth_spreading trending_today kittens custom_dejavu42 custom_universallambda42"
for n in $REPLAY; do [ -f "$INST_DIR/$n.in" ] || REPLAY=""; done   # the replay page needs all six
echo "$(echo "$FILES" | wc -l) instances in $INST_DIR, $JOBS jobs in parallel"
# each <parallel jobs> '<sh script using $1 as the instance file and $n as its name>'
each() { printf '%s\n' $FILES | xargs -P "$1" -n 1 sh -c 'n=$(basename "${1%.*}"); '"$2" _; }

echo "== build"
g++ -std=c++17 -O2 -w -o bin/team_driver "$HERE/team_driver.cpp"

NI=$(echo "$FILES" | wc -l)
echo "== team algorithms: greedy, greedy90, and rr, rr_knapsack for ${RR_SECONDS}s each (about $(( (2 * NI + JOBS - 1) / JOBS * RR_SECONDS / 60 )) min)"
# one run per (instance, algorithm), the time-limited ones first so the parallel runs end together
for a in rr_knapsack rr greedy90 greedy; do printf "%s $a\n" $FILES; done | xargs -P "$JOBS" -n 2 sh -c 'n=$(basename "${1%.*}");
  bin/team_driver "sol/$n.$2.txt" "$2" "$RR_SECONDS" < "$1" > "res/$n.$2.json" 2> /dev/null || { echo "FAILED $2: $n"; rm -f "res/$n.$2.json"; }' _

echo "== features and statistics"
python3 "$HERE/features.py" > /dev/null
python3 "$HERE/extra_stats.py" > /dev/null
python3 "$HERE/plots_data.py"

echo "== upper bounds (chosen by size; full LPs can take up to 15 min each)"
rm -f res/*.bounds.*.json
each 3 'python3 "$HERE/bounds.py" "$n" auto > "res/bounds/$n.log" 2>&1 || echo "FAILED bounds: $n"'

echo "== pages"
python3 "$HERE/report/build_atlas.py"
if [ -n "$REPLAY" ]; then
  echo "== execution traces for the replay page"
  cxx() { g++ -std=c++17 "$@" -march=native 2> /dev/null || g++ -std=c++17 "$@"; }   # Apple clang may refuse -march=native
  cxx -O3 -o bin/trace "$HERE/trace.cpp"
  printf '%s\n' $REPLAY | xargs -P "$JOBS" -n 1 sh -c 'fl=0; [ "$1" = custom_dejavu42 ] && fl=1;
    bin/trace "$STREAM_INSTANCES/$1.in" "traces/$1.json" $fl 2> "res/trace_$1.log" || echo "FAILED trace: $1"' _
  python3 "$HERE/report/build_replay.py"
fi
echo "done: $OUT/atlas.html"
