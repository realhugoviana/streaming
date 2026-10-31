# Streaming Videos — Google HashCode 2017

Advanced Problem Solving project (M2) by Vianna, Diter and Lesage.

Videos have to be placed in capacity-limited cache servers so that endpoints fetch them with the
least latency. The score is the latency saved per request, averaged over all requests and scaled by
1000 (the contest instances use the same scoring).

## Approach

`parser.cpp` reads an instance on stdin and chains the solvers in `src/solvers/`:

1. **Greedy by density** (`greedy_density.cpp`): repeatedly places the (video, cache) pair with the
   highest gain per MB, using a lazy max-heap. With probability `1 - p_best` it places the runner-up
   instead, so that repeated runs explore different solutions.
2. **Ruin & recreate** (`local_search.cpp`): empties part of a few random caches (each video is
   removed with probability `p_ruin`), refills them with the greedy and keeps the result if it is
   better. The same file has the add/remove and swap moves of the local search.
3. **Knapsack re-optimisation** (`knapsack_reopt.cpp`): solves each cache in turn exactly as a 0/1
   knapsack over its candidate videos, with the other caches fixed.

Steps 2 and 3 alternate until the iteration budget runs out or the process gets `SIGINT`/`SIGTERM`.
Either way it prints the best solution found to stdout in the judge format. Progress goes to stderr.

## Build and run

Needs `g++` with C++17.

```bash
make
```

This builds `bin/parser` and `bin/judge` (the official HashCode judge, `src/tools/judge.cpp`).

```bash
timeout --foreground -s INT 60 bin/parser < instances/example.in > example.out
```

```bash
bin/judge instances/example.in example.out
```

Instance files are not in the repository; see [instances/README.md](instances/README.md).

### Contest run

`run_final.sh` runs the parser on every instance in `instances/instances_contest/` in parallel,
judges each output, and keeps the best valid solution per instance in `solutions_contest/`:

```bash
./run_final.sh 600 15
```

The arguments are the seconds per instance and the number of parallel jobs.

## Results

Judge scores of the final solutions on the 15 contest instances:

| Instance | Score | Instance | Score | Instance | Score |
|---|---:|---|---:|---|---:|
| 01 | 986 404 | 06 | 1 542 411 | 11 | 132 066 |
| 02 | 984 635 | 07 | 1 094 813 | 12 | 329 758 |
| 03 | 2 550 000 | 08 | 53 370 | 13 | 5 066 462 |
| 04 | 1 956 202 | 09 | 1 022 070 | 14 | 57 482 |
| 05 | 5 297 270 | 10 | 581 696 | 15 | 292 212 |

**Total: 21 946 851**

## Repository layout

| Path | Contents |
|---|---|
| `parser.cpp` | Instance parser, main search loop, solution output |
| `src/instance.hpp` | Instance data structures, scoring, copy/free helpers |
| `src/solvers/` | Greedy, local search / ruin & recreate, knapsack re-optimisation |
| `src/tools/judge.cpp` | Official judge |
| `run_final.sh` | Contest run with a time limit per instance |
| `analysis/` | Instance study: features, upper bounds, hardness experiments, report pages ([README](analysis/README.md)) |
| `generator/` | Instance generator (git submodule: `git submodule update --init`) |
| `presentations/` | Slides of the two presentations, R code for the PCA/correlation plots |
