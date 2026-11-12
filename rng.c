#include "rng.h"

Rng rng;

static uint64_t splitmix64(uint64_t* x)
{
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t rotl(const uint64_t x, const int k)
{
    return (x << k) | (x >> (64 - k));
}

void rngSeed(Rng* r, uint64_t seed)
{
    register int i;
    for (i = 0; i < 4; i++)
    {
        r->s[i] = splitmix64(&seed);
    }
}

uint64_t rngNext(Rng* r)
{
    const uint64_t result = rotl(r->s[1] * 5, 7) * 9;
    const uint64_t t      = r->s[1] << 17;
    r->s[2] ^= r->s[0];
    r->s[3] ^= r->s[1];
    r->s[1] ^= r->s[2];
    r->s[0] ^= r->s[3];
    r->s[2] ^= t;
    r->s[3]  = rotl(r->s[3], 45);
    return result;
}

/**
 * @return An unbiased integer in [0, n) (Lemire's method).
 */
uint32_t rngInt(Rng* r, uint32_t n)
{
    uint64_t m = (rngNext(r) >> 32) * n;
    uint32_t l = (uint32_t) m;
    if (l < n)
    {
        uint32_t t = -n % n;
        while (l < t)
        {
            m = (rngNext(r) >> 32) * n;
            l = (uint32_t) m;
        }
    }
    return m >> 32;
}

/**
 * @return A double in [0, 1).
 */
double rngDouble(Rng* r)
{
    return (rngNext(r) >> 11) * 0x1.0p-53;
}

/**
 * Seed of an independent stream, so the results do not depend on the number of
 * threads or the order in which the work is done.
 */
uint64_t rngStream(uint64_t seed, uint64_t a, uint64_t b)
{
    return seed ^ ((a + 1) * 0x9E3779B97F4A7C15ULL) ^ ((b + 1) * 0xD1B54A32D192ED03ULL);
}
