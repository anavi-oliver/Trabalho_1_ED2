#include <stdio.h>
#include <stdlib.h>

#include "svg.h"

/*
 * Estado interno para posicionamento automático dos badges laterais.
 * Os badges são empilhados em uma grade à direita do mapa.
 * Configurado para começar em x=1300, y=10, com células de 120x30.
 */
#define BADGE_AREA_X   1300.0
#define BADGE_AREA_Y     10.0
#define BADGE_W         110.0
#define BADGE_H          28.0
#define BADGE_COLS         10
#define BADGE_GAP_X      115.0
#define BADGE_GAP_Y       35.0

struct stArqSvg {
    FILE *f;
    int   nBadgesOut;   /* contador de badges OUT (despejo) */
    int   nBadgesRip;   /* contador de badges RIP (falecimento) */
    int   nBadgesHab;   /* contador de badges HAB (h?) */
};

ArqSvg abrirSvg(const char *caminho) {
    struct stArqSvg *svg = malloc(sizeof(struct stArqSvg));
    if (!svg) return NULL;

    svg->f = fopen(caminho, "w");
    if (!svg->f) { free(svg); return NULL; }

    svg->nBadgesOut = 0;
    svg->nBadgesRip = 0;
    svg->nBadgesHab = 0;

    fprintf(svg->f,
            "<svg xmlns=\"http://www.w3.org/2000/svg\""
            " xmlns:svg=\"http://www.w3.org/2000/svg\""
            " version=\"1.1\">\n");
    return (ArqSvg)svg;
}

void fecharSvg(ArqSvg svg) {
    struct stArqSvg *s = svg;
    if (!s) return;
    fprintf(s->f, "</svg>\n");
    fclose(s->f);
    free(s);
}

/* ── helper interno para posição do próximo badge ── */
static void badgePos(int idx, int row_offset, double *bx, double *by) {
    int col = idx % BADGE_COLS;
    int row = idx / BADGE_COLS;
    *bx = BADGE_AREA_X + col * BADGE_GAP_X;
    *by = BADGE_AREA_Y + (row_offset + row) * BADGE_GAP_Y;
}

/* ── formas básicas ── */

void svgQuadra(ArqSvg svg, double x, double y, double w, double h, double sw,
               const char *corFill, const char *corStroke, const char *cep)
{
    struct stArqSvg *s = svg;
    /* Âncora = canto sudeste → retângulo SVG começa em (x, y-h) */
    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " style=\"fill:%s;fill-opacity:0.5;stroke:%s;stroke-width:%.2f\"/>\n",
            x, y - h, w, h, corFill, corStroke, sw);

    /* Label com o CEP no canto superior esquerdo da quadra */
    if (cep && cep[0] != '\0') {
        fprintf(s->f,
                "  <text x=\"%.2f\" y=\"%.2f\""
                " style=\"font-size:6px;fill:black\">%s</text>\n",
                x + 2.0, y - h + 8.0, cep);
    }
}

void svgTexto(ArqSvg svg,
              double x, double y,
              const char *cor, double tamanho,
              const char *texto)
{
    struct stArqSvg *s = svg;
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:%.0fpx;fill:%s\">%s</text>\n",
            x, y, tamanho, cor, texto);
}

/* Tamanho dos marcadores visuais */
#define MARCA_SZ 6.0

void svgMarcaX(ArqSvg svg, double x, double y) {
    struct stArqSvg *s = svg;
    double d = MARCA_SZ;
    fprintf(s->f,
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n"
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n",
            x - d, y - d, x + d, y + d,
            x + d, y - d, x - d, y + d);
}

void svgMarcaCruz(ArqSvg svg, double x, double y) {
    struct stArqSvg *s = svg;
    double d = MARCA_SZ;
    fprintf(s->f,
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n"
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n",
            x,     y - d, x,     y + d,
            x - d, y,     x + d, y);
}

void svgMarcaQuadrado(ArqSvg svg, double x, double y, const char *cpf) {
    struct stArqSvg *s = svg;
    double d = MARCA_SZ;
    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " style=\"fill:none;stroke:red;stroke-width:2\"/>\n",
            x - d, y - d, 2 * d, 2 * d);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:4px;fill:red\">%s</text>\n",
            x - d, y + d + 5, cpf);
}

void svgMarcaCirculo(ArqSvg svg, double x, double y) {
    struct stArqSvg *s = svg;
    fprintf(s->f,
            "  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\""
            " style=\"fill:black\"/>\n",
            x, y, MARCA_SZ / 2.0);
}

/* ── badges laterais ── */

/*
 * Badge "OUT" — fundo verde-oliva escuro, texto branco.
 * Posicionado em duas linhas: "OUT" em cima, CPF embaixo.
 * Grade: linha 0 (row_offset=0).
 */
void svgBadgeOut(ArqSvg svg, const char *cpf) {
    struct stArqSvg *s = svg;
    double bx, by;
    badgePos(s->nBadgesOut, 0, &bx, &by);
    s->nBadgesOut++;

    double rx = bx, ry = by;
    double rw = BADGE_W, rh = BADGE_H;
    double cr = 6.0; /* corner radius */

    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " rx=\"%.2f\" ry=\"%.2f\""
            " style=\"fill:#5a5a00;stroke:none\"/>\n",
            rx, ry, rw, rh, cr, cr);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:7px;fill:white;font-weight:bold\">OUT</text>\n",
            rx + 4, ry + 11);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:6px;fill:white\">%s</text>\n",
            rx + 4, ry + 22, cpf);
}

/*
 * Badge "R.I.P." — fundo preto, texto branco.
 * Grade: logo abaixo dos badges OUT (row_offset dinâmico baseado em OUT).
 */
void svgBadgeRip(ArqSvg svg, const char *cpf) {
    struct stArqSvg *s = svg;
    /* Calcula quantas linhas os badges OUT ocupam */
    int out_rows = (s->nBadgesOut + BADGE_COLS - 1) / BADGE_COLS;
    if (out_rows < 1) out_rows = 0;

    double bx, by;
    badgePos(s->nBadgesRip, out_rows, &bx, &by);
    s->nBadgesRip++;

    double rx = bx, ry = by;
    double rw = BADGE_W, rh = BADGE_H;
    double cr = 6.0;

    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " rx=\"%.2f\" ry=\"%.2f\""
            " style=\"fill:black;stroke:none\"/>\n",
            rx, ry, rw, rh, cr, cr);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:7px;fill:white;font-weight:bold\">R.I.P.</text>\n",
            rx + 4, ry + 11);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:6px;fill:white\">%s</text>\n",
            rx + 4, ry + 22, cpf);
}

/*
 * Badge de habitante consultado (h?) — fundo rosa, texto escuro.
 * Posicionado ao lado dos badges OUT na mesma linha (mesmo row).
 */
void svgBadgeHab(ArqSvg svg, const char *cpf) {
    struct stArqSvg *s = svg;
    /* Hab badges ficam na mesma área dos OUT, mas à direita (offset de coluna) */
    int out_rows = (s->nBadgesOut + BADGE_COLS - 1) / BADGE_COLS;
    if (out_rows < 1) out_rows = 1;

    /* Posiciona hab na mesma linha do último OUT, coluna seguinte */
    double bx, by;
    badgePos(s->nBadgesHab, 0, &bx, &by);
    /* desloca para ao lado direito (colunas extras) */
    bx += (BADGE_COLS + 1) * BADGE_GAP_X;
    (void)out_rows;
    s->nBadgesHab++;

    double rx = bx, ry = by;
    double rw = BADGE_W, rh = BADGE_H;
    double cr = 6.0;

    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " rx=\"%.2f\" ry=\"%.2f\""
            " style=\"fill:#ffb6c1;stroke:none\"/>\n",
            rx, ry, rw, rh, cr, cr);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:7px;fill:#333;font-weight:bold\">h?</text>\n",
            rx + 4, ry + 11);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:6px;fill:#333\">%s</text>\n",
            rx + 4, ry + 22, cpf);
}

/*
 * Calcula a posição SVG de um endereço dentro de uma quadra.
 *
 * Convenção:
 *   Âncora (ax, ay) = canto sudeste da quadra.
 *   Face N: parede de cima  → y = ay - h,  x varia de ax até ax+w
 *   Face S: parede de baixo → y = ay,       x varia de ax até ax+w
 *   Face L: parede direita  → x = ax + w,   y varia de ay até ay-h
 *   Face O: parede esquerda → x = ax,        y varia de ay até ay-h
 *
 *   num é a distância a partir da âncora ao longo da face.
 */
void svgPosEndereco(double ax, double ay, double w, double h, char face,
                    double num, double *cx, double *cy)
{
    switch (face) {
        case 'N': *cx = ax + num; *cy = ay - h; break;
        case 'S': *cx = ax + num; *cy = ay;     break;
        case 'L': *cx = ax + w;   *cy = ay - num; break;
        case 'O': *cx = ax;       *cy = ay - num; break;
        default:  *cx = ax;       *cy = ay;     break;
    }
}