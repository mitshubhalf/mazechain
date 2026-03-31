#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include <vector>

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;

public:
    Blockchain();

    Block createGenesisBlock() const;

    const Block& getLatestBlock() const;

    void addBlock(Block newBlock);

    bool isChainValid() const;

    bool isEmpty() const;

    const std::vector<Block>& getChain() const;

    int getDifficulty() const;
};

#endif
