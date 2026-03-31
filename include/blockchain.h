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
    const Block& getLatestBlock() const;

    void addBlock(Block newBlock);

    bool isEmpty() const;
    bool isChainValid() const;
};

#endif
