#include "../include/blockchain.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 3;
    chain.push_back(createGenesisBlock());
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
