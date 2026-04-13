#include "../include/crypto.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace Crypto {

// Função utilitária básica de SHA256
std::string sha256_util(std::string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Cálculo da Raiz de Merkle para segurança dos blocos
std::string calculateMerkleRoot(const std::vector<Transaction>& txs) {
    if (txs.empty()) return sha256_util("empty");

    std::vector<std::string> tree;
    for (const auto& tx : txs) {
        tree.push_back(tx.id);
    }

    while (tree.size() > 1) {
        if (tree.size() % 2 != 0) {
            tree.push_back(tree.back());
        }
        std::vector<std::string> newLevel;
        for (size_t i = 0; i < tree.size(); i += 2) {
            newLevel.push_back(sha256_util(tree[i] + tree[i+1]));
        }
        tree = newLevel;
    }
    return tree[0];
}

} // namespace Crypto
