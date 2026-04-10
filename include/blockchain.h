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

    const int DIFFICULTY_ADJUSTMENT_INTERVAL = 10; 
    const int TARGET_BLOCK_TIME = 30;             

public:
    Blockchain();
    Block getLastBlock();
    double getBlockReward(int height);
    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);
    
    // Funções de Gerenciamento e Persistência
    void adjustDifficulty();
    std::vector<Block> getChain() const;
    void addBlock(const Block& block);
    void clearChain();
    int getDifficulty() const;
    void setDifficulty(int d); // Novo: Para carregar do arquivo
};

#endif
