# Video Streaming — Google HashCode 2017

This document records the work on the problem. Each phase has its own section,
in the order it was done, including the attempts that failed. `README.md` says
how to build and run everything.

## 0. Summary

| Instance | Best score | Upper bound | Gap | Found by |
| --- | ---: | ---: | ---: | --- |
| me_at_the_zoo | **516,557** | 516,557 | **optimal, proven** | phase 4, 60 s |
| videos_worth_spreading | **614,080** | 620,471 | 1.03 % | phase 4, 300 s |
| trending_today | **499,994** | 500,000 | 0.00 % | phase 6, 60 s |
| kittens | **1,024,977** | 1,035,842 | 1.05 % | phase 3, 300 s |
| **Total** | **2,655,608** | | | |

Seven phases. What each one settled:

| Phase | Method | Result |
| ---: | --- | --- |
| 1 | Random | 1,038,208. A baseline. |
| 2 | Greedy on (video, cache) pairs | 2,637,790. The lazy refresh and the density key carry it. |
| 3 | Ruin and recreate | 2,653,074. Every later phase fights for the last 0.1 %. |
| 4 | Threshold accepting | +415. Two attempts of three gave nothing. |
| 5 | Simulated annealing | Loses on the four instances, with 10,000 times more moves. |
| 6 | Memetic genetic algorithm | Ties. The crossover contributes nothing measurable. |
| 7 | LP relaxation and Lagrangian bound | The solutions were already within 1.5 %. |

The result that matters is phase 7. The bound of section 4.4 ignores the
capacity, so it read 31 % missing on kittens and 25 % on
videos_worth_spreading. The real figures are 1.05 % and 1.03 %, and
me_at_the_zoo was already optimal at phase 4. Phases 5 and 6 searched a space
that was nearly closed, and the document could not tell, because it had no
instrument to tell with. The missing piece was a bound, not an algorithm.

**Section 10 collects the five ideas the measurements produced.** Read that
section and this table; the phases in between are the evidence for them.

## 1. The problem

Videos are on a datacenter. Endpoints request the videos. Each endpoint has a
latency to the datacenter. Each endpoint also connects to some cache servers,
with a lower latency. Every cache server holds `X` Mo of videos.

Put videos on the cache servers. The score is the average latency that you save
for each request, in microseconds:

```
score = 1000 * Σ_r count_r * (dc_latency(e_r) − best_latency(r)) / Σ_r count_r
```

The divisor counts every request of the raw instance.

## 2. Common code

| File | Function |
| --- | --- |
| `src/instance.hpp` | Structures, parser and filters |
| `src/solution.hpp` | `Solution` type, validation, score and output |
| `src/greedy.hpp` | `Index`, `State` and the greedy algorithms |
| `src/solvers/*.cpp` | One solver for each algorithm |
| `src/tools/scorer.cpp` | Independent check of a solution file |
| `src/tools/bound.cpp` | Submodular bound of section 9.3, and the bound of section 4.4 |
| `src/tools/lagrangian.cpp` | LP bound by decomposition, section 9.5 |
| `src/tools/lp_bound.py` | The linear program of section 9.1, solved by HiGHS |
| `parser.cpp` | Dump of an instance |

`Index` holds the fixed data. `State` holds the data that changes: the caches,
the free space, the latency of each request and the running total. A solver
changes a `State` only with `place()` and `unplace()`. These two functions keep
the running total correct. Thus an undo is a list of these two operations.

Build all the programs with `make`. The Makefile finds each new solver
automatically.

### 2.1 Filters

Three filters make the instance smaller before a solver starts:

- `filterLargeVideo` removes the videos that no cache can hold.
- `filterInefficientCache` removes each connection that is slower than the
  datacenter.
- `filterUnconnectedCache` removes the caches that no endpoint can reach.

The filters do not change `gd.V` and `gd.C`. Video ids and cache ids stay
absolute. A solver thus keeps an index by id after a filter.

### 2.2 Rule for the score

A solver works on the reduced instance. A solver always measures the score on
the raw instance. `report()` makes this rule safe: it takes the raw instance.
The rule is necessary because the divisor counts the requests that the filters
remove.

### 2.3 Verification

`src/tools/scorer.cpp` reads an instance and a solution file, then prints the
score. It gives 462500 on the example of the problem statement. This is the
value that the statement gives.

## 3. Phase 1 — Random baseline

### 3.1 Method

For each reachable cache, the solver shuffles the candidate videos. Then it
reads the shuffled list and keeps each video that has sufficient space. The
result is a random assignment, but each cache stays full.

Option `-n` repeats the draw and keeps the best solution. Option `-s` sets the
seed.

```
./bin/random_solver instances/kittens.in -o output/kittens.random.txt -n 20 -s 42
```

### 3.2 Results

| Instance | 1 draw | Best of 20 draws |
| --- | ---: | ---: |
| me_at_the_zoo | 109,941 | 186,092 |
| videos_worth_spreading | 17,084 | 20,189 |
| trending_today | 318,802 | 323,568 |
| kittens | 500,444 | 508,359 |
| **Total** | **946,271** | **1,038,208** |

## 4. Phase 2 — Greedy knapsack on (video, cache) pairs

### 4.1 The gain of a pair

The ground set is the set of the `(video, cache)` pairs. The gain of a pair is
the latency that the pair saves now:

```
gain(v,c) = Σ_{r=(v,e)} count_r * max(0, best[r] − L(e,c))
```

`best[r]` is the latency that request `r` gets at this moment. Its initial value
is the datacenter latency.

The pairs are dependent. A placement lowers `best[r]`. Thus it also lowers the
gain of every other cache that serves the same requests. The objective function
is monotone and submodular.

### 4.2 The index

A direct computation of a gain must find the endpoints of a cache in the
requests for a video. The instances have too many of these incidences:

| Instance | Useful connections | Connections for each endpoint | Pairs | Incidences |
| --- | ---: | ---: | ---: | ---: |
| me_at_the_zoo | 32 | 3.2 | 183 | 337 |
| videos_worth_spreading | 521 | 5.2 | 135,023 | 521,116 |
| trending_today | 10,000 | 100 | 1,000,000 | 10,000,000 |
| kittens | 351,881 | 352 | 4,999,996 | 70,365,697 |

The solver thus uses a different index:

- `reqs[v]` — the requests for video `v`. Each bucket holds `R/V` requests on
  average. This is 20 requests on kittens.
- `lat[e][c]` — a dense latency matrix. It is 500,000 values on kittens.
- `best[r]` — one latency for each request.

A gain computation is then a loop on `reqs[v]`, with one lookup in `lat`. A
placement uses the same loop and writes `best[r]`. Both operations cost
`O(|reqs[v]|)`. The count of the incidences has no effect.

### 4.3 The two modes

**Mode `static`** computes each gain one time against the datacenter. Then it
sorts the pairs and places them in one pass. The keys become too high while the
caches fill.

**Mode `lazy`** keeps the pairs in a maximum heap with keys that can be too
high. It takes the top pair and computes its true gain. If the new key stays
above the next key in the heap, the solver places the pair. If not, the solver
puts the pair back with its new key. This method gives the exact greedy order,
but it does not compute all the gains again. It is the CELF method.

Option `-k` selects the key: `gain` or `density`. The density is the gain for
each Mo. All the caches have the same capacity `X`, thus the densities are
comparable.

```
./bin/greedy instances/kittens.in -m lazy -k density -o output/kittens.greedy.txt
```

### 4.4 Results

| Instance | Random | static/gain | static/density | lazy/gain | lazy/density | Bound |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| me_at_the_zoo | 186,092 | 429,043 | 418,451 | 497,204 | **507,906** | 561,356 |
| videos_worth_spreading | 20,189 | 485,019 | 501,537 | 580,026 | **608,287** | 817,516 |
| trending_today | 323,568 | 10,495 | 25,791 | **499,982** | 499,916 | 500,000 |
| kittens | 508,359 | 78,310 | 75,011 | 956,226 | **1,021,681** | 1,492,627 |
| **Total** | **1,038,208** | 1,002,867 | 1,020,790 | 2,533,438 | **2,637,790** | 3,371,499 |

The bound is the score with no capacity limit. Each request gets its best
reachable cache. The bound is thus loose, but it is a true ceiling.
Section 9 replaces it: this one is loose enough to be misleading, and every
reading of a distance to it in sections 4 to 8 is wrong.

The run time stays low. Kittens needs 3.5 s and 157 Mo in mode `lazy`.

### 4.5 Analysis

**Mode `static` fails on the dense instances.** It gives 25,791 on
trending_today and 75,011 on kittens. These scores are below the random
baseline. The cause is the count of connections for each endpoint. Every
endpoint of trending_today reaches all the 100 caches. Therefore the static
order puts the same popular videos on every cache. Each placement after the
first one saves almost nothing, but its key still has the full value. The
random solver keeps a better score because it makes the caches different.

**Mode `static` is acceptable on the sparse instances.** An endpoint of
videos_worth_spreading reaches 5.2 caches. A key is thus only a little too
high. The mode gives 501,537 against 608,287 for mode `lazy`.

**The refresh costs little.** Kittens needs 7.7 million pops for 5.0 million
pairs. This is 1.5 pops for each pair.

**The density key is better.** It wins on 3 instances of 4. The gain of the
density is large on kittens: 1,021,681 against 956,226. Kittens holds 12 videos
for each cache, thus the size of a video is important. Mode `lazy` with the key
`gain` wins on trending_today, but only by 66 points.

**Trending_today is complete.** The score 499,982 is 99.996 % of the bound
500,000. No further work on this instance is useful.

**Kittens has the largest margin.** The score is 68 % of the bound. The bound
ignores the capacity, and the caches of kittens hold only 60 % of the videos.
The true margin is thus smaller.

> Section 9.6 measures it: the margin on kittens is 1.05 %, not 32 %. This
> paragraph, and every other reading of a distance to the bound before section
> 9, measures the bound and not the solution.

## 5. Phase 3 — Local search with ruin and recreate

### 5.1 Method

The greedy solver commits to a video early. A later placement can serve the
same requests. The first video then becomes partly redundant, but the greedy
solver cannot remove it. The local search repairs this.

Each iteration does four steps:

1. Ruin. Take `r` caches at random. Remove each of their videos with the
   probability `p`.
2. Recreate. Fill the free space again with the lazy greedy of phase 2.
3. Compare. Read the new total.
4. Keep the result if the total gets better. If not, undo the iteration.

The recreate step uses only the ruined caches. A compressed row layout gives
the candidate videos of each cache. The layout comes from the pairs of phase 2.

```
./bin/local_search instances/kittens.in -t 60 -r 5 -o output/kittens.local.txt
```

### 5.2 The undo

The iteration records each removal and each placement. To undo, the solver
reads the two lists in the opposite order. `place()` and `unplace()` are exact
opposites, thus the state comes back to its previous value.

A removal is more expensive than a placement. The latency of a request is a
minimum on the caches that hold the video. A placement only compares one new
latency to the current one. A removal must read the other caches of the video
again.

Two guards run at the end. The first compares the running total to a full
computation. The second compares the running total to the best total. The
second guard finds an undo that does not restore the state. Neither guard
reports an error on the four instances.

### 5.3 Effect of the parameters

**A ruin of one cache gives nothing.** The refill is deterministic. One emptied
cache thus receives the same videos again. me_at_the_zoo did 2.6 million
iterations with `r=1` and found no improvement.

**Noise helps only on me_at_the_zoo.** Option `-a` multiplies each key by a
random value. The same ruin can then give a different fill. me_at_the_zoo goes
from 514,010 with no noise to 516,384 with `-a 1.0`. The score goes up with the
amplitude: 515,508 at `-a 0.3` and 515,634 at `-a 0.6`. The larger instances
lose points with the same option. videos_worth_spreading gives 608,287 with
`-a 0.6` against 612,268 with no noise. The value 608,287 is the greedy start,
thus the noise stopped every improvement. me_at_the_zoo has 10 caches, thus its
neighbourhood is small and the search stops early. The other instances have a
large neighbourhood, and the noise only makes the refill worse.

**A small ruin is better on kittens.** `r=5` gives 1,023,547 and `r=50` gives
1,022,846 for the same time. A small ruin costs less, thus the solver does more
iterations. The count of the iterations is more important than their size.

### 5.4 Results

Each run takes 60 s. Column "Greedy" repeats the best result of phase 2.

| Instance | Greedy | Local search | Change | Bound |
| --- | ---: | ---: | ---: | ---: |
| me_at_the_zoo | 507,906 | **516,384** | +1.67 % | 561,356 |
| videos_worth_spreading | 608,287 | **612,732** | +0.73 % | 817,516 |
| trending_today | **499,982** | 499,970 | −0.00 % | 500,000 |
| kittens | 1,021,681 | **1,023,988** | +0.23 % | 1,492,627 |
| **Total** | **2,637,856** | **2,653,074** | **+0.58 %** | 3,371,499 |

The parameters are `-r 3 -a 1.0` for me_at_the_zoo, `-r 10` for
videos_worth_spreading and `-r 5` for the two other instances.

The best result for trending_today stays the result of phase 2. The local
search starts from the key `density`, which gives 499,916. It reaches 499,970.
The greedy solver with the key `gain` gives 499,982 alone. The best total of
all the phases is thus 2,653,086.

### 5.5 Analysis

**The gain is small but real.** The total goes up by 0.57 %. The greedy
solution is already a strong local optimum for this neighbourhood.

**The improvements become rare.** kittens finds 1,045 improvements in 2,446
iterations, but each improvement is very small. The solver finds the easy
exchanges in the first seconds. Then it stops.

**A running total did not make the solver faster.** `State` now holds the
total, and `place()` and `unplace()` keep it correct. This removes a loop on
all the requests at each iteration. The speed did not change: kittens went from
2,069 to 2,171 iterations in 30 s. The limit is the recreate step. It computes
a gain for each candidate video of each ruined cache. These computations read
the latency matrix at random positions, thus they lose the processor cache.

**The bound stays far for two instances.** videos_worth_spreading reaches 75 %
of its bound and kittens reaches 69 %. The bound ignores the capacity. The
caches hold 33 % of the videos on videos_worth_spreading and 60 % on kittens.
The true distance is thus much smaller than these numbers show.

> It is much smaller. Section 9.6 gives 98.9 % and 98.9 %. The sentence above is
> correct and useless: it says the bound is loose without saying by how much,
> which is the whole question.

### 5.6 Defects found in the phase 3 code

A review of `src/greedy.hpp` and `src/solvers/local_search.cpp` found nine
defects. All of them are corrected.

The most important one was in the noise. `refill()` built its keys with no
noise, but `greedyLazy()` multiplied the key that it computes again by the
noise. A key in the heap was thus no longer an upper limit of the key that the
solver computes again. The lazy comparison then compared two different scales.
The noise only had an effect on the pairs that arrived at the top of the heap.
Both functions now use the same multiplier.

A second defect made a large amplitude dangerous. A multiplier of zero or less
removed a video from every refill. Option `-a 1.5` thus gave no improvement at
all. A multiplier now has a floor of 0.01.

The other defects are: a division by zero on an instance with no request, an
option value that is not a number, an option in the last position that the
parser never read, a duplicate cache that consumed a ruin attempt, a division
by a video size of zero, and an erase with no check in `unplace()`.

One defect stays open. The solver reads the clock only between two iterations.
A large `-r` on a large instance thus passes the limit of `-t` by one
iteration.

The correction of the noise and of the duplicate cache changed the random
sequence. All the results of section 5.4 come from the corrected code.

## 6. Phase 4 — A faster recreate step and a different acceptance

Phase 4 made three attempts. Two of them failed. The section keeps them,
because they show a property of the problem.

### 6.1 The upper limit of a gain

A gain only becomes smaller. `best[r]` starts at the datacenter latency, which
is its largest value, and each placement makes it smaller. The key of the empty
state, which phase 2 computes, is thus a permanent upper limit of the current
key.

**Attempt 1: use the upper limit as the key.** The recreate step then computes
no gain at all. It pushes the stored limit, and the lazy greedy computes a gain
only for a pair that arrives at the top. The attempt gave no gain in speed. The
limits are much too large when the solution is full, thus the lazy greedy puts
almost every pair back in the heap. The work moves from the recreate step to
the heap.

**Attempt 2: keep only the best `cap` candidates of each cache.** The list of
each cache goes in the order of the upper limit, the largest first. The
recreate step reads only the head of the list.

The attempt made the solver 40 times faster. It also removed all the
improvements:

| Cap | Iterations in 30 s | Improvements | Score |
| ---: | ---: | ---: | ---: |
| 50 | 63,385 | 0 | 1,021,681 |
| 200 | 27,490 | 0 | 1,021,681 |
| 1,000 | 11,952 | 0 | 1,021,681 |
| no cap | 1,577 | 834 | 1,023,496 |

The score 1,021,681 is the greedy start. The search found nothing.

The cause is important. The upper limit is correct, but an order by the upper
limit is the opposite of an order by the current gain. A video with a large
upper limit is a popular video. The greedy solver already put it on many
caches. Its current gain is thus near zero. A video that helps the recreate
step has a small upper limit, but no other cache serves its requests, thus it
keeps all of its gain. The cap removes exactly the useful candidates.

Both attempts are removed from the code. The recreate step must compute a gain
for every candidate. This work is not reducible with an upper limit.

### 6.2 Threshold accepting

The phase 3 search takes only a better total. It thus stops at the first local
optimum. Option `-e` gives a limit. The search takes a result that is not worse
than this limit:

```
limit = e * best_total * (1 − elapsed / budget)
```

The limit goes down to zero at the end of the budget. The search thus explores
at the start and only climbs at the end. The solver keeps the best solution
apart, because the current solution can become worse.

The option helps two instances and damages one:

| Instance | `-e 0` | Best `-e` | Value |
| --- | ---: | ---: | ---: |
| me_at_the_zoo | 516,384 | 516,418 | 0.001 |
| videos_worth_spreading | 612,192 | 612,820 | 0.0001 |
| kittens | 1,023,550 | 1,023,286 | none |

On videos_worth_spreading, all the four values of `-e` that the sweep tested
gave more than `-e 0`. On kittens, all of them gave less. The default value is
0, which gives the phase 3 behaviour.

### 6.3 Method of measurement

Three runs of the same configuration gave 989, 1,034 and 1,055 iterations. The
difference in one group of runs is thus near 3 %. Two groups at a different
moment gave 2,446 and 1,679 iterations for the same configuration. The
difference between two groups is thus near 45 %.

A comparison of two configurations is only valid in the same group of runs. The
phase 3 document compared two groups one time. That comparison is not valid.
All the comparisons of phase 4 are in the same group.

### 6.4 Results

Each run takes 60 s.

| Instance | Phase 3 | Phase 4 | Change | Bound |
| --- | ---: | ---: | ---: | ---: |
| me_at_the_zoo | 516,384 | **516,557** | +173 | 561,356 |
| videos_worth_spreading | 612,732 | **613,123** | +391 | 817,516 |
| trending_today | 499,970 | 499,979 | +9 | 500,000 |
| kittens | **1,023,988** | 1,023,830 | −158 | 1,492,627 |
| **Total** | 2,653,074 | **2,653,489** | **+415** | 3,371,499 |

The parameters are `-r 3 -a 1.0 -e 0.001` for me_at_the_zoo, `-r 10 -e 0.0001`
for videos_worth_spreading and `-r 5 -e 0` for the two other instances.

kittens uses the same configuration as phase 3. Its change of −158 is only the
difference between two runs. It is not a result.

The best score of each instance, from all the phases, gives 2,653,650.
trending_today keeps its phase 2 score of 499,982 and kittens keeps its phase 3
score of 1,023,988.

### 6.5 Analysis

**Phase 4 gives very little.** The total goes up by 415 points, which is
0.016 %. Two attempts of three gave nothing.

**The recreate step is not the limit that phase 3 described.** Phase 3 said
that a faster gain computation was necessary. Phase 4 shows that the work is
necessary, not that it is slow. A shorter candidate list removes the
improvements.

**The search is at a strong local optimum.** The greedy solution, the ruin and
recreate, and the threshold acceptance all arrive near 1,023,900 on kittens.
Three different methods give the same value. The next gain needs a different
model, not a better search.

> Half right. Three methods agree because they are near the optimum, not because
> they share a blind spot: section 9.6 shows at most 1.05 % left on kittens. No
> model was going to find the 30 % that the bound of section 4.4 suggested.
> Phases 5 and 6 followed this sentence and found what was there to find, which
> was almost nothing.

## 7. Phase 5 — Simulated annealing on single video moves

### 7.1 Method

Phase 3 and phase 4 move the solution with a ruin and a recreate. One iteration
calls the lazy greedy, thus it computes a gain for every candidate of every
ruined cache. kittens does 2,400 iterations in 60 s.

Phase 5 uses a much smaller neighbourhood. One move touches one video:

- **Insert.** Put a candidate video on a cache. Remove other videos first if
  the cache has no room.
- **Transfer.** Take a video off a cache and put it on another cache. This is
  the move that section 7 of the phase 4 document asked for. The ruin and
  recreate never makes it directly.
- **Remove.** Take a video off a cache. It always loses, thus only the
  temperature accepts it.

A move costs `place()` and `unplace()`, which is `O(|reqs[v]|)`. kittens holds
20 requests for each video, thus one move is near 10,000 times cheaper than a
ruin and a recreate. The Metropolis rule accepts a move that loses `d` with the
probability `exp(-d/T)`. `T` goes down from `T0` to `T1` over the budget.

```
./bin/annealing instances/kittens.in -t 60 -0 1000 -o output/kittens.anneal.txt
```

`-0` and `-1` set the two temperatures. With no `-0`, the solver samples the
neighbourhood and reads the average loss of a move that loses, then sets `T0`
so that such a move is accepted with the probability `-w`.

### 7.2 The move count is not the difficulty

The neighbourhood delivers what it promised. me_at_the_zoo does 190 million
iterations in 60 s, against 2.6 million for the ruin and recreate. kittens does
14.7 million, against 2,400.

That count buys almost nothing:

| Instance | T0 auto | T0 auto/100 | T0 auto/10 000 | T0 auto/100 000 |
| --- | ---: | ---: | ---: | ---: |
| videos_worth_spreading | 608,287 | 608,869 | 608,894 | 608,939 |
| kittens | 1,021,681 | 1,021,715 | 1,021,753 | 1,021,776 |

The value 608,287 and the value 1,021,681 are the greedy start. At the
automatic temperature the search never comes back above its own start.

Section 4.4 gives the scale. One score point is 499,687 units of total on
videos_worth_spreading and 1,000,242 units on kittens. The column `auto/100 000`
is therefore a temperature of 0.0004 score points: it is already a plain hill
climb. The temperature changes nothing, because the acceptance is not the
limit.

### 7.3 The proposal is the limit

A uniform draw among the candidates of a cache is too weak. kittens holds
10,000 candidates for each cache, and almost all of them already receive their
latency from another cache, thus their gain is zero.

The solver therefore draws `k` candidates and keeps the one with the best key
(`-i`). A removal draws `k` videos of the cache and keeps the one that loses
the least latency (`-j`). `lossOf()` computes that loss without changing the
state. It is the counterpart of `gain()`, which is always zero for a video that
the cache already holds and thus cannot rank a removal.

| Sample `k` | videos_worth_spreading | kittens |
| ---: | ---: | ---: |
| 1 | 608,550 | 1,021,696 |
| 8 | **608,942** | **1,021,747** |
| 64 | 608,693 | 1,021,724 |
| 256 | 608,539 | 1,021,711 |

The sample helps, then it stops helping. A larger sample costs iterations and
gives them back nothing.

### 7.4 Results

Each run takes 60 s. Annealing uses `-0 1000 -i 8 -j 8`, which section 7.2 and
section 7.3 selected. The local search column repeats the phase 4
configuration, run again in this group.

| Instance | Local search | Annealing | Bound |
| --- | ---: | ---: | ---: |
| me_at_the_zoo | **516,557** | 513,694 | 561,356 |
| videos_worth_spreading | **613,596** | 609,128 | 817,516 |
| trending_today | **499,979** | 499,943 | 500,000 |
| kittens | **1,024,034** | 1,021,941 | 1,492,627 |

Annealing loses on the four instances. Its worst case is kittens, at 2,093
points below the local search. 1,021,941 is barely above the greedy start of
1,021,681: in 60 s and 14.7 million moves the search recovered 260 points of
the 2,353 that the ruin and recreate finds.

### 7.5 Analysis

The neighbourhood is 10,000 times cheaper and it still loses. Neither the
temperature nor the sample size recovers the difference, so the cause is the
neighbourhood itself.

**The annealing can express the right move. It cannot find it.** This is the
point, and it is not the usual one. A full cache needs an exchange to improve:
evict something, put something better in its place. The annealing does have
that move, because `makeRoom()` evicts up to four videos before an insertion.
The move is in the neighbourhood. What is missing is the ability to *choose*
it: the solver draws 8 candidates out of the 10,000 that a kittens cache has,
and the exchange that wins is one particular combination among them.

**Search against construction.** The ruin and recreate does not search for that
combination, it builds it. It empties five caches, reads the exact current gain
of every one of their candidates — about 50,000 on kittens — and fills them
greedily with the best. It returns the best fill a greedy can produce. One
annealing move reads 16 candidates and returns one sampled guess.

The two solvers therefore spend their 60 s in opposite ways:

| | Candidates read per step | Steps in 60 s | Points gained |
| --- | ---: | ---: | ---: |
| Ruin and recreate | ~50,000 | 2,400 | 2,353 |
| Annealing | 16 | 14,700,000 | 260 |

Reading 3,000 times more for each step, and taking 6,000 times fewer steps, is
the better trade on this problem. The value is in how completely a step is
evaluated, not in how many steps are taken. (The two ratios differ by a factor
of two because an annealing move also pays for its undo, and a removal costs
more than a gain.)

**This is section 6.1 from the other side.** Phase 4 shortened the candidate
list and every improvement disappeared. Phase 5 keeps the list and samples it,
and the improvements disappear again. Both experiments remove the same thing —
the exhaustive read of a cache's candidates — by two different routes, and both
lose the same 90 % of the gain. That read is where the score comes from.

## 8. Phase 6 — A memetic genetic algorithm

### 8.1 The crossover is free, which is the reason to try it

A solution is one list of videos for each cache. Each cache respects its
capacity alone, and no constraint links two caches. Therefore a child that
takes the content of each cache whole, from one parent or the other, is always
feasible.

The crossover needs no repair. Most assignment problems need one, and the
repair usually destroys what the crossover was supposed to preserve. Here it
costs nothing, which is what makes a genetic algorithm worth an attempt at all.

Free of constraints is not the same as useful, and section 8.4 measures the
difference. Keep the two apart while reading this phase: the crossover is
always legal, and it turns out to be always worthless.

Phase 6 keeps the neighbourhood that phase 5 showed to be necessary. The ruin
and recreate becomes the mutation.

One generation:

1. Select two parents with a tournament of size `-T`.
2. Cross them: each cache comes from one parent, drawn at random.
3. Refill. A mixed solution can serve the same request twice and leave free
   space elsewhere. The lazy greedy fills every cache that kept room.
4. Mutate. `-M` rounds of ruin and recreate improve the child.
5. Replace the worst individual, if the child is better and if no individual
   already holds its fitness.

```
./bin/genetic instances/kittens.in -t 60 -P 3 -M 5 -r 5 -a 0.3
```

### 8.2 The initial population

Individual 0 is the plain greedy solution, thus the algorithm never returns
less than phase 2. Every other individual multiplies the greedy keys by a
random vector (`-a`), which gives a different greedy order and thus a different
basin. This is the "different start" that the phase 4 document asked for.

The population has a price. The greedy solver needs 4.86 s on kittens, thus a
population of 12 spends 58 s before the first generation. The population size
must follow the instance:

| Instance | Population | Generations in 60 s |
| --- | ---: | ---: |
| me_at_the_zoo | 12 | 283,418 |
| videos_worth_spreading | 8 | 103 |
| kittens | 3 | 12 |

kittens spends a quarter of its budget on the population and then runs 12
generations.

### 8.3 Results

Each run takes 60 s, one after the other, in the same group of runs.

| Instance | Local search | Genetic | Change | Bound |
| --- | ---: | ---: | ---: | ---: |
| me_at_the_zoo | **516,557** | 516,418 | −139 | 561,356 |
| videos_worth_spreading | **613,596** | 613,048 | −548 | 817,516 |
| trending_today | 499,979 | **499,994** | +15 | 500,000 |
| kittens | **1,024,034** | 1,023,951 | −83 | 1,492,627 |
| **Total** | **2,654,166** | 2,653,411 | −755 | 3,371,499 |

`src/tools/scorer.cpp` read the four solution files again and returned the same
four values.

**trending_today is a new best.** 499,994 passes the 499,982 of phase 2, which
no later phase had beaten. It is 99.9988 % of the bound 500,000.

The best score of each instance, over all the phases, gives 2,654,181. The
300 s runs below raise it to 2,655,608, which is the total of section 0.

### 8.4 Analysis

**The genetic algorithm ties, it does not win.** It loses on three instances by
139, 548 and 83 points, which is the order of the difference between two runs
that section 6.3 measured. It wins on trending_today. The honest reading is
that the two methods are equal on this budget, and that only trending_today
gained something real.

**A larger budget does not change the order.** Section 9 of the phase 4
document supposed that the population would pay for itself over a longer run,
because the local search has no diversity to spend. It does not:

| Instance | Local 60 s | Genetic 60 s | Local 300 s | Genetic 300 s |
| --- | ---: | ---: | ---: | ---: |
| videos_worth_spreading | 613,596 | 613,048 | **613,808** | 613,455 |
| kittens | 1,024,034 | 1,023,951 | **1,024,856** | 1,024,211 |

Five times the budget gives the local search 212 points on
videos_worth_spreading and 822 on kittens, and the genetic algorithm stays
behind by the same margin as at 60 s. The two curves are parallel, thus the
diversity never becomes profitable.

**The population competes with the search for the budget.** The local search
spends 60 s on one solution. The genetic algorithm spends a part of its budget
on the greedy runs of the population, and then splits the rest over several
individuals. On kittens this leaves 12 generations. The diversity is paid for
in iterations, and on this budget it does not pay for itself.

**The crossover contributes nothing.** Option `-M 0` removes the ruin and
recreate and leaves the crossover alone:

| Instance | `-M 0` | Greedy start | `-M 20` |
| --- | ---: | ---: | ---: |
| videos_worth_spreading | 608,287 | 608,287 | **612,811** |
| trending_today | 499,982 | 499,982 | **499,991** |
| me_at_the_zoo | 509,818 | 507,906 | **515,508** |

On videos_worth_spreading and on trending_today the column `-M 0` is exactly
the greedy start, to the unit. In 30 s the crossover never produced a child
better than individual 0 of the population. Only me_at_the_zoo gains something
from the crossover alone, and it gains 1,912 points where the mutation gains
7,602.

An earlier draft of this section supposed that trending_today was the instance
where the crossover works, because its endpoints all reach its 100 caches and
its caches are therefore interchangeable. The measurement above refutes that.
The +15 of section 8.3 comes from the mutation, like every other gain.

**The algorithm is a multi-start local search.** The recombination of two good
solutions produces nothing on this problem. What the population gives is a set
of different starting points, and what produces the score is the ruin and
recreate that runs on each of them. The word "genetic" describes the shape of
the program, not the source of its results.

Two caches that both come from a good solution hold videos that were chosen
against a different context. A child takes the content of cache `c` from one
parent and the content of cache `c'` from the other, and the two contents
overlap: the same popular video is on both. The child then wastes the capacity
that the refill has to repair. The crossover breaks exactly the property that
makes a solution good, which is that the caches are complementary.

### 8.5 Defects found in the phase 5 and phase 6 code

A review of the two new solvers found two defects. Both are corrected.

`improve()` in `src/solvers/genetic.cpp` skipped a ruin attempt when the draw
gave a cache that the iteration had already ruined. This is the same defect
that section 5.6 records as corrected in `src/solvers/local_search.cpp`: the
code was written again from the description of the method instead of from the
corrected source. The loop now draws again until it finds a new cache. The
guard on `ruined.size()` makes sure a free cache exists, thus the loop ends.

`src/solvers/annealing.cpp` counted an accepted worsening move under the name
`uphill`. The solver maximises the saved latency, thus such a move goes down.
The counter is now named `worse`. This changed only the report line.

## 9. Phase 7 — An LP relaxation and a capacity aware bound

Every bound of the document so far ignores the capacity. Section 4.4 gives each
request its best reachable cache, which assumes that every video is on every
cache. That bound cannot say whether a solution is close to the optimum. Phase 7
builds a bound that keeps the capacity.

### 9.1 The program is not linear as written

The saving of a request is a maximum over the caches that hold its video. A
maximum is not linear, thus the problem has no direct linear form. The program
uses the assignment form instead:

```
x[v,c] in [0,1]     video v is on cache c
w[r,c] in [0,1]     the part of request r that cache c serves

max  sum_(r,c) count_r * g(r,c) * w[r,c]
s.t. sum_c w[r,c] <= 1                    one request is served one time
     w[r,c] <= x[v_r,c]                   a cache serves only what it holds
     sum_v size_v * x[v,c] <= X           the capacity of a cache
     g(r,c) = dc_latency(e_r) - L(e_r,c)
```

For a fixed integral `x` the inner problem gives each request the best cache
that holds its video, because the objective is a maximum over a simplex. The
form is thus exact for the original problem, and its relaxation is a true upper
bound. The maximum disappears into the constraint `sum_c w[r,c] <= 1`.

`src/tools/lp_bound.py` builds the program and solves it with HiGHS, through
`scipy.optimize`. It parses the instance again in Python and applies the same
filters as section 2.1. The parser is correct: it returns the pair counts and
the incidence counts of section 4.2 exactly.

### 9.2 me_at_the_zoo is closed

me_at_the_zoo has 183 pairs and 337 incidences, thus 520 variables. That is
small enough to keep `x` integral and solve the MILP, which gives the exact
optimum and not a bound.

| Quantity | Score |
| --- | ---: |
| Best solution of phase 4 | **516,557** |
| **Exact optimum (MILP)** | **516,557** |
| LP relaxation | 524,397 |
| Loose bound of section 4.4 | 561,356 |

HiGHS reports status 7, a MIP gap of 0.0 and a dual bound equal to the primal
value, thus the optimum is proven. The saved latency of the optimum is
27,538,220. `output/me_at_the_zoo.ls4.txt` saves 27,538,220, the same value to
the unit.

**The local search of phase 4 found the optimum of me_at_the_zoo.** The
document reported a distance of 8 % to the bound 561,356 for four phases. That
distance never existed. It was an artefact of a bound that ignores the
capacity.

The LP relaxation is 1.5 % above the optimum. The integrality gap of this
formulation is therefore small on this instance, which makes the LP bound
useful on the instances that the MILP cannot close.

### 9.3 A submodular bound that does not work

Before the LP, phase 7 tried a combinatorial bound. `f` is monotone and
submodular over the (video, cache) pairs, thus for a solution `S`:

```
f(OPT) <= f(S) + max { sum_{e in T} f(e|S) : T feasible }
```

`OPT\S` is a subset of a feasible set, thus it is feasible. No constraint links
two caches, so the maximum splits into one knapsack for each cache: the value of
a video is its marginal gain `f(e|S)`, which is exactly `gain()`, its weight is
its size and the capacity is `X`. The fractional knapsack gives the value.

`src/tools/bound.cpp` computes it. On me_at_the_zoo it gives 577,875, against
561,356 for the bound that ignores the capacity. **The bound is valid and
useless**: it is looser than the bound that it was meant to replace.

The cause is the capacity of the knapsack. `T` is `OPT\S`, thus it may use the
whole capacity `X` of every cache, on top of what `S` already holds. The bound
therefore allows a second full solution above the first one. A marginal gain is
small at a local optimum, but there are `X` Mo of them for each cache, and the
sum is larger than what the loose bound already gives. The program is kept
because it measures that, and because it prints the two bounds together.

### 9.4 Aggregating the caches

The program has one `x` for each pair and one `w` for each incidence. The large
instances do not fit:

| Instance | Pairs | Incidences | Variables |
| --- | ---: | ---: | ---: |
| me_at_the_zoo | 183 | 337 | 520 |
| videos_worth_spreading | 135,023 | 521,116 | 656,139 |
| trending_today | 1,000,000 | 10,000,000 | 11,000,000 |
| kittens | 4,999,996 | 70,365,697 | 75,365,693 |

Option `-g m` merges the caches into `m` groups. A group `G` becomes one cache
of capacity `|G| * X`, that an endpoint `e` reaches with the latency
`min_{c in G} L(e,c)`.

This is a relaxation. Take a feasible solution of the original instance and put
every video of every cache of `G` into `G`. The total size of `G` is then at
most `|G| * X`, thus the map respects the capacity. A request loses nothing,
because the latency of a group is at most the latency of each of its members.
The optimum of the grouped instance is therefore at least the optimum of the
original one, and the bound stays valid. With `m = C` the two instances are the
same.

The groups follow the mean latency of a cache over the endpoints that reach it,
and each group is a slice of that order. Caches of a group then behave alike,
so the minimum over a group stays close to each of its members.

me_at_the_zoo calibrates the cost of the aggregation, because its optimum is
known:

| Groups | Bound | Above the optimum |
| ---: | ---: | ---: |
| 10 (no aggregation) | 524,397 | +1.5 % |
| 5 | 543,759 | +5.3 % |
| 3 | 555,201 | +7.5 % |
| 2 | 556,172 | +7.7 % |
| 1 | 561,356 | +8.7 % |

At `m = 1` the bound is exactly the bound of section 4.4. That is the expected
value: a single cache of capacity `C * X` holds every video of the instance,
thus the capacity stops binding and each request gets its best reachable cache.
The aggregation therefore moves between the exact LP and the loose bound, and
it never leaves the valid side.

### 9.5 The program does not fit, so it is decomposed

HiGHS does not solve the program of section 9.1 on the large instances. The
simplex ran 60 minutes on videos_worth_spreading without finishing, and 30
minutes on the same instance aggregated into 50 groups. The interior point
method (`--method highs-ipm`) is much better suited to a program with many
rows: it closed videos_worth_spreading at 25 groups in 11 minutes, and gave
697,397. That is still one aggregated bound out of one instance.

The bound needs another route. Relax the constraint `sum_c w[r,c] <= 1` with a
multiplier `mu_r >= 0`:

```
L(mu) = sum_r mu_r + max_{x feasible} sum_(v,c) x[v,c] * A[v,c](mu)

A[v,c](mu) = sum_{r on v, c reaches e_r} max(0, count_r * g(r,c) - mu_r)
```

The `w` variables disappear. For a fixed `x`, the best `w` takes a pair as soon
as its reduced value `count_r * g(r,c) - mu_r` is positive, thus the inner
problem keeps only `x`. No constraint links two caches, so the maximum splits
into one knapsack for each cache, and the fractional knapsack solves each of
them in one sort.

Two properties make this usable:

- `L(mu)` is a valid upper bound for **every** `mu >= 0`. The program can stop
  at any iteration and still return a true bound.
- `min_mu L(mu)` is the value of the LP relaxation itself, by duality, because
  the fractional knapsack is the LP relaxation of the knapsack.

`src/tools/lagrangian.cpp` walks `mu` with a subgradient. The subgradient of
`mu_r` is `1 - sum_c x*[v_r,c]` over the caches whose reduced value is
positive: it is positive when no cache serves the request and negative when
several do. The step follows Polyak, towards the best known solution.

One iteration reads every incidence twice and sorts the candidates of each
cache. kittens needs 0.7 s for one iteration, with its 70 million incidences.

**The decomposition returns the same number.** On me_at_the_zoo, 3,000
iterations give 524,397.3. HiGHS gives 524,397.3 for the same program. The two
values agree to the decimal, which checks the derivation and the code together.

### 9.6 Results

The bound of a solution is the smaller of the two valid bounds: the Lagrangian
one and the one of section 4.4.

| Instance | Best solution | LP bound | Gap | Bound of section 4.4 | Gap it showed |
| --- | ---: | ---: | ---: | ---: | ---: |
| me_at_the_zoo | **516,557** | 524,397 | 1.50 % | 561,356 | 8.0 % |
| videos_worth_spreading | **614,080** | 620,471 | 1.03 % | 817,516 | 24.9 % |
| trending_today | **499,994** | 500,000 | 0.00 % | 500,000 | 0.0 % |
| kittens | **1,024,977** | 1,035,842 | 1.05 % | 1,492,627 | 31.3 % |

me_at_the_zoo is closed exactly by the MILP of section 9.2, thus its true gap
is 0 and the 1.50 % above is the integrality gap of the formulation.

trending_today is the one instance where the decomposition loses: 8,000
iterations still give 517,897, above the 500,000 of section 4.4. Its endpoints
all reach its 100 caches, thus one request has 100 multipliers to settle and the
subgradient is slow. The table keeps 500,000, because both bounds are valid and
the smaller one wins.

The run times: 3 s for videos_worth_spreading with 2,000 iterations, 9 s for
trending_today, 35 minutes for kittens with 3,000 iterations. The program that did
not fit in HiGHS in one hour is bounded in three seconds by the decomposition.

### 9.7 Analysis

**Every instance is within 1.5 % of its bound.** The document reported 69 % of
the bound on kittens and 75 % on videos_worth_spreading for four phases. Those
numbers measured the bound, not the solutions. The real distances are 1.05 %
and 1.03 %, and the true distances are smaller still, because a part of what
remains is the integrality gap of the LP and not a reachable score.

**The phases after phase 2 were searching a closed space.** me_at_the_zoo was
already optimal at phase 4, and the document spent phases 5 and 6 on an
instance with nothing left to find. kittens moved from 1,021,681 to 1,024,977
over four phases, which is 0.32 %, while at most 1.05 % remains. The searches
were not failing; there was almost nothing left to take.

**The bound was the missing instrument, not the missing algorithm.** Section 6.5
concluded that "the next gain needs a different model, not a better search".
That was right about the search and wrong about the reason. No model was going
to find 30 % on kittens, because 30 % was never there. A capacity aware bound
at phase 3 would have ended the search three phases earlier.

**The decomposition beats the solver on this structure.** HiGHS failed on
655,139 variables while the subgradient bounded a 75 million variable program
in minutes. The reason is the shape of the problem: the capacity constraints
are independent across caches, thus the only thing that couples them is the
request constraint, and that is exactly the one constraint the relaxation
removes. A problem that separates after one relaxation does not need a general
solver.

## 10. What the seven phases established

Five ideas came out of the measurements. They are the part of this work that
survives the instances.

**1. A bound that drops a constraint can be worse than no bound.** The bound of
section 4.4 ignores the capacity and reads 31 % missing on kittens. The real
figure is 1.05 %. For four phases the document used that number to decide where
to work, and it pointed at the instance with the least left to give. A bound is
not a decoration on a results table: it is the instrument that says whether a
phase is worth starting. Build it early, and build it against the constraint
that actually binds.

**2. How completely a step is evaluated beats how many steps are taken.** Two
independent experiments say it. Section 6.1 shortened the candidate list of the
refill and every improvement vanished. Section 7.5 kept the list, sampled 16
candidates out of 50,000, and lost 90 % of the gain while running 6,000 times
more steps. The gain on this problem comes from reading every candidate of a
cache before choosing. Any acceleration that skips that read removes the thing
being accelerated.

**3. Measure in the units of the objective, not in the units of the score.**
The score divides by the request count, so one score point is 1,000,242 units
of saved latency on kittens. Until that was written down, the annealing
temperature looked like a free parameter worth sweeping. Afterwards it was
obvious that the whole sweep from `auto` to `auto/100000` was already a hill
climb, and that the acceptance rule could not be the problem. One division
replaced a day of tuning.

**4. A crossover needs parts that keep their value when recombined.** The
caches of a good solution are complementary: each one holds what the others do
not. A child that takes cache `c` from one parent and cache `c'` from another
takes two sets chosen against different contexts, and they overlap. Section 8.4
shows the result — the crossover alone returns the greedy start exactly, on two
instances of three. Feasibility was free and value was zero. The question to
ask of a representation is not whether the pieces can be swapped legally, but
whether a piece means the same thing in the other parent.

**5. When one constraint couples an otherwise separable problem, relax that
one.** The capacity constraints are per cache and independent. The only thing
tying the caches together is that a request should be counted once. Dualising
exactly that constraint makes the problem fall apart into one fractional
knapsack per cache, each solved by a sort. HiGHS could not finish 655,139
variables; the decomposition bounds 75 million in minutes, and lands on the
same number to the decimal where both can run. Look at which constraint is
doing the coupling before reaching for a general solver.

## 11. Next phase

The question that opened phase 7 is answered. The solutions are within 1.5 % of
their bounds on the four instances, and one of them is proven optimal. The
total of the best solutions is 2,655,608.

Nothing in the search directions is worth a phase any more. What remains is
narrow:

- **Close the gaps with the bound, not with the search.** The subgradient
  stops at a bound, and the LP has an integrality gap above the true optimum.
  A branch and bound on the Lagrangian, or a cut on the fractional solutions,
  would lower the bound rather than raise the score. It would say how much of
  the remaining 1 % exists.
- **kittens and videos_worth_spreading, for the last percent.** The margin is
  real but small. The ruin and recreate gains about 0.1 % for every five times
  the budget, thus reaching the bound by search alone is not plausible.
- **Use the bound as a stopping rule.** `bin/lagrangian` costs seconds on three
  instances of four. Any future search should print its distance to the bound,
  so that a phase ends on a measurement instead of on an impression.
