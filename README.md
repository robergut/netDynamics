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
  - *maxStates*: Maximum number of states to inspect an attractor before to look for another attractor (could be enormous and need a huge quantity of memory storing attractors)
  - *seed*: Seed of the random numbers, `0` uses the time. The seed used is printed and stored in the output files, so an experiment can be repeated.
  - *exhaustiveMaxN*: Networks with up to this number of nodes (max 26) visit all their 2^N states: all the attractors and their basins are found, and `get_phase` computes the state after the transient from the cycles instead of running the steps. It needs about 20 bytes per state (320 MB for N = 24). Larger networks, or `0`, sample `maxStates` initial states.

`seed` and `exhaustiveMaxN` are optional and must be at the end of the file.

## Output files

  - `attractors.txt`: number of attractors and states in attractors of each network.
  - `basins.txt`: basin size of each attractor, one network per line.
  - `phase-hamming.txt`: Hamming distance before and after the transient for each initial state.

## Build

```
make            # default
make NATIVE=1   # optimize for this processor
make OPENMP=1   # run networks and initial states in parallel (needs OpenMP)
```

The results do not depend on the number of threads: each network and each group of
64 initial states use their own random number stream derived from the seed.

