#ifndef EXPERIMENTS_H_INCLUDED
#define EXPERIMENTS_H_INCLUDED

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "classicrbn.h"
#include "rbnio.h"

// TODO: move to attractor.h
typedef struct chain
{
    char*           state;
    struct chain*   next;
} Chain;

typedef struct att
{
    int         period;
    Chain*      states;
    Chain*      last;   // Last state, to append in constant time
    struct att* next;
} Attractor;

typedef struct
{
    int size;
    Attractor* att;
} List;

void    addAttractor( List*, Attractor* );
void    freeAttractor ( Attractor* );
void    freeAttractors( List* );
int     statesInAttr( List*);
void    addState    ( Attractor*, const char*, const int );
int     contains    ( Attractor*, const char*, const int );
/******************************************************************************/

void    reportPhase     ( FILE*, const int, const int, const int, RBN* );
void    reportAttractors( FILE*, const int, const int, RBN* );
List*   getAttractors   ( RBN*,  const int, const int );
float   hamming         ( const char*, const char*, const int );

void    getInfReport    (RBN*, const int, const int);
double  getShannonInf   (char*, const int);

// Max period of attractor in RBNs
#define MAX_ATTRACTOR 3000
// Initial runs before attractor search.
#define INI_STEPS 10002
// Margin to keep checking after {@link #INI_STEPS}.
#define MARGIN 100
// Max number of states per net to be analyzed for stats.
#define MAX_STATES 4096/*2048*/

#endif // EXPERIMENTS_H_INCLUDED
