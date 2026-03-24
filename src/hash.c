#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>  
#include <string.h>
#define TAM_MAX_VALOR 256

//arrumar inicializarhash

#include "hash.h"

/*
Registro — slot com valido, chave, tamValor e void *valor
Bucket — array de Registro com profundidadeLocal e contador de ocupação, array de entradas
stHashExtensivel — profundidadeGlobal, array de ponteiros **diretorio e tamBucket
*/
//registro

typedef struct {
    uint64_t chave;
    size_t tam;
    uint8_t  valor[TAM_MAX_VALOR]; // buffer fixo em disco
    bool ocupado;
} Registro;

typedef struct {
    int proflocal;
    int cont;
    Registro * registro; //array de tamanho "tambucket"
}Bucket;

struct stHashExtensivel{
    int profglobal;
    long *diretorio;
    int tamBucket;
    void (*destruirValor)(void *);
    FILE  *arquivo;
};

// =============== AUXILIARES ========================
//retornar ultimos "profundidade"pra achar o indice
uint64_t pegarIndice(uint64_t chave, int profundidade) {
    return chave & ((1ULL << profundidade) - 1);
}

// Salva um bucket no disco em um offset específico
void salvarBucket(FILE *arquivo, long offset, Bucket *b, int tamBucket) {
    fseek(arquivo, offset, SEEK_SET);
    fwrite(&b->proflocal, sizeof(int),      1,        arquivo);
    fwrite(&b->cont,      sizeof(int),      1,        arquivo);
    fwrite(b->registro,   sizeof(Registro), tamBucket, arquivo);
}

// Lê um bucket do disco e monta na RAM
Bucket* lerBucket(FILE *arquivo, long offset, int tamBucket) {
    Bucket *b = malloc(sizeof(Bucket));
    if (!b) return NULL;

    b->registro = calloc(tamBucket, sizeof(Registro));
    if (!b->registro) { free(b); return NULL; }

    fseek(arquivo, offset, SEEK_SET);
    fread(&b->proflocal,  sizeof(int), 1, arquivo);
    fread(&b->cont,       sizeof(int), 1, arquivo);
    fread(b->registro, sizeof(Registro), tamBucket, arquivo);

    return b;
}

// Libera a memória de um bucket na RAM
void liberarBucketRAM(Bucket *b, int tamBucket, void (*destruirValor)(void *)) {
    (void)tamBucket;
    (void)destruirValor;
    free(b->registro);
    free(b);
}

// =============== split
static void dividirBucket(HashExtensivel hash, uint64_t chave_causadora, long offset_b_antigo, Bucket *b_antigo) {
    //se precisa dobrar o diretório
    if (b_antigo->proflocal == hash->profglobal) {
        int tam_antigo = 1 << hash->profglobal;
        int tam_novo = 1 << (hash->profglobal + 1);
        
        hash->diretorio = realloc(hash->diretorio, tam_novo * sizeof(long));
        
        //espelha os ponteiros
        for (int i = 0; i < tam_antigo; i++) {
            hash->diretorio[i + tam_antigo] = hash->diretorio[i];
        }
        hash->profglobal++;
    }

    //cria o novo bucket na RAM
    Bucket *b_novo = malloc(sizeof(Bucket));
    b_novo->proflocal = b_antigo->proflocal + 1;
    b_novo->cont = 0;
    b_novo->registro = calloc(hash->tamBucket, sizeof(Registro));

    b_antigo->proflocal++; //aumenta a profundidade do antigo também
    
//array temporario
    Registro *temp_registros = b_antigo->registro;
    b_antigo->registro = calloc(hash->tamBucket, sizeof(Registro));
    b_antigo->cont = 0;

    uint64_t mascara_bit = 1ULL << (b_antigo->proflocal - 1);

    for (int i = 0; i < hash->tamBucket; i++) {
        if (temp_registros[i].ocupado) {
            Bucket *b_destino = (temp_registros[i].chave & mascara_bit) ? b_novo : b_antigo;
            
            for (int j = 0; j < hash->tamBucket; j++) {
                if (!b_destino->registro[j].ocupado) {
                    b_destino->registro[j] = temp_registros[i]; //copia a struct inteira
                    b_destino->cont++;
                    break;
                }
            }
        }
    }
    free(temp_registros); 

    //salva o novo bucket no final do arquivo
    fseek(hash->arquivo, 0, SEEK_END);
    long offset_b_novo = ftell(hash->arquivo);
    salvarBucket(hash->arquivo, offset_b_novo, b_novo, hash->tamBucket);

    //sobrescreve o b antigo
    salvarBucket(hash->arquivo, offset_b_antigo, b_antigo, hash->tamBucket);

    int proflocal_anterior = b_novo->proflocal - 1;
    uint64_t mascara_antiga = (1ULL << proflocal_anterior) - 1;
    uint64_t padrao_antigo  = chave_causadora & mascara_antiga;

    int num_entradas_diretorio = 1 << hash->profglobal;
    
    //atualizacao
    for (int i = 0; i < num_entradas_diretorio; i++) {
        // Se o índice no diretório tiver os últimos bits iguais ao padrão do bucket novo, atualiza o offset
            if ((i & mascara_antiga) != padrao_antigo) continue;
                if (i & mascara_bit)
        hash->diretorio[i] = offset_b_novo;
    else
        hash->diretorio[i] = offset_b_antigo;

    }

    liberarBucketRAM(b_novo, hash->tamBucket, NULL); //limpa da RAM
}


// ==================== IMPLEMENTACAO =====================
HashExtensivel inicializarHash(const char *nomeArquivo, int tamBucket, void (*destruirValor)(void *)) {
    HashExtensivel hash = malloc(sizeof(struct stHashExtensivel));
    hash->tamBucket = tamBucket;
    hash->destruirValor = destruirValor;
    hash->profglobal = 1; // Começa com 2 buckets (índices 0 e 1)
    
    int numBuckets = 1 << hash->profglobal; // 2^1 = 2
    hash->diretorio = malloc(numBuckets * sizeof(long));
    
    hash->arquivo = fopen(nomeArquivo, "r+b");
    if (!hash->arquivo) {

        hash->arquivo = fopen(nomeArquivo, "w+b");
        
        long offsetAtual = sizeof(int) + (numBuckets * sizeof(long)); 
        
        for (int i = 0; i < numBuckets; i++) {
            Bucket *b = malloc(sizeof(Bucket));
            b->proflocal = 1;
            b->cont = 0;
            b->registro = calloc(tamBucket, sizeof(Registro));
            
            hash->diretorio[i] = offsetAtual;
            salvarBucket(hash->arquivo, offsetAtual, b, tamBucket);
            
            // Avança o offset para o próximo bucket (tamanho fixo de um bucket no disco)
            // (Para simplificar, você pode usar fseek(SEEK_END) e ftell() para pegar a posição)
            fseek(hash->arquivo, 0, SEEK_END);
            offsetAtual = ftell(hash->arquivo);
            
            liberarBucketRAM(b, tamBucket, NULL);
        }
    } else {
        //arquivo já existe, carregar oq ja tem
        
        //cursor pra posicao 0
        fseek(hash->arquivo, 0, SEEK_SET);
        
        //le a profundidade global salva
        fread(&hash->profglobal, sizeof(int), 1, hash->arquivo);
        
        //recalcula a quant de buckets
        int numBucketsExistentes = 1 << hash->profglobal;
        
        //realoca espaco do diretorio
        hash->diretorio = realloc(hash->diretorio, numBucketsExistentes * sizeof(long));
        
        fread(hash->diretorio, sizeof(long), numBucketsExistentes, hash->arquivo);    }
    
    return hash;
}

void *procurarHash(HashExtensivel hash, uint64_t chave, size_t *tamRetornado) {
    uint64_t indice = pegarIndice(chave, hash->profglobal);
    long offset = hash->diretorio[indice];
    
    Bucket *b = lerBucket(hash->arquivo, offset, hash->tamBucket);
    void *resultado = NULL;
    
    for (int i = 0; i < hash->tamBucket; i++) {
        if (b->registro[i].ocupado && b->registro[i].chave == chave) {
            *tamRetornado = b->registro[i].tam;
            resultado = malloc(*tamRetornado);
            memcpy(resultado, b->registro[i].valor, *tamRetornado);
            break;
        }
    }
    
liberarBucketRAM(b, hash->tamBucket, NULL);
    return resultado; //retorna cópia do dado ou NULL se nao achou
}

bool removerHash(HashExtensivel hash, uint64_t chave) {
    uint64_t indice = pegarIndice(chave, hash->profglobal);
    long     offset = hash->diretorio[indice];

    Bucket *b = lerBucket(hash->arquivo, offset, hash->tamBucket);
    if (!b) return false;

    bool removido = false;
    for (int i = 0; i < hash->tamBucket; i++) {
        if (!b->registro[i].ocupado || b->registro[i].chave != chave) continue;

        /* valor é buffer fixo embutido na struct — sem heap, sem free.
           destruirValor não faz sentido aqui: não há ponteiro para liberar. */
        memset(b->registro[i].valor, 0, TAM_MAX_VALOR); // opcional: limpa o buffer
        b->registro[i].ocupado = false;
        b->registro[i].tam     = 0;
        b->cont--;
        removido = true;
        break;
    }

    if (removido)
        salvarBucket(hash->arquivo, offset, b, hash->tamBucket);

    /* NULL: Registro não tem heap — liberarBucketRAM só faz free(registro) e free(b) */
    liberarBucketRAM(b, hash->tamBucket, NULL);
    return removido;
}

bool inserirHash(HashExtensivel hash, uint64_t chave, void *valor, size_t tamValor) {
    /* Guarda de segurança: payload não cabe no buffer fixo */
    if (tamValor > TAM_MAX_VALOR) return false;

    uint64_t indice = pegarIndice(chave, hash->profglobal);
    long     offset = hash->diretorio[indice];

    Bucket *b = lerBucket(hash->arquivo, offset, hash->tamBucket);
    if (!b) return false;

    /* Rejeita chave duplicada */
    for (int i = 0; i < hash->tamBucket; i++) {
        if (b->registro[i].ocupado && b->registro[i].chave == chave) {
            liberarBucketRAM(b, hash->tamBucket, NULL);
            return false;
        }
    }

    if (b->cont < hash->tamBucket) {
        for (int i = 0; i < hash->tamBucket; i++) {
            if (b->registro[i].ocupado) continue;

            b->registro[i].chave   = chave;
            b->registro[i].tam     = tamValor;
            b->registro[i].ocupado = true;
            /* Copia para o buffer fixo — sem malloc */
            memcpy(b->registro[i].valor, valor, tamValor);
            b->cont++;
            break;
        }
        salvarBucket(hash->arquivo, offset, b, hash->tamBucket);
        liberarBucketRAM(b, hash->tamBucket, NULL); /* NULL: sem heap nos registros */
        return true;
    }

    /* Bucket cheio: split e tenta de novo (recursão) */
    dividirBucket(hash, chave, offset, b);
    liberarBucketRAM(b, hash->tamBucket, NULL);
    return inserirHash(hash, chave, valor, tamValor);
}

void destruirHash(HashExtensivel hash) {
    if (!hash) return;
    
    if (hash->arquivo) { //salvar antes de fechar
        fseek(hash->arquivo, 0, SEEK_SET);
        
        fwrite(&hash->profglobal, sizeof(int), 1, hash->arquivo);
        
        int numBuckets = 1 << hash->profglobal;
        fwrite(hash->diretorio, sizeof(long), numBuckets, hash->arquivo);
        
        fclose(hash->arquivo);
    }
    
    if (hash->diretorio) free(hash->diretorio);
    free(hash);
}