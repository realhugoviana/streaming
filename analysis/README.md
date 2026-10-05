# Instance analysis

Scripts behind the two study pages: the **instance atlas** (features, provable upper bounds and
how each instance reacts to a change of algorithm) and the **cache replay** (the problem on
`instances/example.in` plus step-by-step replays of `greedy_density` and the knapsack
re-optimisation).

## Run everything

```bash
./run_all.sh
```

Needs `g++` (C++17), Python 3 with `numpy` and `scipy` ≥ 1.9 (for HiGHS `milp`/`linprog`).
All output goes to `analysis/out/` (git-ignored); the pages are `out/atlas.html` and
`out/replay.html`. A full run takes about 45 minutes; the slow steps are the team greedy on
kittens (~4 min), the two full LPs (~2 and ~8 min) and the kittens re-optimisation, which runs
twice (~6 min each).

Every Python step runs with `analysis/out` as the working directory and reads/writes
`res/`, `sol/` and `traces/` there; run them the same way if you run one by hand.

## Files

| File | What it does | Writes |
|---|---|---|
| `team_driver.cpp` | Compiles the team's `parser.cpp` + solvers unchanged (only `main` is swapped) and runs `greedy_density`, optionally `local_search`. | `sol/<inst>.greedy.txt`, JSON scores on stdout |
| `inst.py` | Shared loader: parses an instance, merges duplicate request lines, builds the endpoint×cache saving matrix; independent scorer. | – |
| `features.py` | Structural features, simple bounds and greedy-solution structure per instance. | `res/features.json` |
| `extra_stats.py` | Instance-specific figures quoted in the atlas (universalLambda parity, dejaVu kernels and duplicated links, trending leftovers…). | `res/extra.json` |
| `plots_data.py` | Size histograms and popularity curves for the atlas profiles. | `res/plots.json` |
| `bounds.py` | Upper bounds: coverage, aggregated LP, request LP, full facility-location LP, and the exact MIP (HiGHS). | `res/<inst>.bounds.*.json`, `sol/<inst>.mip.txt` |
| `experiments.cpp` | Lazy greedy by density or by raw gain, and the per-cache exact knapsack re-optimisation. | JSON lines → `res/exp.log` |
| `trace.cpp` | Records every greedy placement (team tie-breaking, identical output) and every knapsack cache rewrite. | `traces/<inst>.json` |
| `report/build_atlas.py` | Merges all results into `res/data.json` and builds the atlas page. | `atlas.html` |
| `report/build_replay.py` | Builds the replay page from the traces. | `replay.html` |

## Things to know

- **Latency reading on custom_dejavu42.** The generator can link an endpoint to the same cache
  twice. The atlas uses the best link; the replay (`trace … 1`) uses the first one, like the
  team's `latencyToCache`, so its scores are lower.
- **universalLambda LP.** Its full or request-level LP does not finish within 25 minutes, so
  `run_all.sh` only computes the coverage bound and aggregated LP for it.
- **Prose is not regenerated.** Tables, charts and the numbers bound to data are rebuilt from
  the results, but the explanatory text in `report/atlas_prose.js`, `report/atlas_body.html`
  and `report/replay_body.html` was written for the 2026-09-27 results. Re-read it after
  changing the solver.
- `team_driver` prints `Rdedup` from the team's parser, which is off by the uninitialised
  `Rprime` in `requestParser` (`parser.cpp`) until that is fixed.
