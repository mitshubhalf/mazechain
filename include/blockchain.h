#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include "block.h"

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

    int adjustDifficulty();

public:
    Blockchain();

    Block getLastBlock();
    void mineBlock(std::string minerAddress);

    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);

    double getBlockReward(int height);
};

#endif
