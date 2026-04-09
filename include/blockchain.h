#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include <vector>
#include <map>

class Blockchain {
public:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

    Blockchain();

    Block getLastBlock();
    void addBlock(Block block);

    double getBlockReward(int height);
    void mineBlock(std::string minerAddress);

    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);
};

#endif
