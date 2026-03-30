#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include <vector>

class Blockchain {
public:
    std::vector<Block> chain;
    int difficulty;

    Blockchain();

    Block createGenesisBlock();
    Block getLatestBlock();

    void addBlock(Block newBlock);
};

#endif
