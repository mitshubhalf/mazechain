#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>

// Forward declaration para evitar inclusão circular 
// (Já que Transaction usa Crypto e Crypto usa Transaction)
struct Transaction; 

namespace Crypto {
    // Retorna o hash SHA256 de qualquer string em formato hexadecimal
    std::string sha256_util(std::string str);

    // Constrói a árvore de Merkle e retorna a raiz (ID de integridade do bloco)
    std::string calculateMerkleRoot(const std::vector<Transaction>& txs);
}

#endif
