#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
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

static int parseLong( const char* s, long min, long max, long* out )
{
    char* end;
    long v = strtol(s, &end, 10);
    if (*s == '\0' || *end != '\0' || v < min || v > max)
    {
        return 0;
    }
    *out = v;
    return 1;
}

static int parseFloat( const char* s, float* out )
{
    char* end;
    float v = strtof(s, &end);
    if (*s == '\0' || *end != '\0')
    {
        return 0;
    }
    *out = v;
    return 1;
}

static int copyStr( char* dst, const char* s, size_t size )
{
    if (*s == '\0' || strlen(s) >= size)
    {
        return 0;
    }
    strcpy(dst, s);
    return 1;
}

/// Sets one configuration value by its key. Returns 1 on success, 0 if the key is unknown or the value is invalid.
int setConfValue( Config* conf, const char* key, const char* value )
{
    long v;
    int ok;
    if      (!strcmp(key, "nm"))         { ok = parseLong(value, 0, SHRT_MAX, &v); if (ok) conf->nm = (short) v; }
    else if (!strcmp(key, "m"))          { ok = parseLong(value, 0, SHRT_MAX, &v); if (ok) conf->m = (short) v; }
    else if (!strcmp(key, "k"))          ok = parseFloat(value, &conf->k);
    else if (!strcmp(key, "l"))          ok = parseFloat(value, &conf->l);
    else if (!strcmp(key, "p"))          ok = parseFloat(value, &conf->p);
    else if (!strcmp(key, "topology"))   { ok = parseLong(value, 0, SHRT_MAX, &v); if (ok) conf->topology = (short) v; }
    else if (!strcmp(key, "redNodes"))   { ok = parseLong(value, 0, SHRT_MAX, &v); if (ok) conf->redNodes = (short) v; }
    else if (!strcmp(key, "fd"))         ok = parseFloat(value, &conf->red[FUNCTION_DEGENERACY]);
    else if (!strcmp(key, "id"))         ok = parseFloat(value, &conf->red[INPUT_DEGENERACY]);
    else if (!strcmp(key, "od"))         ok = parseFloat(value, &conf->red[OUTPUT_DEGENERACY]);
    else if (!strcmp(key, "iterations")) { ok = parseLong(value, 0, SHRT_MAX, &v); if (ok) conf->iterations = (short) v; }
    else if (!strcmp(key, "file"))       ok = copyStr(conf->file, value, MAX);
    else if (!strcmp(key, "operations")) ok = copyStr(conf->operations, value, MAX);
    else if (!strcmp(key, "outDir"))     ok = copyStr(conf->outDir, value, DIR_MAX);
    else if (!strcmp(key, "maxStates"))  { ok = parseLong(value, 0, INT_MAX, &v); if (ok) conf->maxStates = (int) v; }
    else if (!strcmp(key, "exhaustiveMaxN")) { ok = parseLong(value, 0, INT_MAX, &v); if (ok) conf->exhaustiveMaxN = (int) v; }
    else if (!strcmp(key, "seed"))
    {
        char* end;
        errno = 0;
        unsigned long long s = strtoull(value, &end, 10);
        ok = *value != '\0' && *value != '-' && *end == '\0' && errno == 0;
        if (ok) conf->seed = s;
    }
    else
    {
        printf("Unknown configuration key: %s\n", key);
        return 0;
    }
    if (!ok)
    {
        printf("Invalid value for %s: '%s'\n", key, value);
    }
    return ok;
}

static char* trim( char* s )
{
    while (isspace((unsigned char) *s)) s++;
    char* e = s + strlen(s);
    while (e > s && isspace((unsigned char) e[-1])) e--;
    *e = '\0';
    return s;
}

/// Reads "key:value" (or "key=value") lines. Blank lines and lines starting with '#' are ignored; the order does not matter.
Config* readConf( const char* name )
{
    FILE* f = fopen(name, "r");
    if (f == NULL)
    {
        printf("Error reading the file: %s\n", name);
        return NULL;
    }
    Config* conf = ( Config* ) calloc( 1, sizeof( Config ) ); /// Missing keys default to 0 / empty,
    strcpy(conf->outDir, "results");                          /// except the output directory.
    char line[256];
    int lineNo = 0, ok = 1;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        lineNo++;
        char* s = trim(line);
        if (*s == '\0' || *s == '#')
        {
            continue;
        }
        char* sep = strpbrk(s, ":=");
        if (sep == NULL)
        {
            printf("%s:%d: expected key:value\n", name, lineNo);
            ok = 0;
            continue;
        }
        *sep = '\0';
        if (!setConfValue(conf, trim(s), trim(sep + 1)))
        {
            printf("  (%s:%d)\n", name, lineNo);
            ok = 0;
        }
    }
    fclose(f);
    if (!ok)
    {
        free(conf);
        return NULL;
    }
    return conf;
}

/// Applies overrides given as "--key=value" or "--key value". Returns 1 on success, 0 on error.
int applyConfArgs( Config* conf, int argc, char* argv[] )
{
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "--", 2) != 0 || argv[i][2] == '\0')
        {
            printf("Unexpected argument: %s\n", argv[i]);
            return 0;
        }
        char key[MAX];
        const char* value;
        const char* eq = strchr(argv[i] + 2, '=');
        size_t len = eq ? (size_t) (eq - (argv[i] + 2)) : strlen(argv[i] + 2);
        if (len >= MAX)
        {
            printf("Argument too long: %s\n", argv[i]);
            return 0;
        }
        memcpy(key, argv[i] + 2, len);
        key[len] = '\0';
        if (eq)
        {
            value = eq + 1;
        }
        else if (i + 1 < argc)
        {
            value = argv[++i];
        }
        else
        {
            printf("Missing value for --%s\n", key);
            return 0;
        }
        if (!setConfValue(conf, key, value))
        {
            return 0;
        }
    }
    return 1;
}

/// Writes the configuration in the format read by readConf, so the file can be used to repeat a run.
int writeConf( FILE* f, const Config* conf )
{
    fprintf(f, "nm:%d\n",             conf->nm);
    fprintf(f, "m:%d\n",              conf->m);
    fprintf(f, "k:%.9g\n",            conf->k);
    fprintf(f, "l:%.9g\n",            conf->l);
    fprintf(f, "p:%.9g\n",            conf->p);
    fprintf(f, "topology:%d\n",       conf->topology);
    fprintf(f, "redNodes:%d\n",       conf->redNodes);
    fprintf(f, "fd:%.9g\n",           conf->red[FUNCTION_DEGENERACY]);
    fprintf(f, "id:%.9g\n",           conf->red[INPUT_DEGENERACY]);
    fprintf(f, "od:%.9g\n",           conf->red[OUTPUT_DEGENERACY]);
    fprintf(f, "iterations:%d\n",     conf->iterations);
    fprintf(f, "file:%s\n",           conf->file);
    fprintf(f, "operations:%s\n",     conf->operations);
    fprintf(f, "outDir:%s\n",         conf->outDir[0] ? conf->outDir : ".");
    fprintf(f, "maxStates:%d\n",      conf->maxStates);
    fprintf(f, "seed:%llu\n",         conf->seed);
    fprintf(f, "exhaustiveMaxN:%d\n", conf->exhaustiveMaxN);
    return fflush(f);
}

static char outputDir[DIR_MAX] = ".";

/// Sets the directory for the result files, creating it (and its parents) if needed. Returns 1 on success.
int setOutputDir( const char* dir )
{
    char path[DIR_MAX];
    struct stat st;
    char* c;
    if (dir == NULL || *dir == '\0')
    {
        dir = ".";
    }
    if (strlen(dir) >= DIR_MAX)
    {
        printf("Output directory name too long: %s\n", dir);
        return 0;
    }
    strcpy(path, dir);
    for (c = path + 1; *c != '\0'; c++)
    {
        if (*c == '/')
        {
            *c = '\0';
            mkdir(path, 0755);
            *c = '/';
        }
    }
    mkdir(path, 0755);
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        printf("Could not create the output directory: %s\n", dir);
        return 0;
    }
    strcpy(outputDir, dir);
    return 1;
}

/// Opens a result file inside the output directory. Exits if it cannot be opened.
FILE* openOutput( const char* name, const char* mode )
{
    char path[DIR_MAX + MAX];
    snprintf(path, sizeof(path), "%s/%s", outputDir, name);
    FILE* f = fopen(path, mode);
    if (f == NULL)
    {
        printf("Could not open the output file: %s\n", path);
        exit(1);
    }
    return f;
}
