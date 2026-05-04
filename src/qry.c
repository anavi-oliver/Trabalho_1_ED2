#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qry.h"
#include "cidade.h"
#include "pessoas.h"

struct stCtxQry {
    HashExtensivel hashQuadras;
    HashExtensivel hashPessoas;
    ArqSvg         svgSaida;
    FILE          *txtSaida;
};

CtxQry criarCtxQry(HashExtensivel hashQuadras, HashExtensivel hashPessoas,
                   ArqSvg svgSaida, FILE *txtSaida)
{
    struct stCtxQry *c = malloc(sizeof(struct stCtxQry));
    if (!c) return NULL;
    c->hashQuadras = hashQuadras;
    c->hashPessoas = hashPessoas;
    c->svgSaida    = svgSaida;
    c->txtSaida    = txtSaida;
    return (CtxQry)c;
}

void destruirCtxQry(CtxQry ctx) { free(ctx); }

/* ── rq ── */

typedef struct { const char *cep; struct stCtxQry *c; } CtxRq;

static void cbRq(uint64_t chave, void *valor, size_t tam, void *ctx) {
    (void)chave; (void)tam;
    CtxRq *r = ctx;
    if (pessoaIsMorador(valor) &&
        strcmp(pessoaGetCep(valor), r->cep) == 0)
    {
        fprintf(r->c->txtSaida, "  %s %s %s\n",
                pessoaGetCpf(valor),
                pessoaGetNome(valor),
                pessoaGetSobrenome(valor));
        removerEndereco(r->c->hashPessoas, pessoaGetCpf(valor));
    }
    free(valor);
}

void cmdRq(const char *cep, CtxQry ctx) {
    struct stCtxQry *c = ctx;
    uint8_t buf[TAM_MAX_VALOR];
    size_t  tam;
    if (!buscarQuadra(c->hashQuadras, cep, buf, &tam)) return;

    /* X vermelho na âncora da quadra */
    svgMarcaX(c->svgSaida, quadraGetX(buf), quadraGetY(buf));

    /* despeja moradores e os lista no TXT */
    CtxRq r = { cep, c };
    iterarHash(c->hashPessoas, cbRq, &r);

    removerQuadra(c->hashQuadras, cep);
}

/* ── pq ── */

void cmdPq(const char *cep, CtxQry ctx) {
    struct stCtxQry *c = ctx;
    uint8_t buf[TAM_MAX_VALOR];
    if (!buscarQuadra(c->hashQuadras, cep, buf, NULL)) return;

    double x = quadraGetX(buf), y = quadraGetY(buf);
    double w = quadraGetW(buf), h = quadraGetH(buf);

    int nN = 0, nS = 0, nL = 0, nO = 0;
    int total = contarMoradoresPorFace(c->hashPessoas, cep,
                                       &nN, &nS, &nL, &nO);
    char num[16];
    snprintf(num, sizeof(num), "%d", nN);
    svgTexto(c->svgSaida, x + w/2, y - h,     "black", 8, num);
    snprintf(num, sizeof(num), "%d", nS);
    svgTexto(c->svgSaida, x + w/2, y,          "black", 8, num);
    snprintf(num, sizeof(num), "%d", nL);
    svgTexto(c->svgSaida, x + w,   y - h/2,    "black", 8, num);
    snprintf(num, sizeof(num), "%d", nO);
    svgTexto(c->svgSaida, x,       y - h/2,    "black", 8, num);
    snprintf(num, sizeof(num), "%d", total);
    svgTexto(c->svgSaida, x + w/2, y - h/2,   "black", 10, num);

    fprintf(c->txtSaida,
            "  N:%d S:%d L:%d O:%d total:%d\n",
            nN, nS, nL, nO, total);
}

/* ── censo ── */

typedef struct {
    int total, moradores, homens, mulheres, moradoresH, moradoresM;
} CtxCenso;

static void cbCenso(uint64_t chave, void *valor, size_t tam, void *ctx) {
    (void)chave; (void)tam;
    CtxCenso *cc = ctx;
    cc->total++;
    char sexo = pessoaGetSexo(valor);
    if (sexo == 'M') cc->homens++;   else cc->mulheres++;
    if (pessoaIsMorador(valor)) {
        cc->moradores++;
        if (sexo == 'M') cc->moradoresH++; else cc->moradoresM++;
    }
    free(valor);
}

void cmdCenso(CtxQry ctx) {
    struct stCtxQry *c = ctx;
    CtxCenso cc = {0,0,0,0,0,0};
    iterarHash(c->hashPessoas, cbCenso, &cc);

    int semTeto  = cc.total    - cc.moradores;
    int semTetoH = cc.homens   - cc.moradoresH;
    int semTetoM = cc.mulheres - cc.moradoresM;

    double pMor  = cc.total     ? 100.0*cc.moradores /cc.total     : 0;
    double pH    = cc.total     ? 100.0*cc.homens    /cc.total     : 0;
    double pM    = cc.total     ? 100.0*cc.mulheres  /cc.total     : 0;
    double pMorH = cc.moradores ? 100.0*cc.moradoresH/cc.moradores : 0;
    double pMorM = cc.moradores ? 100.0*cc.moradoresM/cc.moradores : 0;
    double pStH  = semTeto      ? 100.0*semTetoH/semTeto           : 0;
    double pStM  = semTeto      ? 100.0*semTetoM/semTeto           : 0;

    fprintf(c->txtSaida,
            "  total habitantes: %d\n"
            "  moradores: %d (%.1f%%)\n"
            "  sem-teto: %d\n"
            "  homens: %d (%.1f%%) | mulheres: %d (%.1f%%)\n"
            "  moradores homens: %d (%.1f%%) | moradores mulheres: %d (%.1f%%)\n"
            "  sem-teto homens: %d (%.1f%%) | sem-teto mulheres: %d (%.1f%%)\n",
            cc.total,
            cc.moradores, pMor,
            semTeto,
            cc.homens, pH, cc.mulheres, pM,
            cc.moradoresH, pMorH, cc.moradoresM, pMorM,
            semTetoH, pStH, semTetoM, pStM);
}

/* ── h? ── */

void cmdH(const char *cpf, CtxQry ctx) {
    struct stCtxQry *c = ctx;
    uint8_t buf[TAM_MAX_VALOR];
    if (!buscarPessoa(c->hashPessoas, cpf, buf, NULL)) {
        fprintf(c->txtSaida, "  nao encontrado: %s\n", cpf);
        return;
    }
    fprintf(c->txtSaida,
            "  cpf:%s nome:%s %s sexo:%c nasc:%s\n",
            pessoaGetCpf(buf), pessoaGetNome(buf), pessoaGetSobrenome(buf),
            pessoaGetSexo(buf), pessoaGetNasc(buf));
    if (pessoaIsMorador(buf))
        fprintf(c->txtSaida, "  endereco: %s/%c/%.0f compl:%s\n",
                pessoaGetCep(buf), pessoaGetFace(buf),
                pessoaGetNum(buf), pessoaGetCompl(buf));
    else
        fprintf(c->txtSaida, "  sem-teto\n");

    /* badge rosa lateral */
    svgBadgeHab(c->svgSaida, cpf);
}

/* ── nasc ── */

void cmdNasc(const char *cpf, const char *nome, const char *sobrenome,
             char sexo, const char *nasc, CtxQry ctx)
{
    struct stCtxQry *c = ctx;
    if (inserirHabitante(c->hashPessoas, cpf, nome, sobrenome, sexo, nasc)) {
        /* badge verde lateral para cada nascimento */
        svgBadgeNasc(c->svgSaida, cpf);
    }
}

/* ── rip ── */

void cmdRip(const char *cpf, CtxQry ctx) {
    struct stCtxQry *c = ctx;
    uint8_t buf[TAM_MAX_VALOR];
    if (!buscarPessoa(c->hashPessoas, cpf, buf, NULL)) return;

    fprintf(c->txtSaida,
            "  cpf:%s nome:%s %s sexo:%c nasc:%s\n",
            pessoaGetCpf(buf), pessoaGetNome(buf), pessoaGetSobrenome(buf),
            pessoaGetSexo(buf), pessoaGetNasc(buf));

    if (pessoaIsMorador(buf)) {
        uint8_t qbuf[TAM_MAX_VALOR];
        if (buscarQuadra(c->hashQuadras, pessoaGetCep(buf), qbuf, NULL)) {
            double cx, cy;
            svgPosEndereco(quadraGetX(qbuf), quadraGetY(qbuf),
                           quadraGetW(qbuf), quadraGetH(qbuf),
                           pessoaGetFace(buf), pessoaGetNum(buf),
                           &cx, &cy);
            svgMarcaCruz(c->svgSaida, cx, cy);
        }
        fprintf(c->txtSaida, "  endereco: %s/%c/%.0f compl:%s\n",
                pessoaGetCep(buf), pessoaGetFace(buf),
                pessoaGetNum(buf), pessoaGetCompl(buf));
    }

    /* badge preto lateral */
    svgBadgeRip(c->svgSaida, cpf);

    removerPessoa(c->hashPessoas, cpf);
}

/* ── mud ── */

void cmdMud(const char *cpf, const char *cep, char face,
            double num, const char *compl, CtxQry ctx)
{
    struct stCtxQry *c = ctx;
    atribuirEndereco(c->hashPessoas, cpf, cep, face, num, compl);

    uint8_t qbuf[TAM_MAX_VALOR];
    if (buscarQuadra(c->hashQuadras, cep, qbuf, NULL)) {
        double cx, cy;
        svgPosEndereco(quadraGetX(qbuf), quadraGetY(qbuf),
                       quadraGetW(qbuf), quadraGetH(qbuf),
                       face, num, &cx, &cy);
        svgMarcaQuadrado(c->svgSaida, cx, cy, cpf);
    }
}

/* ── dspj ── */

void cmdDspj(const char *cpf, CtxQry ctx) {
    struct stCtxQry *c = ctx;
    uint8_t buf[TAM_MAX_VALOR];
    if (!buscarPessoa(c->hashPessoas, cpf, buf, NULL)) return;
    if (!pessoaIsMorador(buf)) return;

    fprintf(c->txtSaida,
            "  cpf:%s nome:%s %s\n"
            "  endereco despejo: %s/%c/%.0f compl:%s\n",
            pessoaGetCpf(buf), pessoaGetNome(buf), pessoaGetSobrenome(buf),
            pessoaGetCep(buf), pessoaGetFace(buf),
            pessoaGetNum(buf), pessoaGetCompl(buf));

    uint8_t qbuf[TAM_MAX_VALOR];
    if (buscarQuadra(c->hashQuadras, pessoaGetCep(buf), qbuf, NULL)) {
        double cx, cy;
        svgPosEndereco(quadraGetX(qbuf), quadraGetY(qbuf),
                       quadraGetW(qbuf), quadraGetH(qbuf),
                       pessoaGetFace(buf), pessoaGetNum(buf),
                       &cx, &cy);
        svgMarcaCirculo(c->svgSaida, cx, cy);
    }

    /* badge oliva lateral */
    svgBadgeOut(c->svgSaida, cpf);

    removerEndereco(c->hashPessoas, cpf);
}

/* ── processarQry ── */

int processarQry(const char *caminhoQry, CtxQry ctx) {
    struct stCtxQry *c = ctx;
    FILE *f = fopen(caminhoQry, "r");
    if (!f) return -1;

    char linha[512];
    while (fgets(linha, sizeof(linha), f)) {
        linha[strcspn(linha, "\n")] = '\0';
        if (linha[0] == '\0') continue;

        fprintf(c->txtSaida, "[*] %s\n", linha);

        char cmd[16];
        if (sscanf(linha, "%15s", cmd) != 1) continue;

        if (strcmp(cmd, "rq") == 0) {
            char cep[32];
            if (sscanf(linha, "%*s %31s", cep) == 1)
                cmdRq(cep, ctx);

        } else if (strcmp(cmd, "pq") == 0) {
            char cep[32];
            if (sscanf(linha, "%*s %31s", cep) == 1)
                cmdPq(cep, ctx);

        } else if (strcmp(cmd, "censo") == 0) {
            cmdCenso(ctx);

        } else if (strcmp(cmd, "h?") == 0) {
            char cpf[16];
            if (sscanf(linha, "%*s %15s", cpf) == 1)
                cmdH(cpf, ctx);

        } else if (strcmp(cmd, "nasc") == 0) {
            char cpf[16], nome[64], sob[64], sexoStr[4], nasc[12];
            if (sscanf(linha, "%*s %15s %63s %63s %3s %11s",
                       cpf, nome, sob, sexoStr, nasc) == 5)
                cmdNasc(cpf, nome, sob, sexoStr[0], nasc, ctx);

        } else if (strcmp(cmd, "rip") == 0) {
            char cpf[16];
            if (sscanf(linha, "%*s %15s", cpf) == 1)
                cmdRip(cpf, ctx);

        } else if (strcmp(cmd, "mud") == 0) {
            char cpf[16], cep[32], faceStr[4], compl[32];
            double num;
            if (sscanf(linha, "%*s %15s %31s %3s %lf %31s",
                       cpf, cep, faceStr, &num, compl) == 5)
                cmdMud(cpf, cep, faceStr[0], num, compl, ctx);

        } else if (strcmp(cmd, "dspj") == 0) {
            char cpf[16];
            if (sscanf(linha, "%*s %15s", cpf) == 1)
                cmdDspj(cpf, ctx);
        }
    }

    fclose(f);
    return 0;
}