#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

public:
    Blockchain();

    Block getLastBlock();

    double getBlockReward(int height);

    void mineBlock(std::string minerAddress);

    double getBalance(std::string address);

    void send(std::string from, std::string to, double amount);

    // 🔥 NOVOS (necessários pro storage e segurança)
    std::vector<Block> getChain() const;
    void addBlock(const Block& block);
    void clearChain();
    int getDifficulty() const;
};

#endif
