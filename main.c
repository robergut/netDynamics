#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "experiments.h"
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
        srand( time(NULL) );
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
        initModular(net, conf->nm + conf->redNodes, conf->k, conf->l, conf->m);
        net->p = conf->p;
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
        if (strstr(conf->operations, "get_attractors") != NULL)
        {
            rewind(out);
            reportAttractors(out, conf->maxStates, conf->nm, net);
        }
        if (strstr(conf->operations, "get_phase") != NULL)
        {
            rewind(out);
            reportPhase(out, conf->iterations, conf->maxStates, conf->nm + conf->redNodes, net);
        }
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
