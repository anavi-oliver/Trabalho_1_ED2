#ifndef ARGS_H
#define ARGS_H

/*
 * args.h — parsing dos parâmetros de linha de comando do programa ted.
 *
 * Parâmetros aceitos:
 *   -e path       Diretório-base de entrada (BED). Opcional; default = "."
 *   -f arq.geo    Arquivo .geo (obrigatório)
 *   -o path       Diretório-base de saída (BSD). Obrigatório.
 *   -q arq.qry    Arquivo de consultas. Opcional.
 *   -pm arq.pm    Arquivo de pessoas/moradores. Opcional.
 */

/** Handle opaco — a struct fica definida apenas em args.c */
typedef void *Args;

/**
 * @brief Aloca e preenche um Args a partir de argc/argv.
 * @return Handle alocado, ou NULL se -f ou -o estiverem ausentes.
 */
Args parsearArgs(int argc, char *argv[]);

/** @brief Libera a memória do handle. */
void destruirArgs(Args args);

/* ── getters ── */
const char *argsGetDirEntrada(const Args args);
const char *argsGetDirSaida  (const Args args);
const char *argsGetGeoPath   (const Args args); /* BED + "/" + arq.geo */
const char *argsGetQryPath   (const Args args); /* NULL se -q ausente  */
const char *argsGetPmPath    (const Args args); /* NULL se -pm ausente */

/**
 * @brief Monta o nome-base dos arquivos de saída.
 *
 * Sem -q  → stem do .geo
 * Com -q  → <stem do .geo>-<stem do .qry>
 *
 * @param args    Handle já preenchido.
 * @param out     Buffer de saída.
 * @param outLen  Tamanho máximo do buffer.
 */
void argsBaseSaida(const Args args, char *out, int outLen);

#endif /* ARGS_H */