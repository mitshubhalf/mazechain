#include "../include/blockchain.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 3;
}

Block Blockchain::createGenesisBlock() {
    return Block(0, {"Genesis Block"}, "0");
}

Block Blockchain::getLatestBlock() {
    return chain.back();
}

void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;

    std::cout << "⛏️ Mining block " << newBlock.index << "...\n";
    newBlock.mineBlock(difficulty);

    chain.push_back(newBlock);

    std::cout << "✅ Block mined: " << newBlock.hash << "\n";
}

// 🔥 IMPLEMENTAÇÕES NOVAS

const std::vector<Block>& Blockchain::getChain() const {
    return chain;
}

void Blockchain::clearChain() {
    chain.clear();
}

void Blockchain::addLoadedBlock(const Block& block) {
    chain.push_back(block);
}
