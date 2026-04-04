#include "../include/block.h"
#include "../include/crypto.h"
#include <ctime>
#include <sstream>

Block::Block(int idx, std::vector<std::string> txs, std::string prevHash) {
    index = idx;
    transactions = txs;
    previousHash = prevHash;
    nonce = 0;

    time_t now = time(0);
    timestamp = std::ctime(&now);
    timestamp.pop_back(); // 🔥 REMOVE \n (CRÍTICO)

    hash = calculateHash();
}

std::string Block::calculateHash() {
    std::stringstream ss;
    ss << index << timestamp << previousHash << nonce;

    for (auto &tx : transactions) {
        ss << tx;
    }

    return sha256(ss.str());
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
