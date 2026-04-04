#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include <vector>

class Blockchain {
private:
    std::vector<Block> chain;

public:
    int difficulty;

    Blockchain();

    Block createGenesisBlock();
    Block getLatestBlock();

    void addBlock(Block newBlock);

    // 🔥 NOVOS MÉTODOS
    const std::vector<Block>& getChain() const;
    void clearChain();
    void addLoadedBlock(const Block& block);
};

#endif
