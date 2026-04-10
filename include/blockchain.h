#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"

const int DIFFICULTY_ADJUSTMENT_INTERVAL = 10;
const int TARGET_BLOCK_TIME = 60;

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

    void adjustDifficulty();
    double getBlockReward(int height);

public:
    Blockchain();

    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);
    
    void printBlockDetails(int height);
    bool isChainValid(); // <-- NOVO: Para verificar fraudes

    Block getLastBlock();
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    void clearChain();
    void addBlock(const Block& block);
};

#endif
