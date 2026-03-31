#include "../include/blockchain.h"
#include <iostream>

Blockchain::Blockchain() {
    difficulty = 3;
    chain.push_back(createGenesisBlock());
}

Block Blockchain::createGenesisBlock() {
    Block genesis(0, {"Genesis Block"}, "0");
    genesis.mineBlock(difficulty);
    return genesis;
}

const Block& Blockchain::getLatestBlock() const {
    return chain.back();
}

void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;

    std::cout << "⛏️ Mining block " << newBlock.index << "...\n";
    newBlock.mineBlock(difficulty);

    chain.push_back(std::move(newBlock));

    std::cout << "✅ Block mined: " << chain.back().hash << "\n";
}
