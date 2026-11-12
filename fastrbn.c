#include "fastrbn.h"
#ifdef __BMI2__
#include <immintrin.h>
#endif

static uint64_t ruleBit(const uint64_t* rule, const uint64_t idx)
{
    return (rule[idx >> 6] >> (idx & 63)) & 1;
}

/**
 * Creates the compact version of the network.
 * @return 0 if the network has more than {@link #FAST_MAX_N} nodes.
 */
int compileNet(const RBN* net, FastNet* f)
{
    register int i, j, t, b;
    if (net->n > FAST_MAX_N)
    {
        return 0;
    }
    f->n      = net->n;
    f->conns  = 0;
    f->k      = ( int*       ) calloc( net->n, sizeof( int       ) );
    f->inputs = ( int**      ) calloc( net->n, sizeof( int*      ) );
    f->mask   = ( uint64_t*  ) calloc( net->n, sizeof( uint64_t  ) );
    f->rule   = ( uint64_t** ) calloc( net->n, sizeof( uint64_t* ) );
    for (i = 0; i < net->n; i++)
    {
        int k = 0;
        for (j = 0; j < net->n; j++)
        {
            k += net->conn[i][j];
        }
        f->k[i]      = k;
        f->conns    += k;
        f->inputs[i] = ( int* ) malloc( (k > 0 ? k : 1) * sizeof( int ) );
        for (j = 0, t = 0; j < net->n; j++)
        {
            if ( net->conn[i][j] )
            {
                f->inputs[i][t++] = j;
                f->mask[i] |= 1ULL << j;
            }
        }
        int size     = 1 << k;
        f->rule[i]   = ( uint64_t* ) calloc( (size + 63) / 64, sizeof( uint64_t ) );
        for (t = 0; t < size; t++)
        {
            // In RBN.rules the highest column is the least significant bit (see step).
            int old = 0;
            for (b = 0; b < k; b++)
            {
                old |= ((t >> b) & 1) << (k - 1 - b);
            }
            if ( net->rules[i][old] )
            {
                f->rule[i][t >> 6] |= 1ULL << (t & 63);
            }
        }
    }
    return 1;
}

void freeFastNet(FastNet* f)
{
    register int i;
    for (i = 0; i < f->n; i++)
    {
        free(f->inputs[i]);
        free(f->rule[i]);
    }
    free(f->k);
    free(f->inputs);
    free(f->mask);
    free(f->rule);
}

/**
 * @return The next state of the network (synchronous update, as next and step).
 */
uint64_t fastNext(const FastNet* f, uint64_t s)
{
    register int i;
    uint64_t next = 0;
    for (i = 0; i < f->n; i++)
    {
#ifdef __BMI2__
        uint64_t idx = _pext_u64(s, f->mask[i]);
#else
        register int j;
        uint64_t idx = 0;
        for (j = 0; j < f->k[i]; j++)
        {
            idx |= ((s >> f->inputs[i][j]) & 1) << j;
        }
#endif
        next |= ruleBit(f->rule[i], idx) << i;
    }
    return next;
}

/**
 * Runs one step for 64 networks at once (bit slicing): bit l of in[i] is the
 * node i in the network l.
 * <p>Nodes with up to {@link #SLICE_MAX_K} inputs are evaluated with a multiplexer
 * tree over the lookup table.</p>
 */
void fastNextSliced(const FastNet* f, const uint64_t* in, uint64_t* out)
{
    register int i, j, t;
    uint64_t v[1 << (SLICE_MAX_K - 1)];
    for (i = 0; i < f->n; i++)
    {
        const int       k    = f->k[i];
        const int*      inp  = f->inputs[i];
        const uint64_t* rule = f->rule[i];
        if (k == 0)
        {
            out[i] = -ruleBit(rule, 0);
        }
        else if (k <= SLICE_MAX_K)
        {
            uint64_t w = in[inp[0]];
            int half   = 1 << (k - 1);
            for (t = 0; t < half; t++)
            {
                v[t] = (w & -ruleBit(rule, 2 * t + 1)) | (~w & -ruleBit(rule, 2 * t));
            }
            for (j = 1; j < k; j++)
            {
                w    = in[inp[j]];
                half = half >> 1;
                for (t = 0; t < half; t++)
                {
                    v[t] = (w & v[2 * t + 1]) | (~w & v[2 * t]);
                }
            }
            out[i] = v[0];
        }
        else
        {
            register int lane;
            uint64_t res = 0;
            for (lane = 0; lane < 64; lane++)
            {
                uint64_t idx = 0;
                for (j = 0; j < k; j++)
                {
                    idx |= ((in[inp[j]] >> lane) & 1) << j;
                }
                res |= ruleBit(rule, idx) << lane;
            }
            out[i] = res;
        }
    }
}
