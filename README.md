# Video Streaming — Google HashCode 2017

Put videos on cache servers so that the endpoints that request them wait less.
The score is the average latency saved for each request, in microseconds.

`DOCUMENTATION.md` records the method and the measurements of each phase. Its
section 0 is a summary and its section 10 collects what the work established;
those two are the ones to read. This file says how to run things.

## Results

| Instance | Best score | Upper bound | Gap | Solver |
| --- | ---: | ---: | ---: | --- |
| me_at_the_zoo | **516,557** | 516,557 | **optimal, proven** | `local_search`, 60 s |
| videos_worth_spreading | **614,080** | 620,471 | 1.03 % | `local_search`, 300 s |
| trending_today | **499,994** | 500,000 | 0.00 % | `genetic`, 60 s |
| kittens | **1,024,977** | 1,035,842 | 1.05 % | `local_search`, 300 s |
| **Total** | **2,655,608** | | | |

The bounds come from the LP relaxation (section 9 of `DOCUMENTATION.md`).
me_at_the_zoo is closed exactly by a MILP. The best solutions are in
`output/<instance>.best.txt`.

## Build

```sh
make          # everything into bin/
make clean    # remove bin/
```

`g++` with C++17 is the only requirement. The Python bound needs `scipy`.

### How the Makefile works

There is no list of programs to maintain. Three wildcard rules find the sources:

| Source | Becomes | Rule |
| --- | --- | --- |
| `src/solvers/*.cpp` | `bin/*` | one solver for each algorithm |
| `src/tools/*.cpp` | `bin/*` | one tool for each measurement |
| `parser.cpp` | `bin/parser` | dump of an instance |

A new solver therefore needs no edit: drop `src/solvers/mine.cpp` and `make`
builds `bin/mine`. Every program depends on all of `src/*.hpp`, thus a change in
a header rebuilds everything that uses it. The `bin` order-only prerequisite
creates `bin/` and `output/` on the first build.

## Running a solver

Every solver takes the instance first, and writes nothing unless `-o` is given.

```sh
./bin/greedy       instances/kittens.in -m lazy -k density -o output/kittens.greedy.txt
./bin/local_search instances/kittens.in -t 60 -r 5        -o output/kittens.local.txt
./bin/genetic      instances/kittens.in -t 60 -P 3 -M 5 -r 5 -a 0.3
./bin/annealing    instances/kittens.in -t 60 -0 1000
./bin/random_solver instances/kittens.in -n 20 -s 42
```

### Reproducing the table above

```sh
./bin/local_search instances/me_at_the_zoo.in          -t 60  -r 3  -a 1.0 -e 0.001
./bin/local_search instances/videos_worth_spreading.in -t 300 -r 10 -e 0.0001
./bin/genetic      instances/trending_today.in         -t 60  -k gain -r 5 -P 6 -M 20
./bin/local_search instances/kittens.in                -t 300 -r 5
```

### Options

Shared: `-o` output file, `-t` seconds, `-s` seed, `-k gain|density` sort key.

| Solver | Option | Meaning |
| --- | --- | --- |
| `greedy` | `-m static\|lazy` | keep the first ranking, or refresh it (CELF) |
| `local_search` | `-r` | caches emptied by one iteration |
| | `-p` | probability of removing each video of a ruined cache |
| | `-a` | noise on the keys of the refill |
| | `-e` | threshold accepting, as a part of the best total |
| `genetic` | `-P` `-M` | population size, ruin and recreate rounds for each child |
| | `-T` `-a` | tournament size, noise of the initial population |
| `annealing` | `-0` `-1` | first and last temperature (auto with neither) |
| | `-i` `-j` | candidates compared for an insertion, for a removal |
| | `-m` | share of transfer moves |

**Timing.** The solvers stop on a wall clock. Two runs of the same command
differ by about 3 %, and two runs at a different moment by up to 45 %. Compare
configurations only inside one group of runs, and never run two timed solvers at
the same time: they share the processor and both lose.

## Checking a solution

```sh
./bin/scorer instances/kittens.in output/kittens.best.txt
```

`scorer` re-reads the instance and the file and recomputes the score from
nothing. It returns 462,500 on the example of the statement. Use it on anything
a solver wrote.

## Bounds

```sh
./bin/lagrangian instances/kittens.in -n 3000 -l 1024977   # LP bound, 9 min
./bin/lagrangian instances/videos_worth_spreading.in -n 2000 -l 614080   # 3 s
python3 src/tools/lp_bound.py instances/me_at_the_zoo.in       # LP, exact
python3 src/tools/lp_bound.py instances/me_at_the_zoo.in -i    # MILP, the optimum
./bin/bound instances/kittens.in output/kittens.best.txt   # both weak bounds
```

`lagrangian` returns a valid bound at **every** iteration, thus `-n` only
decides how tight it gets. `-l` is the best known score and only sets the step
size. This is the tool to use: it bounds kittens, which has 75 million
variables and fits in no LP solver.

`lp_bound.py` builds the real linear program. It closes me_at_the_zoo, and with
`-g m` it merges the caches into `m` groups for a looser bound on a larger
instance. Use `--method highs-ipm` above a few thousand rows: the simplex does
not finish.

## Layout

```
instances/      the four .in files
src/*.hpp       structures, parser, filters, score, greedy
src/solvers/    one .cpp for each algorithm
src/tools/      scorer and the bounds
output/         solutions, <instance>.<solver>.txt
```
