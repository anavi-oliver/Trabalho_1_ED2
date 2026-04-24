// src/main.c
//
// Ponto de entrada do programa ted.
//
// Fluxo:
//   1. Parseia os argumentos de linha de comando.
//   2. Cria os hashfiles de quadras e pessoas no diretório de saída.
//   3. Lê o .geo e insere as quadras.
//   4. Gera arq.svg com o estado inicial (só quadras).
//   5. Se -pm: lê o .pm e insere os habitantes/moradores.
//   6. Se -q: abre arq-qry.svg e arq-qry.txt,
//             redesenha as quadras, processa o .qry e fecha os arquivos.
//   7. Gera os arquivos .hfd (dump textual dos hashfiles).
//   8. Fecha os hashfiles e libera a memória.
//
// Arquivos produzidos em DIR_SAIDA:
//   Sempre : <stem>.svg
//            <stem>_quadras.hf  /  <stem>_pessoas.hf   (hashfiles binários)
//            <stem>_quadras.hfd /  <stem>_pessoas.hfd  (dump textual legível)
//   Com -q : <stem>-<qry>.svg  +  <stem>-<qry>.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "hash.h"
#include "svg.h"
#include "cidade.h"
#include "pessoas.h"
#include "qry.h"

#define TAM_BUCKET  8
#define PATHLEN     512

int main(int argc, char *argv[]) {

    /* ── 1. Argumentos ──────────────────────────────────────────────────── */
    Args args = parsearArgs(argc, argv);
    if (!args) {
        fprintf(stderr, "Uso: ted [-e dir] -f arq.geo [-pm arq.pm] "
                        "[-q arq.qry] -o dir\n");
        return 1;
    }

    const char *dirSaida = argsGetDirSaida(args);
    const char *geoPath  = argsGetGeoPath(args);
    const char *pmPath   = argsGetPmPath(args);   /* NULL se ausente */
    const char *qryPath  = argsGetQryPath(args);  /* NULL se ausente */

    /* Nome-base combinado (ex: "t001-q1") e stem só do geo (ex: "t001") */
    char base[256], baseSoGeo[256];
    argsBaseSaida(args, base, sizeof(base));
    strncpy(baseSoGeo, base, sizeof(baseSoGeo) - 1);
    char *traco = strchr(baseSoGeo, '-');
    if (traco) *traco = '\0';

    /* ── 2. Hashfiles (.hf) ─────────────────────────────────────────────── */
    char hqPath[PATHLEN], hpPath[PATHLEN];
    snprintf(hqPath, PATHLEN, "%s/%s_quadras.hf", dirSaida, baseSoGeo);
    snprintf(hpPath, PATHLEN, "%s/%s_pessoas.hf", dirSaida, baseSoGeo);

    HashExtensivel hq = inicializarHash(hqPath, TAM_BUCKET, NULL);
    HashExtensivel hp = inicializarHash(hpPath, TAM_BUCKET, NULL);
    if (!hq || !hp) {
        fprintf(stderr, "Erro: nao foi possivel criar os hashfiles em '%s'.\n",
                dirSaida);
        destruirArgs(args);
        return 1;
    }

    /* ── 3. Lê o .geo ───────────────────────────────────────────────────── */
    if (lerGeo(geoPath, hq) < 0) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'.\n", geoPath);
        destruirHash(hq); destruirHash(hp);
        destruirArgs(args);
        return 1;
    }

    /* ── 4. SVG inicial (só quadras) ────────────────────────────────────── */
    char svgGeoPath[PATHLEN];
    snprintf(svgGeoPath, PATHLEN, "%s/%s.svg", dirSaida, baseSoGeo);

    ArqSvg svgGeo = abrirSvg(svgGeoPath);
    if (!svgGeo) {
        fprintf(stderr, "Erro: nao foi possivel criar '%s'.\n", svgGeoPath);
        destruirHash(hq); destruirHash(hp);
        destruirArgs(args);
        return 1;
    }
    desenharQuadras(hq, svgGeo);
    fecharSvg(svgGeo);

    /* ── 5. Lê o .pm (opcional) ─────────────────────────────────────────── */
    if (pmPath) {
        if (lerPm(pmPath, hp) < 0)
            fprintf(stderr, "Aviso: nao foi possivel abrir '%s'.\n", pmPath);
    }

    /* ── 6. Processa o .qry (opcional) ──────────────────────────────────── */
    if (qryPath) {
        char svgQryPath[PATHLEN], txtQryPath[PATHLEN];
        snprintf(svgQryPath, PATHLEN, "%s/%s.svg", dirSaida, base);
        snprintf(txtQryPath, PATHLEN, "%s/%s.txt", dirSaida, base);

        ArqSvg svgQry = abrirSvg(svgQryPath);
        FILE  *txtQry = fopen(txtQryPath, "w");

        if (!svgQry || !txtQry) {
            fprintf(stderr, "Erro: nao foi possivel criar arquivos de saida "
                            "em '%s'.\n", dirSaida);
            if (svgQry) fecharSvg(svgQry);
            if (txtQry) fclose(txtQry);
            destruirHash(hq); destruirHash(hp);
            destruirArgs(args);
            return 1;
        }

        desenharQuadras(hq, svgQry);

        CtxQry ctx = criarCtxQry(hq, hp, svgQry, txtQry);
        processarQry(qryPath, ctx);
        destruirCtxQry(ctx);

        fecharSvg(svgQry);
        fclose(txtQry);
    }

    /* ── 7. Dump textual dos hashfiles (.hfd) ───────────────────────────── */
    char hfdQPath[PATHLEN], hfdPPath[PATHLEN];
    snprintf(hfdQPath, PATHLEN, "%s/%s_quadras.hfd", dirSaida, baseSoGeo);
    snprintf(hfdPPath, PATHLEN, "%s/%s_pessoas.hfd", dirSaida, baseSoGeo);

    FILE *hfdQ = fopen(hfdQPath, "w");
    if (hfdQ) { imprimirHash(hq, hfdQ, NULL); fclose(hfdQ); }

    FILE *hfdP = fopen(hfdPPath, "w");
    if (hfdP) { imprimirHash(hp, hfdP, NULL); fclose(hfdP); }

    /* ── 8. Libera recursos ─────────────────────────────────────────────── */
    destruirHash(hq);
    destruirHash(hp);
    destruirArgs(args);

    return 0;
}