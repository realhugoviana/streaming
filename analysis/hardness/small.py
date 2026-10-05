"""Small instances: generator, the team's greedy_density, the exact optimum (MIP), and .in writer.

The greedy follows src/solvers/greedy_density.cpp exactly: repeatedly place the (video, cache)
pair with the highest gain / size among pairs that fit, ties going to the lowest video id and
then the lowest cache id (its scan uses a strict `>`), until no pair has a positive gain.
check_team.py verifies that it reproduces the team's compiled solver placement by placement.
"""
from dataclasses import dataclass, field
import numpy as np
from scipy.optimize import milp, LinearConstraint, Bounds
import scipy.sparse as sp


@dataclass
class Small:
    X: int
    size: np.ndarray            # (V,) MB
    dc: np.ndarray              # (E,) data-center latency
    S: np.ndarray               # (E, C) latency saved through each link, 0 = no link
    rv: np.ndarray              # (R,) video of each request (unique (video, endpoint) pairs)
    re: np.ndarray              # (R,) endpoint of each request
    rn: np.ndarray              # (R,) request count
    meta: dict = field(default_factory=dict)

    @property
    def V(self): return len(self.size)
    @property
    def E(self): return len(self.dc)
    @property
    def C(self): return self.S.shape[1]
    @property
    def R(self): return len(self.rv)

    def score(self, placed):
        """Weighted score: sum over requests of count x best saving among caches holding the video."""
        sav = self.S[self.re] * placed[self.rv]            # (R, C)
        return float((sav.max(1) * self.rn).sum()) if self.R else 0.0

    def contest(self, weighted):
        return int(weighted * 1000 // self.rn.sum())

    def to_in(self):
        lines = [f"{self.V} {self.E} {self.R} {self.C} {self.X}", " ".join(map(str, self.size.tolist()))]
        for e in range(self.E):
            links = [(c, int(self.dc[e] - self.S[e, c])) for c in range(self.C) if self.S[e, c] > 0]
            lines.append(f"{int(self.dc[e])} {len(links)}")
            lines += [f"{c} {l}" for c, l in links]
        lines += [f"{v} {e} {n}" for v, e, n in zip(self.rv.tolist(), self.re.tolist(), self.rn.tolist())]
        return "\n".join(lines) + "\n"


def greedy(inst, trace=False, key="density"):
    """The team's greedy_density (key="density"). key="gain" ranks by raw gain instead, a variant
    used only to test the best-of-both fix. Returns (placed V x C bool, weighted score[, steps])."""
    V, C = inst.V, inst.C
    free = np.full(C, inst.X, dtype=np.int64)
    placed = np.zeros((V, C), bool)
    Sreq = inst.S[inst.re].astype(np.float64)            # (R, C)
    n = inst.rn.astype(np.float64)
    cur = np.zeros(inst.R)
    size = inst.size.astype(np.float64)
    steps = []
    while True:
        G = np.zeros((V, C))
        np.add.at(G, inst.rv, n[:, None] * np.maximum(0.0, Sreq - cur[:, None]))
        ok = (~placed) & (inst.size[:, None] <= free[None, :]) & (G > 0)
        if not ok.any():
            break
        d = np.where(ok, G / size[:, None] if key == "density" else G, -1.0)
        k = int(np.argmax(d))                             # first maximum in video-major order = team tie-break
        v, c = divmod(k, C)
        placed[v, c] = True
        free[c] -= inst.size[v]
        m = inst.rv == v
        cur[m] = np.maximum(cur[m], Sreq[m, c])
        if trace:
            steps.append((v, c, G[v, c], d[v, c]))
    w = float((cur * n).sum())
    return (placed, w, steps) if trace else (placed, w)


def optimum(inst, time_limit=20.0):
    """Exact optimum with HiGHS: x_vc binary, y_rc <= x_vc, sum_c y_rc <= 1, per-cache capacity.
    Returns (placed, weighted score of that placement, proven upper bound, status)."""
    V, C, R = inst.V, inst.C, inst.R
    gain = np.zeros((V, C))
    np.add.at(gain, inst.rv, inst.rn[:, None] * inst.S[inst.re])
    ok = (gain > 0) & (inst.size[:, None] <= inst.X)
    pv, pc = np.nonzero(ok)
    P = len(pv)
    if P == 0:
        return np.zeros((V, C), bool), 0.0, 0.0, "trivial"
    pid = -np.ones((V, C), np.int64)
    pid[pv, pc] = np.arange(P)
    rr, cc = np.nonzero(inst.S[inst.re] > 0)
    xp = pid[inst.rv[rr], cc]
    keep = xp >= 0
    rr, cc, xp = rr[keep], cc[keep], xp[keep]
    Y = len(rr)
    link = sp.csr_matrix((np.r_[np.ones(Y), -np.ones(Y)], (np.r_[np.arange(Y), np.arange(Y)], np.r_[P + np.arange(Y), xp])), shape=(Y, P + Y))
    one = sp.csr_matrix((np.ones(Y), (rr, P + np.arange(Y))), shape=(R, P + Y))
    cap = sp.csr_matrix((inst.size[pv].astype(float), (pc, np.arange(P))), shape=(C, P + Y))
    A = sp.vstack([link, one, cap]).tocsr()
    ub = np.r_[np.zeros(Y), np.ones(R), np.full(C, float(inst.X))]
    obj = np.r_[np.zeros(P), (inst.rn[rr] * inst.S[inst.re][rr, cc]).astype(float)]
    res = milp(-obj, constraints=LinearConstraint(A, -np.inf, ub), bounds=Bounds(0, 1),
               integrality=np.r_[np.ones(P), np.zeros(Y)],
               options={"time_limit": time_limit, "mip_rel_gap": 1e-9, "disp": False})
    placed = np.zeros((V, C), bool)
    if res.x is not None:
        placed[pv[res.x[:P] > 0.5], pc[res.x[:P] > 0.5]] = True
    w = inst.score(placed)
    bound = -getattr(res, "mip_dual_bound", -np.inf) if res.status == 0 else np.inf
    return placed, w, max(bound, w), ("optimal" if res.status == 0 else "limit")


def gap(inst, time_limit=20.0):
    """Relative gap 1 - greedy / optimum, plus details."""
    gp, gw = greedy(inst)
    op, ow, ob, st = optimum(inst, time_limit)
    best = max(gw, ow)
    return {"greedy": gw, "opt": best, "bound": ob, "status": st,
            "gap": (1 - gw / best) if best > 0 else 0.0, "greedy_placed": gp, "opt_placed": op if ow >= gw else gp}


def from_json(d):
    """Instance from the dict written by adversary.describe()."""
    req = np.array(d["requests"], dtype=np.int64)
    return Small(d["X"], np.array(d["size"], np.int64), np.array(d["dc"], np.int64), np.array(d["saving"], np.int64),
                 req[:, 0], req[:, 1], req[:, 2])


def normalise(inst):
    """Fit the official limits (request count <= 10000, 1 <= cache latency <= 500, data-center latency
    <= 4000) by rescaling counts and savings. The score only depends on savings, so the gap is unchanged
    up to integer rounding; callers re-evaluate it."""
    from dataclasses import replace
    rn = np.maximum(1, np.round(inst.rn * min(1.0, 10000 / inst.rn.max()))).astype(np.int64)
    S = inst.S.astype(float)
    spread = max(1.0, max((row[row > 0].max() - row[row > 0].min()) for row in S if (row > 0).any()))
    S = np.where(S > 0, np.maximum(1, np.round(S * min(1.0, 499 / spread, 3999 / S.max()))), 0).astype(np.int64)
    dc = np.array([row[row > 0].max() + 1 if (row > 0).any() else 2 for row in S])   # latency >= 1 on every link
    return replace(inst, S=S, dc=dc, rn=rn)


# ---------------------------------------------------------------- random family
def random_instance(rng, **fixed):
    """One random small instance; shape parameters are drawn unless given in `fixed`."""
    p = {
        "V": int(rng.integers(3, 26)), "E": int(rng.integers(1, 9)), "C": int(rng.integers(1, 6)),
        "rho": float(np.exp(rng.uniform(np.log(0.05), np.log(1.6)))),   # capacity / requested volume
        "size_cv": float(rng.uniform(0.0, 1.3)),
        "link_p": float(rng.uniform(0.15, 1.0)),
        "lat_spread": float(rng.uniform(0.0, 0.95)),                    # 0 = all links of an endpoint equal
        "req_p": float(rng.uniform(0.15, 1.0)),
        "zipf": float(rng.uniform(0.0, 2.0)),
        "count_noise": float(rng.uniform(0.0, 1.2)),
    }
    p.update(fixed)
    V, E, C = p["V"], p["E"], p["C"]
    if p["size_cv"] < 1e-3:
        size = np.full(V, 50)
    else:
        k = 1 / p["size_cv"] ** 2
        size = np.clip(np.round(50 * rng.gamma(k, 1 / k, V)), 1, 1000).astype(np.int64)
    dc = rng.integers(100, 1001, E)
    L = rng.random((E, C)) < p["link_p"]
    L[np.arange(E), rng.integers(0, C, E)] = True                        # every endpoint reaches a cache
    frac = 0.9 * (1 - p["lat_spread"] * rng.random((E, C)))
    S = np.where(L, np.clip(np.round(dc[:, None] * frac), 1, dc[:, None] - 1), 0).astype(np.int64)
    pop = (1 + rng.permutation(V)) ** -p["zipf"]
    ask = rng.random((V, E)) < p["req_p"]
    ask[rng.integers(0, V), rng.integers(0, E)] = True
    rv, re = np.nonzero(ask)
    rn = np.maximum(1, np.round(1000 * pop[rv] * rng.lognormal(0, p["count_noise"], len(rv)))).astype(np.int64)
    vol = size[np.unique(rv)].sum()
    X = int(max(1, round(p["rho"] * vol / C)))
    return Small(X, size, dc, S, rv, re, rn, meta=p)


def features(inst):
    """Instance features (same definitions as the atlas where they overlap)."""
    req = np.unique(inst.rv)
    s = inst.size[req]
    fit = s <= inst.X
    pop = np.bincount(inst.rv, weights=inst.rn, minlength=inst.V)[req]
    ps = np.sort(pop)
    n = len(ps)
    gini = float((2 * np.arange(1, n + 1) - n - 1) @ ps / (n * ps.sum())) if n > 1 else 0.0
    links = inst.S > 0
    K = links.sum(1)
    on = K > 0                                            # latency ratios only exist for endpoints with a cache
    srt = -np.sort(-inst.S[on], 1)
    second = srt[:, 1] / srt[:, 0] if inst.C > 1 else np.ones(on.sum())
    worst = np.where(links[on], inst.S[on], np.iinfo(np.int64).max).min(1) / srt[:, 0]
    dem_e = np.bincount(inst.re, weights=inst.rn, minlength=inst.E)[on]
    deg = links.sum(0)
    inter = links.T.astype(float) @ links.astype(float)
    uni = deg[:, None] + deg[None, :] - inter
    iu = np.triu_indices(inst.C, 1)
    jac = float(np.mean(np.where(uni[iu] > 0, inter[iu] / np.maximum(uni[iu], 1), 0))) if inst.C > 1 else 1.0
    gain = np.zeros((inst.V, inst.C))
    np.add.at(gain, inst.rv, inst.rn[:, None] * inst.S[inst.re])
    dens = (gain / inst.size[:, None])[gain > 0]
    return {
        "V": inst.V, "E": inst.E, "C": inst.C, "R": inst.R,
        "rho": inst.C * inst.X / max(1, s[fit].sum()),
        "items_per_cache": inst.X / s.mean(),
        "max_size_over_X": s.max() / inst.X,
        "frac_videos_too_big": float((~fit).mean()),
        "size_cv": float(s.std() / s.mean()),
        "pop_gini": gini,
        "links_per_endpoint": float(K.mean()),
        "link_density": float(K.mean() / inst.C),
        "cache_overlap": jac,
        "second_over_best": float(np.average(second, weights=dem_e + 1e-12)) if on.any() else 1.0,
        "worst_over_best": float(np.average(worst, weights=dem_e + 1e-12)) if on.any() else 1.0,
        "density_cv": float(dens.std() / dens.mean()) if len(dens) else 0.0,
    }
