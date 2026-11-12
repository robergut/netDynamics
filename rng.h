#ifndef RNG_H_INCLUDED
#define RNG_H_INCLUDED

#include <stdint.h>

/**
 \brief State of a xoshiro256** pseudo random number generator.
 <p>Each thread must use its own generator.</p>
 */
typedef struct
{
    uint64_t s[4];
} Rng;

/// Generator used to create the networks.
extern Rng rng;

void     rngSeed  ( Rng*, uint64_t );
uint64_t rngNext  ( Rng* );
uint32_t rngInt   ( Rng*, uint32_t );
double   rngDouble( Rng* );
uint64_t rngStream( uint64_t, uint64_t, uint64_t );

#endif // RNG_H_INCLUDED
