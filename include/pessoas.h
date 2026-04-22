#ifndef PESSOAS_H
#define PESSOAS_H

/*
 * pessoas.h — habitantes e moradores de Bitnópolis.
 *
 * Todo habitante tem CPF, nome, sexo e data de nascimento.
 * Um morador é um habitante com endereço (cep, face, número, complemento).
 * Sem-teto são habitantes sem endereço.
 *
 * Chave do hashfile = CPF convertido por cpfParaChave().
 *
 * Arquivo .pm — comandos tratados aqui:
 *   p cpf nome sobrenome sexo nasc   insere habitante
 *   m cpf cep face num compl         registra endereço (torna morador)
 */

#include <stdint.h>
#include <stdbool.h>
#include "hash.h"

/**
 * @brief Converte CPF (string) em chave uint64_t.
 *        Remove pontos e traços e interpreta os 11 dígitos como inteiro.
 *        Ex: "123.456.789-09" → 12345678909ULL.
 */
uint64_t cpfParaChave(const char *cpf);

/**
 * @brief Lê o arquivo .pm e popula o hashfile de pessoas.
 *
 * Processa 'p' (inserir habitante) e 'm' (atribuir endereço).
 *
 * @param caminhoPm    Caminho completo do arquivo .pm.
 * @param hashPessoas  Hash de destino.
 * @return Número de habitantes inseridos, ou -1 em erro de abertura.
 */
int lerPm(const char *caminhoPm, HashExtensivel hashPessoas);

/**
 * @brief Retorna o tamanho em bytes da struct interna Pessoa.
 */
size_t tamPessoa(void);

/**
 * @brief Busca uma pessoa pelo CPF e copia o bloco serializado para out.
 * @return 1 se encontrada, 0 caso contrário.
 */
int buscarPessoa(HashExtensivel hashPessoas, const char *cpf,
                 void *out, size_t *outTam);

/**
 * @brief Cria e insere um novo habitante (ainda sem endereço).
 *
 * @param hashPessoas             Hash de destino.
 * @param cpf, nome, sobrenome    Dados de identificação.
 * @param sexo                    'M' ou 'F'.
 * @param nasc                    Data "dd/mm/aaaa".
 * @return true se inserido; false se CPF já existir.
 */
bool inserirHabitante(HashExtensivel hashPessoas,
                      const char *cpf, const char *nome,
                      const char *sobrenome, char sexo,
                      const char *nasc);

/**
 * @brief Atribui (ou substitui) o endereço de um habitante, tornando-o morador.
 * @return true se atualizado; false se CPF não encontrado.
 */
bool atribuirEndereco(HashExtensivel hashPessoas, const char *cpf, const char *cep, char face, double num, const char *);

/**
 * @brief Remove o endereço de um morador, tornando-o sem-teto.
 * @return true se atualizado; false se CPF não encontrado.
 */
bool removerEndereco(HashExtensivel hashPessoas, const char *cpf);

/**
 * @brief Remove um habitante do hashfile (falecimento).
 * @return true se removido; false se não encontrado.
 */
bool removerPessoa(HashExtensivel hashPessoas, const char *cpf);

/**
 * @brief Conta moradores de uma quadra por face.
 *
 * @param hashPessoas       Hash com as pessoas.
 * @param cep               CEP da quadra.
 * @param nN, nS, nL, nO   Ponteiros de saída — contagem por face (podem ser NULL).
 * @return Total de moradores da quadra.
 */
int contarMoradoresPorFace(HashExtensivel hashPessoas, const char *cep,
                           int *nN, int *nS, int *nL, int *nO);

/* ── acessores para os campos da Pessoa serializada ─────────────────────────
 * Recebem um ponteiro void* para o bloco copiado do hashfile.             */

const char *pessoaGetCpf       (const void *p);
const char *pessoaGetNome      (const void *p);
const char *pessoaGetSobrenome (const void *p);
char        pessoaGetSexo      (const void *p);
const char *pessoaGetNasc      (const void *p);
bool        pessoaIsMorador    (const void *p);
const char *pessoaGetCep       (const void *p);
char        pessoaGetFace      (const void *p);
double      pessoaGetNum       (const void *p);
const char *pessoaGetCompl     (const void *p);

#endif 