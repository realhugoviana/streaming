const CARDS = [
  {
    tag: "Packing vs. selection",
    title: "Capacity pressure ρ",
    kv: [["ρ", "trending 1.000 · zoo 1.127 · kittens 0.600 · videos 0.465 · dejaVu 0.071 · universalλ 0.021"]],
    p: [
      "ρ ≥ 1 means every requested video could be cached at once, so the problem becomes bin packing. On trending_today the greedy leaves 7 videos (6,522 MB) unplaced, while the same 6,522 MB of free space is scattered in pieces of at most 242 MB. Placing big videos first does better there: the raw-gain greedy reaches 499,991 against 499,966.",
      "ρ ≪ 1 turns it into a selection problem where every MB has to earn its place. Density ranking is then essential: ranking by raw gain loses 54% on dejaVu (ρ = 0.071) and 81% on universalLambda (ρ = 0.021).",
    ],
  },
  {
    tag: "Integrality",
    title: "Granularity: videos per cache",
    kv: [["X ÷ mean size", "zoo 4.0 · universalλ 10.4 · kittens 12.0 · videos 33.3 · dejaVu 35.6 · trending 100"]],
    p: [
      "With only a few items per cache, rounding matters. The greedy leaves 6.4% of me_at_the_zoo's capacity empty. An exact per-cache knapsack recovers +0.83%, the largest relative gain on any instance. The whole instance has 444 variables, and the MIP proves the optimum of 516,557 in 6 seconds.",
      "With dozens or hundreds of items per cache, density greedy behaves like the fractional knapsack optimum. Caches end at least 99.87% full on every other instance.",
    ],
  },
  {
    tag: "Ranking key",
    title: "Size variability",
    kv: [["CV of size", "universalλ 1.66 (67% are 1 MB) · zoo 0.60 · kittens 0.58 · videos 0.57 · trending 0.57 · dejaVu 0.54"]],
    p: [
      "When sizes span three orders of magnitude, cheap items dominate value per MB: 98.9% of the greedy's placements on universalLambda are 1 MB videos. There, the ranking key (gain ÷ size or raw gain) matters far more than the search strategy.",
      "With roughly uniform sizes (CV ≈ 0.55) the two rankings differ by 2–6%: me_at_the_zoo 2.1%, videos_worth_spreading 4.6%, kittens 6.4%.",
    ],
  },
  {
    tag: "Where the points hide",
    title: "Popularity concentration",
    kv: [["Gini", "videos 0.86 (top 1% = 62%) · zoo 0.57 · universalλ 0.50 · dejaVu 0.21 · trending 0.21 · kittens 0.14"]],
    p: [
      "On videos_worth_spreading, the 100 most-requested videos carry 65% of all requests and take only 31,272 MB, 3% of total capacity. Any sensible method caches them everywhere, so the remaining points are all in the long tail.",
      "Flat popularity (kittens, dejaVu, trending) leaves thousands of near-tied candidates, and small differences decide the greedy's order. Randomised tie-breaking or ruin-and-recreate perturbation have the most room to find different solutions here.",
    ],
  },
  {
    tag: "Decomposition",
    title: "Cache overlap",
    kv: [["Jaccard", "trending 1.00 · kittens 0.59 · universalλ 0.47 · zoo 0.21 · dejaVu 0.08 · videos 0.03"]],
    p: [
      "With low overlap, each cache serves its own endpoints and the problem nearly splits into independent knapsacks. That is where the per-cache re-optimisation gains most (+0.35% on videos, +0.53% on dejaVu) and where the LP bound is tight (under 2% above the greedy).",
      "With high overlap, it becomes a coverage problem: a video is copied until every endpoint that wants it can reach a copy (5.76 copies per placed video on universalLambda, 1.46 on kittens). Rewriting one cache at a time can't reorganise that coverage. Cross-cache exchanges or a coverage-aware construction are the next step.",
    ],
  },
  {
    tag: "Symmetry",
    title: "Cache interchangeability",
    kv: [["2nd ÷ best saving", "trending 1.000 · kittens 0.999 · universalλ 0.981 · zoo 0.848 · videos 0.830 · dejaVu 0.803"]],
    p: [
      "On kittens, an endpoint's best and second-best caches differ by 1 ms at the median, against a mean best saving of 1,491 ms. Which cache a copy lands in barely matters. Only 1.9% of served requests use their endpoint's best cache, yet served requests still get 89% of the best possible saving.",
      "Such symmetry creates plateaus of equivalent solutions, which stall first-improvement local search. On dejaVu, by contrast, the worst of an endpoint's 4 links saves only 45% as much as the best, so where each copy goes carries real value.",
    ],
  },
];

const PROFILES = {
  me_at_the_zoo: {
    origin: "official · V 100 · E 10 · C 10 · X 100 MB",
    p: ["Tiny and coarse: videos of 1–50 MB against 100 MB caches, about four per cache. Capacity slightly exceeds the requested catalogue (ρ = 1.13), but it is split across ten caches reached through 3.2 links per endpoint, so packing each cache is the real problem. The greedy ends at 93.6% fill."],
    verdict: "exact methods. The MIP solves it to 516,557 in 6 s, 1.7% above the greedy. The per-cache knapsack alone recovers half of that gap.",
  },
  videos_worth_spreading: {
    origin: "official · V 10,000 · E 100 · C 100 · X 10,000 MB",
    p: ["Sparse network: 5.2 caches per endpoint and almost no overlap between caches (Jaccard 0.03), so the instance nearly splits into 100 separate knapsacks. 7 endpoints have no cache at all, which leaves 7.0% of requests unservable. Popularity is extremely skewed (top 1% of videos = 62% of requests), and 60% of request lines are duplicates."],
    verdict: "per-cache knapsack re-optimisation (+0.35%) and LP-based methods. The full LP bound is only 1.96% above the greedy, and what remains is in the popularity tail.",
  },
  trending_today: {
    origin: "official · V 10,000 · E 100 · C 100 · X 50,000 MB",
    p: ["Fully symmetric: every endpoint reaches every cache at 100 ms against 600 ms to the data centre, so every placement saves exactly 500 ms and all caches are interchangeable. The requested catalogue is exactly 5,000,000 MB, equal to the total capacity. The greedy leaves out 7 large videos (676–1,000 MB) while 6,522 MB of free space sits in pieces of at most 242 MB."],
    verdict: "bin packing and nothing else. Place by size (the raw-gain greedy already reaches 499,991), or consolidate free space by exchanging videos between caches. At most 34 points are left.",
  },
  kittens: {
    origin: "official · V 10,000 · E 1,000 · C 500 · X 6,000 MB",
    p: ["Dense and redundant: every endpoint links to 200–500 caches (352 on average) and each cache reaches about 705 endpoints. The best and second-best cache differ by 1 ms at the median, so caches are near-interchangeable. Popularity is flat (top 10% of videos = 15% of requests), and 60% of the catalogue fits (ρ = 0.60)."],
    verdict: "coverage: which videos, and how many copies of each. The coverage bound puts the greedy within 1.99% of optimal. Speed matters most here: the lazy greedy takes 9 s instead of 257 s. /*KITTENS_KCA*/",
  },
  custom_dejavu42: {
    origin: "dejaVu generator, seed 42 · V 10,000 · E 115 · C 20 · X 2,000 MB",
    p: ["Built by resampling: each endpoint draws its ~8,700 request lines from a private kernel of 566–692 videos, so 92.7% of lines repeat an earlier pair. Capacity is starved (ρ = 0.071): the greedy serves only 12.8% of requests, and the per-cache knapsack bound (89,066) already falls far below the uncapacitated one (491,516). Each endpoint has 4 random links with widely spread savings; 39 endpoints list one cache twice."],
    verdict: "value density above everything (raw gain loses 54%). Low overlap makes per-cache knapsack re-optimisation effective (+0.53%). The full LP bound (65,837) is 1.8% above the greedy.",
  },
  custom_universallambda42: {
    origin: "universalLambda generator, seed 42 · V 10,000 · E 115 · C 20 · X 2,000 MB",
    p: ["A hidden parity pattern: even endpoints request odd videos 8,000–12,000 times each (290,000 requests, 99.3% of total count); every other pair is noise at 1–100. Sizes shrink with the video id, so 67% of videos are 1 MB, and the 3,329 odd 1 MB videos carry 67% of the valuable demand. Each endpoint reaches about 13 of the 20 caches."],
    verdict: "a set cover over about 3,300 unit-size videos. The greedy already spends 98.9% of its placements on 1 MB videos, 5.8 copies each, and ranking by gain instead of density loses 81%. What remains is how those copies are spread across caches. /*UL_BOUND*/",
  },
};
