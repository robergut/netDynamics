#ifndef CLASSICRBN_H_INCLUDED
#define CLASSICRBN_H_INCLUDED
#define DEGENERACY_TYPES 3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//This must replace the type of data for rbn's elements
#define T char

/**
 \brief Random Boolean Network for the classic model.
 */
typedef struct
{
    short n;        // N total
    short nm;       // N per module
    short m;        // Number of modules
    float k;        // Connectivity
    float l;        // Module connectivity level
    float p;        // Probability of 1's within rules
    char** conn;    // Connection matrix
    char** rules;   // Rules for each network
    char*  state;    // Current state
    char*  nextState;// Buffer to get the next state
    int**  links;//char*** links;    /// TODO: Pointers
} RBN;

typedef enum {CRBN, ARBN, DARBN, GARBN, DGARBN, MxRBN} NetType;
typedef enum {HOMOGENEOUS, NORMAL, SCALE_FREE, COUPLED} Topology;
typedef enum {INPUT_DEGENERACY, OUTPUT_DEGENERACY, FUNCTION_DEGENERACY, REDUNDANT_NODES} Redundancy;

void initModular    ( RBN*, int, float, float, int );
void init           ( RBN*, int, float );
void freeRBN        ( RBN*, const int );
void initM          ( RBN*, const int, const int );
void initRules      ( RBN*, float );
void randomInitial  ( RBN* );
void step           ( RBN* );
void addNode        ( RBN*, float* );
char** calcRed      ( RBN* );
void removeRedLink  ( RBN*, char** );

/// TODO: Test and pointers
void next(RBN*);
void generateLinksAdyacentList(RBN*);

void shuffle(int* cards, int n);

void generateHomogeneousConnectionMatrix( RBN* );
void generateNormalConnectionMatrix     ( RBN* );
void generateScaleFreeConnectionMatrix  ( RBN* );
void generateCoupledConnectionMatrix    ( RBN* );
void generateInterModuleConnections     ( RBN* , int** );

// TODO: move to util.
void    toString    ( RBN* );
float   kTot        ( RBN* );
_Bool   anyKAt      ( RBN*,   int       );
int     getK        ( RBN* );
int     kAt         ( RBN*,   int       );
int     kTh         ( RBN*,   int, int  );
void    print       ( char*,  int           );
void    printMxN    ( char**, int, int      );
void    initMatrix  ( int**,  int, int, int );

#endif // CLASSICRBN_H_INCLUDED
