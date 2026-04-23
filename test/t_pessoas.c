#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "unity/src/unity.h"
#include "pessoas.h"


#define TAM_BUCKET  4
#define HASH_FILE   "test_pessoas.dat"
#define PM_FILE     "test_pessoas.pm"

static HashExtensivel hash;

void setUp(void) {
    hash = inicializarHash(HASH_FILE, TAM_BUCKET, NULL);
    TEST_ASSERT_NOT_NULL_MESSAGE(hash, "inicializarHash nao deve retornar NULL");
}

void tearDown(void) {
    destruirHash(hash);
    remove(HASH_FILE);
    remove(PM_FILE);
}

static void escreverPm(const char *conteudo) {
    FILE *f = fopen(PM_FILE, "w");
    fputs(conteudo, f);
    fclose(f);
}

/* ── cpfParaChave ────────────────────────────────────────────────────────── */

/* Mesmo CPF sempre gera a mesma chave */
void cpf_mesma_chave(void) {
    uint64_t a = cpfParaChave("123.456.789-09");
    uint64_t b = cpfParaChave("123.456.789-09");
    TEST_ASSERT_EQUAL_UINT64(a, b);
}

/* CPF com e sem formatação geram a mesma chave */
void cpf_com_sem_pontuacao_igual(void) {
    uint64_t a = cpfParaChave("12345678909");
    uint64_t b = cpfParaChave("123.456.789-09");
    TEST_ASSERT_EQUAL_UINT64(a, b);
}

/* CPFs diferentes geram chaves diferentes */
void cpfs_diferentes_chaves_diferentes(void) {
    uint64_t a = cpfParaChave("11111111111");
    uint64_t b = cpfParaChave("22222222222");
    TEST_ASSERT_NOT_EQUAL(a, b);
}

/* ── lerPm ───────────────────────────────────────────────────────────────── */

/* Arquivo inexistente retorna -1 */
void lerPm_arquivo_inexistente(void) {
    TEST_ASSERT_EQUAL_INT(-1, lerPm("nao_existe.pm", hash));
}

/* Habitante inserido via 'p' é encontrado */
void lerPm_insere_habitante(void) {
    escreverPm("p 11111111111 Ana Silva F 01/01/1990\n");
    TEST_ASSERT_EQUAL_INT(1, lerPm(PM_FILE, hash));

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarPessoa(hash, "11111111111", buf, NULL));
}

/* Múltiplos habitantes são inseridos */
void lerPm_insere_multiplos(void) {
    escreverPm(
        "p 11111111111 Ana Silva F 01/01/1990\n"
        "p 22222222222 Bruno Melo M 15/06/1985\n"
        "p 33333333333 Carla Dias F 20/03/2000\n"
    );
    TEST_ASSERT_EQUAL_INT(3, lerPm(PM_FILE, hash));
}

/* Comando 'm' torna habitante morador */
void lerPm_m_torna_morador(void) {
    escreverPm(
        "p 11111111111 Ana Silva F 01/01/1990\n"
        "m 11111111111 cep01 N 10.0 apto1\n"
    );
    lerPm(PM_FILE, hash);

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hash, "11111111111", buf, NULL);
    TEST_ASSERT_TRUE(pessoaIsMorador(buf));
    TEST_ASSERT_EQUAL_STRING("cep01", pessoaGetCep(buf));
    TEST_ASSERT_EQUAL_STRING("apto1", pessoaGetCompl(buf));
}

/* ── inserirHabitante ────────────────────────────────────────────────────── */

/* Inserção bem-sucedida retorna true */
void inserir_habitante_retorna_true(void) {
    TEST_ASSERT_TRUE(
        inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990")
    );
}

/* CPF duplicado é rejeitado */
void inserir_cpf_duplicado_rejeitado(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    TEST_ASSERT_FALSE(
        inserirHabitante(hash, "11111111111", "Outro", "Nome", 'M', "01/01/2000")
    );
}

/* Habitante inserido começa como sem-teto */
void habitante_novo_nao_e_morador(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hash, "11111111111", buf, NULL);
    TEST_ASSERT_FALSE(pessoaIsMorador(buf));
}

/* ── buscarPessoa ─────────────────────────────────────────────────────────── */

/* Busca em hash vazio retorna 0 */
void buscar_em_hash_vazio(void) {
    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(0, buscarPessoa(hash, "99999999999", buf, NULL));
}

/* Campos retornados estão corretos */
void buscar_campos_corretos(void) {
    inserirHabitante(hash, "12345678909", "Bruno", "Melo", 'M', "15/06/1985");

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarPessoa(hash, "12345678909", buf, NULL));
    TEST_ASSERT_EQUAL_STRING("12345678909", pessoaGetCpf(buf));
    TEST_ASSERT_EQUAL_STRING("Bruno",       pessoaGetNome(buf));
    TEST_ASSERT_EQUAL_STRING("Melo",        pessoaGetSobrenome(buf));
    TEST_ASSERT_EQUAL_INT('M',              pessoaGetSexo(buf));
    TEST_ASSERT_EQUAL_STRING("15/06/1985",  pessoaGetNasc(buf));
}

/* outTam é preenchido com tamPessoa() */
void buscar_preenche_outTam(void) {
    inserirHabitante(hash, "11111111111", "X", "Y", 'F', "01/01/2000");
    uint8_t buf[TAM_MAX_VALOR];
    size_t tam = 0;
    buscarPessoa(hash, "11111111111", buf, &tam);
    TEST_ASSERT_EQUAL_UINT(tamPessoa(), tam);
}

/* ── atribuirEndereco / removerEndereco ──────────────────────────────────── */

/* atribuirEndereco torna habitante morador com os dados corretos */
void atribuir_torna_morador(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    TEST_ASSERT_TRUE(
        atribuirEndereco(hash, "11111111111", "cep05", 'S', 20.0, "casa3")
    );

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hash, "11111111111", buf, NULL);
    TEST_ASSERT_TRUE(pessoaIsMorador(buf));
    TEST_ASSERT_EQUAL_STRING("cep05", pessoaGetCep(buf));
    TEST_ASSERT_EQUAL_INT('S',        pessoaGetFace(buf));
    TEST_ASSERT_EQUAL_DOUBLE(20.0,    pessoaGetNum(buf));
    TEST_ASSERT_EQUAL_STRING("casa3", pessoaGetCompl(buf));
}

/* atribuirEndereco em CPF inexistente retorna false */
void atribuir_cpf_inexistente_retorna_false(void) {
    TEST_ASSERT_FALSE(
        atribuirEndereco(hash, "99999999999", "cep01", 'N', 5.0, "")
    );
}

/* atribuirEndereco substitui endereço anterior */
void atribuir_substitui_endereco(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    atribuirEndereco(hash, "11111111111", "cep01", 'N', 5.0, "ap1");
    atribuirEndereco(hash, "11111111111", "cep99", 'L', 30.0, "ap2");

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hash, "11111111111", buf, NULL);
    TEST_ASSERT_EQUAL_STRING("cep99", pessoaGetCep(buf));
}

/* removerEndereco torna morador sem-teto */
void remover_endereco_vira_sem_teto(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    atribuirEndereco(hash, "11111111111", "cep01", 'N', 5.0, "");
    TEST_ASSERT_TRUE(removerEndereco(hash, "11111111111"));

    uint8_t buf[TAM_MAX_VALOR];
    buscarPessoa(hash, "11111111111", buf, NULL);
    TEST_ASSERT_FALSE(pessoaIsMorador(buf));
}

/* removerEndereco em CPF inexistente retorna false */
void remover_endereco_inexistente_retorna_false(void) {
    TEST_ASSERT_FALSE(removerEndereco(hash, "99999999999"));
}

/* ── removerPessoa ───────────────────────────────────────────────────────── */

/* Remover existente retorna true */
void remover_pessoa_existente(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    TEST_ASSERT_TRUE(removerPessoa(hash, "11111111111"));
}

/* Após remoção, busca retorna 0 */
void buscar_apos_remover_retorna_0(void) {
    inserirHabitante(hash, "11111111111", "Ana", "Silva", 'F', "01/01/1990");
    removerPessoa(hash, "11111111111");
    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(0, buscarPessoa(hash, "11111111111", buf, NULL));
}

/* Remover inexistente retorna false */
void remover_pessoa_inexistente(void) {
    TEST_ASSERT_FALSE(removerPessoa(hash, "99999999999"));
}

/* Remover uma não afeta as outras */
void remover_nao_afeta_outros(void) {
    inserirHabitante(hash, "11111111111", "A", "A", 'F', "01/01/2000");
    inserirHabitante(hash, "22222222222", "B", "B", 'M', "01/01/2000");
    inserirHabitante(hash, "33333333333", "C", "C", 'F', "01/01/2000");
    removerPessoa(hash, "22222222222");

    uint8_t buf[TAM_MAX_VALOR];
    TEST_ASSERT_EQUAL_INT(1, buscarPessoa(hash, "11111111111", buf, NULL));
    TEST_ASSERT_EQUAL_INT(1, buscarPessoa(hash, "33333333333", buf, NULL));
}

/* ── contarMoradoresPorFace ──────────────────────────────────────────────── */

/* Hash vazio → total 0 */
void contar_hash_vazio(void) {
    int nN, nS, nL, nO;
    int total = contarMoradoresPorFace(hash, "cep01", &nN, &nS, &nL, &nO);
    TEST_ASSERT_EQUAL_INT(0, total);
    TEST_ASSERT_EQUAL_INT(0, nN);
}

/* Contagem por face está correta */
void contar_por_face_correto(void) {
    inserirHabitante(hash, "11111111111", "A", "A", 'F', "01/01/2000");
    inserirHabitante(hash, "22222222222", "B", "B", 'M', "01/01/2000");
    inserirHabitante(hash, "33333333333", "C", "C", 'F', "01/01/2000");
    inserirHabitante(hash, "44444444444", "D", "D", 'M', "01/01/2000");

    atribuirEndereco(hash, "11111111111", "cep01", 'N', 5.0,  "");
    atribuirEndereco(hash, "22222222222", "cep01", 'N', 10.0, "");
    atribuirEndereco(hash, "33333333333", "cep01", 'S', 5.0,  "");
    atribuirEndereco(hash, "44444444444", "cep02", 'N', 5.0,  ""); /* outra quadra */

    int nN, nS, nL, nO;
    int total = contarMoradoresPorFace(hash, "cep01", &nN, &nS, &nL, &nO);
    TEST_ASSERT_EQUAL_INT(3, total);
    TEST_ASSERT_EQUAL_INT(2, nN);
    TEST_ASSERT_EQUAL_INT(1, nS);
    TEST_ASSERT_EQUAL_INT(0, nL);
    TEST_ASSERT_EQUAL_INT(0, nO);
}

/* Sem-tetos não são contados */
void contar_sem_teto_nao_conta(void) {
    inserirHabitante(hash, "11111111111", "A", "A", 'F', "01/01/2000");
    /* sem atribuirEndereco → sem-teto */
    int total = contarMoradoresPorFace(hash, "cep01", NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(0, total);
}

// ================================= MAIN =============================================
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(cpf_mesma_chave);
    RUN_TEST(cpf_com_sem_pontuacao_igual);
    RUN_TEST(cpfs_diferentes_chaves_diferentes);

    RUN_TEST(lerPm_arquivo_inexistente);
    RUN_TEST(lerPm_insere_habitante);
    RUN_TEST(lerPm_insere_multiplos);
    RUN_TEST(lerPm_m_torna_morador);

    RUN_TEST(inserir_habitante_retorna_true);
    RUN_TEST(inserir_cpf_duplicado_rejeitado);
    RUN_TEST(habitante_novo_nao_e_morador);

    RUN_TEST(buscar_em_hash_vazio);
    RUN_TEST(buscar_campos_corretos);
    RUN_TEST(buscar_preenche_outTam);

    RUN_TEST(atribuir_torna_morador);
    RUN_TEST(atribuir_cpf_inexistente_retorna_false);
    RUN_TEST(atribuir_substitui_endereco);
    RUN_TEST(remover_endereco_vira_sem_teto);
    RUN_TEST(remover_endereco_inexistente_retorna_false);

    RUN_TEST(remover_pessoa_existente);
    RUN_TEST(buscar_apos_remover_retorna_0);
    RUN_TEST(remover_pessoa_inexistente);
    RUN_TEST(remover_nao_afeta_outros);

    RUN_TEST(contar_hash_vazio);
    RUN_TEST(contar_por_face_correto);
    RUN_TEST(contar_sem_teto_nao_conta);

    return UNITY_END();
}