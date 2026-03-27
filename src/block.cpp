#include "../include/block.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

std::string sha256(std::string input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}

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

    for (auto &tx : transactions)
        ss << tx.toString();

    return sha256(ss.str());
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
