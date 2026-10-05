"""Upper bounds (in contest-score units) of increasing strength / cost.

UB_cov : per video, value of k copies <= min(U_v, sum of its k best standalone gains); relax
         integrality + per-cache capacity into global capacity C*X -> fractional knapsack.
ALP    : z_v <= U_v, z_v <= sum_c g_vc x_vc, per-cache knapsack rows, 0<=x<=1 (LP).
RLP    : same but per request: z_r <= n_r*best_r, z_r <= sum_c n_r w_rc x_{v_r c}.
FULL LP: classic facility-location LP relaxation (y_rc <= x_vc, sum_c y_rc <= 1).
MIP    : FULL with x binary (HiGHS, time-limited; reports primal + dual bound).
"""
import json
import sys
import time
import numpy as np
import scipy.sparse as sp
from scipy.optimize import linprog, milp, LinearConstraint, Bounds
from inst import Instance


def ub_cov(I, g):
    s, X, C = I.size, I.X, I.C
    best = I.S[I.re].max(1)
    U = np.bincount(I.rv, weights=I.rn * best, minlength=I.V)
    U[s > X] = 0
    gs = -np.sort(-g, 1)                              # per video, gains sorted desc
    cum = np.cumsum(gs, 1)
    capped = np.minimum(cum, U[:, None])
    marg = np.diff(np.concatenate([np.zeros((I.V, 1)), capped], 1), axis=1)  # decreasing marginals
    vals = marg.ravel(); wts = np.repeat(s, C).astype(np.float64)
    m = vals > 0
    vals, wts = vals[m], wts[m]
    o = np.argsort(-vals / wts)
    vals, wts = vals[o], wts[o]
    cs = np.cumsum(wts); cap = C * X
    k = np.searchsorted(cs, cap, side="right")
    tot = vals[:k].sum() + (vals[k] * (cap - (cs[k - 1] if k else 0)) / wts[k] if k < len(vals) else 0)
    return I.contest(tot)


def pairs(I, g):
    ok = (g > 0) & (I.size[:, None] <= I.X)
    pv, pc = np.nonzero(ok)
    pid = -np.ones((I.V, I.C), np.int64); pid[pv, pc] = np.arange(len(pv))
    return pv, pc, pid


def solve_lp(c_obj, A, b, ub, integrality=None, time_limit=600):
    t = time.time()
    if integrality is None:
        r = linprog(-c_obj, A_ub=A, b_ub=b, bounds=np.c_[np.zeros_like(ub), ub], method="highs",
                    options={"time_limit": time_limit, "presolve": True})
        return {"status": r.status, "msg": r.message, "value": -r.fun if r.x is not None else None,
                "time_s": time.time() - t}, r.x
    r = milp(-c_obj, constraints=LinearConstraint(A, -np.inf, b), bounds=Bounds(np.zeros_like(ub), ub),
             integrality=integrality, options={"time_limit": time_limit, "disp": False, "mip_rel_gap": 1e-6})
    return {"status": r.status, "msg": r.message, "value": -r.fun if r.x is not None else None,
            "dual_bound": -getattr(r, "mip_dual_bound", np.nan), "gap": getattr(r, "mip_gap", None),
            "time_s": time.time() - t}, r.x


def scale(I):
    return 1000.0 / I.total_count          # objective in contest units


def alp(I, g, time_limit):
    pv, pc, pid = pairs(I, g)
    P, V, C = len(pv), I.V, I.C
    best = I.S[I.re].max(1)
    U = np.bincount(I.rv, weights=I.rn * best, minlength=V) * scale(I)
    # vars: x (P) then z (V)
    rows1 = sp.csr_matrix((np.r_[-g[pv, pc] * scale(I), np.ones(V)], (np.r_[pv, np.arange(V)], np.r_[np.arange(P), P + np.arange(V)])), shape=(V, P + V))
    rows2 = sp.csr_matrix((I.size[pv].astype(float), (pc, np.arange(P))), shape=(C, P + V))
    A = sp.vstack([rows1, rows2]).tocsr()
    b = np.r_[np.zeros(V), np.full(C, I.X, float)]
    ub = np.r_[np.ones(P), U]
    cobj = np.r_[np.zeros(P), np.ones(V)]
    return solve_lp(cobj, A, b, ub, time_limit=time_limit)[0]


def rlp(I, g, time_limit):
    pv, pc, pid = pairs(I, g)
    P, R, C = len(pv), I.R, I.C
    sc = scale(I)
    best = I.S[I.re].max(1)
    # z_r - sum_c n_r w_rc x_{v_r,c} <= 0
    Sr = I.S[I.re]                                   # R x C
    rr, cc = np.nonzero(Sr > 0)
    xp = pid[I.rv[rr], cc]
    keep = xp >= 0
    rr, cc, xp = rr[keep], cc[keep], xp[keep]
    vals = -(I.rn[rr] * Sr[rr, cc]) * sc
    rows1 = sp.csr_matrix((np.r_[vals, np.ones(R)], (np.r_[rr, np.arange(R)], np.r_[xp, P + np.arange(R)])), shape=(R, P + R))
    rows2 = sp.csr_matrix((I.size[pv].astype(float), (pc, np.arange(P))), shape=(C, P + R))
    A = sp.vstack([rows1, rows2]).tocsr()
    b = np.r_[np.zeros(R), np.full(C, I.X, float)]
    ub = np.r_[np.ones(P), I.rn * best * sc]
    return solve_lp(np.r_[np.zeros(P), np.ones(R)], A, b, ub, time_limit=time_limit)[0]


def full(I, g, time_limit, integer=False):
    pv, pc, pid = pairs(I, g)
    P, R, C = len(pv), I.R, I.C
    sc = scale(I)
    Sr = I.S[I.re]
    rr, cc = np.nonzero(Sr > 0)
    xp = pid[I.rv[rr], cc]
    keep = xp >= 0
    rr, cc, xp = rr[keep], cc[keep], xp[keep]
    Y = len(rr)
    # vars: x (P) then y (Y)
    link = sp.csr_matrix((np.r_[np.ones(Y), -np.ones(Y)], (np.r_[np.arange(Y), np.arange(Y)], np.r_[P + np.arange(Y), xp])), shape=(Y, P + Y))
    one = sp.csr_matrix((np.ones(Y), (rr, P + np.arange(Y))), shape=(R, P + Y))
    capr = sp.csr_matrix((I.size[pv].astype(float), (pc, np.arange(P))), shape=(C, P + Y))
    A = sp.vstack([link, one, capr]).tocsr()
    b = np.r_[np.zeros(Y), np.ones(R), np.full(C, I.X, float)]
    cobj = np.r_[np.zeros(P), I.rn[rr] * Sr[rr, cc] * sc]
    ub = np.ones(P + Y)
    integ = np.r_[np.ones(P), np.zeros(Y)] if integer else None
    res, x = solve_lp(cobj, A, b, ub, integrality=integ, time_limit=time_limit)
    res["nvars"] = P + Y; res["nrows"] = A.shape[0]; res["nnz"] = A.nnz
    sol = None
    if integer and x is not None:
        sol = {}
        for k in np.nonzero(x[:P] > 0.5)[0]:
            sol.setdefault(int(pc[k]), []).append(int(pv[k]))
    return res, sol


if __name__ == "__main__":
    name, which = sys.argv[1], sys.argv[2:]
    I = Instance(name)
    g = I.gain_matrix()
    g[I.size > I.X, :] = 0
    out = {"name": name}
    for w in which:
        t = time.time()
        if w == "cov":
            out["UB_cov"] = ub_cov(I, g)
        elif w == "alp":
            out["ALP"] = alp(I, g, 1500)
        elif w == "rlp":
            out["RLP"] = rlp(I, g, 1500)
        elif w == "lp":
            out["LP"] = full(I, g, 1500)[0]
        elif w == "mip":
            res, sol = full(I, g, 900, integer=True)
            out["MIP"] = res
            if sol is not None:
                wsc = I.score_solution(sol)[0]
                out["MIP"]["verified_contest"] = I.contest(wsc)
                with open(f"sol/{name}.mip.txt", "w") as fh:
                    for c in sorted(sol):
                        fh.write(" ".join(map(str, [c] + sorted(sol[c]))) + "\n")
        print(w, f"{time.time()-t:.1f}s", json.dumps(out.get({'cov':'UB_cov','alp':'ALP','rlp':'RLP','lp':'LP','mip':'MIP'}[w]), default=str), flush=True)
    json.dump(out, open(f"res/{name}.bounds.{'_'.join(which)}.json", "w"), indent=1, default=str)
