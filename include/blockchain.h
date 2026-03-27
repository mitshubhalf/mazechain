#pragma once
#include "block.h"
#include <vector>
#include <string>

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> mempool;

    int difficulty;
    double totalSupply;

public:
    Blockchain();

    void mineBlock(std::string miner);
    double getBalance(std::string address);

    int getBlockReward(int height);

    void saveChain();
    void loadChain();
};
