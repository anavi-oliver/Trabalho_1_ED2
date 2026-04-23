// testa os handlers individuais (cmd*) diretamente (sem processar arquivo.qry em disco.  
//monta CtxQry com hashes em memória + SVG + TXT temporarios, chama o handler e inspeciona o resultado.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity/src/unity.h"
#include "qry.h"
#include "cidade.h"
#include "pessoas.h"
#include "svg.h"

#define TAM_BUCKET    4
#define HASH_Q_FILE   "tqry_quadras.dat"
#define HASH_P_FILE   "tqry_pessoas.dat"
#define SVG_FILE      "tqry_saida.svg"
#define TXT_FILE      "tqry_saida.txt"
#define QRY_FILE      "tqry_teste.qry"

static HashExtensivel hq, hp;
static ArqSvg         svg;
static FILE          *txt;
static CtxQry         ctx;

//helpers

/* Insere uma quadra diretamente via lerGeo a partir de uma string */
static void inserirQuadra(const char *cep,
                          double x, double y, double w, double h)
{
    char linha[256];
    snprintf(linha, sizeof(linha),
             "q %s %.1f %.1f %.1f %.1f\n", cep, x, y, w, h);
    FILE *f = fopen("tmp_q.geo", "w");
    fputs(linha, f);
    fclose(f);
    lerGeo("tmp_q.geo", hq);
    remove("tmp_q.geo");
}

static void inserirMorador(const char *cpf, const char *nome, const char *sob, char sexo, const char *cep, char face, double num){
    inserirHabitante(hp, cpf, nome, sob, sexo, "01/01/2000");
    if (cep)
        atribuirEndereco(hp, cpf, cep, face, num, "");
}

static char *lerTxt(void) {
    fflush(txt);
    FILE *f = fopen(TXT_FILE, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);
    char *buf = malloc(tam + 1);
    fread(buf, 1, tam, f);
    buf[tam] = '\0';
    fclose(f);
    return buf;
}

static int contar(const char *buf, const char *sub) {
    int n = 0;
    const char *p = buf;
    while ((p = strstr(p, sub)) != NULL) { n++; p++; }
    return n;
}

//le svg
static char *lerSvg(void) {
    fecharSvg(svg);
    FILE *f = fopen(SVG_FILE, "r");
    if (!f) { svg = abrirSvg(SVG_FILE); return NULL; }
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);
    char *buf = malloc(tam + 1);
    fread(buf, 1, tam, f);
    buf[tam] = '\0';
    fclose(f);
    svg = abrirSvg(SVG_FILE);           //reabre para próximos testes
    destruirCtxQry(ctx);
    ctx = criarCtxQry(hq, hp, svg, txt);
    return buf;
}

/*  setUp / tearDown  */

void setUp(void) {
    hq  = inicializarHash(HASH_Q_FILE, TAM_BUCKET, NULL);
    hp  = inicializarHash(HASH_P_FILE, TAM_BUCKET, NULL);
    svg = abrirSvg(SVG_FILE);
    txt = fopen(TXT_FILE, "w+");
    ctx = criarCtxQry(hq, hp, svg, txt);

    TEST_ASSERT_NOT_NULL(hq);
    TEST_ASSERT_NOT_NULL(hp);
    TEST_ASSERT_NOT_NULL(svg);
    TEST_ASSERT_NOT_NULL(txt);
    TEST_ASSERT_NOT_NULL(ctx);
}

void tearDown(void) {
    destruirCtxQry(ctx);
    fecharSvg(svg);
    fclose(txt);
    destruirHash(hq);
    destruirHash(hp);
    remove(HASH_Q_FILE);
    remove(HASH_P_FILE);
    remove(SVG_FILE);
    remove(TXT_FILE);
    remove(QRY_FILE);
}

// ============ processa qry ============
void processarQry_arquivo_inexistente(void) {
    TEST_ASSERT_EQUAL_INT(-1, processarQry("nao_existe.qry", ctx)); //inexistente retorna 1
}

void processarQry_arquivo_valido_retorna_0(void) {
    FILE *f = fopen(QRY_FILE, "w");
    fputs("censo\n", f);
    fclose(f);
    TEST_ASSERT_EQUAL_INT(0, processarQry(QRY_FILE, ctx)); //valido retorna 0
}

void processarQry_ecoa_linha_no_txt(void) {
    FILE *f = fopen(QRY_FILE, "w");
    fputs("censo\n", f);
    fclose(f);
    processarQry(QRY_FILE, ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "[*] censo"), "deve ecoar [*] censo");
    free(buf);
}

/*  cmdNasc  */
/* Insere novo habitante que pode ser encontrado depois */
void cmdNasc_insere_habitante(void) {
    cmdNasc("12345678901", "Pedro", "Lima", 'M', "10/10/1995", ctx);
    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarPessoa(hp, "12345678901", buf, NULL));
    TEST_ASSERT_EQUAL_STRING("Pedro", pessoaGetNome(buf));
}

/* cmdH  */
/* CPF inexistente reporta "nao encontrado" no TXT */
void cmdH_cpf_inexistente(void) {
    cmdH("99999999999", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "nao encontrado"),"deve reportar nao encontrado");
    free(buf);
}

/* Habitante sem endereço é reportado como sem-teto */
void cmdH_sem_teto(void) {
    inserirHabitante(hp, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    cmdH("11111111111", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "sem-teto"), "deve indicar sem-teto");
    free(buf);
}

//imprimir no txt
void cmdH_morador_imprime_endereco(void) {
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdH("11111111111", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "cep01"), "deve imprimir o CEP");
    free(buf);
}

/* cmdRip  */
//faleido -> remove 
void cmdRip_remove_pessoa(void) {
    inserirHabitante(hp, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    cmdRip("11111111111", ctx);
    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(0, buscarPessoa(hp, "11111111111", buf, NULL));
}

void cmdRip_imprime_dados(void) {
    inserirHabitante(hp, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    cmdRip("11111111111", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "Ana"), "deve imprimir o nome");
    free(buf);
}

//morte gera cruz
void cmdRip_morador_gera_cruz_svg(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdRip("11111111111", ctx);
    char *buf = lerSvg();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<line"), "deve gerar linhas da cruz");
    free(buf);
}

/* cmdRq  */
void cmdRq_remove_quadra(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    cmdRq("cep01", ctx);
    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(0, buscarQuadra(hq, "cep01", buf, NULL));
}

/* Moradores da quadra removida viram sem-teto */
void cmdRq_moradores_viram_sem_teto(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    inserirMorador("22222222222", "Bruno", "Melo", 'M', "cep01", 'S', 8.0);
    cmdRq("cep01", ctx);

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hp, "11111111111", buf, NULL);
    TEST_ASSERT_FALSE(pessoaIsMorador(buf));
    buscarPessoa(hp, "22222222222", buf, NULL);
    TEST_ASSERT_FALSE(pessoaIsMorador(buf));
}

/* CPF e nome dos afetados aparecem no TXT */
void cmdRq_imprime_afetados_no_txt(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdRq("cep01", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "11111111111"), "deve imprimir CPF");
    free(buf);
}

void cmdRq_cep_inexistente_nao_crasha(void) {
    cmdRq("nao_existe", ctx);
    TEST_PASS();
}

//quadra removida gera um X
void cmdRq_gera_x_no_svg(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    cmdRq("cep01", ctx);
    char *buf = lerSvg();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<line"), "deve gerar X no SVG");
    free(buf);
}

/* cmdPq  */
//quadra sem morador -> 0
void cmdPq_sem_moradores(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    cmdPq("cep01", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "total:0"), "total deve ser 0");
    free(buf);
}

void cmdPq_contagem_por_face(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "A", "A", 'F', "cep01", 'N', 5.0);
    inserirMorador("22222222222", "B", "B", 'M', "cep01", 'N', 8.0);
    inserirMorador("33333333333", "C", "C", 'F', "cep01", 'S', 5.0);
    cmdPq("cep01", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "N:2"), "face N deve ter 2");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "S:1"), "face S deve ter 1");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "total:3"), "total deve ser 3");
    free(buf);
}

/* cmdCenso  */

void cmdCenso_hash_vazio(void) {
    cmdCenso(ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "total habitantes: 0"),"total deve ser 0"); 
    free(buf);
}

//contagem sem teto
void cmdCenso_contagens_corretas(void) {
    inserirHabitante(hp, "11111111111", "A", "A", 'F', "01/01/2000");
    inserirHabitante(hp, "22222222222", "B", "B", 'M', "01/01/2000");
    inserirHabitante(hp, "33333333333", "C", "C", 'F', "01/01/2000");
    atribuirEndereco(hp, "11111111111", "cep01", 'N', 5.0, "");

    cmdCenso(ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "total habitantes: 3"), "total 3");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "moradores: 1"),        "1 morador");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "sem-teto: 2"),         "2 sem-teto");
    free(buf);
}

/* cmdMud  */
void cmdMud_atualiza_endereco(void) {
    inserirQuadra("cep02", 50.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdMud("11111111111", "cep02", 'S', 10.0, "ap2", ctx);

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hp, "11111111111", buf, NULL);
    TEST_ASSERT_EQUAL_STRING("cep02", pessoaGetCep(buf));
    TEST_ASSERT_EQUAL_INT('S', pessoaGetFace(buf));
}

//mudanca gera quadrado
void cmdMud_gera_quadrado_svg(void) {
    inserirQuadra("cep02", 50.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdMud("11111111111", "cep02", 'S', 10.0, "ap2", ctx);
    char *buf = lerSvg();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<rect"), "deve gerar quadrado");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "11111111111"), "CPF no quadrado");
    free(buf);
}

/* cmdDspj  */
void cmdDspj_vira_sem_teto(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdDspj("11111111111", ctx);

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hp, "11111111111", buf, NULL);
    TEST_ASSERT_FALSE(pessoaIsMorador(buf));
}

void cmdDspj_imprime_dados(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdDspj("11111111111", ctx);
    char *buf = lerTxt();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "cep01"), "deve imprimir CEP do despejo");
    free(buf);
}

//despejado vira sem teto
void cmdDspj_gera_circulo_svg(void) {
    inserirQuadra("cep01", 0.0, 50.0, 40.0, 20.0);
    inserirMorador("11111111111", "Ana", "Silva", 'F', "cep01", 'N', 5.0);
    cmdDspj("11111111111", ctx);
    char *buf = lerSvg();
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<circle"), "deve gerar circulo");
    free(buf);
}

void cmdDspj_sem_teto_nao_crasha(void) {
    inserirHabitante(hp, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    cmdDspj("11111111111", ctx);
    TEST_PASS();
}

void cmdDspj_inexistente_nao_crasha(void) {
    cmdDspj("99999999999", ctx);
    TEST_PASS();
}




// ================================= MAIN =============================================
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(processarQry_arquivo_inexistente);
    RUN_TEST(processarQry_arquivo_valido_retorna_0);
    RUN_TEST(processarQry_ecoa_linha_no_txt);

    RUN_TEST(cmdNasc_insere_habitante);

    RUN_TEST(cmdH_cpf_inexistente);
    RUN_TEST(cmdH_sem_teto);
    RUN_TEST(cmdH_morador_imprime_endereco);

    RUN_TEST(cmdRip_remove_pessoa);
    RUN_TEST(cmdRip_imprime_dados);
    RUN_TEST(cmdRip_morador_gera_cruz_svg);

    RUN_TEST(cmdRq_remove_quadra);
    RUN_TEST(cmdRq_moradores_viram_sem_teto);
    RUN_TEST(cmdRq_imprime_afetados_no_txt);
    RUN_TEST(cmdRq_cep_inexistente_nao_crasha);
    RUN_TEST(cmdRq_gera_x_no_svg);

    RUN_TEST(cmdPq_sem_moradores);
    RUN_TEST(cmdPq_contagem_por_face);

    RUN_TEST(cmdCenso_hash_vazio);
    RUN_TEST(cmdCenso_contagens_corretas);

    RUN_TEST(cmdMud_atualiza_endereco);
    RUN_TEST(cmdMud_gera_quadrado_svg);

    RUN_TEST(cmdDspj_vira_sem_teto);
    RUN_TEST(cmdDspj_imprime_dados);
    RUN_TEST(cmdDspj_gera_circulo_svg);
    RUN_TEST(cmdDspj_sem_teto_nao_crasha);
    RUN_TEST(cmdDspj_inexistente_nao_crasha);

    return UNITY_END();
}