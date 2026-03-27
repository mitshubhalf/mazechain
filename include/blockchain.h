#pragma once
#include <vector>
#include <string>
#include "block.h"
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    std::vector<Transaction> mempool;
    int difficulty;
    double totalSupply;

public:
    Blockchain();

    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);

    bool addTransaction(Transaction tx);

    int getBlockReward(int height);
    double getTotalSupply();
};
