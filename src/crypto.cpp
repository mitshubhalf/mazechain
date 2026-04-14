#include "../include/crypto.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace Crypto {

// Função utilitária de SHA256
// Mantive a estrutura original, mas otimizei a conversão para string hexadecimal
std::string sha256_util(std::string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        // setfill('0') e setw(2) são vitais para que hashes com zeros à esquerda
        // não percam caracteres e mantenham os 64 caracteres padrão.
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Cálculo da Raiz de Merkle (Merkle Root)
// Adaptado para garantir que os Mits e IDs de transação estejam blindados
std::string calculateMerkleRoot(const std::vector<Transaction>& txs) {
    // Caso o bloco esteja vazio (como o nosso novo Bloco Gênesis #0)
    if (txs.empty()) return sha256_util("genesis_empty_block");

    std::vector<std::string> tree;
    for (const auto& tx : txs) {
        // Usamos o ID da transação, que já é um hash de:
        // (remetente + destino + valor + timestamp)
        tree.push_back(tx.id);
    }

    // Processo de redução da árvore até sobrar apenas a raiz (Root)
    while (tree.size() > 1) {
        // Se o número de transações for ímpar, o Bitcoin duplica a última 
        // para fechar o par na árvore. Mantivemos essa regra clássica.
        if (tree.size() % 2 != 0) {
            tree.push_back(tree.back());
        }

        std::vector<std::string> newLevel;
        for (size_t i = 0; i < tree.size(); i += 2) {
            // Concatena dois hashes e gera um novo hash acima deles
            newLevel.push_back(sha256_util(tree[i] + tree[i+1]));
        }
        tree = newLevel;
    }

    return tree[0];
}

} // namespace Crypto
