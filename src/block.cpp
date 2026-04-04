#include "../include/block.h"
#include "../include/crypto.h"
#include <sstream>
#include <ctime>

Block::Block(int idx, std::vector<Transaction> txs, std::string prevHash) {
    index = idx;
    transactions = txs;
    previousHash = prevHash;
    nonce = 0;

    time_t now = time(0);
    timestamp = ctime(&now);

    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;

    ss << index << timestamp << previousHash << nonce;

    for (const auto& tx : transactions) {
        ss << tx.from << tx.to << tx.amount;
    }

    return Crypto::sha256(ss.str());
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculateHash();
    }
}
