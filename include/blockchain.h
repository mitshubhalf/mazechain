#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include "block.h"
#include "utxo.h"      
#include "transaction.h"

class Blockchain {
private:
    std::vector<Block> chain;
    // Lista interna de transações aguardando mineração
    std::vector<Transaction> pendingTransactions; 
    
    int difficulty;
    double totalSupply;
    
    // Max Supply oficial: 20 Milhões de MZ
    const double MAX_SUPPLY = 20000000.0;

    void adjustDifficulty();
    int getCurrentCycle(int height);

public:
    UTXOSet utxoSet; 

    Blockchain();
    
    void mineBlock(std::string minerAddress);
    void addBlock(const Block& block);
    Block getLastBlock() const { return chain.back(); }
    void clearChain();
    
    double getBlockReward(int height);
    double getBalance(std::string address);
    
    double getTotalSupply() const;
    double getMaxSupply() const;
    
    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    bool isChainValid();
    
    std::vector<Block> getChain() const;
    int getDifficulty() const;

    // FUNÇÃO CORRIGIDA PARA O MAIN.CPP:
    std::vector<Transaction> getPendingTransactions() const { return pendingTransactions; }

    void printStats();
};

#endif
