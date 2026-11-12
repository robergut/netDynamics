#include "experiments.h"

void addAttractor(List* l, Attractor* newAtt)
{
    if (l->att == NULL)
    {
        l->att = newAtt;
    }
    else
    {
        Attractor* a = l->att;
        while (a->next != NULL)
        {
            a = a->next;
        }
        a->next = newAtt;
    }
    l->size++;
}

void freeAttractor(Attractor* at)
{
    Chain* s = at->states;
    while (s != NULL)
    {
        Chain* tmp = s->next;
        free(s->state);
        free(s);
        s = tmp;
    }
    free(at);
}

void freeAttractors(List* l)
{
    Attractor* a = l->att;
    while (a != NULL)
    {
        Attractor* tmp = a->next;
        freeAttractor(a);
        a = tmp;
    }
    free(l);
}

void addState(Attractor* at, const char* state, const int n)
{
    Chain* c = (Chain*) calloc(1, sizeof( Chain ) );
    //c->next = NULL;
    c->state = (char*) calloc(n, sizeof( char ) );
    memcpy(c->state, state, n);
    if (at->states == NULL)
    {
        at->states = c;
    }
    else
    {
        Chain* s = at->states;
        while (s->next != NULL)
        {
            s = s->next;
        }
        s->next = c;
    }
    at->period++;
}

int contains(Attractor* at, const char* state, const int n)
{
    Chain* s = at->states;
    while (s != NULL)
    {
        if ( !memcmp(s->state, state, sizeof(char) * n) )
        {
            return 1;
        }
        s = s->next;
    }
    return 0;
}

/**
 * Check phase for the given type of RBN and does not iterate the <i>k</i> value of the net.
 */
void reportPhase(FILE* file, const int NET_COUNT, const int INITIAL_STATES_COUNT, const int n, RBN* net)
{
    register int i, j = 0, j2, i4;
    int conns;
    time_t start    = time(NULL);
    double avgd     = 0;
    double deltas[NET_COUNT];
    char nodesAt [net->n];
    char nodesBt [net->n];
    char nodesAt1[net->n];
    printf("Executing phase-short function\n");
    printf("***Analysing Phases for %d networks\n", NET_COUNT);
    printf("  N: %d, K: %f, M: %d, L: %f, P: %f, Q:f\n", net->n, net->k, net->m, net->l, net->p);

    FILE* hammingFile = fopen("phase-hamming.txt", "w+");
    fprintf(hammingFile, "# File to be read by R\n");
    fprintf(hammingFile, "ktot\tdt\tdt1\tdelta\n");//headers for the file

    while ( j < NET_COUNT && readRBN(file, net) )
    {generateLinksAdyacentList(net);
        printf("New net#%d of %d\n", j, NET_COUNT);
        double avgdt    = 0;
        double avgdt1   = 0;
        conns = getK(net);
        for (i = 0; i < INITIAL_STATES_COUNT; i++)
        {
            if (i % 2500 == 0)
            {
                printf("Initial State %d of %d\n", i, INITIAL_STATES_COUNT);
            }
            randomInitial(net);
            memcpy(nodesAt,         net->state, sizeof(char) * net->n);
            for (j2 = 0; j2 < INI_STEPS; j2++)   //run maxAsynch steps
            {
                step(net);
            } /// should be optimized to check before running 10000 steps for deterministic cases...
            memcpy(nodesAt1,        net->state, sizeof(char) * net->n);
            memcpy(net->state,      nodesAt,    sizeof(char) * net->n);
            memcpy(net->nextState,  nodesAt,    sizeof(char) * net->n);
            //flip node zero
            net->state[0] = net->nextState[0] = net->state[0] == 0; // TODO: mut conn or rules
            memcpy(nodesBt,         net->state, sizeof(char) * net->n);
            for (j2 = 0; j2 < INI_STEPS; j2++)   //run maxAsynch steps
            {
                step(net);
            } /// should be optimized to check before running 10000 steps for deterministic cases...

            double dt    = hamming(nodesAt,   nodesBt,      n);
            double dt1   = hamming(nodesAt1,  net->state,   n);
            avgdt       += dt;
            avgdt1      += dt1;
            double delta = dt1 - dt;
            avgd        += delta;

            fprintf(hammingFile, "%d\t%f\t%f\t%f\n", conns, dt, dt1, delta);
        }
        avgdt   /= INITIAL_STATES_COUNT;
        avgdt1  /= INITIAL_STATES_COUNT;
        deltas[j++] = avgdt1 - avgdt;
    }
    if (j == 0)
    {
        printf("No networks were read.\n");
        fclose(hammingFile);
        return;
    }
    double stddevs = 0;
    avgd /= (double) j * INITIAL_STATES_COUNT;
    printf("%f\t\n", avgd);
    for (i4 = 0; i4 < j; i4++)
    {
        stddevs += pow(deltas[i4] - avgd, 2);
    }
    printf("\t\n");
    stddevs = sqrt(stddevs / j);
    printf("%f\n", stddevs);

    // detailedPhase
    fprintf(hammingFile, "# Average:\t%f\n", avgd);
    fclose(hammingFile);

    printf ("\nIt took %.2lf seconds.\n", difftime(time(NULL), start));
}

/**
 * Generates a file with the number of attractors and states found in attractors from different RBNs
 * @param file FILE* from RBNs' are defined.
 * @param maxStates Number of initial states to test for during attractors function.
 */
void reportAttractors( FILE* file,  const int maxStates, const int n, RBN* net )
{
    List* attractors;
    register int i = 0;
    time_t start = time(NULL);
    printf("*** Attractors Found\n");
    printf("  N: %d, K: %.2f, M: %d, L: %.2f, P: %.2f, Q:.2f\n\n", net->n, net->k, net->m, net->l, net->p);

    FILE* f = fopen("attractors.txt", "w");
    fprintf(f, "# File to be read by R\n");
    fprintf(f, "# Attractors found (maximum period is %d).\n", MAX_ATTRACTOR);
    fprintf(f, "# States found in attractors. from %d possible states in network\n", (int)pow(2, net->n));
    while ( readRBN(file, net) )
    {generateLinksAdyacentList(net);//
        randomInitial(net);
        printf("Net #%d. State ->", i);//printf(i % 5000 ? ". " : "\r%d ", i);
        attractors = getAttractors(net, maxStates, n);
        fprintf(f, "%d\t%d\n", attractors->size, statesInAttr(attractors));
        i++;
        freeAttractors(attractors);
    }
    fclose(f);
    printf ("\nIt took %.2lf seconds.\n", difftime(time(NULL), start));
}

int statesInAttr( List* attractors)
{
    Attractor* att = attractors->att;
    register int states = 0;
    while (att != NULL)
    {
        states += att->period;
        att = att->next;
    }
    return states;
}

/**
 * Finds attractors within the Net.
 * <p>The full {@link redrbn.Attractor#INI_STEPS} steps of the Net will be run to
 * skip the transient; Then, the cycle is detected with Brent's algorithm, allowing
 * up to {@link redrbn.Attractor#MARGIN} additional transient steps.<br>
 * Cycles longer than {@link #MAX_ATTRACTOR} are not stored, they are counted as
 * unresolved initial states instead.
 * </p>
 *
 * @return The attractors found inside the Net.
 *
 * @see #getAttractors(boolean)
 */
List* getAttractors(RBN* net, int maximal, const int n)
{
    List* attractors = ( List* ) calloc(1, sizeof( List ) );
    Attractor* a;
    char* tortoise = ( char* ) malloc( net->n * sizeof( char ) );
    int attractorFound;//indicates if the attractor has already been found
    int power, period, unresolved = 0;
    register int i = 0, k;

    while (i < maximal) // try the maximal initial states
    {
        if ( i % 5000 == 0 ) {
            printf(" %d", i);
        }
        randomInitial(net);
        for (k = 0; k < INI_STEPS; k++) // run initial steps
        {
            next(net);//step(net);
        }
        /// Brent's cycle detection: the full state is compared to detect the cycle.
        memcpy(tortoise, net->state, net->n * sizeof( char ));
        next(net);
        power = period = 1;
        while ( memcmp(tortoise, net->state, net->n * sizeof( char )) )
        {
            if (power == period)
            {
                if (power > MAX_ATTRACTOR + MARGIN)
                {
                    break;
                }
                memcpy(tortoise, net->state, net->n * sizeof( char ));
                power *= 2;
                period = 0;
            }
            next(net);
            period++;
        }
        if ( memcmp(tortoise, net->state, net->n * sizeof( char )) || period > MAX_ATTRACTOR )
        {
            unresolved++;
            i++;
            continue;
        }
        /// net->state is within the cycle, check if already in attractors
        attractorFound = 0;
        a = attractors->att;
        while (a != NULL)
        {
            if (contains(a, net->state, n))//
            {
                attractorFound = 1;
                break;
            }
            a = a->next;
        }
        ///if none was found, generate the attractor
        if (!attractorFound)
        {
            Attractor* aux = ( Attractor* ) calloc(1, sizeof( Attractor ) );
            for (k = 0; k < period; k++)
            {
                if (!contains(aux, net->state, n))//
                {
                    addState(aux, net->state, n);//
                }
                next(net);//step(net);
            }
            addAttractor(attractors, aux);
        }
        i++;
    }
    printf(" %d.", i);
    if (unresolved)
    {
        printf(" (%d initial states without attractor of period <= %d)", unresolved, MAX_ATTRACTOR);
    }
    printf("\n");
    free(tortoise);
    return attractors;
}

/**
 * Computes the normalised Hamming Distance between two boolean Lists.
 *
 * @param a the first boolean List
 * @param b the second boolean List
 *
 * @return The (normalised) Hamming Distance between <code>a</code> and <code>b</code>.
 */
float hamming(const char* a, const char* b, const int n)
{
    register int i;
    float d = 0.0f;
    for (i = 0; i < n; i++)
    {
        d += a[i] != b[i];
    }
    return d / n;
}

void getInfReport(RBN* net, const int numStates, const int core)
{
    register int i;
    char state[numStates];
    randomInitial(net);
    for (i = 0; i < numStates; i++)
    {
        step(net);
    }
    for (i = 0; i < numStates; i++)
    {
        state[i] = net->state[0];
        step(net);
    }
    printf("%d\t%f", getK(net), getShannonInf(state, numStates));
}

double getShannonInf(char* x, const int n)
{
    register int i;
    double h = 0;
    for (i = 0; i < n; i++)
    {
        h += log2((int)x[i]);
    }
    return -h;
}
