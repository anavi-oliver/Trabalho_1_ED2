#ifndef HASH_H
#define HASH_H

#define TAM_MAX_VALOR 256

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * @file hash.h
 * @brief Módulo para manipulação de um arquivo de Hash Extensível.
 * * Este módulo gerencia o armazenamento persistente de dados em disco, 
 * permitindo o crescimento dinâmico da estrutura através do desdobramento
 * de buckets e expansão do diretório. 
 */

typedef void *HashExtensivel;


/**
 * @brief Inicializa a estrutura de Hash Extensível.
 * @param nomeArquivo Nome do arquivo físico em disco.
 * @param tamBucket Número máximo de registros que cabem em um bucket.
 * @param destruirValor Ponteiro para função que limpa a memória do dado.
 * @return Retorna um ponteiro para a estrutura ou NULL em caso de falha.
 */
HashExtensivel inicializarHash(const char *nomeArquivo, int tamBucket, void (*destruirValor)(void *));

/**
 * @brief Insere um valor associado a uma chave no arquivo.
 * @param hash Ponteiro para o Hash Extensivel.
 * @param chave Chave única (64 bits) gerada pela função de hashing.
 * @param valor Ponteiro para os dados que serão armazenados.
 * @param tamValor Tamanho em bytes do dado apontado por 'valor'.
 * @return true se a inserção foi bem-sucedida; false caso contrário.
 */
bool inserirHash(HashExtensivel hash, uint64_t chave, void *valor, size_t tamValor);

/**
 * @brief Procura por um dado no arquivo através da sua chave.
 * @param hash Ponteiro para o Hash Extensivel.
 * @param chave Chave de busca.
 * @param tamRetornado Ponteiro de saída que receberá o tamanho do dado encontrado.
 * @return Retorna um ponteiro para o dado encontrado ou NULL se não existir.
 */
void *procurarHash(HashExtensivel hash, uint64_t chave, size_t *tamRetornado);

/**
 * @brief Remove um dado do Hash e libera o espaço no bucket.
 * @param hash Ponteiro para o Hash Extensivel.
 * @param chave Chave do dado a ser removido.
 * @return true se o dado foi removido; false se a chave não foi encontrada.
 */
/**
 * @brief Itera sobre todos os registros válidos do hashfile.
 * @param hash  Ponteiro para o Hash Extensível.
 * @param cb    Callback chamado para cada entrada: (chave, valor, tam, ctx).
 * @param ctx   Contexto opaco repassado ao callback (pode ser NULL).
 */
void iterarHash(HashExtensivel hash, void (*cb)(uint64_t chave, void *valor, size_t tam, void *ctx), void *ctx);

/**
 * @brief Atualiza o valor de uma chave existente.
 * @return true se a chave existia e foi atualizada; false se não encontrada.
 */
bool atualizarHash(HashExtensivel hash, uint64_t chave, void *novoValor, size_t novoTam);

/**
 * @brief Gera dump textual do hashfile (para o arquivo .hfd).
 * @param saida         Arquivo de saída (aberto para escrita).
 * @param imprimirValor Callback que formata um valor em texto; pode ser NULL.
 */
void imprimirHash(HashExtensivel hash, FILE *saida, void (*imprimirValor)(FILE *saida, void *valor, size_t tam));

bool removerHash(HashExtensivel hash, uint64_t chave);

/**
 * @brief Fecha os arquivos e libera toda a memória alocada pelo módulo.
 * @param hash Ponteiro para a estrutura a ser destruída.
 */
void destruirHash(HashExtensivel hash);

#endif