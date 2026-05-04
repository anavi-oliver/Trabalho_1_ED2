#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "cidade.h"

typedef struct {
    char   cep[32];
    double x, y;
    double w, h;
    double sw;
    char   corFill[32];
    char   corStroke[32];
} Quadra;

// FNV-1a 64 bits
uint64_t cepParaChave(const char *cep) {
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)cep; *p; p++) {
        hash ^= (uint64_t)*p;
        hash *= 1099511628211ULL;
    }
    return hash;
}

// leitura do .geo
int lerGeo(const char *caminhoGeo, HashExtensivel hashQuadras) {
    FILE *f = fopen(caminhoGeo, "r");
    if (!f) return -1;

    double swPad = 1.0;
    char   fillPad[32]   = "blue";  /* preenchimento padrão: azul  */
    char   strokePad[32] = "gray";  /* borda padrão: cinza         */

    int inseridos = 0;
    char linha[512];

    while (fgets(linha, sizeof(linha), f)) {
        char cmd[8];
        if (sscanf(linha, "%7s", cmd) != 1) continue;

        if (strcmp(cmd, "q") == 0) {
            Quadra q;
            memset(&q, 0, sizeof(q));
            if (sscanf(linha, "%*s %31s %lf %lf %lf %lf",
                       q.cep, &q.x, &q.y, &q.w, &q.h) != 5)
                continue;
            q.sw = swPad;
            strncpy(q.corFill,   fillPad,   sizeof(q.corFill)   - 1);
            strncpy(q.corStroke, strokePad, sizeof(q.corStroke) - 1);

            inserirHash(hashQuadras, cepParaChave(q.cep), &q, sizeof(Quadra));
            inseridos++;

        } else if (strcmp(cmd, "cq") == 0) {
            char swStr[16];
            if (sscanf(linha, "%*s %15s %31s %31s", swStr, fillPad, strokePad) == 3)
                swPad = atof(swStr); /* atof ignora sufixo "px" automaticamente */
        }
    }

    fclose(f);
    return inseridos;
}

// callback para desenharQuadras
static void cbDesenhar(uint64_t chave, void *valor, size_t tam, void *ctx) {
    (void)chave; (void)tam;
    Quadra *q  = valor;
    ArqSvg svg = ctx;
    svgQuadra(svg, q->x, q->y, q->w, q->h, q->sw,
              q->corFill, q->corStroke, q->cep);
    free(valor); /* iterarHash entrega cópia alocada */
}

void desenharQuadras(HashExtensivel hashQuadras, ArqSvg svg) {
    iterarHash(hashQuadras, cbDesenhar, svg);
}

// busca / remoção
int buscarQuadra(HashExtensivel hashQuadras, const char *cep,
                 void *out, size_t *outTam)
{
    size_t tam = 0;
    void *r = procurarHash(hashQuadras, cepParaChave(cep), &tam);
    if (!r) return 0;
    memcpy(out, r, tam);
    if (outTam) *outTam = tam;
    free(r);
    return 1;
}

int removerQuadra(HashExtensivel hashQuadras, const char *cep) {
    return removerHash(hashQuadras, cepParaChave(cep)) ? 1 : 0;
}

size_t tamQuadra(void) {
    return sizeof(Quadra);
}

// acessores
const char *quadraGetCep      (const void *q) { return ((const Quadra *)q)->cep;       }
double      quadraGetX        (const void *q) { return ((const Quadra *)q)->x;         }
double      quadraGetY        (const void *q) { return ((const Quadra *)q)->y;         }
double      quadraGetW        (const void *q) { return ((const Quadra *)q)->w;         }
double      quadraGetH        (const void *q) { return ((const Quadra *)q)->h;         }
double      quadraGetSw       (const void *q) { return ((const Quadra *)q)->sw;        }
const char *quadraGetCorFill  (const void *q) { return ((const Quadra *)q)->corFill;   }
const char *quadraGetCorStroke(const void *q) { return ((const Quadra *)q)->corStroke; }