#ifndef FASTRBN_H_INCLUDED
#define FASTRBN_H_INCLUDED

#include <stdint.h>
#include "classicrbn.h"

// Max number of nodes, the state of the network is stored in a uint64_t.
#define FAST_MAX_N 64
// Max connectivity of a node evaluated with a multiplexer tree in fastNextSliced.
#define SLICE_MAX_K 10

/**
 \brief Compact version of a RBN to run the dynamics.
 <p>Bit i of a state is node i. The bit j of a rule index is the j-th input of the
 node in ascending column order, the rules are reordered from RBN.rules to follow
 this convention.</p>
 */
typedef struct
{
    int        n;       // Number of nodes
    int        conns;   // Total number of links (see getK)
    int*       k;       // Inputs per node
    int**      inputs;  // Inputs of each node in ascending order
    uint64_t*  mask;    // Inputs of each node as a bit mask
    uint64_t** rule;    // Lookup table of each node, one bit per entry
} FastNet;

int      compileNet     ( const RBN*, FastNet* );
void     freeFastNet    ( FastNet* );
uint64_t fastNext       ( const FastNet*, uint64_t );
void     fastNextSliced ( const FastNet*, const uint64_t*, uint64_t* );

#endif // FASTRBN_H_INCLUDED
