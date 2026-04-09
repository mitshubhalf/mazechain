#include "../include/block.h"
#include <sstream>
#include <openssl/sha.h>
#include <iostream>
#include <iomanip> // 🔥 IMPORTANTE (pra corrigir o hash)

Block::Block(int idx, std::string prev, std::vector<Transaction> txs)
    : index(idx), prevHash(prev), transactions(txs), nonce(0) {
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    ss << index << prevHash << nonce;

    for (auto &tx : transactions)
        ss << tx.id;

    std::string data = ss.str();

    unsigned char hashBytes[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hashBytes);

    std::stringstream result;
    result << std::hex << std::setfill('0'); // 🔥 garante formato correto

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        result << std::setw(2) << (int)hashBytes[i]; // 🔥 sempre 2 dígitos
    }

    return result.str();
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();

        if (nonce % 10000 == 0) {
            std::cout << "Nonce: " << nonce 
                      << " | Hash: " << hash.substr(0,16) 
                      << "...\n";
        }
    }

    std::cout << "✅ Block mined! Hash: " << hash << "\n";
}
