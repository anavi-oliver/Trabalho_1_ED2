

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity/src/unity.h"
#include "svg.h"

#define SVG_FILE "test_saida.svg"

static ArqSvg svg;

void setUp(void) {
    svg = abrirSvg(SVG_FILE);
    TEST_ASSERT_NOT_NULL_MESSAGE(svg, "abrirSvg nao deve retornar NULL");
}

void tearDown(void) {
    fecharSvg(svg);
    remove(SVG_FILE);
}

/* Lê o conteúdo do SVG como string para inspeção */
static char *lerArquivo(void) {
    FILE *f = fopen(SVG_FILE, "r");
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

/* ── ciclo de vida ───────────────────────────────────────────────────────── */

/* abrirSvg em caminho inválido deve retornar NULL */
void abrir_caminho_invalido_retorna_null(void) {
    ArqSvg s = abrirSvg("/diretorio/nao/existe/arq.svg");
    TEST_ASSERT_NULL(s);
}

/* Arquivo criado deve conter a tag raiz <svg */
void arquivo_contem_tag_svg(void) {
    fecharSvg(svg);
    svg = NULL;          /* evita duplo fechar no tearDown */

    svg = abrirSvg(SVG_FILE);
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<svg"), "arquivo deve conter <svg");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "</svg>"), "arquivo deve conter </svg>");
    free(buf);

    /* Reabre para o tearDown fechar normalmente */
    svg = abrirSvg(SVG_FILE);
}

/* ── formas básicas ──────────────────────────────────────────────────────── */

/* svgQuadra deve emitir uma tag <rect */
void quadra_emite_rect(void) {
    svgQuadra(svg, 10.0, 50.0, 30.0, 20.0, 1.0, "white", "black");
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<rect"), "svgQuadra deve emitir <rect");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* svgQuadra: coordenada y do rect deve ser y-h (âncora sudeste) */
void quadra_y_corrigido_para_sudeste(void) {
    /* âncora y=50, h=20 → rect y deve ser 30.00 */
    svgQuadra(svg, 10.0, 50.0, 30.0, 20.0, 1.0, "white", "black");
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "y=\"30.00\""),
                                 "rect y deve ser ancoragem - altura (50-20=30)");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* svgTexto deve emitir uma tag <text */
void texto_emite_text(void) {
    svgTexto(svg, 5.0, 15.0, "red", 8.0, "ola");
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<text"), "svgTexto deve emitir <text");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "ola"),   "conteudo do texto deve aparecer");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* ── marcações ───────────────────────────────────────────────────────────── */

/* svgMarcaX deve emitir duas linhas */
void marcaX_emite_duas_linhas(void) {
    svgMarcaX(svg, 20.0, 30.0);
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    /* conta ocorrências de <line */
    int cnt = 0;
    const char *p = buf;
    while ((p = strstr(p, "<line")) != NULL) { cnt++; p++; }
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, cnt, "svgMarcaX deve emitir exatamente 2 linhas");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* svgMarcaCruz deve emitir duas linhas */
void marcaCruz_emite_duas_linhas(void) {
    svgMarcaCruz(svg, 20.0, 30.0);
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    int cnt = 0;
    const char *p = buf;
    while ((p = strstr(p, "<line")) != NULL) { cnt++; p++; }
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, cnt, "svgMarcaCruz deve emitir exatamente 2 linhas");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* svgMarcaQuadrado deve conter o CPF passado */
void marcaQuadrado_contem_cpf(void) {
    svgMarcaQuadrado(svg, 10.0, 10.0, "12345678901");
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "12345678901"),
                                 "svgMarcaQuadrado deve inscrever o CPF");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* svgMarcaCirculo deve emitir um <circle */
void marcaCirculo_emite_circle(void) {
    svgMarcaCirculo(svg, 15.0, 25.0);
    fecharSvg(svg);
    svg = NULL;

    char *buf = lerArquivo();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, "<circle"),
                                 "svgMarcaCirculo deve emitir <circle");
    free(buf);

    svg = abrirSvg(SVG_FILE);
}

/* ── svgPosEndereco ──────────────────────────────────────────────────────── */

/* Face N: cy deve ser ay-h */
void posEndereco_face_N(void) {
    double cx, cy;
    svgPosEndereco(10.0, 50.0, 30.0, 20.0, 'N', 5.0, &cx, &cy);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, cx); /* ax + num */
    TEST_ASSERT_EQUAL_DOUBLE(30.0, cy); /* ay - h   */
}

/* Face S: cy deve ser ay */
void posEndereco_face_S(void) {
    double cx, cy;
    svgPosEndereco(10.0, 50.0, 30.0, 20.0, 'S', 5.0, &cx, &cy);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, cx);
    TEST_ASSERT_EQUAL_DOUBLE(50.0, cy);
}

/* Face L: cx deve ser ax+w */
void posEndereco_face_L(void) {
    double cx, cy;
    svgPosEndereco(10.0, 50.0, 30.0, 20.0, 'L', 8.0, &cx, &cy);
    TEST_ASSERT_EQUAL_DOUBLE(40.0, cx); /* ax + w    */
    TEST_ASSERT_EQUAL_DOUBLE(42.0, cy); /* ay - num  */
}

/* Face O: cx deve ser ax */
void posEndereco_face_O(void) {
    double cx, cy;
    svgPosEndereco(10.0, 50.0, 30.0, 20.0, 'O', 8.0, &cx, &cy);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, cx);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, cy);
}





// ================================= MAIN =============================================
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(abrir_caminho_invalido_retorna_null);
    RUN_TEST(arquivo_contem_tag_svg);

    RUN_TEST(quadra_emite_rect);
    RUN_TEST(quadra_y_corrigido_para_sudeste);
    RUN_TEST(texto_emite_text);

    RUN_TEST(marcaX_emite_duas_linhas);
    RUN_TEST(marcaCruz_emite_duas_linhas);
    RUN_TEST(marcaQuadrado_contem_cpf);
    RUN_TEST(marcaCirculo_emite_circle);

    RUN_TEST(posEndereco_face_N);
    RUN_TEST(posEndereco_face_S);
    RUN_TEST(posEndereco_face_L);
    RUN_TEST(posEndereco_face_O);

    return UNITY_END();
}
