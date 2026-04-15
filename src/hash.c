//hash_extensivel/src/hash.c

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

// fazer para iterarHash atualizarHash, imprimirHash 

// ─────────────────────── structs ────────────────────────────────────────────

typedef struct {
    uint64_t chave;
    size_t   tam;
    uint8_t  valor[TAM_MAX_VALOR];
    bool     ocupado;
} Registro;

typedef struct {
    int       proflocal;
    int       cont;
    Registro *registro; // array de tamanho tamBucket (apenas em RAM)
} Bucket;

struct stHashExtensivel {
    int    profglobal;
    long  *diretorio;
    int    tamBucket;
    void (*destruirValor)(void *);
    FILE  *arquivo;
};

// ─────────────────────── helpers de bucket ──────────────────────────────────

static uint64_t pegarIndice(uint64_t chave, int profundidade) {
    return chave & ((1ULL << profundidade) - 1);
}

static void salvarBucket(FILE *arquivo, long offset, Bucket *b, int tamBucket) {
    fseek(arquivo, offset, SEEK_SET);
    fwrite(&b->proflocal, sizeof(int),      1,         arquivo);
    fwrite(&b->cont,      sizeof(int),      1,         arquivo);
    fwrite(b->registro,   sizeof(Registro), tamBucket, arquivo);
}

static Bucket *lerBucket(FILE *arquivo, long offset, int tamBucket) {
    Bucket *b = malloc(sizeof(Bucket));
    if (!b) return NULL;
    b->registro = calloc(tamBucket, sizeof(Registro));
    if (!b->registro) { free(b); return NULL; }

    fseek(arquivo, offset, SEEK_SET);
    fread(&b->proflocal, sizeof(int),      1,         arquivo);
    fread(&b->cont,      sizeof(int),      1,         arquivo);
    fread(b->registro,   sizeof(Registro), tamBucket, arquivo);
    return b;
}

static void liberarBucketRAM(Bucket *b) {
    if (!b) return;
    free(b->registro);
    free(b);
}

// ─────────────────────── helper de cabeçalho (trailer) ──────────────────────

/*
 * Anexa profglobal + diretório no fim do arquivo e atualiza o ponteiro
 * nos primeiros 8 bytes. Chamado após qualquer mudança no diretório.
 */
static void salvarCabecalho(HashExtensivel hash) {
    /* Vai para o fim do arquivo para anexar o novo cabeçalho */
    fseek(hash->arquivo, 0, SEEK_END);
    long trailerOffset = ftell(hash->arquivo);

    int numEntradas = 1 << hash->profglobal;
    fwrite(&hash->profglobal, sizeof(int),  1,           hash->arquivo);
    fwrite(hash->diretorio,   sizeof(long), numEntradas, hash->arquivo);

    /* Atualiza o ponteiro nos primeiros 8 bytes */
    fseek(hash->arquivo, 0, SEEK_SET);
    fwrite(&trailerOffset, sizeof(long), 1, hash->arquivo);

    fflush(hash->arquivo);
}

// ─────────────────────── split ──────────────────────────────────────────────

static void dividirBucket(HashExtensivel hash,
                           uint64_t chave_causadora,
                           long     offset_b_antigo,
                           Bucket  *b_antigo)
{
    /* Dobra o diretório se necessário */
    if (b_antigo->proflocal == hash->profglobal) {
        int tam_antigo = 1 << hash->profglobal;
        hash->diretorio = realloc(hash->diretorio,
                                  2 * tam_antigo * sizeof(long));
        for (int i = 0; i < tam_antigo; i++)
            hash->diretorio[i + tam_antigo] = hash->diretorio[i];
        hash->profglobal++;
    }

    /* Cria o novo bucket em RAM */
    Bucket *b_novo = malloc(sizeof(Bucket));
    b_novo->proflocal = b_antigo->proflocal + 1;
    b_novo->cont      = 0;
    b_novo->registro  = calloc(hash->tamBucket, sizeof(Registro));

    b_antigo->proflocal++;

    /* Redistribui registros */
    Registro *temp  = b_antigo->registro;
    b_antigo->registro = calloc(hash->tamBucket, sizeof(Registro));
    b_antigo->cont     = 0;

    uint64_t mascara_bit = 1ULL << (b_antigo->proflocal - 1);

    for (int i = 0; i < hash->tamBucket; i++) {
        if (!temp[i].ocupado) continue;
        Bucket *dest = (temp[i].chave & mascara_bit) ? b_novo : b_antigo;
        for (int j = 0; j < hash->tamBucket; j++) {
            if (!dest->registro[j].ocupado) {
                dest->registro[j] = temp[i];
                dest->cont++;
                break;
            }
        }
    }
    free(temp);

    /* Salva o novo bucket ANTES de salvar o cabeçalho, para que o offset
       calculado pelo SEEK_END já inclua o novo bucket. */
    fseek(hash->arquivo, 0, SEEK_END);
    long offset_b_novo = ftell(hash->arquivo);
    salvarBucket(hash->arquivo, offset_b_novo, b_novo, hash->tamBucket);

    /* Sobrescreve o bucket antigo (mesma posição, apenas conteúdo mudou) */
    salvarBucket(hash->arquivo, offset_b_antigo, b_antigo, hash->tamBucket);

    /* Atualiza o diretório em RAM */
    int      proflocal_anterior = b_novo->proflocal - 1;
    uint64_t mascara_antiga     = (1ULL << proflocal_anterior) - 1;
    uint64_t padrao_antigo      = chave_causadora & mascara_antiga;
    int      num_entradas       = 1 << hash->profglobal;

    for (int i = 0; i < num_entradas; i++) {
        if ((i & mascara_antiga) != padrao_antigo) continue;
        hash->diretorio[i] = (i & mascara_bit) ? offset_b_novo : offset_b_antigo;
    }

    /* Persiste o cabeçalho como trailer — nunca sobrescreve buckets */
    salvarCabecalho(hash);

    liberarBucketRAM(b_novo);
}

// ─────────────────────── API pública ────────────────────────────────────────

HashExtensivel inicializarHash(const char *nomeArquivo,
                               int         tamBucket,
                               void (*destruirValor)(void *))
{
    HashExtensivel hash = malloc(sizeof(struct stHashExtensivel));
    if (!hash) return NULL;

    hash->tamBucket     = tamBucket;
    hash->destruirValor = destruirValor;
    hash->profglobal    = 1;
    hash->diretorio     = malloc((1 << 1) * sizeof(long));

    hash->arquivo = fopen(nomeArquivo, "r+b");

    if (!hash->arquivo) {
        /* ── arquivo novo ── */
        hash->arquivo = fopen(nomeArquivo, "w+b");
        if (!hash->arquivo) { free(hash->diretorio); free(hash); return NULL; }

        /* Reserva 8 bytes para o trailer pointer (ainda sem valor real) */
        long placeholder = 0;
        fwrite(&placeholder, sizeof(long), 1, hash->arquivo);

        /* Escreve os 2 buckets iniciais a partir do byte 8 */
        int numBuckets = 1 << hash->profglobal;
        for (int i = 0; i < numBuckets; i++) {
            fseek(hash->arquivo, 0, SEEK_END);
            hash->diretorio[i] = ftell(hash->arquivo);

            Bucket *b    = malloc(sizeof(Bucket));
            b->proflocal = 1;
            b->cont      = 0;
            b->registro  = calloc(tamBucket, sizeof(Registro));
            salvarBucket(hash->arquivo, hash->diretorio[i], b, tamBucket);
            liberarBucketRAM(b);
        }

        /* Cabeçalho vai para o trailer — trailer pointer é atualizado */
        salvarCabecalho(hash);

    } else {
        /* ── arquivo existente ── */
        fseek(hash->arquivo, 0, SEEK_SET);

        /* Lê o trailer pointer */
        long trailerOffset = 0;
        fread(&trailerOffset, sizeof(long), 1, hash->arquivo);

        /* Vai até o cabeçalho e lê profglobal + diretório */
        fseek(hash->arquivo, trailerOffset, SEEK_SET);
        fread(&hash->profglobal, sizeof(int), 1, hash->arquivo);

        int numBuckets = 1 << hash->profglobal;
        hash->diretorio = realloc(hash->diretorio, numBuckets * sizeof(long));
        fread(hash->diretorio, sizeof(long), numBuckets, hash->arquivo);
    }

    return hash;
}

void *procurarHash(HashExtensivel hash, uint64_t chave, size_t *tamRetornado) {
    uint64_t indice = pegarIndice(chave, hash->profglobal);
    long     offset = hash->diretorio[indice];

    Bucket *b = lerBucket(hash->arquivo, offset, hash->tamBucket);
    if (!b) return NULL;

    void *resultado = NULL;
    for (int i = 0; i < hash->tamBucket; i++) {
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

bool removerHash(HashExtensivel hash, uint64_t chave) {
    uint64_t indice = pegarIndice(chave, hash->profglobal);
    long     offset = hash->diretorio[indice];

    Bucket *b = lerBucket(hash->arquivo, offset, hash->tamBucket);
    if (!b) return false;

    bool removido = false;
    for (int i = 0; i < hash->tamBucket; i++) {
        if (!b->registro[i].ocupado || b->registro[i].chave != chave) continue;
        memset(b->registro[i].valor, 0, TAM_MAX_VALOR);
        b->registro[i].ocupado = false;
        b->registro[i].tam     = 0;
        b->cont--;
        removido = true;
        break;
    }

    if (removido)
        salvarBucket(hash->arquivo, offset, b, hash->tamBucket);

    liberarBucketRAM(b);
    return removido;
}

bool inserirHash(HashExtensivel hash, uint64_t chave, void *valor, size_t tamValor) {
    if (tamValor > TAM_MAX_VALOR) return false;

    uint64_t indice = pegarIndice(chave, hash->profglobal);
    long     offset = hash->diretorio[indice];

    Bucket *b = lerBucket(hash->arquivo, offset, hash->tamBucket);
    if (!b) return false;

    /* Rejeita chave duplicada */
    for (int i = 0; i < hash->tamBucket; i++) {
        if (b->registro[i].ocupado && b->registro[i].chave == chave) {
            liberarBucketRAM(b);
            return false;
        }
    }

    if (b->cont < hash->tamBucket) {
        for (int i = 0; i < hash->tamBucket; i++) {
            if (b->registro[i].ocupado) continue;
            b->registro[i].chave   = chave;
            b->registro[i].tam     = tamValor;
            b->registro[i].ocupado = true;
            memcpy(b->registro[i].valor, valor, tamValor);
            b->cont++;
            break;
        }
        salvarBucket(hash->arquivo, offset, b, hash->tamBucket);
        liberarBucketRAM(b);
        return true;
    }

    /* Bucket cheio → split e tenta de novo */
    dividirBucket(hash, chave, offset, b);
    liberarBucketRAM(b);
    return inserirHash(hash, chave, valor, tamValor);
}

void destruirHash(HashExtensivel hash) {
    if (!hash) return;

    if (hash->arquivo) {
        salvarCabecalho(hash); /* Persiste o estado final como trailer */
        fclose(hash->arquivo);
    }

    free(hash->diretorio);
    free(hash);
}