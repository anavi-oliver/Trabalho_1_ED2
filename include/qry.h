#ifndef QRY_H
#define QRY_H

/*
 * qry.h — processamento do arquivo de consultas (.qry).
 *
 *   rq  cep
 *       Remove a quadra; moradores viram sem-teto.
 *       SVG: X vermelho na âncora.   TXT: CPF e nome dos afetados.
 *
 *   pq  cep
 *       Conta moradores por face e total.
 *       SVG: contagem por face próxima ao limite; total no centro.
 *
 *   censo
 *       Estatísticas gerais (total habitantes, moradores, sem-teto,
 *       proporções, % por sexo em cada categoria).
 *       TXT: relatório completo.
 *
 *   h?  cpf
 *       TXT: todos os dados do habitante; se morador, inclui endereço.
 *
 *   nasc  cpf nome sobrenome sexo nasc
 *       Insere novo habitante.
 *
 *   rip  cpf
 *       Remove habitante (falecimento).
 *       SVG: cruz vermelha no endereço (se morador).
 *       TXT: dados e endereço (se morador).
 *
 *   mud  cpf cep face num compl
 *       Muda endereço do morador.
 *       SVG: quadrado vermelho com CPF no destino.
 *
 *   dspj  cpf
 *       Despeja morador (perde o endereço).
 *       SVG: círculo preto no local do despejo.
 *       TXT: dados e endereço do despejo.
 */

#include <stdio.h>
#include "hash.h"
#include "svg.h"

/** Handle opaco do contexto de processamento — definido apenas em qry.c */
typedef void *CtxQry;

/**
 * @brief Aloca e inicializa o contexto de processamento do .qry.
 *
 * @param hashQuadras  Hashfile das quadras (já aberto).
 * @param hashPessoas  Hashfile dos habitantes (já aberto).
 * @param svgSaida     Handle do SVG de saída (já aberto).
 * @param txtSaida     Arquivo TXT de saída (já aberto para escrita).
 * @return Handle alocado, ou NULL em caso de falha.
 */
CtxQry criarCtxQry(HashExtensivel hashQuadras,HashExtensivel hashPessoas,ArqSvg svgSaida,FILE *txtSaida);

/** @brief Libera o handle (não fecha os recursos internos). */
void destruirCtxQry(CtxQry ctx);

/**
 * @brief Lê o arquivo .qry e processa cada comando.
 * @return 0 em sucesso; -1 se o arquivo não puder ser aberto.
 */
int processarQry(const char *caminhoQry, CtxQry ctx);

/* ── handlers individuais — exportados para uso direto nos testes ──────── */

void cmdRq  (const char *cep, CtxQry ctx);
void cmdPq  (const char *cep, CtxQry ctx);
void cmdCenso(CtxQry ctx);
void cmdH   (const char *cpf, CtxQry ctx);
void cmdNasc(const char *cpf, const char *nome, const char *sobrenome, char sexo, const char *nasc, CtxQry ctx);
void cmdRip (const char *cpf, CtxQry ctx);
void cmdMud(const char *cpf, const char *cep, char face, double num, const char *, CtxQry ctx);
void cmdDspj(const char *cpf, CtxQry ctx);

#endif 