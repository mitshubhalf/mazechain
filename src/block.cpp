#include "../include/block.h"
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include <iostream>

Block::Block(int idx, std::string prev, std::vector<Transaction> txs)
    : index(idx), prevHash(prev), transactions(txs), nonce(0) {

    timestamp = time(nullptr);
    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;

    ss << index << prevHash << timestamp << nonce;

    for (auto &tx : transactions)
        ss << tx.id;

    std::string data = ss.str();

    unsigned char hashBytes[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hashBytes);

    std::stringstream result;

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        result << std::hex << std::setw(2) << std::setfill('0') << (int)hashBytes[i];

    return result.str();
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();

        if (nonce % 500000 == 0)
            std::cout << "Nonce: " << nonce << " | Hash: " << hash.substr(0,16) << "...\n";
    }

    // ❌ REMOVIDO print duplicado daqui
}
