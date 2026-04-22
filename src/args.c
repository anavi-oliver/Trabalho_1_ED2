#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"

#define PATH_MAX_LEN 512
#define FILE_MAX_LEN 256

struct stArgs {
    char dirEntrada[PATH_MAX_LEN];
    char geoArq[FILE_MAX_LEN];
    char geoPath[PATH_MAX_LEN];
    char dirSaida[PATH_MAX_LEN];
    char qryArq[FILE_MAX_LEN];
    char qryPath[PATH_MAX_LEN];
    char pmArq[FILE_MAX_LEN];
    char pmPath[PATH_MAX_LEN];
};

/* Remove barra final de um path, se houver */
static void normalizarPath(char *dst, int maxLen, const char *src) {
    strncpy(dst, src, maxLen - 1);
    dst[maxLen - 1] = '\0';
    int len = strlen(dst);
    if (len > 1 && dst[len - 1] == '/')
        dst[len - 1] = '\0';
}

/* Extrai o stem de um nome de arquivo (sem extensão) */
static void stemArquivo(const char *arq, char *out, int maxLen) {
    const char *base = strrchr(arq, '/');
    base = base ? base + 1 : arq;
    strncpy(out, base, maxLen - 1);
    out[maxLen - 1] = '\0';
    char *ponto = strrchr(out, '.');
    if (ponto) *ponto = '\0';
}

Args parsearArgs(int argc, char *argv[]) {
    struct stArgs *a = calloc(1, sizeof(struct stArgs));
    if (!a) return NULL;

    /* default para dirEntrada */
    strcpy(a->dirEntrada, ".");

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            normalizarPath(a->dirEntrada, PATH_MAX_LEN, argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            strncpy(a->geoArq, argv[++i], FILE_MAX_LEN - 1);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            normalizarPath(a->dirSaida, PATH_MAX_LEN, argv[++i]);
        } else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc) {
            strncpy(a->qryArq, argv[++i], FILE_MAX_LEN - 1);
        } else if (strcmp(argv[i], "-pm") == 0 && i + 1 < argc) {
            strncpy(a->pmArq, argv[++i], FILE_MAX_LEN - 1);
        }
        i++;
    }

    /* -f e -o são obrigatórios */
    if (a->geoArq[0] == '\0' || a->dirSaida[0] == '\0') {
        free(a);
        return NULL;
    }

    /* Monta caminhos completos */
    strncpy(a->geoPath, a->dirEntrada, PATH_MAX_LEN - 1);
    strncat(a->geoPath, "/",       PATH_MAX_LEN - strlen(a->geoPath) - 1);
    strncat(a->geoPath, a->geoArq, PATH_MAX_LEN - strlen(a->geoPath) - 1);
    if (a->qryArq[0] != '\0') {
        strncpy(a->qryPath, a->dirEntrada, PATH_MAX_LEN - 1);
        strncat(a->qryPath, "/",       PATH_MAX_LEN - strlen(a->qryPath) - 1);
        strncat(a->qryPath, a->qryArq, PATH_MAX_LEN - strlen(a->qryPath) - 1);
    }
    if (a->pmArq[0] != '\0') {
        strncpy(a->pmPath, a->dirEntrada, PATH_MAX_LEN - 1);
        strncat(a->pmPath, "/",      PATH_MAX_LEN - strlen(a->pmPath) - 1);
        strncat(a->pmPath, a->pmArq, PATH_MAX_LEN - strlen(a->pmPath) - 1);
    }

    return (Args)a;
}

void destruirArgs(Args args) {
    free(args);
}

const char *argsGetDirEntrada(const Args args) {
    struct stArgs *a = args;
    return a->dirEntrada;
}

const char *argsGetDirSaida(const Args args) {
    struct stArgs *a = args;
    return a->dirSaida;
}

const char *argsGetGeoPath(const Args args) {
    struct stArgs *a = args;
    return a->geoPath;
}

const char *argsGetQryPath(const Args args) {
    struct stArgs *a = args;
    return a->qryArq[0] != '\0' ? a->qryPath : NULL;
}

const char *argsGetPmPath(const Args args) {
    struct stArgs *a = args;
    return a->pmArq[0] != '\0' ? a->pmPath : NULL;
}

void argsBaseSaida(const Args args, char *out, int outLen) {
    struct stArgs *a = args;
    char stemGeo[FILE_MAX_LEN];
    stemArquivo(a->geoArq, stemGeo, FILE_MAX_LEN);

    if (a->qryArq[0] == '\0') {
        snprintf(out, outLen, "%s", stemGeo);
    } else {
        char stemQry[FILE_MAX_LEN];
        stemArquivo(a->qryArq, stemQry, FILE_MAX_LEN);
        snprintf(out, outLen, "%s-%s", stemGeo, stemQry);
    }
}