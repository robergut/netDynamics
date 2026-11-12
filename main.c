#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "experiments.h"
#include "fastexp.h"
#include "classicrbn.h"
#include "rbnio.h"
#define N 10
#define K 4.0f
//#include <bitset.h>

void generateConnectionMatrix(RBN*, Topology);

int main(int argc, char* argv[])
{
    if (argc == 1) // 1
    {
        RBN* net = ( RBN* ) malloc( sizeof( RBN ) );
        register int i;
        //Config* conf    = readConf("/home/robert/config");
        //Config* conf    = readConf(argv[1]);
        Config* conf    = readConf("config");
        if (conf == NULL)
        {
            free(net);
            return 1;
        }
        FILE* out       = fopen(conf->file, "a+");
        if (out == NULL)
        {
            printf("No such file or directory.\n");
            free(net);
            free(conf);
            return 0;
        }
        uint64_t seed   = conf->seed ? conf->seed : (uint64_t) time(NULL) ^ ((uint64_t) getpid() << 32);
        rngSeed(&rng, seed);
        printf("Seed: %llu\n", (unsigned long long) seed);
        initModular(net, conf->nm + conf->redNodes, conf->k, conf->l, conf->m);
        net->p = conf->p;
        rewind(out); // the initial read position with "a+" is the end of file in some systems
        fscanf(out, "\n");
        if ( feof(out) )
        {
            printf("Create and store %d networks\n", conf->iterations);
            for (i = 0; i < conf->iterations; i++)
            {
                initM(net, 0, net->n);
                net->nm = conf->nm;
                net->n  = net->nm * net->m;
                generateConnectionMatrix(net, conf->topology);
                net->nm = conf->nm + conf->redNodes;
                net->n  = net->nm * net->m;
                initRules(net, net->p);
                appendRBN(out, net);
            }
        }
        if ( conf->redNodes != 0 )
        {
            rewind(out);
            printf("Creating RBNs with Redundand nodes.\n");
            FILE* deg = fopen("deg_nodes", "w+");
            while ( readRBN(out, net) )
            {
                net->nm = conf->nm;
                net->n  = net->nm * net->m;
                for (i = 0; i < conf->redNodes; i++)
                {
                    addNode(net, conf->red);
                }
                appendRBN(deg, net);
            }
            fclose(out);
            out = deg;
        }
        if (strstr(conf->operations, "topology") != NULL)
        {
            rewind(out);
            printf("Image module has not been implemented yet.\n");
        }
        int attractors  = strstr(conf->operations, "get_attractors") != NULL;
        int phase       = strstr(conf->operations, "get_phase") != NULL;
        int count       = 0;
        FastNet* nets   = NULL;
        if (attractors || phase)
        {
            rewind(out);
            nets = loadNets(out, net, &count);
            if (nets == NULL)
            {
                printf("Networks with more than %d nodes, using the slow implementation.\n", FAST_MAX_N);
            }
        }
        if (attractors)
        {
            if (nets != NULL)
            {
                reportAttractorsFast(nets, count, conf->maxStates, conf->nm, conf->exhaustiveMaxN, seed, net);
            }
            else
            {
                rewind(out);
                reportAttractors(out, conf->maxStates, conf->nm, net);
            }
        }
        if (phase)
        {
            if (nets != NULL)
            {
                reportPhaseFast(nets, count < conf->iterations ? count : conf->iterations,
                                conf->maxStates, conf->nm + conf->redNodes, conf->exhaustiveMaxN, seed, net);
            }
            else
            {
                rewind(out);
                reportPhase(out, conf->iterations, conf->maxStates, conf->nm + conf->redNodes, net);
            }
        }
        freeNets(nets, count);
        fclose(out);
        freeRBN(net, (conf->nm + conf->redNodes) * conf->m);
        free(conf);
    }
    else
    {
        printf("PARAM 1. Config file.\n");
    }
    /*RBN* net = ( RBN* ) malloc( sizeof( RBN ) );
    init(net, N, K);
    generateConnectionMatrix(net, 0);
    generateLinksAdyacentList(net);
    initRules(net, net->p);
    FILE* fMatch = fopen("matching.txt", "w");
    register int i, j;
    printMxN(net->conn, N, N);
    printf("\n");
    char* state = (char*) calloc(net->n, sizeof(char));
    char* aux   = (char*) calloc(net->n, sizeof(char));
    for (i = 0; i < 1000; i++)
    {
        randomInitial(net);
        memcpy(state, net->state, net->n * sizeof(char));
        step(net);
        memcpy(aux, net->state, net->n * sizeof(char));
        for (j = 0; j < net->n; j++)
        {
            fprintf(fMatch, "%d", net->state[j]);
        }fprintf(fMatch, " ");
        memcpy(net->state, state, net->n * sizeof(char));
        next(net);
        for (j = 0; j < net->n; j++)
        {
            fprintf(fMatch, "%d", net->state[j]);
        }fprintf(fMatch, " %f\n", hamming(aux, net->state, N));
    }
    fclose(fMatch);
    free(net);
    free(state);
    free(aux);*/
    return 0;
}

void generateConnectionMatrix(RBN* net, Topology topology)
{
    switch(topology)
    {
    case HOMOGENEOUS:
        generateHomogeneousConnectionMatrix( net );
        break;
    case NORMAL:
        generateNormalConnectionMatrix     ( net );
        break;
    case SCALE_FREE:
        generateScaleFreeConnectionMatrix  ( net );
        break;
    case COUPLED:
        generateCoupledConnectionMatrix    ( net );
        break;
    }
}
