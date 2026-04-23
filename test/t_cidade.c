#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "unity/src/unity.h"
#include "cidade.h"
#include "svg.h"

#define TAM_BUCKET  4
#define HASH_FILE   "test_cidade.dat"
#define GEO_FILE    "test_cidade.geo"
#define SVG_FILE    "test_cidade.svg"

static HashExtensivel hash;

void setUp(void) {
    hash = inicializarHash(HASH_FILE, TAM_BUCKET, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(hash, "inicializarHash nao deve retornar NULL");
}

void tearDown(void) {
    destruirHash(hash);
    remove(HASH_FILE);
    remove(GEO_FILE);
    remove(SVG_FILE);
}

/* Escreve um .geo mínimo em disco */
static void escreverGeo(const char *conteudo) {
    FILE *f = fopen(GEO_FILE, "w");
    fputs(conteudo, f);
    fclose(f);
}

/* ── cepParaChave ────────────────────────────────────────────────────────── */

/* Mesmo CEP sempre gera a mesma chave */
void cep_mesma_chave(void) {
    uint64_t a = cepParaChave("cep01");
    uint64_t b = cepParaChave("cep01");
    TEST_ASSERT_EQUAL_UINT64(a, b);
}

/* CEPs diferentes devem gerar chaves diferentes */
void ceps_diferentes_chaves_diferentes(void) {
    uint64_t a = cepParaChave("cep01");
    uint64_t b = cepParaChave("cep02");
    TEST_ASSERT_NOT_EQUAL(a, b);
}

/* CEP vazio não pode crashar */
void cep_vazio_nao_crasha(void) {
    uint64_t k = cepParaChave("");
    (void)k; /* só verifica que não segfaultou */
    TEST_PASS();
}

/* ── lerGeo ──────────────────────────────────────────────────────────────── */

/* Arquivo inexistente retorna -1 */
void lerGeo_arquivo_inexistente(void) {
    int r = lerGeo("nao_existe.geo", hash);
    TEST_ASSERT_EQUAL_INT(-1, r);
}

/* Quadra simples é inserida e encontrada */
void lerGeo_insere_quadra(void) {
    escreverGeo("q cep01 10.0 50.0 30.0 20.0\n");
    int r = lerGeo(GEO_FILE, hash);
    TEST_ASSERT_EQUAL_INT(1, r);

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep01", buf, NULL));
}

/* Múltiplas quadras são todas inseridas */
void lerGeo_insere_multiplas_quadras(void) {
    escreverGeo(
        "q cep01 10.0 50.0 30.0 20.0\n"
        "q cep02 60.0 50.0 30.0 20.0\n"
        "q cep03 110.0 50.0 30.0 20.0\n"
    );
    int r = lerGeo(GEO_FILE, hash);
    TEST_ASSERT_EQUAL_INT(3, r);

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep01", buf, NULL));
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep02", buf, NULL));
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep03", buf, NULL));
}

/* Comando cq altera cores das quadras seguintes */
void lerGeo_cq_altera_cores(void) {
    escreverGeo(
        "cq 2.0 cyan yellow\n"
        "q cep10 10.0 50.0 30.0 20.0\n"
    );
    lerGeo(GEO_FILE, hash);

    uint8_t buf[TAM_MAX_VALOR];
    buscarQuadra(hash, "cep10", buf, NULL);
    TEST_ASSERT_EQUAL_STRING("cyan",   quadraGetCorFill(buf));
    TEST_ASSERT_EQUAL_STRING("yellow", quadraGetCorStroke(buf));
}

/* Linhas com comando desconhecido são ignoradas sem erro */
void lerGeo_ignora_comandos_desconhecidos(void) {
    escreverGeo(
        "# comentario\n"
        "q cep01 10.0 50.0 30.0 20.0\n"
        "x ignorar isso aqui\n"
    );
    int r = lerGeo(GEO_FILE, hash);
    TEST_ASSERT_EQUAL_INT(1, r);
}

/* ── buscarQuadra ─────────────────────────────────────────────────────────── */

/* Busca em hash vazio retorna 0 */
void buscar_em_hash_vazio(void) {
    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(0, buscarQuadra(hash, "cep99", buf, NULL));
}

/* Busca retorna os campos corretos */
void buscar_campos_corretos(void) {
    escreverGeo("q cep05 15.0 60.0 40.0 25.0\n");
    lerGeo(GEO_FILE, hash);

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep05", buf, NULL));
    TEST_ASSERT_EQUAL_STRING("cep05", quadraGetCep(buf));
    TEST_ASSERT_EQUAL_DOUBLE(15.0, quadraGetX(buf));
    TEST_ASSERT_EQUAL_DOUBLE(60.0, quadraGetY(buf));
    TEST_ASSERT_EQUAL_DOUBLE(40.0, quadraGetW(buf));
    TEST_ASSERT_EQUAL_DOUBLE(25.0, quadraGetH(buf));
}

/* outTam é preenchido corretamente */
void buscar_preenche_outTam(void) {
    escreverGeo("q cep06 0.0 0.0 10.0 10.0\n");
    lerGeo(GEO_FILE, hash);

    uint8_t buf[TAM_MAX_VALOR];
    size_t tam = 0;
    buscarQuadra(hash, "cep06", buf, &tam);
    TEST_ASSERT_EQUAL_UINT(tamQuadra(), tam);
}

/* ── removerQuadra ───────────────────────────────────────────────────────── */

/* Remover existente retorna 1 */
void remover_existente_retorna_1(void) {
    escreverGeo("q cepA 0.0 0.0 10.0 10.0\n");
    lerGeo(GEO_FILE, hash);
    TEST_ASSERT_EQUAL_INT(1, removerQuadra(hash, "cepA"));
}

/* Após remoção, busca retorna 0 */
void buscar_apos_remover_retorna_0(void) {
    escreverGeo("q cepB 0.0 0.0 10.0 10.0\n");
    lerGeo(GEO_FILE, hash);
    removerQuadra(hash, "cepB");

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(0, buscarQuadra(hash, "cepB", buf, NULL));
}

/* Remover inexistente retorna 0 */
void remover_inexistente_retorna_0(void) {
    TEST_ASSERT_EQUAL_INT(0, removerQuadra(hash, "naoexiste"));
}

/* Remover uma não afeta as outras */
void remover_nao_afeta_vizinhas(void) {
    escreverGeo(
        "q cep1 0.0  0.0 10.0 10.0\n"
        "q cep2 20.0 0.0 10.0 10.0\n"
        "q cep3 40.0 0.0 10.0 10.0\n"
    );
    lerGeo(GEO_FILE, hash);
    removerQuadra(hash, "cep2");

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep1", buf, NULL));
    TEST_ASSERT_EQUAL_INT(1, buscarQuadra(hash, "cep3", buf, NULL));
}

/* ── desenharQuadras ─────────────────────────────────────────────────────── */

/* Não crasha com hash vazio */
void desenhar_hash_vazio_nao_crasha(void) {
    ArqSvg svg = abrirSvg(SVG_FILE);
    TEST_ASSERT_NOT_NULL(svg);
    desenharQuadras(hash, svg);
    fecharSvg(svg);
    TEST_PASS();
}

/* SVG resultante deve conter uma <rect por quadra */
void desenhar_gera_rects(void) {
    escreverGeo(
        "q cepX 0.0  0.0 10.0 10.0\n"
        "q cepY 20.0 0.0 10.0 10.0\n"
    );
    lerGeo(GEO_FILE, hash);

    ArqSvg svg = abrirSvg(SVG_FILE);
    desenharQuadras(hash, svg);
    fecharSvg(svg);

    FILE *f = fopen(SVG_FILE, "r");
    TEST_ASSERT_NOT_NULL(f);
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);

    int cnt = 0;
    const char *p = buf;
    while ((p = strstr(p, "<rect")) != NULL) { cnt++; p++; }
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, cnt, "deve haver 1 <rect por quadra");
}

// ================================= MAIN =============================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(cep_mesma_chave);
    RUN_TEST(ceps_diferentes_chaves_diferentes);
    RUN_TEST(cep_vazio_nao_crasha);

    RUN_TEST(lerGeo_arquivo_inexistente);
    RUN_TEST(lerGeo_insere_quadra);
    RUN_TEST(lerGeo_insere_multiplas_quadras);
    RUN_TEST(lerGeo_cq_altera_cores);
    RUN_TEST(lerGeo_ignora_comandos_desconhecidos);

    RUN_TEST(buscar_em_hash_vazio);
    RUN_TEST(buscar_campos_corretos);
    RUN_TEST(buscar_preenche_outTam);

    RUN_TEST(remover_existente_retorna_1);
    RUN_TEST(buscar_apos_remover_retorna_0);
    RUN_TEST(remover_inexistente_retorna_0);
    RUN_TEST(remover_nao_afeta_vizinhas);

    RUN_TEST(desenhar_hash_vazio_nao_crasha);
    RUN_TEST(desenhar_gera_rects);

    return UNITY_END();
}
