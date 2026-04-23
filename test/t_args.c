#include <stdlib.h>
#include <string.h>

#include "unity/src/unity.h"
#include "args.h"

void setUp(void)    {}
void tearDown(void) {}

/* Monta um argv[] a partir de strings literais */
#define ARGC(...) \
    (int)(sizeof((char *[]){__VA_ARGS__}) / sizeof(char *))
#define ARGV(...) \
    (char *[]){__VA_ARGS__}

/*parsearArgs: casos de falha  */
void sem_f_retorna_null(void) {
    char *argv[] = { "ted", "-o", "./saida" };
    Args a = parsearArgs(3, argv);
    TEST_ASSERT_NULL(a);
}

void sem_o_retorna_null(void) { //sem -o
    char *argv[] = { "ted", "-f", "mapa.geo" };
    Args a = parsearArgs(3, argv);
    TEST_ASSERT_NULL(a);
}

void apenas_nome_programa_retorna_null(void) { //argc ==1
    char *argv[] = { "ted" };
    Args a = parsearArgs(1, argv);
    TEST_ASSERT_NULL(a);
}

/* parsearArgs: caso mínimo obrigatório  */
void minimo_f_e_o_funciona(void) {
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_NOT_NULL(a);
    destruirArgs(a);
}

/* argsGetDirEntrada  */
void sem_e_dir_entrada_e_ponto(void) { //sem -e
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_EQUAL_STRING(".", argsGetDirEntrada(a));
    destruirArgs(a);
}

//-e
void com_e_dir_entrada_correto(void) {
    char *argv[] = { "ted", "-e", "/home/ed/testes", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(7, argv);
    TEST_ASSERT_EQUAL_STRING("/home/ed/testes", argsGetDirEntrada(a));
    destruirArgs(a);
}

void barra_final_em_e_removida(void) {
    char *argv[] = { "ted", "-e", "/home/ed/", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(7, argv);
    TEST_ASSERT_EQUAL_STRING("/home/ed", argsGetDirEntrada(a));
    destruirArgs(a);
}

/* argsGetDirSaida  -o*/
void barra_final_em_o_removida(void) {
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "/saida/" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_EQUAL_STRING("/saida", argsGetDirSaida(a));
    destruirArgs(a);
}

void dir_saida_sem_barra_preservado(void) {
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "/saida" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_EQUAL_STRING("/saida", argsGetDirSaida(a));
    destruirArgs(a);
}

/* argsGetGeoPath  */
/* Sem -e, geoPath = "./<arq.geo>" */
void geoPath_sem_e(void) {
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_EQUAL_STRING("./mapa.geo", argsGetGeoPath(a));
    destruirArgs(a);
}

/* Com -e, geoPath = "<BED>/<arq.geo>" */
void geoPath_com_e(void) {
    char *argv[] = { "ted", "-e", "/tst", "-f", "t001.geo", "-o", "./saida" };
    Args a = parsearArgs(7, argv);
    TEST_ASSERT_EQUAL_STRING("/tst/t001.geo", argsGetGeoPath(a));
    destruirArgs(a);
}

/* argsGetQryPath  */
void sem_q_qryPath_null(void) { //retorna NULL
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_NULL(argsGetQryPath(a));
    destruirArgs(a);
}
void com_q_qryPath_correto(void) {
    char *argv[] = { "ted", "-e", "/tst", "-f", "t001.geo",
                     "-o", "./saida", "-q", "q1.qry" };
    Args a = parsearArgs(9, argv);
    TEST_ASSERT_EQUAL_STRING("/tst/q1.qry", argsGetQryPath(a));
    destruirArgs(a);
}

/* argsGetPmPath  */
void sem_pm_pmPath_null(void) {//retorna NULL
    char *argv[] = { "ted", "-f", "mapa.geo", "-o", "./saida" };
    Args a = parsearArgs(5, argv);
    TEST_ASSERT_NULL(argsGetPmPath(a));
    destruirArgs(a);
}

void com_pm_pmPath_correto(void) {
    char *argv[] = { "ted", "-e", "/tst", "-f", "t001.geo",
                     "-o", "./saida", "-pm", "pessoas.pm" };
    Args a = parsearArgs(9, argv);
    TEST_ASSERT_EQUAL_STRING("/tst/pessoas.pm", argsGetPmPath(a));
    destruirArgs(a);
}

/* argsBaseSaida  */
/* Sem -q: base = stem do .geo */
void base_saida_sem_q(void) { 
    char *argv[] = { "ted", "-f", "t001.geo", "-o", "./saida" };
    Args a = parsearArgs(5, argv);
    char out[256];
    argsBaseSaida(a, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("t001", out);
    destruirArgs(a);
}

/* Com -q: base = <stem geo>-<stem qry> */
void base_saida_com_q(void) {
    char *argv[] = { "ted", "-f", "t001.geo", "-o", "./saida", "-q", "q1.qry" };
    Args a = parsearArgs(7, argv);
    char out[256];
    argsBaseSaida(a, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("t001-q1", out);
    destruirArgs(a);
}

//ordem dos parâmetros é indiferente 
/* -o antes de -f deve funcionar igualmente */
void ordem_parametros_indiferente(void) {
    char *argv[] = { "ted", "-o", "./saida", "-q", "q1.qry",
                     "-e", "/tst", "-f", "t001.geo" };
    Args a = parsearArgs(9, argv);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL_STRING("/tst/t001.geo", argsGetGeoPath(a));
    TEST_ASSERT_EQUAL_STRING("/tst/q1.qry",  argsGetQryPath(a));
    destruirArgs(a);
}



// ================================= MAIN =============================================
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(sem_f_retorna_null);
    RUN_TEST(sem_o_retorna_null);
    RUN_TEST(apenas_nome_programa_retorna_null);

    RUN_TEST(minimo_f_e_o_funciona);

    RUN_TEST(sem_e_dir_entrada_e_ponto);
    RUN_TEST(com_e_dir_entrada_correto);
    RUN_TEST(barra_final_em_e_removida);

    RUN_TEST(barra_final_em_o_removida);
    RUN_TEST(dir_saida_sem_barra_preservado);

    RUN_TEST(geoPath_sem_e);
    RUN_TEST(geoPath_com_e);

    RUN_TEST(sem_q_qryPath_null);
    RUN_TEST(com_q_qryPath_correto);

    RUN_TEST(sem_pm_pmPath_null);
    RUN_TEST(com_pm_pmPath_correto);

    RUN_TEST(base_saida_sem_q);
    RUN_TEST(base_saida_com_q);

    RUN_TEST(ordem_parametros_indiferente);

    return UNITY_END();
}