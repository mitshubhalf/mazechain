#include "../include/blockchain.h"

Blockchain::Blockchain() {
    difficulty = 3;
    chain.push_back(createGenesisBlock());
}

Block Blockchain::createGenesisBlock() {
    return Block(0, {"Genesis Block"}, "0");
}

const Block& Blockchain::getLatestBlock() const {
    return chain.back();
}

void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;
    newBlock.mineBlock(difficulty);
    chain.push_back(newBlock);
}

bool Blockchain::isEmpty() const {
    return chain.empty();
}

bool Blockchain::isChainValid() const {
    for (size_t i = 1; i < chain.size(); i++) {
        const Block& current = chain[i];
        const Block& previous = chain[i - 1];

        if (current.hash != current.calculateHash())
            return false;

        if (current.previousHash != previous.hash)
            return false;
    }
    return true;
}
