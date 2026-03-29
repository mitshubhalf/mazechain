#include "../include/block.h"
#include "../include/crypto.h"
#include <ctime>

Block::Block(int idx, std::vector<std::string> txs, std::string prevHash) {
    index = idx;
    transactions = txs;
    previousHash = prevHash;
    nonce = 0;

    time_t now = time(0);
    timestamp = ctime(&now);

    hash = calculateHash();
}

std::string Block::calculateHash() {
    std::string data = std::to_string(index) + timestamp + previousHash + std::to_string(nonce);

    for (auto &tx : transactions) {
        data += tx;
    }

    return sha256(data);
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
