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
    timestamp = std::to_string(now);

    hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    ss << index << timestamp << previousHash << nonce;

    for (const auto &tx : transactions) {
        ss << tx;
    }

    return sha256(ss.str());
}

void Block::mineBlock(int difficulty) {
    std::string target(difficulty, '0');

    while (hash.compare(0, difficulty, target) != 0) {
        nonce++;
        hash = calculateHash();

        // opcional: log de progresso
        // if (nonce % 100000 == 0) {
        //     std::cout << "Nonce: " << nonce << "\r";
        // }
    }
}
