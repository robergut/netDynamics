#include "rbnio.h"

int readRBN( FILE* f, RBN* net )
{
    register int i, j;
    if (feof(f))
    {
        return 0;
    }
    initM(net, 0, net->n);
    for (i = 0; i < net->n; i++)
    {
        fscanf(f, "%c", (net->state + i) );
        net->state[i] -= 48;
    }
    fscanf(f, "\n");
    for (i = 0; i < net->n; i++)
    {
        for (j = 0; j < net->n; j++)
        {
            fscanf(f, "%c", (net->conn[i] + j) );
            net->conn[i][j] -= 48;
        }
    }
    fscanf(f, "\n");
    initRules(net, 0.0f);
    for (i = 0; i < net->n; i++)
    {
        for (j = 0; j < pow(2, kAt(net, i)); j++)
        {
            fscanf(f, "%c", (net->rules[i] + j) );
            net->rules[i][j] -= 48;
        }
    }
    fscanf(f, "\n");
    return 1;
}

int appendRBN(FILE* f, RBN* net)
{
    register int i, j;
    for (i = 0; i < net->n; i++)
    {
        fprintf(f, "%d",    net->state[i]);
    }
    fprintf(f, "\n");
    for (i = 0; i < net->n; i++)
    {
        for (j = 0; j < net->n; j++)
        {
            fprintf(f, "%d", net->conn[i][j]);
        }
    }
    fprintf(f, "\n");
    for (i = 0; i < net->n; i++)
    {
        for (j = 0; j < pow(2, kAt(net, i)); j++)
        {
            fprintf(f, "%d", net->rules[i][j]);
        }
    }
    fprintf(f, "\n");
    return fflush(f);
}

Config* readConf( const char* name )
{
    Config* conf    = ( Config* ) malloc( sizeof( Config ) );
    FILE* f = fopen(name, "r");
    if (f == NULL)
    {
        printf("Error reading the file: %s\n", name);
        // TODO: create file
        free(conf);
        return NULL;
    }
    fscanf(f, "#operations:topology,get_attractors,get_phase\n");
    fscanf(f, "nm:%hd\n",           &conf->nm);
    fscanf(f, "m:%hd\n",            &conf->m);
    fscanf(f, "k:%f\n",             &conf->k);
    fscanf(f, "l:%f\n",             &conf->l);
    fscanf(f, "p:%f\n",             &conf->p);
    fscanf(f, "topology:%hd\n",     &conf->topology);
    fscanf(f, "redNodes:%hd\n",     &conf->redNodes);
    fscanf(f, "fd:%f\n",            &conf->red[FUNCTION_DEGENERACY]);
    fscanf(f, "id:%f\n",            &conf->red[INPUT_DEGENERACY]);
    fscanf(f, "od:%f\n",            &conf->red[OUTPUT_DEGENERACY]);
    fscanf(f, "iterations:%hd\n",   &conf->iterations);
    fscanf(f, "file:%64s\n",        conf->file       );
    fscanf(f, "operations:%64s\n",  conf->operations );
    /// Constants
    fscanf(f, "maxStates:%d\n",     &conf->maxStates );
    fclose(f);
    return conf;
}
