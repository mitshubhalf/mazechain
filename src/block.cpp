#include "../include/block.h"
#include "../include/crypto.h"

Block::Block(int i, std::vector<Transaction> txs, std::string prev) {
    index = i;
    transactions = txs;
    prevHash = prev;
    nonce = 0;
    hash = calculateHash();
}

std::string Block::calculateHash() {
    std::string data = std::to_string(index) + prevHash + std::to_string(nonce);

    for (auto &tx : transactions)
        data += tx.toString();

    return sha256(data);
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
