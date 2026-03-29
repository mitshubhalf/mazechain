#include "../include/blockchain.h"

Blockchain::Blockchain() {
    difficulty = 4;

    // Genesis block
    chain.push_back(Block(0, {"Genesis Block"}, "0"));
}

Block Blockchain::getLatestBlock() {
    return chain.back();
}

void Blockchain::addBlock(Block newBlock) {
    newBlock.previousHash = getLatestBlock().hash;
    newBlock.mineBlock(difficulty);
    chain.push_back(newBlock);
}
