#ifndef FASTEXP_H_INCLUDED
#define FASTEXP_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include "fastrbn.h"
#include "experiments.h"
#include "rng.h"

// Max number of nodes for the exhaustive search (2^N states, 20 bytes per state).
#define EXHAUSTIVE_HARD_MAX 26
// Max power of Brent's algorithm, it must be greater than INI_STEPS + MARGIN.
#define BRENT_MAX_POWER 16384
// Max number of states remembered with their attractor in the sampled search.
#define MEMO_MAX (1 << 20)

typedef enum {EXHAUSTIVE, SAMPLED} SearchMethod;

/**
 \brief Attractors of a network. Attractors with common states in the core
 nodes are counted as one attractor.
 */
typedef struct
{
    int     attractors; // Number of attractors
    int     states;     // Distinct core states in attractors
    int     unresolved; // Initial states without attractor of period <= MAX_ATTRACTOR (sampled)
    double* basins;     // Basin size of each attractor in descending order
} AttractorStats;

FastNet*        loadNets            ( FILE*, RBN*, int* );
void            freeNets            ( FastNet*, const int );
SearchMethod    searchMethod        ( const FastNet*, const int );
AttractorStats  fastAttractors      ( const FastNet*, const uint64_t, const int, const int, Rng* );
void            reportAttractorsFast( FastNet*, const int, const int, const int, const int, const uint64_t, RBN* );
void            reportPhaseFast     ( FastNet*, const int, const int, const int, const int, const uint64_t, RBN* );

#endif // FASTEXP_H_INCLUDED
