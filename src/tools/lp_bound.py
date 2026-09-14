"""
LP RELAXATION BOUND — Google HashCode 2017 video streaming

The bound of section 4.4 ignores the capacity, thus it cannot say how far a
solution is from the optimum. This program builds a linear program that keeps
the capacity and solves it with HiGHS.

The saving of a request is a maximum over the caches that hold its video, which
is not linear. The program therefore uses the assignment form:

    x[v,c] in [0,1]   video v is on cache c
    w[r,c] in [0,1]   the part of request r that cache c serves

    max  sum_(r,c) count_r * g(r,c) * w[r,c]
    s.t. sum_c w[r,c] <= 1                       one request is served one time
         w[r,c] <= x[v_r,c]                      a cache serves only what it holds
         sum_v size_v * x[v,c] <= X              the capacity of a cache
         g(r,c) = dc_latency(e_r) - latency(e_r,c)

For an integral x the inner problem gives each request its best available
cache, thus the form is exact for the original problem. Its relaxation is
therefore a true upper bound, and it includes the capacity.

With -i the program keeps x integral and solves the MILP instead. The result is
then the exact optimum, not a bound.

The program has V*C pair variables and one w variable for each incidence. The
two large instances do not fit: trending_today needs 11 million variables and
kittens needs 75 million. Option -g aggregates the caches into m groups and
makes the program smaller.

A group G becomes one cache of capacity |G|*X, reachable from an endpoint e
with the latency min_{c in G} L(e,c). This is a relaxation: a feasible solution
of the original instance maps to the group solution that puts every video of
every cache of G into G. That map respects the group capacity, and it can only
lower the latency of a request. The optimum of the grouped instance is
therefore at least the optimum of the original one, thus it stays a valid upper
bound. With m = C the two instances are the same.
"""

import argparse
import sys
import numpy as np
from scipy.sparse import coo_matrix
from scipy.optimize import linprog, milp, LinearConstraint, Bounds


def parse(path):
    """Read an instance. Mirrors parser() in src/instance.hpp."""
    it = iter(open(path).read().split())
    nxt = lambda: int(next(it))

    V, E, R, C, X = (nxt() for _ in range(5))
    size = [nxt() for _ in range(V)]

    dc, conn = [], []
    for _ in range(E):
        d, K = nxt(), nxt()
        dc.append(d)
        conn.append({(lambda a, b: (a, b))(nxt(), nxt()) for _ in range(K)})

    reqs = [(nxt(), nxt(), nxt()) for _ in range(R)]  # video, endpoint, count
    return dict(V=V, E=E, R=R, C=C, X=X, size=size, dc=dc, conn=conn, reqs=reqs)


def reduce_instance(ins):
    """Apply the three filters of section 2.1, in the same order."""
    X, size = ins["X"], ins["size"]

    # filterLargeVideo: a video that no cache can hold serves nothing
    ok_video = {v for v in range(ins["V"]) if size[v] <= X}
    reqs = [r for r in ins["reqs"] if r[0] in ok_video]

    # filterInefficientCache: a connection slower than the datacenter is useless
    conn = [{(c, l) for (c, l) in ins["conn"][e] if l < ins["dc"][e]} for e in range(ins["E"])]

    ins = dict(ins)
    ins["reqs"], ins["conn"] = reqs, conn
    return ins


def aggregate(ins, m, strategy="latency"):
    """Merge the caches into `m` groups. See the header for the argument."""
    C = ins["C"]
    if m is None or m >= C:
        return ins

    # A group holds caches that behave alike, so that the minimum over the group
    # stays close to each of its members. The mean latency of a cache over the
    # endpoints that reach it is the cheapest usable signature.
    tot = [0] * C
    cnt = [0] * C
    for e in range(ins["E"]):
        for (c, l) in ins["conn"][e]:
            tot[c] += l
            cnt[c] += 1
    if strategy == "latency":
        order = sorted(range(C), key=lambda c: (tot[c] / cnt[c]) if cnt[c] else 1e18)
    else:
        order = list(range(C))

    # Consecutive slices of the order. Sizes differ by at most one.
    group_of = [0] * C
    members = [0] * m
    for i, c in enumerate(order):
        g = i * m // C
        group_of[c] = g
        members[g] += 1

    # An endpoint reaches a group with the best latency of its members
    conn = []
    for e in range(ins["E"]):
        best = {}
        for (c, l) in ins["conn"][e]:
            g = group_of[c]
            if g not in best or l < best[g]:
                best[g] = l
        conn.append({(g, l) for g, l in best.items()})

    ins = dict(ins)
    ins["conn"] = conn
    ins["C"] = m
    ins["cap"] = [members[g] * ins["X"] for g in range(m)]
    return ins


def build(ins):
    """Build the index sets of the program: the pairs and the incidences."""
    size, dc, conn = ins["size"], ins["dc"], ins["conn"]

    # An incidence is a (request, cache) couple with a strictly positive saving.
    # A pair (video, cache) exists as soon as one incidence needs it.
    pair_id, inc = {}, []
    for r, (v, e, n) in enumerate(ins["reqs"]):
        for (c, lat) in conn[e]:
            g = dc[e] - lat
            if g <= 0:
                continue
            if (v, c) not in pair_id:
                pair_id[(v, c)] = len(pair_id)
            inc.append((r, pair_id[(v, c)], n * g))
    return pair_id, inc


def solve(path, integral=False, verbose=True, groups=None, strategy="latency", method="highs"):
    raw = parse(path)
    ins = reduce_instance(raw)
    ins = aggregate(ins, groups, strategy)
    pair_id, inc = build(ins)

    P, I, R, C, X = len(pair_id), len(inc), len(ins["reqs"]), ins["C"], ins["X"]
    size = ins["size"]
    cap = ins.get("cap") or [X] * C
    weight = sum(n for (_, _, n) in raw["reqs"])  # the score divides by the RAW requests

    if verbose:
        print(f"  pairs {P:,}  incidences {I:,}  variables {P + I:,}  rows {R + I + C:,}", file=sys.stderr)
    if P == 0:
        return 0.0, weight, P, I

    # Objective: only the w variables carry a value. linprog minimises.
    obj = np.zeros(P + I)
    for j, (_, _, val) in enumerate(inc):
        obj[P + j] = -float(val)

    rows, cols, vals = [], [], []
    row = 0

    # sum_c w[r,c] <= 1 : one row for each request that has an incidence
    r_row = {}
    for j, (r, _, _) in enumerate(inc):
        if r not in r_row:
            r_row[r] = row
            row += 1
        rows.append(r_row[r]); cols.append(P + j); vals.append(1.0)

    # w[r,c] - x[v,c] <= 0 : one row for each incidence
    for j, (_, p, _) in enumerate(inc):
        rows.append(row); cols.append(P + j); vals.append(1.0)
        rows.append(row); cols.append(p);     vals.append(-1.0)
        row += 1

    # sum_v size_v * x[v,c] <= X : one row for each cache that holds a pair
    c_row = {}
    for (v, c), p in pair_id.items():
        if c not in c_row:
            c_row[c] = row
            row += 1
        rows.append(c_row[c]); cols.append(p); vals.append(float(size[v]))

    n_rows = row
    b = np.zeros(n_rows)
    for r, i in r_row.items():
        b[i] = 1.0
    for c, i in c_row.items():
        b[i] = float(cap[c])

    A = coo_matrix((vals, (rows, cols)), shape=(n_rows, P + I)).tocsr()

    if integral:
        integrality = np.zeros(P + I)
        integrality[:P] = 1  # only x is integral; w follows from x
        res = milp(c=obj,
                   constraints=LinearConstraint(A, -np.inf, b),
                   integrality=integrality,
                   bounds=Bounds(0, 1))
        if not res.success:
            raise RuntimeError(res.message)
        return -res.fun, weight, P, I

    # "highs" lets HiGHS choose, which picks the simplex and does not finish on
    # the larger programs. "highs-ipm" is the interior point method, which suits
    # a program with many rows much better.
    res = linprog(obj, A_ub=A, b_ub=b, bounds=(0, 1), method=method)
    if not res.success:
        raise RuntimeError(res.message)
    return -res.fun, weight, P, I


def main():
    ap = argparse.ArgumentParser(description="LP relaxation bound, or exact optimum with -i")
    ap.add_argument("instance")
    ap.add_argument("-i", "--integral", action="store_true", help="solve the MILP: exact optimum")
    ap.add_argument("-q", "--quiet", action="store_true")
    ap.add_argument("-g", "--groups", type=int, default=None,
                    help="aggregate the caches into this many groups (a looser bound)")
    ap.add_argument("--strategy", default="latency", choices=["latency", "id"])
    ap.add_argument("--method", default="highs", choices=["highs", "highs-ds", "highs-ipm"],
                    help="LP algorithm; highs-ipm is interior point, better on the large programs")
    a = ap.parse_args()

    total, weight, P, I = solve(a.instance, a.integral, not a.quiet, a.groups, a.strategy, a.method)
    kind = "optimum" if a.integral else "LP bound"
    # The score is the average saving for each request of the raw instance
    print(f"[{kind}] {total * 1000 / weight:.1f}   (total saved {total:,.1f})")


if __name__ == "__main__":
    main()
