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
        net->state[i] = getc(f) - 48;
    }
    fscanf(f, "\n");
    for (i = 0; i < net->n; i++)
    {
        for (j = 0; j < net->n; j++)
        {
            net->conn[i][j] = getc(f) - 48;
        }
    }
    fscanf(f, "\n");
    for (i = 0; i < net->n; i++)
    {
        int size = 1 << kAt(net, i);
        net->rules[i] = ( char* ) realloc(net->rules[i], size * sizeof( char ) );
        for (j = 0; j < size; j++)
        {
            net->rules[i][j] = getc(f) - 48;
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
        int size = 1 << kAt(net, i);
        for (j = 0; j < size; j++)
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
    /// Optional keys at the end of the file
    conf->seed           = 0;
    conf->exhaustiveMaxN = 0;
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
    fscanf(f, "seed:%llu\n",        &conf->seed );
    fscanf(f, "exhaustiveMaxN:%d\n", &conf->exhaustiveMaxN );
    fclose(f);
    return conf;
}
