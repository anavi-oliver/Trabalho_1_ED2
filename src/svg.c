#include <stdio.h>
#include <stdlib.h>

#include "svg.h"

/*
 * Layout do SVG:
 *   Mapa de quadras em coordenadas positivas (tipicamente 0..~1200 x 0..~1000).
 *   Badges OUT/RIP/HAB no CANTO SUPERIOR DIREITO, à direita do mapa.
 *   <svg> declarado com width e height grandes para não cortar nada.
 */
#define SVG_WIDTH      3000.0
#define SVG_HEIGHT     2000.0

/* Área dos badges: canto superior direito (após mapa típico ~1200px) */
#define BADGE_AREA_X   1250.0
#define BADGE_AREA_Y     10.0
#define BADGE_W         120.0
#define BADGE_H          40.0
#define BADGE_COLS         10
#define BADGE_GAP_X      125.0
#define BADGE_GAP_Y       45.0

struct stArqSvg {
    FILE *f;
    int   nBadgesOut;
    int   nBadgesRip;
    int   nBadgesHab;
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
            " version=\"1.1\""
            " width=\"%.0f\" height=\"%.0f\""
            " viewBox=\"0 0 %.0f %.0f\">\n",
            SVG_WIDTH, SVG_HEIGHT,
            SVG_WIDTH, SVG_HEIGHT);
    return (ArqSvg)svg;
}

void fecharSvg(ArqSvg svg) {
    struct stArqSvg *s = svg;
    if (!s) return;
    fprintf(s->f, "</svg>\n");
    fclose(s->f);
    free(s);
}

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
    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " style=\"fill:%s;fill-opacity:0.5;stroke:%s;stroke-width:%.2f\"/>\n",
            x, y - h, w, h, corFill, corStroke, sw);

    if (cep && cep[0] != '\0') {
        double fs = h * 0.13;
        if (fs < 8.0)  fs = 8.0;
        if (fs > 14.0) fs = 14.0;
        fprintf(s->f,
                "  <text x=\"%.2f\" y=\"%.2f\""
                " style=\"font-size:%.1fpx;fill:black;font-weight:bold\">%s</text>\n",
                x + 2.0, y - h + fs + 1.0, fs, cep);
    }
}

void svgTexto(ArqSvg svg, double x, double y,
              const char *cor, double tamanho, const char *texto)
{
    struct stArqSvg *s = svg;
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:%.0fpx;fill:%s\">%s</text>\n",
            x, y, tamanho, cor, texto);
}

#define MARCA_SZ 6.0

void svgMarcaX(ArqSvg svg, double x, double y) {
    struct stArqSvg *s = svg;
    double d = MARCA_SZ;
    fprintf(s->f,
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n"
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n",
            x-d, y-d, x+d, y+d,
            x+d, y-d, x-d, y+d);
}

void svgMarcaCruz(ArqSvg svg, double x, double y) {
    struct stArqSvg *s = svg;
    double d = MARCA_SZ;
    fprintf(s->f,
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n"
            "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
            " style=\"stroke:red;stroke-width:2\"/>\n",
            x,   y-d, x,   y+d,
            x-d, y,   x+d, y);
}

void svgMarcaQuadrado(ArqSvg svg, double x, double y, const char *cpf) {
    struct stArqSvg *s = svg;
    double d = MARCA_SZ;
    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " style=\"fill:none;stroke:red;stroke-width:2\"/>\n",
            x-d, y-d, 2*d, 2*d);
    fprintf(s->f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " style=\"font-size:4px;fill:red\">%s</text>\n",
            x-d, y+d+5, cpf);
}

void svgMarcaCirculo(ArqSvg svg, double x, double y) {
    struct stArqSvg *s = svg;
    fprintf(s->f,
            "  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\""
            " style=\"fill:black\"/>\n",
            x, y, MARCA_SZ / 2.0);
}

/* ── badges laterais ── */

static void emitirBadge(FILE *f, double rx, double ry, double rw, double rh,
                        const char *bgColor, const char *textColor,
                        const char *titulo, const char *subtitulo)
{
    double cr = rh * 0.3;
    fprintf(f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " rx=\"%.2f\" ry=\"%.2f\""
            " style=\"fill:%s;stroke:none\"/>\n",
            rx, ry, rw, rh, cr, cr, bgColor);
    fprintf(f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " text-anchor=\"middle\""
            " style=\"font-size:11px;fill:%s;font-weight:bold;"
            "font-family:serif\">%s</text>\n",
            rx + rw/2.0, ry + rh*0.42, textColor, titulo);
    fprintf(f,
            "  <text x=\"%.2f\" y=\"%.2f\""
            " text-anchor=\"middle\""
            " style=\"font-size:9px;fill:%s;"
            "font-family:monospace\">%s</text>\n",
            rx + rw/2.0, ry + rh*0.78, textColor, subtitulo);
}

void svgBadgeOut(ArqSvg svg, const char *cpf) {
    struct stArqSvg *s = svg;
    double bx, by;
    badgePos(s->nBadgesOut, 0, &bx, &by);
    s->nBadgesOut++;
    emitirBadge(s->f, bx, by, BADGE_W, BADGE_H, "#5a5a00", "white", "OUT", cpf);
}

void svgBadgeRip(ArqSvg svg, const char *cpf) {
    struct stArqSvg *s = svg;
    int out_rows = (s->nBadgesOut > 0)
                   ? (s->nBadgesOut + BADGE_COLS - 1) / BADGE_COLS : 0;
    double bx, by;
    badgePos(s->nBadgesRip, out_rows, &bx, &by);
    s->nBadgesRip++;
    emitirBadge(s->f, bx, by, BADGE_W, BADGE_H, "black", "white", "R.I.P.", cpf);
}

void svgBadgeHab(ArqSvg svg, const char *cpf) {
    struct stArqSvg *s = svg;
    double bx, by;
    badgePos(s->nBadgesHab, 0, &bx, &by);
    bx += (BADGE_COLS + 1) * BADGE_GAP_X;
    s->nBadgesHab++;
    emitirBadge(s->f, bx, by, BADGE_W, BADGE_H, "#ffb6c1", "#333333", "h?", cpf);
}

void svgPosEndereco(double ax, double ay, double w, double h, char face,
                    double num, double *cx, double *cy)
{
    switch (face) {
        case 'N': *cx = ax + num; *cy = ay - h;   break;
        case 'S': *cx = ax + num; *cy = ay;        break;
        case 'L': *cx = ax + w;   *cy = ay - num;  break;
        case 'O': *cx = ax;       *cy = ay - num;  break;
        default:  *cx = ax;       *cy = ay;        break;
    }
}