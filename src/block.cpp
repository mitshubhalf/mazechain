#include "../include/block.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

Block::Block(int idx, std::vector<Transaction> txs, std::string prev) {
    index = idx;
    transactions = txs;
    prevHash = prev;
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() {
    std::stringstream ss;
    ss << index << prevHash << nonce;

    std::string data = ss.str();

    unsigned char hashBytes[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.size(), hashBytes);

    std::stringstream hex;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hex << std::hex << std::setw(2) << std::setfill('0') << (int)hashBytes[i];
    }

    return hex.str();
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
