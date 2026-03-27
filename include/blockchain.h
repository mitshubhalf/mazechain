#pragma once
#include <vector>
#include "block.h"

class Blockchain {
public:
    std::vector<Block> chain;
    std::vector<Transaction> mempool;
    int difficulty;
    double totalSupply;

    Blockchain();

    void mineBlock(std::string address);
    double getBalance(std::string address);

    int getBlockReward(int height);

    void saveChain();
    void loadChain();
};
