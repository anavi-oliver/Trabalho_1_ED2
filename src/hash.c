// src/hash.c

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "hash.h"

/*
 * LAYOUT DO ARQUIVO EM DISCO
 * ──────────────────────────
 *  Bytes 0..7          : long trailerOffset  — ponteiro para o cabeçalho
 *  Bytes 8..N          : buckets (posições fixas; nunca movidos)
 *  Bytes trailerOffset : int  profglobal
 *                        long diretorio[2^profglobal]
 */

// ================ structs internas ===============*/

typedef struct {
    uint64_t chave;
    size_t   tam;
    uint8_t  valor[TAM_MAX_VALOR];
    bool     ocupado;
} Registro;

typedef struct {
    int       proflocal;
    int       cont;
    Registro *registro;
} Bucket;

//struct real
struct stHashExtensivel {
    int    profglobal;
    long  *diretorio;
    int    tamBucket;
    void (*destruirValor)(void *);
    FILE  *arquivo;
};

//  helpers de I/O 

static uint64_t pegarIndice(uint64_t chave, int prof) {
    return chave & ((1ULL << prof) - 1);
}

static void salvarBucket(FILE *f, long offset, Bucket *b, int tamBucket) {
    fseek(f, offset, SEEK_SET);
    fwrite(&b->proflocal, sizeof(int),      1,         f);
    fwrite(&b->cont,      sizeof(int),      1,         f);
    fwrite(b->registro,   sizeof(Registro), tamBucket, f);
}

static Bucket *lerBucket(FILE *f, long offset, int tamBucket) {
    Bucket *b = malloc(sizeof(Bucket));
    if (!b) return NULL;
    b->registro = calloc(tamBucket, sizeof(Registro));
    if (!b->registro) { free(b); return NULL; }
    fseek(f, offset, SEEK_SET);
    fread(&b->proflocal, sizeof(int),      1,         f);
    fread(&b->cont,      sizeof(int),      1,         f);
    fread(b->registro,   sizeof(Registro), tamBucket, f);
    return b;
}

static void liberarBucketRAM(Bucket *b) {
    if (!b) return;
    free(b->registro);
    free(b);
}

static void salvarTrailer(struct stHashExtensivel *self) {
    fseek(self->arquivo, 0, SEEK_END);
    long trailerOffset = ftell(self->arquivo);

    int numEntradas = 1 << self->profglobal;
    fwrite(&self->profglobal, sizeof(int),  1,           self->arquivo);
    fwrite(self->diretorio,   sizeof(long), numEntradas, self->arquivo);

    fseek(self->arquivo, 0, SEEK_SET);
    fwrite(&trailerOffset, sizeof(long), 1, self->arquivo);
    fflush(self->arquivo);
}

//  split 

static void dividirBucket(struct stHashExtensivel *self,
                           uint64_t chave_causadora,
                           long     offset_antigo,
                           Bucket  *b_antigo)
{
    if (b_antigo->proflocal == self->profglobal) {
        int tam = 1 << self->profglobal;
        self->diretorio = realloc(self->diretorio, 2 * tam * sizeof(long));
        for (int i = 0; i < tam; i++)
            self->diretorio[i + tam] = self->diretorio[i];
        self->profglobal++;
    }

    Bucket *b_novo    = malloc(sizeof(Bucket));
    b_novo->proflocal = b_antigo->proflocal + 1;
    b_novo->cont      = 0;
    b_novo->registro  = calloc(self->tamBucket, sizeof(Registro));
    b_antigo->proflocal++;

    Registro *tmp      = b_antigo->registro;
    b_antigo->registro = calloc(self->tamBucket, sizeof(Registro));
    b_antigo->cont     = 0;

    uint64_t bit = 1ULL << (b_antigo->proflocal - 1);
    for (int i = 0; i < self->tamBucket; i++) {
        if (!tmp[i].ocupado) continue;
        Bucket *dest = (tmp[i].chave & bit) ? b_novo : b_antigo;
        for (int j = 0; j < self->tamBucket; j++) {
            if (!dest->registro[j].ocupado) {
                dest->registro[j] = tmp[i];
                dest->cont++;
                break;
            }
        }
    }
    free(tmp);

    fseek(self->arquivo, 0, SEEK_END);
    long offset_novo = ftell(self->arquivo);
    salvarBucket(self->arquivo, offset_novo,   b_novo,   self->tamBucket);
    salvarBucket(self->arquivo, offset_antigo, b_antigo, self->tamBucket);

    int      profAntes   = b_novo->proflocal - 1;
    uint64_t mascAntes   = (1ULL << profAntes) - 1;
    uint64_t padrao      = chave_causadora & mascAntes;
    int      numEntradas = 1 << self->profglobal;

    for (int i = 0; i < numEntradas; i++) {
        if ((i & mascAntes) != padrao) continue;
        self->diretorio[i] = (i & bit) ? offset_novo : offset_antigo;
    }

    salvarTrailer(self);
    liberarBucketRAM(b_novo);
}

//API pública 

HashExtensivel inicializarHash(const char *nomeArquivo,
                               int         tamBucket,
                               void (*destruirValor)(void *))
{
    struct stHashExtensivel *self = malloc(sizeof(struct stHashExtensivel));
    if (!self) return NULL;

    self->tamBucket     = tamBucket;
    self->destruirValor = destruirValor;
    self->profglobal    = 1;
    self->diretorio     = malloc(2 * sizeof(long));

    self->arquivo = fopen(nomeArquivo, "r+b");

    if (!self->arquivo) {
        self->arquivo = fopen(nomeArquivo, "w+b");
        if (!self->arquivo) { free(self->diretorio); free(self); return NULL; }

        long placeholder = 0;
        fwrite(&placeholder, sizeof(long), 1, self->arquivo);

        for (int i = 0; i < 2; i++) {
            fseek(self->arquivo, 0, SEEK_END);
            self->diretorio[i] = ftell(self->arquivo);
            Bucket *b    = malloc(sizeof(Bucket));
            b->proflocal = 1;
            b->cont      = 0;
            b->registro  = calloc(tamBucket, sizeof(Registro));
            salvarBucket(self->arquivo, self->diretorio[i], b, tamBucket);
            liberarBucketRAM(b);
        }

        salvarTrailer(self);

    } else {
        long trailerOffset = 0;
        fseek(self->arquivo, 0, SEEK_SET);
        fread(&trailerOffset, sizeof(long), 1, self->arquivo);

        fseek(self->arquivo, trailerOffset, SEEK_SET);
        fread(&self->profglobal, sizeof(int), 1, self->arquivo);

        int numEntradas = 1 << self->profglobal;
        self->diretorio = realloc(self->diretorio, numEntradas * sizeof(long));
        fread(self->diretorio, sizeof(long), numEntradas, self->arquivo);
    }

    return self;
}

bool inserirHash(HashExtensivel hash, uint64_t chave, void *valor, size_t tamValor) {
    struct stHashExtensivel *self = hash;
    if (tamValor > TAM_MAX_VALOR) return false;

    uint64_t indice = pegarIndice(chave, self->profglobal);
    long     offset = self->diretorio[indice];

    Bucket *b = lerBucket(self->arquivo, offset, self->tamBucket);
    if (!b) return false;

    for (int i = 0; i < self->tamBucket; i++) {
        if (b->registro[i].ocupado && b->registro[i].chave == chave) {
            liberarBucketRAM(b);
            return false;
        }
    }

    if (b->cont < self->tamBucket) {
        for (int i = 0; i < self->tamBucket; i++) {
            if (b->registro[i].ocupado) continue;
            b->registro[i].chave   = chave;
            b->registro[i].tam     = tamValor;
            b->registro[i].ocupado = true;
            memcpy(b->registro[i].valor, valor, tamValor);
            b->cont++;
            break;
        }
        salvarBucket(self->arquivo, offset, b, self->tamBucket);
        liberarBucketRAM(b);
        return true;
    }

    dividirBucket(self, chave, offset, b);
    liberarBucketRAM(b);
    return inserirHash(hash, chave, valor, tamValor);
}

void *procurarHash(HashExtensivel hash, uint64_t chave, size_t *tamRetornado) {
    struct stHashExtensivel *self = hash;

    uint64_t indice = pegarIndice(chave, self->profglobal);
    long     offset = self->diretorio[indice];

    Bucket *b = lerBucket(self->arquivo, offset, self->tamBucket);
    if (!b) return NULL;

    void *resultado = NULL;
    for (int i = 0; i < self->tamBucket; i++) {
        if (b->registro[i].ocupado && b->registro[i].chave == chave) {
            *tamRetornado = b->registro[i].tam;
            resultado     = malloc(*tamRetornado);
            memcpy(resultado, b->registro[i].valor, *tamRetornado);
            break;
        }
    }

    liberarBucketRAM(b);
    return resultado;
}

bool atualizarHash(HashExtensivel hash, uint64_t chave,
                   void *novoValor, size_t novoTam)
{
    struct stHashExtensivel *self = hash;
    if (novoTam > TAM_MAX_VALOR) return false;

    uint64_t indice = pegarIndice(chave, self->profglobal);
    long     offset = self->diretorio[indice];

    Bucket *b = lerBucket(self->arquivo, offset, self->tamBucket);
    if (!b) return false;

    bool ok = false;
    for (int i = 0; i < self->tamBucket; i++) {
        if (!b->registro[i].ocupado || b->registro[i].chave != chave) continue;
        memset(b->registro[i].valor, 0, TAM_MAX_VALOR);
        memcpy(b->registro[i].valor, novoValor, novoTam);
        b->registro[i].tam = novoTam;
        ok = true;
        break;
    }

    if (ok) salvarBucket(self->arquivo, offset, b, self->tamBucket);
    liberarBucketRAM(b);
    return ok;
}

bool removerHash(HashExtensivel hash, uint64_t chave) {
    struct stHashExtensivel *self = hash;

    uint64_t indice = pegarIndice(chave, self->profglobal);
    long     offset = self->diretorio[indice];

    Bucket *b = lerBucket(self->arquivo, offset, self->tamBucket);
    if (!b) return false;

    bool removido = false;
    for (int i = 0; i < self->tamBucket; i++) {
        if (!b->registro[i].ocupado || b->registro[i].chave != chave) continue;
        memset(b->registro[i].valor, 0, TAM_MAX_VALOR);
        b->registro[i].ocupado = false;
        b->registro[i].tam     = 0;
        b->cont--;
        removido = true;
        break;
    }

    if (removido) salvarBucket(self->arquivo, offset, b, self->tamBucket);
    liberarBucketRAM(b);
    return removido;
}

void iterarHash(HashExtensivel hash,
                void (*cb)(uint64_t chave, void *valor, size_t tam, void *ctx),
                void *ctx)
{
    struct stHashExtensivel *self = hash;
    if (!self || !cb) return;

    int   numEntradas  = 1 << self->profglobal;
    long *visitados    = calloc(numEntradas, sizeof(long));
    int   numVisitados = 0;

    for (int i = 0; i < numEntradas; i++) {
        long offset = self->diretorio[i];

        bool jaVisto = false;
        for (int v = 0; v < numVisitados; v++) {
            if (visitados[v] == offset) { jaVisto = true; break; }
        }
        if (jaVisto) continue;
        visitados[numVisitados++] = offset;

        Bucket *b = lerBucket(self->arquivo, offset, self->tamBucket);
        if (!b) continue;

        for (int j = 0; j < self->tamBucket; j++) {
            if (!b->registro[j].ocupado) continue;
            void *copia = malloc(b->registro[j].tam);
            memcpy(copia, b->registro[j].valor, b->registro[j].tam);
            cb(b->registro[j].chave, copia, b->registro[j].tam, ctx);
        }

        liberarBucketRAM(b);
    }

    free(visitados);
}

void imprimirHash(HashExtensivel hash, FILE *saida,
                  void (*imprimirValor)(FILE *saida, void *valor, size_t tam))
{
    struct stHashExtensivel *self = hash;
    if (!self || !saida) return;

    int numEntradas = 1 << self->profglobal;
    fprintf(saida, "profglobal: %d\n", self->profglobal);

    for (int i = 0; i < numEntradas; i++)
        fprintf(saida, "  diretorio[%3d] -> offset %ld\n", i, self->diretorio[i]);
    fprintf(saida, "\n");

    long *visitados    = calloc(numEntradas, sizeof(long));
    int   numVisitados = 0;

    for (int i = 0; i < numEntradas; i++) {
        long offset = self->diretorio[i];

        bool jaVisto = false;
        for (int v = 0; v < numVisitados; v++) {
            if (visitados[v] == offset) { jaVisto = true; break; }
        }
        if (jaVisto) continue;
        visitados[numVisitados++] = offset;

        Bucket *b = lerBucket(self->arquivo, offset, self->tamBucket);
        if (!b) continue;

        fprintf(saida,
                "--- bucket @ offset %-8ld  (proflocal=%d, ocupados=%d/%d) ---\n",
                offset, b->proflocal, b->cont, self->tamBucket);

        for (int j = 0; j < self->tamBucket; j++) {
            if (!b->registro[j].ocupado) continue;
            fprintf(saida, "  [slot %d] chave=%" PRIu64 "  tam=%zu  ",
                    j, b->registro[j].chave, b->registro[j].tam);

            if (imprimirValor) {
                imprimirValor(saida, b->registro[j].valor, b->registro[j].tam);
            } else {
                size_t lim = b->registro[j].tam < 16 ? b->registro[j].tam : 16;
                for (size_t k = 0; k < lim; k++)
                    fprintf(saida, "%02x ", b->registro[j].valor[k]);
                if (b->registro[j].tam > 16) fprintf(saida, "...");
            }
            fprintf(saida, "\n");
        }

        liberarBucketRAM(b);
    }

    free(visitados);
}

void destruirHash(HashExtensivel hash) {
    struct stHashExtensivel *self = hash;
    if (!self) return;
    if (self->arquivo) {
        salvarTrailer(self);
        fclose(self->arquivo);
    }
    free(self->diretorio);
    free(self);
}