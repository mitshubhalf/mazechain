#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>

// Forward declaration para evitar inclusão circular 
struct Transaction; 

namespace Crypto {
    // --- Funções Originais ---

    // Retorna o hash SHA256 (utilizado para Blocos e IDs)
    std::string sha256_util(std::string str);

    // Constrói a árvore de Merkle (integridade das transações no bloco)
    std::string calculateMerkleRoot(const std::vector<Transaction>& txs);

    // --- Funções Novas (Robustez e Segurança) ---

    // Gera um par de chaves (Privada e Pública) para a Wallet
    // A chave privada assina, a chave pública vira o endereço (ou parte dele)
    struct KeyPair {
        std::string private_key;
        std::string public_key;
    };
    KeyPair generate_keys_from_seed(const std::string& seed);

    // Cria uma assinatura digital para uma transação usando a chave privada
    // Isso prova que você autorizou o envio sem mostrar sua seed pra ninguém
    std::string sign_data(const std::string& data, const std::string& private_key);

    // Verifica se uma assinatura é válida usando a chave pública
    bool verify_signature(const std::string& data, const std::string& signature, const std::string& public_key);
}

#endif