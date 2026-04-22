#ifndef SVG_H
#define SVG_H

/*
 * svg.h — criação e escrita de arquivos SVG.
 *
 * Encapsula abertura/fechamento do arquivo SVG e todas as primitivas
 * de desenho: quadras (rect), rótulos (text) e marcações visuais dos
 * comandos do .qry (X, cruz, quadrado, círculo).
 *
 * Convenção de coordenadas: origem (0,0) no canto superior-esquerdo,
 * X cresce para direita, Y cresce para baixo.
 * Âncora da quadra = canto sudeste (x, y); retângulo SVG em (x, y-h).
 */

/** Handle opaco — a struct fica definida apenas em svg.c */
typedef void *ArqSvg;

/* ── ciclo de vida ───────────────────────────────────────────────────────── */

/**
 * @brief Abre (cria) um arquivo SVG para escrita e emite o cabeçalho XML.
 * @param caminho  Caminho completo do arquivo a criar.
 * @return Handle, ou NULL em caso de falha ao abrir o arquivo.
 */
ArqSvg abrirSvg(const char *caminho);

/**
 * @brief Emite </svg>, fecha o arquivo e libera o handle.
 * @param svg  Handle retornado por abrirSvg.
 */
void fecharSvg(ArqSvg svg);

/* ── formas básicas ──────────────────────────────────────────────────────── */

/**
 * @brief Desenha uma quadra como <rect> SVG.
 *
 * @param svg                Handle do SVG.
 * @param x, y               Âncora da quadra (canto sudeste).
 * @param w, h               Largura e altura.
 * @param sw                 Espessura da borda (stroke-width).
 * @param corFill, corStroke Cores de preenchimento e borda (nomes CSS).
 */
void svgQuadra(ArqSvg svg,
               double x, double y, double w, double h,
               double sw,
               const char *corFill, const char *corStroke);

/**
 * @brief Escreve um <text> no SVG.
 *
 * @param svg      Handle do SVG.
 * @param x, y     Posição de ancoragem do texto.
 * @param cor      Cor do texto (nome CSS).
 * @param tamanho  Tamanho da fonte em px.
 * @param texto    String a renderizar.
 */
void svgTexto(ArqSvg svg,
              double x, double y,
              const char *cor, double tamanho,
              const char *texto);

/* ── marcações dos comandos .qry ─────────────────────────────────────────── */

/**
 * @brief X vermelho na âncora da quadra removida. (cmd rq)
 */
void svgMarcaX(ArqSvg svg, double x, double y);

/**
 * @brief Cruz vermelha no endereço de falecimento. (cmd rip)
 */
void svgMarcaCruz(ArqSvg svg, double x, double y);

/**
 * @brief Quadrado vermelho com CPF inscrito. (cmd mud)
 */
void svgMarcaQuadrado(ArqSvg svg, double x, double y, const char *cpf);

/**
 * @brief Círculo preto no local do despejo. (cmd dspj)
 */
void svgMarcaCirculo(ArqSvg svg, double x, double y);

/* ── utilitários ─────────────────────────────────────────────────────────── */

/**
 * @brief Calcula as coordenadas SVG de um endereço dentro de uma quadra.
 *
 * @param ax, ay  Âncora da quadra (canto sudeste).
 * @param w, h    Dimensões da quadra.
 * @param face    'N', 'S', 'L' ou 'O'.
 * @param num     Número da casa (distância da âncora ao longo da face).
 * @param cx, cy  Coordenadas SVG resultantes (saída).
 */
void svgPosEndereco(double ax, double ay, double w, double h, char face, double num, double *cx, double *cy);

#endif 