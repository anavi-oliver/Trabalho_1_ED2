#include <stdint.h>
#include <stdbool.h>
#include "hash.h"
#include "svg.h"

#define CPF_MAX_LEN   16
#define NOME_MAX_LEN  64
#define COMP_MAX_LEN  32
#define DATA_MAX_LEN  12   /* dd/mm/aaaa */

typedef struct {
    char cpf[CPF_MAX_LEN];       /* identificador único                   */
    char nome[NOME_MAX_LEN];     /* primeiro nome                         */
    char sobrenome[NOME_MAX_LEN];/* sobrenome                             */
    char sexo;                   /* 'M' ou 'F'                            */
    char nasc[DATA_MAX_LEN];     /* data no formato dd/mm/aaaa            */

    /* Endereço — preenchido apenas se for morador */
    bool morador;                /* true se tiver endereço                */
    char cep[32];                /* CEP da quadra onde mora               */
    char face;                   /* 'N', 'S', 'L' ou 'O'                 */
    double num;                  /* número da casa (distância da âncora)  */
    char compl[COMP_MAX_LEN];    /* complemento (ex: "apto 3")            */
} Pessoa;