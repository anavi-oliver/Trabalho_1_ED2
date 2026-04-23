#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unity/src/unity.h"
#include "pessoas.h"

typedef struct {
    char   cpf[16];
    char   nome[64];
    char   sobrenome[64];
    char   nasc[12];
    char   cep[32];
    char   compl[32];
    char   sexo;
    char   face;
    char   morador;   // 0 = sem-teto, 1 = morador; parece q nao precisa ser bool
    double num;
} Pessoa;

/* conversão CPF → chave  */
uint64_t cpfParaChave(const char *cpf) {
    uint64_t resultado = 0;
    for (const char *p = cpf; *p; p++) {
        if (*p >= '0' && *p <= '9')
            resultado = resultado * 10 + (uint64_t)(*p - '0');
    }
    return resultado;
}

/* leitura do .pm  */
int lerPm(const char *caminhoPm, HashExtensivel hashPessoas) {
    FILE *f = fopen(caminhoPm, "r");
    if (!f) return -1;

    int inseridos = 0;
    char linha[512];

    while (fgets(linha, sizeof(linha), f)) {
        char cmd[8];
        if (sscanf(linha, "%7s", cmd) != 1) continue;

        if (strcmp(cmd, "p") == 0) {
            Pessoa p;
            memset(&p, 0, sizeof(p));
            char sexoStr[4];
            if (sscanf(linha, "%*s %15s %63s %63s %3s %11s",
                       p.cpf, p.nome, p.sobrenome, sexoStr, p.nasc) != 5)
                continue;
            p.sexo = sexoStr[0];
            p.morador = 0;
            inserirHash(hashPessoas, cpfParaChave(p.cpf), &p, sizeof(Pessoa));
            inseridos++;

        } else if (strcmp(cmd, "m") == 0) {
            char cpf[16], cep[32], faceStr[4], compl[32];
            double num;
            if (sscanf(linha, "%*s %15s %31s %3s %lf %31s",
                       cpf, cep, faceStr, &num, compl) != 5)
                continue;
            atribuirEndereco(hashPessoas, cpf, cep, faceStr[0], num, compl);
        }
    }

    fclose(f);
    return inseridos;
}

/* operações CRUD  */
size_t tamPessoa(void) {
    return sizeof(Pessoa);
}

int buscarPessoa(HashExtensivel hashPessoas, const char *cpf,
                 void *out, size_t *outTam)
{
    size_t tam = 0;
    void *r = procurarHash(hashPessoas, cpfParaChave(cpf), &tam);
    if (!r) return 0;
    memcpy(out, r, tam);
    if (outTam) *outTam = tam;
    free(r);
    return 1;
}

bool inserirHabitante(HashExtensivel hashPessoas,
                      const char *cpf, const char *nome,
                      const char *sobrenome, char sexo,
                      const char *nasc)
{
    Pessoa p;
    memset(&p, 0, sizeof(p));
    strncpy(p.cpf,       cpf,       sizeof(p.cpf)       - 1);
    strncpy(p.nome,      nome,      sizeof(p.nome)      - 1);
    strncpy(p.sobrenome, sobrenome, sizeof(p.sobrenome) - 1);
    p.sexo = sexo;
    strncpy(p.nasc, nasc, sizeof(p.nasc) - 1);
    p.morador = 0;
    return inserirHash(hashPessoas, cpfParaChave(cpf), &p, sizeof(Pessoa));
}

bool atribuirEndereco(HashExtensivel hashPessoas,
                      const char *cpf, const char *cep,
                      char face, double num, const char *compl)
{
    Pessoa p;
    if (!buscarPessoa(hashPessoas, cpf, &p, NULL)) return false;
    strncpy(p.cep,   cep,   sizeof(p.cep)   - 1);
    strncpy(p.compl, compl, sizeof(p.compl) - 1);
    p.face    = face;
    p.num     = num;
    p.morador = 1;
    return atualizarHash(hashPessoas, cpfParaChave(cpf), &p, sizeof(Pessoa));
}

bool removerEndereco(HashExtensivel hashPessoas, const char *cpf) {
    Pessoa p;
    if (!buscarPessoa(hashPessoas, cpf, &p, NULL)) return false;
    p.morador = 0;
    memset(p.cep,   0, sizeof(p.cep));
    memset(p.compl, 0, sizeof(p.compl));
    p.face = '\0';
    p.num  = 0.0;
    return atualizarHash(hashPessoas, cpfParaChave(cpf), &p, sizeof(Pessoa));
}

bool removerPessoa(HashExtensivel hashPessoas, const char *cpf) {
    return removerHash(hashPessoas, cpfParaChave(cpf));
}

/* contagem por face */
typedef struct {
    const char *cep;
    int n, nN, nS, nL, nO;
} CtxContar;

static void cbContar(uint64_t chave, void *valor, size_t tam, void *ctx) {
    (void)chave; (void)tam;
    Pessoa    *p = valor;
    CtxContar *c = ctx;
    if (p->morador && strcmp(p->cep, c->cep) == 0) {
        c->n++;
        switch (p->face) {
            case 'N': c->nN++; break;
            case 'S': c->nS++; break;
            case 'L': c->nL++; break;
            case 'O': c->nO++; break;
        }
    }
    free(valor);
}

int contarMoradoresPorFace(HashExtensivel hashPessoas, const char *cep,
                           int *nN, int *nS, int *nL, int *nO)
{
    CtxContar c = { cep, 0, 0, 0, 0, 0 };
    iterarHash(hashPessoas, cbContar, &c);
    if (nN) *nN = c.nN;
    if (nS) *nS = c.nS;
    if (nL) *nL = c.nL;
    if (nO) *nO = c.nO;
    return c.n;
}

/* acessores */
const char *pessoaGetCpf      (const void *p){ return ((const Pessoa *)p)->cpf;       }
const char *pessoaGetNome     (const void *p){ return ((const Pessoa *)p)->nome;      }
const char *pessoaGetSobrenome(const void *p){ return ((const Pessoa *)p)->sobrenome; }
char        pessoaGetSexo     (const void *p){ return ((const Pessoa *)p)->sexo;      }
const char *pessoaGetNasc     (const void *p){ return ((const Pessoa *)p)->nasc;      }
bool        pessoaIsMorador   (const void *p){ return ((const Pessoa *)p)->morador;   }
const char *pessoaGetCep      (const void *p){ return ((const Pessoa *)p)->cep;       }
char        pessoaGetFace     (const void *p){ return ((const Pessoa *)p)->face;      }
double      pessoaGetNum      (const void *p){ return ((const Pessoa *)p)->num;       }
const char *pessoaGetCompl    (const void *p){ return ((const Pessoa *)p)->compl;     }