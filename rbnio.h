#ifndef RBNIO_H_INCLUDED
#define RBNIO_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "classicrbn.h"
#define MAX 65
#define DIR_MAX 256

typedef struct
{
    short nm;                   /// N per module
    short m;                    /// Number of modules
    float k;                    /// Connectivity
    float l;                    /// Module connectivity level
    float p;                    /// Probability of 1's within rules
    short topology;             /// The topology for the connection in the network see \link{Topology}.
    short iterations;           /// Nuber of nets
    short redNodes;             /// Number of redundant Nodes.
    float red[DEGENERACY_TYPES];/// Number of @link{Redundancy} Nodes (just deg types)
    char file[MAX];             /// Output file
    char operations[MAX];       /// topology,get_attractors,get_phase
    char outDir[DIR_MAX];       /// Directory for the result files (default: current directory)
    /// constants:
    int maxStates;
    unsigned long long seed;    /// Seed of the random numbers, 0 to use the time.
    int exhaustiveMaxN;         /// Max number of nodes to find all the attractors, 0 to sample.
} Config;

int     readRBN  ( FILE*, RBN* );
int     appendRBN( FILE*, RBN* );
Config*  readConf( const char* );
int     setConfValue  ( Config*, const char* key, const char* value );
int     applyConfArgs ( Config*, int argc, char* argv[] );
int     writeConf     ( FILE*, const Config* );
int     setOutputDir  ( const char* );
FILE*   openOutput    ( const char* name, const char* mode );

#endif // RBNIO_H_INCLUDED
