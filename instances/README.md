# Instances

Instance files are not versioned (some are over 10 MB). Put them here:

- `instances/*.in`: the HashCode 2017 qualification instances (`example.in`, `me_at_the_zoo.in`,
  `videos_worth_spreading.in`, `trending_today.in`, `kittens.in`) and any custom ones, e.g. made with
  the generator in `generator/instanceCreatorHashcode2016`.
- `instances/instances_contest/instanceNN.txt`: the 15 contest instances. `run_final.sh` and
  `analysis/run_all.sh` read this folder by default.
