# NetDynamics

This program is based on [RBNLab](http://turing.iimas.unam.mx/~cgg/rbn/index.html)

It let to explore some properties of different types of *Random Boolean Networks* (RBNs), specialy to look for degeneracy effects in RBNs. 

The `config` file defines the properties of the random Boolean network as listed below:
  - *nm*: Number of nodes (N) per module
    - when `m = 1`, the total number of nodes in the network is N
    - when `m > 1`, the total number of nodes in the network is N * m
  - *m*: Number of modules
  - *k*: Connectivity (0, 1]
  - *l*: Module connectivity level (0, 1]
  - *p*: Probability of 1's within rules (0, 1)
  - *topology*: The topology for the connection in the network:
    - Homogeneous = 0
    - Normal = 1
    - Scale free = 2
    - Coupled = 3
  - *redNodes*: Number of redundant Nodes.
  - *fd*: Function degeneracy degree [0, 1]
  - *id*: Input degeneracy degree [0, 1]
  - *od*: Output degeneracy degree [0, 1]
  - *iterations*: Nuber of nets to sample
  - *file*: Output file name with the network state, network connection
  - *operations*: topology,get_attractors,get_phase
  - *outDir*: Directory for the result files (optional, default `results`). It is created, with its parents, if it does not exist.
  - *maxStates*: Maximum number of states to inspect an attractor before to look for another attractor (could be enormous and need a huge quantity of memory storing attractors)
  - *seed*: Seed of the random numbers, `0` uses the time. The seed used is printed and stored in the output files, so an experiment can be repeated.
  - *exhaustiveMaxN*: Networks with up to this number of nodes (max 26) visit all their 2^N states: all the attractors and their basins are found, and `get_phase` computes the state after the transient from the cycles instead of running the steps. It needs about 20 bytes per state (320 MB for N = 24). Larger networks, or `0`, sample `maxStates` initial states.

Each line is `key:value` (`key=value` also works). Keys can appear in any order, blank
lines and lines starting with `#` are ignored, and missing keys default to `0`. `file` and
`operations` are limited to 64 characters, `outDir` to 255. An unknown key or an invalid value stops the
program and reports the line number.

## Output files

They are written to `outDir` (`results/` by default):

  - `attractors.txt`: number of attractors and states in attractors of each network.
  - `basins.txt`: basin size of each attractor, one network per line.
  - `phase-hamming.txt`: Hamming distance before and after the transient for each initial state.
  - `config_used.txt`: the configuration of the run, after the command line overrides and
    with the seed actually used (also when `seed` is `0`). It has the same format as
    `config`, so `./netDynamics <outDir>/config_used.txt` repeats the run. Comments at the top record
    the date, the command and whether the networks were generated or loaded from `file`.

## Build

```
make            # default
make NATIVE=1   # optimize for this processor
make OPENMP=1   # run networks and initial states in parallel (needs OpenMP)
```

The results do not depend on the number of threads: each network and each group of
64 initial states use their own random number stream derived from the seed.

## Running

```
./netDynamics [config_file] [--key=value | --key value]...
```

  - `config_file` is optional and defaults to `./config`.
  - Any key of the config file can be overridden with `--key=value` or `--key value`.
    Overrides are applied after reading the file.
  - `./netDynamics --help` lists the keys.

Examples:

```
./netDynamics                              # use ./config
./netDynamics experiments/k3.conf          # use another config file
./netDynamics config --k 3.0 --seed=42     # override k and the seed
./netDynamics --iterations=100 --outDir=results/n100  # use ./config and override two keys
```

Overrides make it easy to sweep a parameter from a script:

```
for k in 1 2 3 4 5; do
    ./netDynamics config --k $k --file=results/k$k/salida --seed=42 --outDir=results/k$k
done
```

Each run overwrites the result files in its `outDir`, so give each run its own
directory, as in the loop above.

**Note on `file`:** the networks are stored in `file`. If that file is empty or does
not exist, `iterations` new networks are generated and saved. If it already has
networks, they are loaded instead of generating new ones. That lets you rerun the
analysis on the same networks, but changing `nm`, `k`, `m`, etc. then has no effect
on them. Use a different `file`, or delete it, to get new networks. `file` is not
relative to `outDir`: it is used exactly as written.

When `redNodes > 0`, the networks with the redundant nodes added are written to
`deg_nodes` in `outDir` and those are the ones analyzed.

`results/` is ignored by git: runs can be repeated from their `config_used.txt`. The
final results used in the article go in [`paper/`](paper/README.md), which is versioned.

