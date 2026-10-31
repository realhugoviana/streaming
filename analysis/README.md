# Instance analysis

Scripts behind the two study pages: the **instance atlas** (features, provable upper bounds and
how each instance reacts to each of the team's algorithms) and the **cache replay** (the problem on
`instances/example.in` plus step-by-step replays of `greedy_density` and the knapsack
re-optimisation).

## Run everything

```bash
./run_all.sh
```

Needs `g++` (C++17), Python 3.9+ with `numpy` and `scipy` ≥ 1.9 (for HiGHS `milp`/`linprog`).
It studies the local contest set, `instances/instances_contest/`, and writes to
`analysis/out/contest/` (git-ignored); the page is `out/contest/atlas.html`. Each instance is run
with four configurations of the team's own code (`team_driver.cpp`):

| Name | Configuration |
|---|---|
| `greedy` | `greedy_density`, always the best pair (deterministic) |
| `greedy90` | `greedy_density`, best pair with probability 0.9, otherwise the runner-up |
| `rr` | `greedy90`, then ruin & recreate until the time limit: each iteration empties C/3 random caches (ruin probability 1) and refills them with `greedy90` |
| `rr_knapsack` | as `parser.cpp`'s `main`: `greedy90`, then rounds of 30 ruin & recreate iterations + `knapsack_reopt`, until the time limit |

The time limit of the last two is `RR_SECONDS` (default 600, as `run_final.sh`), counted from
the start. A full run takes about an hour: four rounds of 600 s runs on 8 cores, then the full LPs
(up to 15 min each). For a quick look, `RR_SECONDS=60 ./run_all.sh`.

To study another folder of instances (`*.in` or `*.txt`), set `INST_DIR` (and `OUT`). The
original study, with both pages in `analysis/out/`, is:

```bash
INST_DIR=../instances OUT=out ./run_all.sh
```

The atlas shows every instance of the folder. The summary, feature cards, issues and
per-instance prose belong to the six study instances, and appear only when they are in the
folder; so does the replay page.

Every Python step runs with the output folder as the working directory and reads/writes
`res/`, `sol/` and `traces/` there; run them the same way if you run one by hand, with
`STREAM_INSTANCES` set to the instance folder if it is not the contest set.

## Files

| File | What it does | Writes |
|---|---|---|
| `team_driver.cpp` | Compiles the team's `parser.cpp` + solvers unchanged (only `main` is swapped) and runs one configuration: `team_driver <solution> <greedy\|greedy90\|rr\|rr_knapsack> [seconds] < instance`. | `sol/<inst>.<algorithm>.txt`, JSON scores and score-over-time trace → `res/<inst>.<algorithm>.json` |
| `inst.py` | Shared loader: parses an instance, merges duplicate request lines, builds the endpoint×cache saving matrix; independent scorer. Reads `instances/instances_contest/`, or `$STREAM_INSTANCES`. | – |
| `features.py` | Structural features, simple bounds, base-greedy solution structure, and every algorithm's solution re-scored, per instance. | `res/features.json` |
| `extra_stats.py` | Instance-specific figures quoted in the atlas (universalLambda parity, dejaVu kernels and duplicated links, trending leftovers…). | `res/extra.json` |
| `plots_data.py` | Size histograms and popularity curves for the atlas profiles. | `res/plots.json` |
| `bounds.py` | Upper bounds: coverage, aggregated LP, request LP, full facility-location LP, and the exact MIP (HiGHS). | `res/<inst>.bounds.*.json`, `sol/<inst>.mip.txt` |
| `trace.cpp` | Records every greedy placement (team tie-breaking, identical output) and every knapsack cache rewrite. | `traces/<inst>.json` |
| `report/build_atlas.py` | Merges all results into `res/data.json` and builds the atlas page. | `atlas.html` |
| `report/build_replay.py` | Builds the replay page from the traces. | `replay.html` |

## Things to know

- **Latency reading on custom_dejavu42.** The generator can link an endpoint to the same cache
  twice. The atlas uses the best link; the replay (`trace … 1`) uses the first one, like the
  team's `latencyToCache`, so its scores are lower.
- **universalLambda LP.** Its full or request-level LP does not finish within 25 minutes, so
  `run_all.sh` only computes the coverage bound and aggregated LP for it.
- **Three of the four configurations are random.** `greedy_density` (with `p_best` < 1) and
  `ruin` use an unseeded generator, so scores change from run to run; each is run once.
- **The probabilities are parameters.** `greedy_density(instance, p_best)`, `ruin(instance,
  num_moves, p_ruin)` and `ruin_recreate(..., p_best, p_ruin)` default to the values `parser.cpp`
  uses; `team_driver.cpp` passes its own.
- **Prose is not regenerated.** Tables, charts and the numbers bound to data are rebuilt from
  the results, but the explanatory text in `report/atlas_prose.js`, `report/atlas_body.html`
  and `report/replay_body.html` was written for the 2026-09-27 results. Re-read it after
  changing the solver.
