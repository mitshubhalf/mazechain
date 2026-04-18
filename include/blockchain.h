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
    
    // --- NOVA LÓGICA ECONÔMICA (INCENTIVO 2000 MZ) ---
    // Regras Atualizadas: 
    // 0-1000 blocos: 2000 MZ
    // 1001-2000 blocos: 250 MZ
    // 2001-4000 blocos: 125 MZ
    double getBlockReward(int height);
    
    double getBalance(std::string address);
    double getTotalSupply() const { return totalSupply; }
    double getMaxSupply() const { return MAX_SUPPLY; }
    
    void send(std::string from, std::string to, double amount, std::string seed);
    bool verifyTransaction(const Transaction& tx);
    bool isChainValid();
    
    std::vector<Block> getChain() const;
    int getDifficulty() const;
    std::vector<Transaction> getMempool() const; 

    void printStats();
};

#endif
