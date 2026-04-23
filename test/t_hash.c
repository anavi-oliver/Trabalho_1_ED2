#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "unity/src/unity.h"
#include "hash.h"

#define TAM_BUCKET 4
#define FILENAME   "test_hash.dat"

static HashExtensivel map;

typedef struct {
    int    id;
    char   nome[32];
    double nota;
} Aluno;

void setUp(void) {
    map = inicializarHash(FILENAME, TAM_BUCKET, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(map, "inicializarHash nao deve retornar NULL");
}

void tearDown(void) {
    destruirHash(map);
    remove(FILENAME);
}

// ===================== inserir ==========================
/* Inserir um inteiro e buscar pelo mesmo valor */
void inserir_e_buscar_inteiro(void) {
    int v = 42;
    TEST_ASSERT_TRUE(inserirHash(map, 1, &v, sizeof(int)));

    size_t s;
    int *res = procurarHash(map, 1, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_UINT(sizeof(int), s);
    TEST_ASSERT_EQUAL_INT(42, *res);
    free(res);
}

/* Inserir uma struct e conferir cada campo individualmente */
void inserir_e_buscar_struct(void) {
    Aluno a = {7, "Maria", 8.75};
    TEST_ASSERT_TRUE(inserirHash(map, (uint64_t)a.id, &a, sizeof(Aluno)));

    size_t s;
    Aluno *res = procurarHash(map, 7, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_INT(7, res->id);
    TEST_ASSERT_EQUAL_STRING("Maria", res->nome);
    TEST_ASSERT_EQUAL_DOUBLE(8.75, res->nota);
    free(res);
}

/* Inserir vários itens e confirmar que cada um é encontrado corretamente */
void inserir_multiplos_e_buscar_todos(void) {
    int vals[6] = {10, 20, 30, 40, 50, 60};
    for (int i = 0; i < 6; i++)
        TEST_ASSERT_TRUE(inserirHash(map, (uint64_t)(i + 1), &vals[i], sizeof(int)));

    for (int i = 0; i < 6; i++) {
        size_t s;
        int *r = procurarHash(map, (uint64_t)(i + 1), &s);
        TEST_ASSERT_NOT_NULL(r);
        TEST_ASSERT_EQUAL_INT(vals[i], *r);
        free(r);
    }
}

// ======================= Chaves duplicadas ========================

/* Segunda inserção com a mesma chave deve retornar false */
void chave_duplicada_rejeitada(void) {
    int a = 1, b = 2;
    TEST_ASSERT_TRUE (inserirHash(map, 10, &a, sizeof(int)));
    TEST_ASSERT_FALSE(inserirHash(map, 10, &b, sizeof(int)));
}

/* O valor original não deve ser substituído após rejeição de duplicata */
void duplicata_nao_sobrescreve_valor(void) {
    int original = 99, outro = 0;
    inserirHash(map, 5, &original, sizeof(int));
    inserirHash(map, 5, &outro,    sizeof(int));

    size_t s;
    int *res = procurarHash(map, 5, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_INT(99, *res);
    free(res);
}

// ==================== Remoção ==================

/* Remover um dado existente deve retornar true */
void remover_existente_retorna_true(void) {
    int v = 7;
    inserirHash(map, 1, &v, sizeof(int));
    TEST_ASSERT_TRUE(removerHash(map, 1));
}

/* Após remoção, busca deve retornar NULL */
void buscar_apos_remocao_retorna_null(void) {
    int v = 7;
    inserirHash(map, 1, &v, sizeof(int));
    removerHash(map, 1);

    size_t s;
    void *res = procurarHash(map, 1, &s);
    TEST_ASSERT_NULL(res);
}

/* Remover chave inexistente deve retornar false sem crash */
void remover_inexistente_retorna_false(void) {
    TEST_ASSERT_FALSE(removerHash(map, 999));
}

/* Segunda remoção da mesma chave deve retornar false */
void remocao_dupla_retorna_false(void) {
    int v = 3;
    inserirHash(map, 2, &v, sizeof(int));
    TEST_ASSERT_TRUE (removerHash(map, 2));
    TEST_ASSERT_FALSE(removerHash(map, 2));
}

/* Após remoção, a mesma chave deve poder ser reinserida */
void reinserir_apos_remocao(void) {
    int v1 = 11, v2 = 22;
    inserirHash(map, 3, &v1, sizeof(int));
    removerHash(map, 3);

    TEST_ASSERT_TRUE(inserirHash(map, 3, &v2, sizeof(int)));

    size_t s;
    int *res = procurarHash(map, 3, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_INT(22, *res);
    free(res);
}

/* Remover apenas uma chave não deve apagar as vizinhas */
void remover_nao_afeta_vizinhos(void) {
    int a = 1, b = 2, c = 3;
    inserirHash(map, 10, &a, sizeof(int));
    inserirHash(map, 11, &b, sizeof(int));
    inserirHash(map, 12, &c, sizeof(int));

    removerHash(map, 11);

    size_t s;
    int *ra = procurarHash(map, 10, &s); TEST_ASSERT_NOT_NULL(ra); free(ra);
    int *rc = procurarHash(map, 12, &s); TEST_ASSERT_NOT_NULL(rc); free(rc);
}

// ================= casos extremos de chave ==================
/* Chave zero deve ser aceita e recuperada normalmente */
void chave_zero_valida(void) {
    int v = 55;
    TEST_ASSERT_TRUE(inserirHash(map, 0, &v, sizeof(int)));

    size_t s;
    int *res = procurarHash(map, 0, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_INT(55, *res);
    free(res);
}

/* Chave máxima de 64 bits deve funcionar */
void chave_maxima_valida(void) {
    int v = 77;
    TEST_ASSERT_TRUE(inserirHash(map, UINT64_MAX, &v, sizeof(int)));

    size_t s;
    int *res = procurarHash(map, UINT64_MAX, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_INT(77, *res);
    free(res);
}

// =============limite de payload =============

/* Payload exatamente no limite deve ser aceito */
void payload_no_limite_aceito(void) {
    uint8_t buf[TAM_MAX_VALOR];
    memset(buf, 0xAB, sizeof(buf));

    TEST_ASSERT_TRUE(inserirHash(map, 1, buf, sizeof(buf)));

    size_t s;
    uint8_t *res = procurarHash(map, 1, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_UINT(sizeof(buf), s);
    TEST_ASSERT_EQUAL_MEMORY(buf, res, sizeof(buf));
    free(res);
}

/* Payload um byte acima do limite deve ser rejeitado */
void payload_acima_do_limite_rejeitado(void) {
    uint8_t grande[TAM_MAX_VALOR + 1];
    memset(grande, 0, sizeof(grande));

    TEST_ASSERT_FALSE(inserirHash(map, 1, grande, sizeof(grande)));

    size_t s;
    void *res = procurarHash(map, 1, &s);
    TEST_ASSERT_NULL(res);
}

 // Busca em hash vazio
 

void buscar_em_hash_vazio_retorna_null(void) {
    size_t s;
    void *res = procurarHash(map, 42, &s);
    TEST_ASSERT_NULL(res);
}

// ========== Split / crescimento dinâmico ===================
/* Inserção massiva que força vários splits; nenhum dado pode ser perdido */
void split_massivo_sem_perda(void) {
    for (uint64_t i = 0; i < 500; i++) {
        uint64_t val = i * 7;
        TEST_ASSERT_TRUE_MESSAGE(
            inserirHash(map, i, &val, sizeof(uint64_t)),
            "Falha na insercao durante split massivo"
        );
    }

    for (uint64_t i = 0; i < 500; i++) {
        size_t s;
        uint64_t *res = procurarHash(map, i, &s);
        TEST_ASSERT_NOT_NULL_MESSAGE(res, "Dado perdido apos split");
        TEST_ASSERT_EQUAL_UINT64(i * 7, *res);
        free(res);
    }
}

/* Chaves com bits baixos iguais forçam splits no mesmo nível;
   dados em ambos os lados devem sobreviver */
void split_chaves_com_mesmo_sufixo(void) {
    /* 0,4,8,12,16 compartilham os 2 bits baixos — forçam split profundo */
    uint64_t chaves[] = {0, 4, 8, 12, 16};
    for (int i = 0; i < 5; i++) {
        uint64_t v = chaves[i] + 1;
        TEST_ASSERT_TRUE(inserirHash(map, chaves[i], &v, sizeof(uint64_t)));
    }

    for (int i = 0; i < 5; i++) {
        size_t s;
        uint64_t *res = procurarHash(map, chaves[i], &s);
        TEST_ASSERT_NOT_NULL(res);
        TEST_ASSERT_EQUAL_UINT64(chaves[i] + 1, *res);
        free(res);
    }
}

// ======================= Persistência ================
/* Dado inserido antes de destruirHash deve ser encontrado após reabrir */
void dado_persiste_apos_reabrir(void) {
    int v = 33;
    inserirHash(map, 10, &v, sizeof(int));
    destruirHash(map);

    map = inicializarHash(FILENAME, TAM_BUCKET, NULL);
    TEST_ASSERT_NOT_NULL(map);

    size_t s;
    int *res = procurarHash(map, 10, &s);
    TEST_ASSERT_NOT_NULL(res);
    TEST_ASSERT_EQUAL_INT(33, *res);
    free(res);
}

/* Múltiplos dados com splits devem todos persistir após reabrir */
void persistencia_apos_splits(void) {
    for (uint64_t i = 0; i < 100; i++) {
        uint64_t v = i + 500;
        inserirHash(map, i, &v, sizeof(uint64_t));
    }
    destruirHash(map);

    map = inicializarHash(FILENAME, TAM_BUCKET, NULL);
    TEST_ASSERT_NOT_NULL(map);

    for (uint64_t i = 0; i < 100; i += 10) {
        size_t s;
        uint64_t *res = procurarHash(map, i, &s);
        TEST_ASSERT_NOT_NULL_MESSAGE(res, "Dado ausente apos reabrir com splits");
        TEST_ASSERT_EQUAL_UINT64(i + 500, *res);
        free(res);
    }
}

/* Remoção feita antes de destruir não deve reaparecer ao reabrir */
void remocao_persiste_apos_reabrir(void) {
    int v = 99;
    inserirHash(map, 7, &v, sizeof(int));
    removerHash(map, 7);
    destruirHash(map);

    map = inicializarHash(FILENAME, TAM_BUCKET, NULL);

    size_t s;
    void *res = procurarHash(map, 7, &s);
    TEST_ASSERT_NULL_MESSAGE(res, "Dado removido nao deve reaparecer apos reabrir");
}






// ================================= MAIN =============================================
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(inserir_e_buscar_inteiro);
    RUN_TEST(inserir_e_buscar_struct);
    RUN_TEST(inserir_multiplos_e_buscar_todos);

    RUN_TEST(chave_duplicada_rejeitada);
    RUN_TEST(duplicata_nao_sobrescreve_valor);

    RUN_TEST(remover_existente_retorna_true);
    RUN_TEST(buscar_apos_remocao_retorna_null);
    RUN_TEST(remover_inexistente_retorna_false);
    RUN_TEST(remocao_dupla_retorna_false);
    RUN_TEST(reinserir_apos_remocao);
    RUN_TEST(remover_nao_afeta_vizinhos);

    RUN_TEST(chave_zero_valida);
    RUN_TEST(chave_maxima_valida);

    RUN_TEST(payload_no_limite_aceito);
    RUN_TEST(payload_acima_do_limite_rejeitado);

    RUN_TEST(buscar_em_hash_vazio_retorna_null);

    RUN_TEST(split_massivo_sem_perda);
    RUN_TEST(split_chaves_com_mesmo_sufixo);

    RUN_TEST(dado_persiste_apos_reabrir);
    RUN_TEST(persistencia_apos_splits);
    RUN_TEST(remocao_persiste_apos_reabrir);

    return UNITY_END();
}

