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
    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        printf("Usage: %s [config_file] [--key=value | --key value]...\n"
               "  config_file defaults to ./config. Each --key overrides the value from the file.\n"
               "  Keys: nm m k l p topology redNodes fd id od iterations file operations\n"
               "        maxStates seed exhaustiveMaxN outDir\n"
               "  Example: %s config --k 3.0 --seed=42\n", argv[0], argv[0]);
        return 0;
    }
    {
        /// argv[1] is the config file unless it is already an override (--key).
        int first            = (argc > 1 && strncmp(argv[1], "--", 2) != 0) ? 2 : 1;
        const char* confFile = first == 2 ? argv[1] : "config";
        Config* conf    = readConf(confFile);
        if (conf == NULL)
        {
            return 1;
        }
        if (!applyConfArgs(conf, argc - first, argv + first))
        {
            printf("Run %s --help for usage.\n", argv[0]);
            free(conf);
            return 1;
        }
        if (!setOutputDir(conf->outDir))
        {
            free(conf);
            return 1;
        }
        printf("Output directory: %s\n", conf->outDir[0] ? conf->outDir : ".");
        RBN* net = ( RBN* ) malloc( sizeof( RBN ) );
        register int i;
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
        int generated = feof(out);
        if ( generated )
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
        /// Store the configuration used (after the overrides and with the actual seed) next to the results.
        FILE* used      = openOutput("config_used.txt", "w");
        {
            time_t now  = time(NULL);
            fprintf(used, "# Configuration used by this run. Repeat it with: %s %s/config_used.txt\n",
                    argv[0], conf->outDir[0] ? conf->outDir : ".");
            fprintf(used, "# Date: %s", ctime(&now));
            fprintf(used, "# Command:");
            for (i = 0; i < argc; i++)
            {
                fprintf(used, " %s", argv[i]);
            }
            fprintf(used, "\n");
            if (generated)
            {
                fprintf(used, "# The networks were generated with this seed and stored in '%s'.\n", conf->file);
            }
            else
            {
                fprintf(used, "# The networks were loaded from '%s' (not generated): network parameters\n"
                              "# such as nm, k, m and the seed did not create them. Keep that file to repeat the run.\n",
                        conf->file);
            }
            conf->seed  = seed;
            writeConf(used, conf);
            fclose(used);
        }
        if ( conf->redNodes != 0 )
        {
            rewind(out);
            printf("Creating RBNs with Redundand nodes.\n");
            FILE* deg = openOutput("deg_nodes", "w+");
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
