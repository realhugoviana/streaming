"""Shared loader for HashCode 2017 'Streaming Videos' instances (numpy, dedups requests)."""
import numpy as np
import scipy.sparse as sp
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent          # repository root (analysis/ lives in it)
GEN = REPO / "instances/new_instance_and_gen/instanceCreatorHashcode2016/instances"
INSTANCES = {
    "example": REPO / "instances/example.in",
    "me_at_the_zoo": REPO / "instances/me_at_the_zoo.in",
    "videos_worth_spreading": REPO / "instances/videos_worth_spreading.in",
    "trending_today": REPO / "instances/trending_today.in",
    "kittens": REPO / "instances/kittens.in",
    "custom_dejavu42": GEN / "custom_dejavu42.in",
    "custom_universallambda42": GEN / "custom_universallambda42.in",
}


class Instance:
    def __init__(self, name):
        self.name = name
        tok = np.array(INSTANCES[name].read_bytes().split(), dtype=np.int64)
        V, E, R, C, X = (int(t) for t in tok[:5])
        self.V, self.E, self.R_raw, self.C, self.X = V, E, R, C, X
        p = 5
        self.size = tok[p:p + V].copy(); p += V
        self.dc = np.zeros(E, np.int64)
        self.K = np.zeros(E, np.int64)
        ce, cc, cl = [], [], []
        for e in range(E):
            self.dc[e], k = tok[p], int(tok[p + 1]); p += 2
            self.K[e] = k
            if k:
                blk = tok[p:p + 2 * k].reshape(k, 2); p += 2 * k
                ce.append(np.full(k, e)); cc.append(blk[:, 0]); cl.append(blk[:, 1])
        self.conn_e = np.concatenate(ce) if ce else np.zeros(0, np.int64)
        self.conn_c = np.concatenate(cc) if cc else np.zeros(0, np.int64)
        self.conn_lat = np.concatenate(cl) if cl else np.zeros(0, np.int64)
        rq = tok[p:p + 3 * R].reshape(R, 3)
        assert len(rq) == R, "truncated request section"
        raw_v, raw_e, raw_n = rq[:, 0], rq[:, 1], rq[:, 2]
        self.raw_counts = raw_n
        key = raw_v * E + raw_e
        ukey, inv = np.unique(key, return_inverse=True)
        self.rv = ukey // E
        self.re = ukey % E
        self.rn = np.bincount(inv, weights=raw_n).astype(np.int64)
        self.R = len(ukey)
        self.dup_multiplicity = np.bincount(inv)
        # saving matrix E x C (0 = not connected); duplicate (e,c) links keep the best one
        self.S = np.zeros((E, C), np.int64)
        np.maximum.at(self.S, (self.conn_e, self.conn_c), self.dc[self.conn_e] - self.conn_lat)
        self.total_count = int(self.rn.sum())

    def gain_matrix(self):
        """g[v, c] = standalone gain (count * saving) of putting video v alone in cache c."""
        A = sp.csr_matrix((self.rn.astype(np.float64), (self.rv, self.re)), shape=(self.V, self.E))
        return np.asarray((A @ sp.csr_matrix(self.S.astype(np.float64))).todense())

    def contest(self, weighted):
        return int(weighted * 1000 // self.total_count)

    def score_solution(self, caches_to_videos):
        """Independent scorer. caches_to_videos: dict c -> list of videos."""
        X = np.zeros((self.V, self.C), bool)
        for c, vs in caches_to_videos.items():
            X[vs, c] = True
        used = np.array([self.size[caches_to_videos.get(c, [])].sum() for c in range(self.C)])
        assert (used <= self.X).all(), "capacity violated"
        # best saving per request among caches holding its video
        best = np.zeros(self.R, np.int64)
        for c in range(self.C):
            has = X[self.rv, c]
            s = self.S[self.re, c] * has
            np.maximum(best, s, out=best)
        return int((best * self.rn).sum()), used, best, X


def read_solution(path):
    sol = {}
    for line in Path(path).read_text().splitlines():
        parts = line.split()
        if parts:
            sol[int(parts[0])] = [int(x) for x in parts[1:]]
    return sol
