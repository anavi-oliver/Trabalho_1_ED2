#include <stdio.h>
#include <stdlib.h>

#include "svg.h"

struct stArqSvg {
    FILE *f;
};

ArqSvg abrirSvg(const char *caminho) {
    struct stArqSvg *svg = malloc(sizeof(struct stArqSvg));
    if (!svg) return NULL;

    svg->f = fopen(caminho, "w");
    if (!svg->f) { free(svg); return NULL; }

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

void svgQuadra(ArqSvg svg,double x, double y, double w, double h, double sw, const char *corFill, const char *corStroke){
    struct stArqSvg *s = svg;
    /* Âncora = canto sudeste → retângulo SVG começa em (x, y-h) */
    fprintf(s->f,
            "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
            " style=\"fill:%s;fill-opacity:0.5;stroke:%s;stroke-width:%.2f\"/>\n",
            x, y - h, w, h, corFill, corStroke, sw);
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
void svgPosEndereco(double ax, double ay, double w, double h, char face, double num, double *cx, double *cy){
    switch (face) {
        case 'N': *cx = ax + num; *cy = ay - h; break;
        case 'S': *cx = ax + num; *cy = ay;     break;
        case 'L': *cx = ax + w;   *cy = ay - num; break;
        case 'O': *cx = ax;       *cy = ay - num; break;
        default:  *cx = ax;       *cy = ay;     break;
    }
}