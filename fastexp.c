#include "fastexp.h"

/********************************* Hash map ***********************************/

/**
 \brief Open addressing hash map from states to attractor ids.
 */
typedef struct
{
    uint64_t* keys;
    int*      vals;     // -1 marks an empty slot
    size_t    cap;      // Power of 2
    size_t    size;
    int       bits;
} StateMap;

static size_t mapSlot(const StateMap* m, const uint64_t key)
{
    return (size_t) ((key * 0x9E3779B97F4A7C15ULL) >> (64 - m->bits));
}

static void mapInit(StateMap* m, const int bits)
{
    m->bits = bits;
    m->cap  = (size_t) 1 << bits;
    m->size = 0;
    m->keys = ( uint64_t* ) malloc( m->cap * sizeof( uint64_t ) );
    m->vals = ( int*      ) malloc( m->cap * sizeof( int      ) );
    memset(m->vals, -1, m->cap * sizeof( int ));
}

static void mapFree(StateMap* m)
{
    free(m->keys);
    free(m->vals);
}

static int mapGet(const StateMap* m, const uint64_t key)
{
    size_t i = mapSlot(m, key);
    while (m->vals[i] != -1)
    {
        if (m->keys[i] == key)
        {
            return m->vals[i];
        }
        i = (i + 1) & (m->cap - 1);
    }
    return -1;
}

static void mapPut(StateMap* m, const uint64_t key, const int val)
{
    size_t i;
    if ( (m->size + 1) * 2 > m->cap ) // keep load factor <= 0.5
    {
        StateMap bigger;
        mapInit(&bigger, m->bits + 1);
        for (i = 0; i < m->cap; i++)
        {
            if (m->vals[i] != -1)
            {
                mapPut(&bigger, m->keys[i], m->vals[i]);
            }
        }
        mapFree(m);
        *m = bigger;
    }
    i = mapSlot(m, key);
    while (m->vals[i] != -1)
    {
        if (m->keys[i] == key)
        {
            m->vals[i] = val;
            return;
        }
        i = (i + 1) & (m->cap - 1);
    }
    m->keys[i] = key;
    m->vals[i] = val;
    m->size++;
}

/********************************** Cycles ************************************/

/**
 \brief Cycles found in a network, their states are stored one after another.
 */
typedef struct
{
    uint64_t* states;
    size_t    len, cap;
    size_t*   start;    // Position of the first state of each cycle in states
    int*      period;
    double*   basin;    // States (exhaustive) or initial states (sampled) reaching each cycle
    int       count, capCycles;
} Cycles;

static int newCycle(Cycles* c)
{
    if (c->count == c->capCycles)
    {
        c->capCycles = c->capCycles ? c->capCycles * 2 : 16;
        c->start  = ( size_t* ) realloc( c->start,  c->capCycles * sizeof( size_t ) );
        c->period = ( int*    ) realloc( c->period, c->capCycles * sizeof( int    ) );
        c->basin  = ( double* ) realloc( c->basin,  c->capCycles * sizeof( double ) );
    }
    c->start [c->count] = c->len;
    c->period[c->count] = 0;
    c->basin [c->count] = 0;
    return c->count++;
}

static void pushState(Cycles* c, const uint64_t state)
{
    if (c->len == c->cap)
    {
        c->cap    = c->cap ? c->cap * 2 : 256;
        c->states = ( uint64_t* ) realloc( c->states, c->cap * sizeof( uint64_t ) );
    }
    c->states[c->len++] = state;
    c->period[c->count - 1]++;
}

static void freeCycles(Cycles* c)
{
    free(c->states);
    free(c->start);
    free(c->period);
    free(c->basin);
}

static int findRoot(int* parent, int x)
{
    while (parent[x] != x)
    {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

static int descending(const void* a, const void* b)
{
    const double x = *(const double*) a, y = *(const double*) b;
    return (x < y) - (x > y);
}

/**
 * Joins the cycles with common states in the core nodes, as {@link #contains} only
 * compares the core nodes.
 */
static AttractorStats summarize(Cycles* c, const uint64_t coreMask, const int unresolved)
{
    AttractorStats st;
    StateMap core;
    register int i;
    size_t t;
    int* parent = ( int* ) malloc( (c->count > 0 ? c->count : 1) * sizeof( int ) );
    int* group  = ( int* ) malloc( (c->count > 0 ? c->count : 1) * sizeof( int ) );
    mapInit(&core, 8);
    for (i = 0; i < c->count; i++)
    {
        parent[i] = i;
        group [i] = -1;
    }
    for (i = 0; i < c->count; i++)
    {
        for (t = c->start[i]; t < c->start[i] + c->period[i]; t++)
        {
            uint64_t p = c->states[t] & coreMask;
            int other  = mapGet(&core, p);
            if (other < 0)
            {
                mapPut(&core, p, i);
            }
            else
            {
                parent[findRoot(parent, i)] = findRoot(parent, other);
            }
        }
    }
    st.attractors = 0;
    st.states     = (int) core.size;
    st.unresolved = unresolved;
    st.basins     = ( double* ) calloc( (c->count > 0 ? c->count : 1), sizeof( double ) );
    for (i = 0; i < c->count; i++)
    {
        int root = findRoot(parent, i);
        if (group[root] < 0)
        {
            group[root] = st.attractors++;
        }
        st.basins[group[root]] += c->basin[i];
    }
    qsort(st.basins, st.attractors, sizeof( double ), descending);
    mapFree(&core);
    free(parent);
    free(group);
    return st;
}

/****************************** Attractor search ******************************/

/**
 \brief All the states of a network with their cycle, as the dynamics is a function
 between states.
 */
typedef struct
{
    uint32_t* next;     // Next state
    uint32_t* label;    // Cycle id + 1
    uint32_t* depth;    // Steps to reach the cycle
    uint32_t* entry;    // Position in the cycle of the first state reached in it
    Cycles    cycles;   // States of each cycle in the order of the dynamics
} StateSpace;

/**
 * Visits the 2^N states of the network once, labeling each one with its cycle.
 */
static void buildStateSpace(const FastNet* f, StateSpace* sp)
{
    const uint32_t PATH = UINT32_MAX; // State in the current path
    const uint32_t N    = (uint32_t) 1 << f->n;
    uint32_t* path      = ( uint32_t* ) malloc( (size_t) N * sizeof( uint32_t ) );
    uint32_t s, x, y, id, len, d, e;
    int64_t i;
    sp->next  = ( uint32_t* ) malloc( (size_t) N * sizeof( uint32_t ) );
    sp->label = ( uint32_t* ) calloc( N, sizeof( uint32_t ) ); // 0 not visited
    sp->depth = ( uint32_t* ) malloc( (size_t) N * sizeof( uint32_t ) );
    sp->entry = ( uint32_t* ) malloc( (size_t) N * sizeof( uint32_t ) );
    memset(&sp->cycles, 0, sizeof( Cycles ));
    for (s = 0; s < N; s++)
    {
        sp->next[s] = (uint32_t) fastNext(f, s);
    }
    for (s = 0; s < N; s++)
    {
        if (sp->label[s])
        {
            continue;
        }
        for (x = s, len = 0; sp->label[x] == 0; x = sp->next[x])
        {
            sp->label[x] = PATH;
            path[len++]  = x;
        }
        if (sp->label[x] == PATH) // new cycle through x
        {
            id = newCycle(&sp->cycles) + 1;
            y  = x;
            e  = 0;
            do
            {
                pushState(&sp->cycles, y);
                sp->label[y] = id;
                sp->depth[y] = 0;
                sp->entry[y] = e++;
                y = sp->next[y];
            }
            while (y != x);
        }
        id = sp->label[x];
        d  = sp->depth[x];
        e  = sp->entry[x];
        for (i = (int64_t) len - 1; i >= 0; i--) // label the transient backwards
        {
            y = path[i];
            if (sp->label[y] != PATH)
            {
                continue;
            }
            sp->label[y] = id;
            sp->depth[y] = ++d;
            sp->entry[y] = e;
        }
    }
    free(path);
}

static void freeStateSpace(StateSpace* sp)
{
    free(sp->next);
    free(sp->label);
    free(sp->depth);
    free(sp->entry);
    freeCycles(&sp->cycles);
}

/**
 * @return The state after t steps, without running the steps once in the cycle.
 */
static uint32_t stateAfter(const StateSpace* sp, uint32_t s, uint32_t t)
{
    if (t <= sp->depth[s])
    {
        while (t--)
        {
            s = sp->next[s];
        }
        return s;
    }
    const uint32_t c = sp->label[s] - 1;
    const uint32_t pos = (sp->entry[s] + (t - sp->depth[s])) % sp->cycles.period[c];
    return (uint32_t) sp->cycles.states[sp->cycles.start[c] + pos];
}

/**
 * Finds all the attractors visiting the 2^N states of the network.
 */
static AttractorStats exhaustiveAttractors(const FastNet* f, const uint64_t coreMask)
{
    const uint32_t N = (uint32_t) 1 << f->n;
    uint32_t s;
    StateSpace sp;
    AttractorStats st;
    buildStateSpace(f, &sp);
    for (s = 0; s < N; s++)
    {
        sp.cycles.basin[sp.label[s] - 1]++;
    }
    st = summarize(&sp.cycles, coreMask, 0);
    freeStateSpace(&sp);
    return st;
}

/**
 * Finds the attractors reached from random initial states with Brent's algorithm.
 * <p>The states already visited are remembered with their attractor (up to
 * {@link #MEMO_MAX} states), so a trajectory stops as soon as it reaches one of them.</p>
 */
static AttractorStats sampledAttractors(const FastNet* f, const uint64_t coreMask, const int samples, Rng* r)
{
    const uint64_t full = f->n == 64 ? ~0ULL : (1ULL << f->n) - 1;
    StateMap memo, reps;
    Cycles c;
    AttractorStats st;
    register int i, t;
    int unresolved = 0;
    memset(&c, 0, sizeof( Cycles ));
    mapInit(&memo, 10);
    mapInit(&reps, 6);
    for (i = 0; i < samples; i++)
    {
        uint64_t x0 = rngNext(r) & full, x;
        int id      = mapGet(&memo, x0);
        if (id < 0)
        {
            uint64_t tortoise = x0, hare = fastNext(f, x0);
            int power = 1, period = 1, steps = 1, cycle = 0;
            while ( (id = mapGet(&memo, hare)) < 0 )
            {
                if (tortoise == hare)
                {
                    cycle = 1;
                    break;
                }
                if (power == period)
                {
                    if (power >= BRENT_MAX_POWER)
                    {
                        break;
                    }
                    tortoise = hare;
                    power   *= 2;
                    period   = 0;
                }
                hare = fastNext(f, hare);
                period++;
                steps++;
            }
            if (id < 0)
            {
                if (!cycle || period > MAX_ATTRACTOR)
                {
                    unresolved++;
                    continue;
                }
                uint64_t rep = hare; // the minimum state identifies the cycle
                for (t = 0, x = hare; t < period; t++, x = fastNext(f, x))
                {
                    rep = x < rep ? x : rep;
                }
                id = mapGet(&reps, rep);
                if (id < 0)
                {
                    id = newCycle(&c);
                    mapPut(&reps, rep, id);
                    for (t = 0, x = rep; t < period; t++, x = fastNext(f, x))
                    {
                        pushState(&c, x);
                    }
                }
                if (memo.size + period <= MEMO_MAX)
                {
                    for (t = 0, x = rep; t < period; t++, x = fastNext(f, x))
                    {
                        mapPut(&memo, x, id);
                    }
                }
            }
            for (t = 0, x = x0; t <= steps && memo.size < MEMO_MAX && mapGet(&memo, x) < 0; t++)
            {
                mapPut(&memo, x, id);
                x = fastNext(f, x);
            }
        }
        c.basin[id]++;
    }
    mapFree(&memo);
    mapFree(&reps);
    st = summarize(&c, coreMask, unresolved);
    freeCycles(&c);
    return st;
}

SearchMethod searchMethod(const FastNet* f, const int exhaustiveMaxN)
{
    return f->n <= exhaustiveMaxN && f->n <= EXHAUSTIVE_HARD_MAX ? EXHAUSTIVE : SAMPLED;
}

/**
 * @param coreMask Nodes compared to identify the attractors.
 * @param samples Number of initial states for the sampled search.
 * @param exhaustiveMaxN Max number of nodes for the exhaustive search.
 */
AttractorStats fastAttractors(const FastNet* f, const uint64_t coreMask, const int samples, const int exhaustiveMaxN, Rng* r)
{
    if (searchMethod(f, exhaustiveMaxN) == EXHAUSTIVE)
    {
        return exhaustiveAttractors(f, coreMask);
    }
    return sampledAttractors(f, coreMask, samples, r);
}

/******************************** Experiments *********************************/

/**
 * Reads all the networks of the file.
 * @return NULL if a network has more than {@link #FAST_MAX_N} nodes.
 */
FastNet* loadNets(FILE* file, RBN* net, int* count)
{
    int cap = 16;
    FastNet* nets = ( FastNet* ) malloc( cap * sizeof( FastNet ) );
    *count = 0;
    while ( readRBN(file, net) )
    {
        if (*count == cap)
        {
            cap *= 2;
            nets = ( FastNet* ) realloc( nets, cap * sizeof( FastNet ) );
        }
        if ( !compileNet(net, &nets[*count]) )
        {
            freeNets(nets, *count);
            *count = 0;
            return NULL;
        }
        (*count)++;
    }
    return nets;
}

void freeNets(FastNet* nets, const int count)
{
    register int i;
    if (nets == NULL)
    {
        return;
    }
    for (i = 0; i < count; i++)
    {
        freeFastNet(&nets[i]);
    }
    free(nets);
}

/**
 * Generates attractors.txt with the number of attractors and states found in
 * attractors (same format as {@link #reportAttractors}), and basins.txt with the
 * basin sizes of each network.
 * @param core Number of nodes compared to identify the attractors.
 */
void reportAttractorsFast(FastNet* nets, const int count, const int maxStates, const int core,
                          const int exhaustiveMaxN, const uint64_t seed, RBN* net)
{
    int i, j;
    time_t start     = time(NULL);
    uint64_t coreMask = core >= 64 ? ~0ULL : (1ULL << core) - 1;
    SearchMethod method = count > 0 ? searchMethod(&nets[0], exhaustiveMaxN) : SAMPLED;
    AttractorStats* st  = ( AttractorStats* ) calloc( (count > 0 ? count : 1), sizeof( AttractorStats ) );
    printf("*** Attractors Found (%s search)\n", method == EXHAUSTIVE ? "exhaustive" : "sampled");
    printf("  N: %d, K: %.2f, M: %d, L: %.2f, P: %.2f\n\n", net->n, net->k, net->m, net->l, net->p);

#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < count; i++)
    {
        Rng r;
        rngSeed(&r, rngStream(seed, i, 0));
        st[i] = fastAttractors(&nets[i], coreMask, maxStates, exhaustiveMaxN, &r);
    }

    FILE* f = openOutput("attractors.txt", "w");
    FILE* b = openOutput("basins.txt", "w");
    fprintf(f, "# File to be read by R\n");
    if (method == EXHAUSTIVE)
    {
        fprintf(f, "# Attractors found (exhaustive search, all periods).\n");
    }
    else
    {
        fprintf(f, "# Attractors found (maximum period is %d, %d initial states).\n", MAX_ATTRACTOR, maxStates);
    }
    fprintf(f, "# States found in attractors. from %.0f possible states in network\n", count > 0 ? ldexp(1, nets[0].n) : 0);
    fprintf(f, "# seed: %llu\n", (unsigned long long) seed);
    fprintf(b, "# Basin size of each attractor in descending order, one network per line.\n");
    fprintf(b, method == EXHAUSTIVE ? "# Number of states in the basin.\n" : "# Number of initial states reaching the attractor.\n");
    for (i = 0; i < count; i++)
    {
        printf("Net #%d. Attractors: %d, states in attractors: %d", i, st[i].attractors, st[i].states);
        if (st[i].unresolved)
        {
            printf(" (%d initial states without attractor of period <= %d)", st[i].unresolved, MAX_ATTRACTOR);
        }
        printf("\n");
        fprintf(f, "%d\t%d\n", st[i].attractors, st[i].states);
        for (j = 0; j < st[i].attractors; j++)
        {
            fprintf(b, j ? "\t%.0f" : "%.0f", st[i].basins[j]);
        }
        fprintf(b, "\n");
        free(st[i].basins);
    }
    fclose(f);
    fclose(b);
    free(st);
    printf ("\nIt took %.2lf seconds.\n", difftime(time(NULL), start));
}

/**
 * Normalised Hamming distance of each network in a bit sliced state.
 */
static void laneHamming(const uint64_t* a, const uint64_t* b, const int n, double* d)
{
    register int i, lane;
    for (lane = 0; lane < 64; lane++)
    {
        int count = 0;
        for (i = 0; i < n; i++)
        {
            count += ((a[i] ^ b[i]) >> lane) & 1;
        }
        d[lane] = (double) count / n;
    }
}

/**
 * Runs 64 initial states and their copies with node zero flipped for
 * {@link #INI_STEPS} steps, stops before if all the pairs are equal.
 */
static void phaseBatch(const FastNet* f, const int n, Rng* r, double* dt, double* dt1)
{
    uint64_t bufA[2][FAST_MAX_N], bufB[2][FAST_MAX_N];
    uint64_t *a = bufA[0], *ta = bufA[1], *b = bufB[0], *tb = bufB[1], *tmp;
    register int i, j;
    for (i = 0; i < f->n; i++)
    {
        a[i] = b[i] = rngNext(r);
    }
    b[0] = ~a[0]; //flip node zero
    laneHamming(a, b, n, dt);
    for (j = 0; j < INI_STEPS; j++)
    {
        fastNextSliced(f, a, ta);
        fastNextSliced(f, b, tb);
        tmp = a; a = ta; ta = tmp;
        tmp = b; b = tb; tb = tmp;
        if ((j & 63) == 63)
        {
            uint64_t diff = 0;
            for (i = 0; i < f->n; i++)
            {
                diff |= a[i] ^ b[i];
            }
            if (!diff) // equal states keep equal
            {
                break;
            }
        }
    }
    laneHamming(a, b, n, dt1);
}

/**
 * Same as {@link #phaseBatch} for a network with all its states labeled, the
 * states after {@link #INI_STEPS} steps are taken from the cycles.
 */
static void phaseBatchExhaustive(const StateSpace* sp, const int nNodes, const int n, Rng* r, double* dt, double* dt1)
{
    const uint64_t full    = (1ULL << nNodes) - 1;
    const uint64_t measure = n >= 64 ? ~0ULL : (1ULL << n) - 1;
    register int lane;
    for (lane = 0; lane < 64; lane++)
    {
        uint32_t a  = (uint32_t) (rngNext(r) & full);
        uint32_t b  = a ^ 1; //flip node zero
        dt[lane]    = (double) __builtin_popcountll((a ^ b) & measure) / n;
        a           = stateAfter(sp, a, INI_STEPS);
        b           = stateAfter(sp, b, INI_STEPS);
        dt1[lane]   = (double) __builtin_popcountll((a ^ b) & measure) / n;
    }
}

/**
 * Same as {@link #reportPhase}. Networks with up to exhaustiveMaxN nodes use all
 * their states labeled (see {@link #buildStateSpace}), the others run 64 initial
 * states at once.
 * @param n Number of nodes for the Hamming distance.
 */
void reportPhaseFast(FastNet* nets, const int count, const int initialStates, const int n,
                     const int exhaustiveMaxN, const uint64_t seed, RBN* net)
{
    int i, s, batch;
    const int batches = (initialStates + 63) / 64;
    time_t start      = time(NULL);
    double avgd       = 0, stddevs = 0;
    double* dt        = ( double* ) malloc( batches * 64 * sizeof( double ) );
    double* dt1       = ( double* ) malloc( batches * 64 * sizeof( double ) );
    double* deltas    = ( double* ) malloc( (count > 0 ? count : 1) * sizeof( double ) );
    printf("Executing phase-short function\n");
    printf("***Analysing Phases for %d networks\n", count);
    printf("  N: %d, K: %f, M: %d, L: %f, P: %f\n", net->n, net->k, net->m, net->l, net->p);

    FILE* hammingFile = openOutput("phase-hamming.txt", "w+");
    fprintf(hammingFile, "# File to be read by R\n");
    fprintf(hammingFile, "# seed: %llu\n", (unsigned long long) seed);
    fprintf(hammingFile, "ktot\tdt\tdt1\tdelta\n");//headers for the file

    for (i = 0; i < count; i++)
    {
        const int nodes = n < nets[i].n ? n : nets[i].n;
        double avgdt    = 0;
        double avgdt1   = 0;
        printf("New net#%d of %d\n", i, count);
        if (searchMethod(&nets[i], exhaustiveMaxN) == EXHAUSTIVE)
        {
            StateSpace sp;
            buildStateSpace(&nets[i], &sp);
#pragma omp parallel for schedule(static)
            for (batch = 0; batch < batches; batch++)
            {
                Rng r;
                rngSeed(&r, rngStream(seed, i, batch + 1));
                phaseBatchExhaustive(&sp, nets[i].n, nodes, &r, dt + batch * 64, dt1 + batch * 64);
            }
            freeStateSpace(&sp);
        }
        else
        {
#pragma omp parallel for schedule(dynamic)
            for (batch = 0; batch < batches; batch++)
            {
                Rng r;
                rngSeed(&r, rngStream(seed, i, batch + 1));
                phaseBatch(&nets[i], nodes, &r, dt + batch * 64, dt1 + batch * 64);
            }
        }
        for (s = 0; s < initialStates; s++)
        {
            double delta = dt1[s] - dt[s];
            avgdt       += dt[s];
            avgdt1      += dt1[s];
            avgd        += delta;
            fprintf(hammingFile, "%d\t%f\t%f\t%f\n", nets[i].conns, dt[s], dt1[s], delta);
        }
        deltas[i] = avgdt1 / initialStates - avgdt / initialStates;
    }
    if (count == 0)
    {
        printf("No networks were read.\n");
    }
    else
    {
        avgd /= (double) count * initialStates;
        printf("%f\t\n", avgd);
        for (i = 0; i < count; i++)
        {
            stddevs += pow(deltas[i] - avgd, 2);
        }
        printf("\t\n");
        stddevs = sqrt(stddevs / count);
        printf("%f\n", stddevs);
        fprintf(hammingFile, "# Average:\t%f\n", avgd);
    }
    fclose(hammingFile);
    free(dt);
    free(dt1);
    free(deltas);
    printf ("\nIt took %.2lf seconds.\n", difftime(time(NULL), start));
}
