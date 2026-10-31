# Streaming Videos — Google HashCode 2017

Advanced Problem Solving, M2 — **Hugo Viana, Arno Lesage, Justin Diter**

## Problem

A video platform serves its videos from a data center and from cache servers of fixed capacity
`X` MB. Each endpoint is linked to the data center and to some of the caches, each with its own
latency. A request description says how many times an endpoint asks for a video. The task is to
choose which videos each cache stores.

A request is served by the lowest-latency cache that is linked to its endpoint and holds the video,
otherwise by the data center. The score is the latency saved over all requests, divided by the
total number of requests and multiplied by 1000:

```
score = floor( 1000 · Σ_r count_r · (L_datacenter(e_r) − L_best_cache(r)) / Σ_r count_r )
```

## Approach

`parser.cpp` reads an instance on stdin and runs the following pipeline, built from the solvers in
`src/solvers/`.

```
            ┌───────────────────────┐
 instance ─▶│ greedy by density     │
            └───────────┬───────────┘
                        ▼
      ┌─────────────────────────────────────┐
      │ 30 × ruin & recreate                │◀─┐
      │ 1  × knapsack re-optimisation       │  │ up to 5000 rounds,
      └─────────────────┬───────────────────┘──┘ or until SIGINT / SIGTERM
                        ▼
                 best solution (stdout)
```

**Greedy by density** (`greedy_density.cpp`). Every (video, cache) pair has a gain: the latency it
would save on top of the current placements. The greedy repeatedly places the pair with the highest
gain per MB, kept in a lazy max-heap: when a placement changes a pair's gain, the new value is
pushed and the outdated entry is dropped when it reaches the top. With
probability `1 − p_best` it places the runner-up instead, which makes each run a different
solution.

**Ruin & recreate** (`local_search.cpp`). Picks `C/5` random caches, removes each of their videos
with probability `p_ruin`, then refills the freed space with the randomized greedy. The new solution
replaces the current one when its score is higher. The same file holds the add/remove and swap
moves and the score update shared by all solvers.

**Knapsack re-optimisation** (`knapsack_reopt.cpp`). Takes the caches one at a time and solves the
cache exactly as a 0/1 knapsack, with the other caches fixed: each candidate video is worth the
extra latency it saves given what the other caches already serve, and the dynamic program picks the
best set that fits in `X`.

### Parameters

| Parameter | Value | Where | Role |
|---|---|---|---|
| `p_best` | 0.85 | `greedy_density.cpp` | probability of placing the best pair (1 = deterministic greedy) |
| `p_ruin` | 0.85 | `local_search.cpp` | probability of removing each video of a ruined cache |
| caches per ruin | `C/5` | `parser.cpp` | number of random caches ruined per step |
| ruin & recreate steps | 30 per round | `parser.cpp` | steps between two knapsack passes |
| rounds | 5000 | `parser.cpp` | outer iterations |

## Build and run

Needs `g++` with C++17.

```bash
make
```

This builds `bin/parser` and `bin/judge`, the official HashCode judge (`src/tools/judge.cpp`).
Instance files go in `instances/` ([instances/README.md](instances/README.md)).

Solve an instance with a time limit. On `SIGINT` the parser finishes its current move and prints
the best solution found:

```bash
timeout --foreground -s INT 60 bin/parser < instances/example.in > example.out
```

Score it:

```bash
bin/judge instances/example.in example.out
```

### Contest run

`run_final.sh` runs the parser on every instance of `instances/instances_contest/` in parallel,
judges each output and keeps the best valid solution per instance in `solutions_contest/`. The
arguments are the seconds per instance and the number of parallel jobs:

```bash
./run_final.sh 600 15
```

## Results

Final judge scores on the 15 contest instances, keeping the best solution found for each:

| Instance | Score | Method |
|---|---:|---|
| 01 | 986 404 | Greedy + ruin & recreate + knapsack |
| 02 | 984 635 | Greedy + ruin & recreate + knapsack |
| 03 | 2 550 000 | Greedy + ruin & recreate + knapsack |
| 04 | 1 956 202 | Greedy + ruin & recreate + knapsack |
| 05 | 5 297 270 | Greedy + ruin & recreate + knapsack |
| 06 | 1 542 411 | Greedy + ruin & recreate + knapsack |
| 07 | 1 094 813 | Greedy + ruin & recreate + knapsack |
| 08 | 53 411 | Greedy + ruin & recreate |
| 09 | 1 022 332 | Greedy + ruin & recreate + knapsack |
| 10 | 581 696 | Greedy + ruin & recreate + knapsack |
| 11 | 132 066 | Greedy + ruin & recreate + knapsack |
| 12 | 329 758 | Greedy + ruin & recreate + knapsack |
| 13 | 5 066 500 | Greedy by density |
| 14 | 57 482 | Greedy + ruin & recreate + knapsack |
| 15 | 292 212 | Greedy + ruin & recreate + knapsack |
| **Total** | **21 947 192** | |

## Instance study

`analysis/` holds the study of the instances: their features, provable upper bounds (coverage
bound, LP relaxations), how each configuration of our solvers performs on each instance, and
experiments on small generated instances. It builds two pages, the instance atlas and a
step-by-step replay of the greedy and the knapsack re-optimisation. See
[analysis/README.md](analysis/README.md).

Main findings:

- Ranking pairs by gain per MB is what makes the greedy strong: on the HashCode instances it lands
  within 2% of the best upper bound.
- Capacity pressure (cache space against the volume of requested videos), videos per cache, overlap
  between caches and spread of video sizes are the features that decide which method gains most.
- Knapsack re-optimisation pays off most where caches are tightly packed: the full pipeline scores
  +81% over the greedy on instance09, +41% on instance01 and +32% on instance15.

The two presentations are in `presentations/`, with the R code of the PCA and correlation plots.
The instance generator written for the study is in `generator/` (`git submodule update --init`).

## Repository layout

| Path | Contents |
|---|---|
| `parser.cpp` | Instance parser, main search loop, solution output |
| `src/instance.hpp` | Instance data structures, scoring, copy and free helpers |
| `src/solvers/` | Greedy by density, local search and ruin & recreate, knapsack re-optimisation |
| `src/tools/judge.cpp` | Official HashCode judge |
| `run_final.sh` | Contest run with a time limit per instance |
| `analysis/` | Instance study and report pages |
| `generator/` | Instance generator (git submodule) |
| `presentations/` | Slides and plotting code |
| `instances/` | Instance files ([instances/README.md](instances/README.md)) |
