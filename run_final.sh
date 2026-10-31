#!/usr/bin/env bash
# Runs bin/parser on every contest instance in parallel with a wall-clock limit, then keeps, for each
# instance, the best judge-valid solution in solutions_contest/instanceNN_A.txt (never overwrites a
# better one).
# Usage: ./run_final.sh [seconds per instance, default 600] [parallel jobs, default 15]
set -u
cd "$(dirname "$0")"

export LIMIT=${1:-600}
JOBS=${2:-15}
export IN=instances/instances_contest OUT=solutions_contest RUNS=runs_contest LOGS=logs_contest
mkdir -p "$OUT" "$RUNS" "$LOGS"
make -s bin/parser bin/judge || exit 1

# Judge score of solution $2 for instance $1, 0 if the file is missing, empty or rejected
score() {
    [ -s "$2" ] || { echo 0; return; }
    local s
    s=$(timeout 120 bin/judge "$1" "$2" | sed -n 's/^Score = //p')
    echo "${s:-0}"
}

# Runs one instance, then promotes its solution if it beats the current _A file
run_one() {
    local f=$1 n new best s_new s_old verdict
    n=$(basename "$f" .txt)
    new=$RUNS/$n.txt
    best=$OUT/${n}_A.txt

    # SIGINT at the limit: the parser stops after its current move and prints its best solution
    timeout --foreground -s INT "$LIMIT" bin/parser < "$f" > "$new" 2> "$LOGS/$n.log"

    s_new=$(score "$f" "$new")
    s_old=$(score "$f" "$best")
    if [ "$s_new" -gt "$s_old" ]; then
        cp "$new" "$best.tmp" && mv "$best.tmp" "$best"
        verdict="improved"
    else
        verdict="kept previous"
    fi
    printf '%s  new=%s  previous=%s  -> %s\n' "$n" "$s_new" "$s_old" "$verdict"
}
export -f score run_one

echo "Running $(ls "$IN"/*.txt | wc -l) instances, $JOBS at a time, ${LIMIT}s each..."
ls "$IN"/*.txt | xargs -P "$JOBS" -I{} bash -c 'run_one {}'

# Final scores of what is in solutions_contest
echo
echo "Final solutions_contest/*_A.txt:"
total=0
for f in "$IN"/*.txt; do
    n=$(basename "$f" .txt)
    s=$(score "$f" "$OUT/${n}_A.txt")
    printf '  %s_A  %s\n' "$n" "$s"
    total=$((total + s))
done
echo "  TOTAL  $total"
