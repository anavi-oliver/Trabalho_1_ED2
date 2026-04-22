#ifndef CIDADE_H
#define CIDADE_H

/*
 * cidade.h — quadras de Bitnópolis.
 *
 * Uma quadra é identificada por um CEP alfanumérico e representada no
 * mapa por um retângulo com âncora no canto sudeste (x, y) e dimensões
 * w × h.  O hashfile usa como chave o CEP convertido por cepParaChave().
 *
 * Arquivo .geo — comandos tratados aqui:
 *   q  cep x y w h       insere quadra
 *   cq sw cfill cstrk    altera cores/espessura padrão
 */

#include <stdint.h>
#include "hash.h"
#include "svg.h"

/**
 * @brief Converte um CEP (string alfanumérica) em chave uint64_t.
 *        Usa FNV-1a 64 bits para boa distribuição.
 */
uint64_t cepParaChave(const char *cep);

/**
 * @brief Lê o arquivo .geo e insere as quadras no hashfile.
 *
 * Comandos processados: 'q' (inserir) e 'cq' (mudar cores padrão).
 *
 * @param caminhoGeo   Caminho completo do arquivo .geo.
 * @param hashQuadras  Hash de destino.
 * @return Número de quadras inseridas, ou -1 em erro de abertura.
 */
int lerGeo(const char *caminhoGeo, HashExtensivel hashQuadras);

/**
 * @brief Desenha todas as quadras do hashfile no SVG.
 *        Itera via iterarHash() e chama svgQuadra() para cada entrada.
 */
void desenharQuadras(HashExtensivel hashQuadras, ArqSvg svg);

/**
 * @brief Busca uma quadra pelo CEP e copia seus dados para o buffer out.
 *
 * O buffer out deve apontar para um bloco de pelo menos TAM_MAX_VALOR bytes
 * (o mesmo tamanho usado no hashfile). Use tamQuadra() para obter o tamanho
 * exato da struct serializada, ou simplesmente use TAM_MAX_VALOR.
 *
 * @param hashQuadras  Hash com as quadras.
 * @param cep          CEP buscado.
 * @param out          Buffer de saída; preenchido em caso de sucesso.
 * @param outTam       Ponteiro que receberá o tamanho copiado (pode ser NULL).
 * @return 1 se encontrada, 0 caso contrário.
 */
int buscarQuadra(HashExtensivel hashQuadras, const char *cep,
                 void *out, size_t *outTam);

/**
 * @brief Remove uma quadra do hashfile pelo CEP.
 * @return 1 se removida, 0 se não encontrada.
 */
int removerQuadra(HashExtensivel hashQuadras, const char *cep);

/**
 * @brief Retorna o tamanho em bytes da struct interna Quadra.
 *        Útil para alocar buffers antes de chamar buscarQuadra().
 */
size_t tamQuadra(void);

/* ── acessores para os campos da Quadra serializada ─────────────────────────
 * Recebem um ponteiro void* que aponta para o bloco retornado/copiado
 * pelo hashfile e retornam o campo solicitado.  Isso evita que o chamador
 * precise conhecer o layout interno da struct.                             */

const char *quadraGetCep    (const void *q);
double      quadraGetX      (const void *q);
double      quadraGetY      (const void *q);
double      quadraGetW      (const void *q);
double      quadraGetH      (const void *q);
double      quadraGetSw     (const void *q);
const char *quadraGetCorFill(const void *q);
const char *quadraGetCorStroke(const void *q);

#endif 