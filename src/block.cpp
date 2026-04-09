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

    for (const auto &tx : transactions) {
        ss << tx.id;
    }

    std::string data = ss.str();

    unsigned char hashBytes[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hashBytes);

    std::stringstream result;

    result << std::hex << std::setfill('0');

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        result << std::setw(2) << static_cast<int>(hashBytes[i]);
    }

    return result.str();
}

void Block::mine(int difficulty) {
    std::string target(difficulty, '0');

    // garante que o hash inicial está sincronizado com nonce = 0
    hash = calculateHash();

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();

        if (nonce % 500000 == 0) {
            std::cout << "Nonce: " << nonce 
                      << " | Hash: " << hash.substr(0, 16) << "...\n";
        }
    }
}
