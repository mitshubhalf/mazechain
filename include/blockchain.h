#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"

// Configurações da Rede
const int DIFFICULTY_ADJUSTMENT_INTERVAL = 10; // Ajusta a cada 10 blocos
const int TARGET_BLOCK_TIME = 60;              // Alvo de 60 segundos por bloco

class Blockchain {
private:
    std::vector<Block> chain;
    int difficulty;
    double totalSupply;

    // Funções internas de auxílio
    void adjustDifficulty();
    double getBlockReward(int height);

public:
    Blockchain();

    // Funções Principais
    void mineBlock(std::string minerAddress);
    double getBalance(std::string address);
    void send(std::string from, std::string to, double amount);
    
    // Visualização e Auditoria
    void printBlockDetails(int height); // A LINHA QUE FALTAVA
    
    // Getters e Utilitários
    Block getLastBlock();
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    void setDifficulty(int d);
    void clearChain();
    void addBlock(const Block& block);
};

#endif
