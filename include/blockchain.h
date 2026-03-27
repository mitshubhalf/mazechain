#pragma once
#include <vector>
#include "block.h"

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> mempool;
    int difficulty;
    double totalSupply;

public:
    Blockchain();

    void mineBlock(std::string miner);
    bool addTransaction(Transaction tx);

    double getBalance(std::string address);
    int getBlockReward(int height);

    void saveChain();
    void loadChain();
};
